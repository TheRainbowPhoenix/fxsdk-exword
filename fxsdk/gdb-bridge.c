#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#include <libusb.h>

#include <fxlink/devices.h>
#include <fxlink/filter.h>
#include <fxlink/logging.h>

/* Establish a connection to the calculator. If nostart is set, assume the
   calculator is ready immediately; otherwise, wait for a message of type
   gdb:start. The latter can be useful is the calculator is using USB prior to
   starting a debugging session. */
static struct fxlink_device *setup_calc(libusb_context *context, bool nostart)
{
	struct fxlink_filter filter = { 0 };
	filter.intf_fxlink = true;
	fxlink_filter_clean_libusb(&filter);

	hlog("calculators");
	log_("waiting for calculator to connect...\n");
	delay_t delay = delay_infinite();
	struct fxlink_device *fdev =
		fxlink_device_find_wait(context, &filter, &delay);

	if(!fdev) {
		elog("unable to open calculator\n");
		return NULL;
	}
	if(!fxlink_device_claim_fxlink(fdev)) {
		elog("unable to claim fxlink interface\n");
		return NULL;
	}

	fxlink_device_start_bulk_IN(fdev);

	if(nostart)
		return fdev;

	hlog("gdb");
	log_("waiting for gdb init message...\n");
	while(true) {
		libusb_handle_events(context);

		struct fxlink_message *msg = fxlink_device_finish_bulk_IN(fdev);
		if(!msg)
			continue;
		hlog("gdb");
		if(fxlink_message_is_apptype(msg, "gdb", "start")) {
			log_("got start message\n");
			break;
		}
		wlog("dropped a message of type %.16s:%.16s\n",
			msg->application, msg->type);

		fxlink_device_start_bulk_IN(fdev);
	}

	return fdev;
}

static int setup_socket(char const *listen_path)
{
	int listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(listen_socket < 0) {
		perror("socket");
		return -1;
	}

	unlink(listen_path);

	struct sockaddr_un listen_address = {
		.sun_family = AF_UNIX,
		.sun_path = {0},
	};
	strncpy(listen_address.sun_path, listen_path,
		sizeof(listen_address.sun_path) - 1);
	if(bind(listen_socket, (struct sockaddr *)&listen_address, sizeof(listen_address)) < 0) {
		close(listen_socket);
		perror("bind");
		return -1;
	}

	if(listen(listen_socket, 1024) < 0) {
		close(listen_socket);
		perror("listen");
		return -1;
	}

	hlog("socket");
	log_("waiting for client on \"%s\"...\n", listen_address.sun_path);

	return listen_socket;
}

static int volatile gdb_terminated_flag = 0;
static int volatile interrupted_flag = 0;

void SIGINT_SIGCHLD_handler(int signal)
{
	if(signal == SIGCHLD)
		gdb_terminated_flag = 1;
	else
		interrupted_flag = 1;
}

static pid_t fork_gdb(char **user_argv, char const *socket_path)
{
	int n = 0;
	while(user_argv[n])
		n++;

	char target_command[256];
	sprintf(target_command, "target remote %s", socket_path);

	char **argv = malloc((n + 5) * sizeof *argv);
	argv[0] = "sh-elf-gdb";
	argv[1] = "-q";
	argv[2] = "-ex";
	argv[3] = target_command;
	memcpy(argv+4, user_argv, (n+1) * sizeof *argv);

	struct sigaction action = {
		.sa_handler = SIGINT_SIGCHLD_handler,
	};
	sigaction(SIGCHLD, &action, NULL);

	pid_t pid = fork();

	/* Child - execvp() only returns if there is an error */
	if(pid == 0) {
		execvp("sh-elf-gdb", argv);
		perror("execvp");
		exit(1);
	}
	/* Parent */
	if(pid == -1)
		perror("fork");
	return pid;
}

static int accept_gdb(int listen_socket)
{
	int client_socket = accept(listen_socket, NULL, NULL);
	if(client_socket < 0) {
		close(listen_socket);
		perror("accept");
		return -1;
	}
	close(listen_socket);
	return client_socket;
}

struct options {
	bool bridge_only;
};

/* Parse options, returns positional arguments to forward to gdb. */
static char **parse_argv(int argc, char **argv, struct options *opts)
{
	int bridge_only = 0;
	struct option longs[] = {
		{ "bridge-only", no_argument, &bridge_only, 1 },
		{ NULL },
	};

	getopt_long(argc, argv, "", longs, NULL);

	opts->bridge_only = (bridge_only != 0);
	return argv + optind + (argv[optind] && !strcmp(argv[optind], "--"));
}

