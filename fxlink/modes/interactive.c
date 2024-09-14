//---------------------------------------------------------------------------//
// ==>/[_]\   fxlink: A community communication tool for CASIO calculators.  //
//    |:::|   Made by Lephe' as part of the fxSDK.                           //
//    \___/   License: MIT <https://opensource.org/licenses/MIT>             //
//---------------------------------------------------------------------------//

#include "../fxlink.h"
#include <fxlink/filter.h>
#include <fxlink/logging.h>
#include <fxlink/protocol.h>
#include <fxlink/devices.h>
#include <fxlink/tooling/libpng.h>
#include <fxlink/tooling/sdl2.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void handle_new_message(struct fxlink_device *fdev,
	struct fxlink_message *msg,
	int socket_fd,
	const struct sockaddr_un *sockout_addr)
{
	char const *path = ".";

	if(fxlink_message_is_fxlink_image(msg)) {
		struct fxlink_message_image_header *img = msg->data;
		char *filename = fxlink_gen_file_name(path, msg->type, ".png");

		struct fxlink_message_image_raw *raw =
			fxlink_message_image_decode(msg);
		if(raw) {
			fxlink_libpng_save_raw(raw, filename);
			fxlink_message_image_raw_free(raw);
			hlog("calculators %s", fxlink_device_id(fdev));
			log_("saved image (%dx%d, format=%d) to '%s'\n",
				img->width, img->height, img->pixel_format, filename);
		}
		free(filename);
		return;
	}

	if(fxlink_message_is_fxlink_text(msg)) {
		char const *str = msg->data;

		if(socket_fd >= 0 && sockout_addr != NULL &&
		   sendto(socket_fd, msg->data, msg->size, 0, sockout_addr, sizeof(*sockout_addr)) < 0) {
			perror("sendto");
		}

		if(options.verbose)
			printf("------------------\n");
		fwrite(str, 1, msg->size, stdout);
#if 0
		if(str[msg->size - 1] != '\n') {
			if(!options.verbose)
				printf("\e[30;47m%%\e[0m");
			printf("\n");
		}
#endif
		if(options.verbose) {
			printf("------------------\n");
		}
		if(options.verbose) {
			for(size_t i = 0; i < msg->size; i++) {
				printf(" %02x", str[i]);
				if((i & 15) == 15 || i == msg->size - 1)
					printf("\n");
			}
		}

		if(options.log_file)
			fwrite(str, 1, msg->size, options.log_file);
		return;
	}

	if(fxlink_message_is_fxlink_video(msg)) {
		struct fxlink_message_image_raw *raw =
			fxlink_message_image_decode(msg);
		if(raw) {
			fxlink_sdl2_display_raw(raw);
			fxlink_message_image_raw_free(raw);
		}
		return;
	}

    /* Default to saving to a blob */
    static char combined_type[48];
    sprintf(combined_type, "%.16s-%.16s", msg->application, msg->type);

    char *filename = fxlink_gen_file_name(path, combined_type, ".bin");
    FILE *fp = fopen(filename, "wb");
    if(!fp) {
        elog("could not save to '%s': %m\n", filename);
        return;
    }

    fwrite(msg->data, 1, msg->size, fp);
    fclose(fp);

    log_("saved blob to '%s'\n", filename);
    free(filename);
}

static int setup_socket(const struct sockaddr_un *sockin_addr,
	const struct sockaddr_un *sockout_addr)
{
	if(sockin_addr == NULL && sockout_addr == NULL) {
		/* -1 is used to indicate that no fd were opened but it's not a fatal error */
		return -1;
	}

	int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(socket_fd < 0) {
		perror("socket");
		/* -2 is used to indicate an error */
		return -2;
	}

	if(sockin_addr != NULL) {
		unlink(sockin_addr->sun_path);

		if (bind(socket_fd, (struct sockaddr*)sockin_addr, sizeof(*sockin_addr)) < 0) {
			perror("bind");
			close(socket_fd);
			/* -2 is used to indicate an error */
			return -2;
		}
	}

	return socket_fd;
}

