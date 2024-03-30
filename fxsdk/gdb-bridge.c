#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <libusb.h>

#include <fxlink/devices.h>
#include <fxlink/filter.h>
#include <fxlink/logging.h>

static struct fxlink_device* setup_calc(libusb_context* context) {
	struct fxlink_filter filter = {
		.intf_fxlink = true,
	};
	delay_t delay = delay_infinite();
	fxlink_filter_clean_libusb(&filter);

	hlog("calculators");
	log_("wating for fxlink capable device...\n");
	struct fxlink_device* dev = fxlink_device_find_wait(context, &filter, &delay);
	if (!dev) {
		elog("unable to open fxlink_device\n");
		return NULL;
	}

	if (!fxlink_device_claim_fxlink(dev)) {
		elog("unable to claim fxlink\n");
		return NULL;
	}

	return dev;
}

static int setup_socket(const char* listen_path) {
	int listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listen_socket < 0) {
		perror("socket");
		return -1;
	}

	struct sockaddr_un listen_address = {
		.sun_family = AF_UNIX,
		.sun_path = {0},
	};
	strncpy(listen_address.sun_path, listen_path, sizeof(listen_address.sun_path) - 1);
	if (bind(listen_socket, (struct sockaddr*)&listen_address, sizeof(listen_address)) < 0) {
		close(listen_socket);
		perror("bind");
		return -1;
	}

	if (listen(listen_socket, 1024) < 0) {
		close(listen_socket);
		perror("listen");
		return -1;
	}

	hlog("socket");
	log_("wating for client on \"%s\"...\n", listen_address.sun_path);
	int client_socket = accept(listen_socket, NULL, NULL);
	if (client_socket < 0) {
		close(listen_socket);
		perror("accept");
		return -1;
	}
	close(listen_socket);
	return client_socket;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		elog("Usage : %s [listen path]\n", argv[0]);
		return -1;
	}

	libusb_context* context = NULL;
	int err = libusb_init(&context);
	if (err != 0) {
		elog_libusb(err, "libusb_init");
		return -1;
	}
	struct fxlink_device* dev = setup_calc(context);
	if (dev == NULL) {
		return -1;
	}
	int client_socket = setup_socket(argv[1]);
	if (client_socket < 0) {
		return -1;
	}

	struct pollfd client_socket_pollfd = {.fd = client_socket, .events = POLLIN};
	struct fxlink_pollfds fxlink_polled_fds;
	fxlink_pollfds_track(&fxlink_polled_fds, context);

	if (!fxlink_device_start_bulk_IN(dev)) {
		elog("unable to start bulk IN trasfer\n");
		return -1;
	}

	// TODO : find a way to properly close socket or USB interface when the other side is disconnected
	int ret = 0;
	while (1) {
		int err = fxlink_multipoll(-1, fxlink_polled_fds.fds, fxlink_polled_fds.count,
					   &client_socket_pollfd, 1,
					   NULL);
		if (err < 0) {
			perror("poll");
			ret = -1;
			break;
		}

		struct timeval zero = {0};
		libusb_handle_events_timeout(context, &zero);
		struct fxlink_message* msg;
		while ((msg = fxlink_device_finish_bulk_IN(dev)) != NULL) {
			if (!fxlink_message_is_apptype(msg, "gdb", "remote")) {
				elog("unknown fxlink message type\n");
				ret = -1;
				break;
			}
			ssize_t send_ret = send(client_socket, msg->data, msg->size, 0);
			if (send_ret != msg->size) {
				perror("send");
				ret = -1;
				break;
			}
			fxlink_message_free(msg, true);

			fxlink_device_start_bulk_IN(dev);
		}

		if (ret != 0) {
			break;
		}

		// We don't want to start a new OUT transfer if another one is still in progress
		if (!dev->comm->ftransfer_OUT) {
			int bytes_socket;
			if (ioctl(client_socket, FIONREAD, &bytes_socket) < 0) {
				perror("ioctl");
				ret = -1;
				break;
			}
			if (bytes_socket > 0) {
				char buf[1024];
				ssize_t recv_ret = recv(client_socket, buf, sizeof(buf), 0);
				if (recv_ret < 0) {
					perror("recv");
					ret = -1;
					break;
				}
				if (!fxlink_device_start_bulk_OUT(dev, "gdb", "remote", buf, recv_ret, false)) {
					elog("unable to start bulk OUT trasfer\n");
					ret = -1;
					break;
				}
			}
		}
	}

	fxlink_pollfds_stop(&fxlink_polled_fds);
	fxlink_device_cleanup(dev);
	free(dev);
	libusb_exit(context);
	return ret;
}