static void noop_log_handler(int display_fmt, char const *str)
{
	(void)display_fmt;
	(void)str;
}

int main(int argc, char **argv)
{
	libusb_context *context = NULL;
	struct fxlink_device *fdev = NULL;
	struct fxlink_pollfds fxlink_polled_fds = { 0 };
	char socket_path[256] = { 0 };
	pid_t gdb_pid = -1;
	int ret = 1;

	struct options opts;
	char **gdb_argv = parse_argv(argc, argv, &opts);

	int err = libusb_init(&context);
	if(err != 0) {
		elog_libusb(err, "libusb_init");
		goto end;
	}
	bool nostart = true;
	fdev = setup_calc(context, nostart);
	if(!fdev)
		goto end;

	sprintf(socket_path, "/tmp/fxsdk-gdb-bridge-%03d-%03d.socket",
		fdev->busNumber, fdev->deviceAddress);

	int listen_socket = setup_socket(socket_path);
	if(listen_socket < 0)
		goto end;

	if(!opts.bridge_only) {
		gdb_pid = fork_gdb(gdb_argv, socket_path);
		if(gdb_pid == -1)
			goto end;
		/* Cut fxlink logs to not mix in with gdb's output */
		fxlink_log_set_handler(noop_log_handler);
	}

	int client_socket = accept_gdb(listen_socket);
	if(client_socket < 0)
		goto end;

	struct sigaction action = {
		.sa_handler = SIGINT_SIGCHLD_handler,
	};
	sigaction(SIGINT, &action, NULL);

	/* Track libusb fds as an indirect way to watch the calculator itself */
	struct pollfd client_socket_pollfd = {
		.fd = client_socket, .events = POLLIN
	};
	fxlink_pollfds_track(&fxlink_polled_fds, context);

	while(!interrupted_flag && !gdb_terminated_flag) {
		int err = fxlink_multipoll(-1,
			fxlink_polled_fds.fds, fxlink_polled_fds.count,
			&client_socket_pollfd, 1,
			NULL);
		if (err < 0) {
			perror("poll");
			goto end;
		}

		struct timeval zero = {0};
		libusb_handle_events_timeout(context, &zero);

		struct fxlink_message *msg;
		while((msg = fxlink_device_finish_bulk_IN(fdev)) != NULL) {
			if(!fxlink_message_is_apptype(msg, "gdb", "remote")) {
				hlog("gdb");
				wlog("dropped a message of type %.16s:%.16s\n",
					msg->application, msg->type);
				fxlink_device_start_bulk_IN(fdev);
				continue;
			}
			if(opts.bridge_only)
				printf("CAL> %.*s\n", msg->size, (char *)msg->data);
			ssize_t send_ret = send(client_socket, msg->data, msg->size, 0);
			if(send_ret != msg->size) {
				perror("send");
				goto end;
			}
			fxlink_message_free(msg, true);

			fxlink_device_start_bulk_IN(fdev);
		}

		/* Don't start an OUT transfer if there's one in progress */
		if(fdev->comm->ftransfer_OUT)
			continue;

		int bytes_socket;
		if(ioctl(client_socket, FIONREAD, &bytes_socket) < 0) {
			perror("ioctl");
			goto end;
		}
		if(bytes_socket > 0) {
			char buf[1024];
			ssize_t recv_ret = recv(client_socket, buf, sizeof(buf), 0);
			if(recv_ret < 0) {
				perror("recv");
				goto end;
			}
			if(opts.bridge_only)
				printf("GDB> %.*s\n", (int)recv_ret, buf);
			if(!fxlink_device_start_bulk_OUT(fdev, "gdb", "remote", buf, recv_ret, false)) {
				elog("unable to start bulk OUT transfer\n");
				goto end;
			}
		}
	}

	ret = 0;

end:
	if(gdb_pid != -1)
		waitpid(gdb_pid, NULL, 0);
	if(socket_path[0])
		unlink(socket_path);
	if(fxlink_polled_fds.ctx)
		fxlink_pollfds_stop(&fxlink_polled_fds);
	if(fdev) {
		fxlink_device_interrupt_transfers(fdev);
		fxlink_device_cleanup(fdev);
		free(fdev);
	}
	if(context)
		libusb_exit(context);
	return ret;
}