static void handle_input_socket(struct fxlink_device *fdev, int socket_fd)
{
	if(socket_fd < 0) {
		return;
	}

	/* Don't start an OUT transfer if there's one in progress */
	if(fdev->comm->ftransfer_OUT) {
		return;
	}

	int dgram_len;
	if(ioctl(socket_fd, FIONREAD, &dgram_len) < 0) {
		perror("ioctl");
		return;
	}
	if(dgram_len <= 0) {
		return;
	}

	/* We need to use a static buffer as libfxlink will use it in it's main loop,
	 * outside of this stack frame
	 */
	static uint8_t* dgram_buf = NULL;
	static ssize_t dgram_buf_len = 0;
	if (dgram_buf_len < dgram_len) {
		dgram_buf = realloc(dgram_buf, dgram_len);
		dgram_buf_len = dgram_len;
	}

	ssize_t recv_ret = recvfrom(socket_fd, dgram_buf, dgram_buf_len, 0, NULL, NULL);
	if(recv_ret < 0) {
		perror("recvfrom");
	} else if(recv_ret > 0) {
		fxlink_device_start_bulk_OUT(fdev, "fxlink", "text", dgram_buf, recv_ret, false);
	}
}

int main_interactive(struct fxlink_filter *filter, delay_t *delay,
	libusb_context *ctx,
	const struct sockaddr_un *sockin_addr,
	const struct sockaddr_un *sockout_addr)
{
	/* Wait for a device to be connected */
	fxlink_filter_clean_libusb(filter);
	filter->intf_fxlink = true;

	struct fxlink_device *fdev = fxlink_device_find_wait(ctx, filter, delay);
	if(!fdev) {
		printf("No device found.\n");
		return 1;
	}
	if(!fxlink_device_claim_fxlink(fdev)) {
		fxlink_device_cleanup(fdev);
		free(fdev);
		return 1;
	}

	/* socket used by sockin and sockout options */
	int socket_fd = setup_socket(sockin_addr, sockout_addr);
	if(socket_fd <= -2) {
		fxlink_device_cleanup(fdev);
		free(fdev);
		return 1;
	}

	hlog("interactive");
	log_("connected to %s\n", fxlink_device_id(fdev));

	/* Buffer used to receive messages */
	static uint8_t buffer[2048];
	/* Current message */
	struct fxlink_transfer *tr = NULL;

	while(1) {
		fxlink_sdl2_handle_events();

		handle_input_socket(fdev, socket_fd);

		int transferred = -1;
		int rc = libusb_bulk_transfer(fdev->dh, fdev->comm->ep_bulk_IN, buffer,
			sizeof buffer, &transferred, 500 /* ms */);

		if(rc == LIBUSB_ERROR_NO_DEVICE) {
			hlog("interactive");
			log_("disconnected, leaving\n");
			break;
		}
		else if(rc && rc != LIBUSB_ERROR_TIMEOUT) {
			elog_libusb(rc, "bulk transfer failed on %s",
				fxlink_device_id(fdev));
			continue;
		}
		if(transferred <= 0)
			continue;

		/* Either start a new message or continue an unfinished one */
		if(tr == NULL)
			tr = fxlink_transfer_make_IN(buffer, transferred);
		else
			fxlink_transfer_receive(tr, buffer, transferred);

		if(tr && fxlink_transfer_complete(tr)) {
			struct fxlink_message *msg = fxlink_transfer_finish_IN(tr);
			if(msg) {
				handle_new_message(fdev, msg, socket_fd, sockout_addr);
				fxlink_message_free(msg, true);
			}
			tr = NULL;
		}
	}

	/* Warning for unfinished transfer */
	if(tr) {
		wlog("unfinished transfer interrupted by disconnection\n");
		fxlink_transfer_free(tr);
	}

	if(socket_fd >= 0) {
		close(socket_fd);
	}

	fxlink_device_cleanup(fdev);
	free(fdev);
	return 0;
}
