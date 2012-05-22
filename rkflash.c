/*-
 * Copyright (c) 2010, 2011 FUKAUMI Naoki.
 * Copyright (c) 2011 Ithamar R. Adema. (added libusb support)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct cmd {
	uint8_t hdr[4];	/* U S B C */
	uint32_t cid;	/* command id */
	uint8_t pad1[4];
	uint8_t flag;
	uint8_t cmd[4];
	uint8_t off[4]; /* 1 sector = 512 byte */
	uint8_t pad2[2];
	uint8_t size;	/* 1 sector = 512 byte */
	uint8_t pad3[7];
} __attribute__ ((packed));

struct ret {
	uint8_t hdr[4];	/* U S B S */
	uint32_t cid;	/* command id */
	uint8_t pad[5];
} __attribute__ ((packed));

#define SETCMD_INIT(c)							\
do {									\
	(c).cid++;							\
	(c).flag = 0x80;						\
	(c).cmd[0] = 0x00;						\
	(c).cmd[1] = 0x06;						\
	(c).cmd[2] = 0x00;						\
	(c).cmd[3] = 0x00;						\
	(c).off[0] = 0x00;						\
	(c).off[1] = 0x00;						\
	(c).off[2] = 0x00;						\
	(c).off[3] = 0x00;						\
	(c).size = 0x00;						\
} while(/* CONSTCOND */0)

#define SETCMD_UNKNOWN(c)							\
do {									\
	(c).cid++;							\
	(c).flag = 0x00;						\
	(c).cmd[0] = 0x00;						\
	(c).cmd[1] = 0x06;						\
	(c).cmd[2] = 0x16;						\
	(c).cmd[3] = 0x00;						\
	(c).off[0] = 0x00;						\
	(c).off[1] = 0x00;						\
	(c).off[2] = 0x00;						\
	(c).off[3] = 0x00;						\
	(c).size = 0x00;						\
} while(/* CONSTCOND */0)

#define SETCMD_BOOT(c)							\
do {									\
	(c).cid++;							\
	(c).flag = 0x00;						\
	(c).cmd[0] = 0x00;						\
	(c).cmd[1] = 0x06;						\
	(c).cmd[2] = 0xff;						\
	(c).cmd[3] = 0x00;						\
	(c).off[0] = 0x00;						\
	(c).off[1] = 0x00;						\
	(c).off[2] = 0x00;						\
	(c).off[3] = 0x00;						\
	(c).size = 0x00;						\
} while(/* CONSTCOND */0)

#define SETCMD_WRITE(c, off)						\
do {									\
	(c).cid++;							\
	(c).flag = 0x00;						\
	(c).cmd[0] = 0x00;						\
	(c).cmd[1] = 0x0a;						\
	(c).cmd[2] = 0x15;						\
	(c).cmd[3] = 0x00;						\
	(c).off[0] = ((off) >> 24) & 0xff;				\
	(c).off[1] = ((off) >> 16) & 0xff;				\
	(c).off[2] = ((off) >>  8) & 0xff;				\
	(c).off[3] = ((off) >>  0) & 0xff;				\
	(c).size = 0x20;						\
} while(/* CONSTCOND */0)

#define SETCMD_READ(c, off)						\
do {									\
	(c).cid++;							\
	(c).flag = 0x80;						\
	(c).cmd[0] = 0x00;						\
	(c).cmd[1] = 0x0a;						\
	(c).cmd[2] = 0x14;						\
	(c).cmd[3] = 0x00;						\
	(c).off[0] = ((off) >> 24) & 0xff;				\
	(c).off[1] = ((off) >> 16) & 0xff;				\
	(c).off[2] = ((off) >>  8) & 0xff;				\
	(c).off[3] = ((off) >>  0) & 0xff;				\
	(c).size = 0x20;						\
} while(/* CONSTCOND */0)

static int verbose = 0;

#ifdef HAS_LIBUSB
#include <usb.h>

static usb_dev_handle *xsv_handle;

static int
bulk_init(const char *device)
{
	struct usb_bus *bus;
	struct usb_device *dev;


	usb_init();

	if (verbose)
		usb_set_debug(verbose);

	usb_find_busses();
	usb_find_devices();

	xsv_handle = NULL;
	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (device && strcmp(dev->filename, device) != 0)
				continue;

			if (dev->descriptor.idVendor == 0x2207 &&
				(dev->descriptor.idProduct == 0x290a ||
				dev->descriptor.idProduct == 0x281a)) {
				xsv_handle = usb_open(dev);
				if (verbose) {
					printf("using rockchip device @ %s\n",
						dev->filename);
				}
				break;
			}
		}
	}

	if (xsv_handle) {
		usb_set_configuration(xsv_handle,1);
		usb_claim_interface(xsv_handle,0);
		usb_set_altinterface(xsv_handle,0);

		return 0;
	} else {
		return -1;
	}
}

#define BULK_SEND_EP2(buf, size)	usb_bulk_write(xsv_handle, 2, (void*)buf, size, 500)
#define BULK_RECV_EP1(buf, size)	usb_bulk_read(xsv_handle, 1, (void*)buf, size, 500)
#else
static int fd_in, fd_out;

static int
bulk_init(const char *device)
{
	if (device == NULL) {
		fd_in = STDIN_FILENO;
		fd_out = STDOUT_FILENO;
	} else
		fd_in = fd_out = open(device, O_RDWR);

	return 0;
}

#define BULK_SEND_EP2(buf, size)	write(fd_out, (buf), (size))
#define BULK_RECV_EP1(buf, size)	read(fd_in, (buf), (size))
#endif

#define _BLOCKSIZE	(16 * 1024)

static char *progname;

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-D device] [-v] [-B] [-r size] offset file\n", progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct cmd c;
	struct ret r;
	uint32_t off;
	int32_t size;
	uint8_t *buf;
	int ch, img, boot;
	const char *device = NULL;
	int res;

	progname = argv[0];

	boot = 0;
	size = 0;
	while ((ch = getopt(argc, argv, "vBr:D:")) != -1) {
		switch (ch) {
		case 'v':
			verbose++;
			break;
		case 'B':
			boot = 1;
			break;
		case 'D':
			device = strdup(optarg);
			break;
		case 'r':
			size = strtoul(optarg, NULL, 0);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (bulk_init(device) != 0)
		errx(EXIT_FAILURE, "Failure finding device\n");

	if (argc != 2)
		usage();

	if ((off = strtoul(argv[0], NULL, 0)) & 0x1f)
		errx(EXIT_FAILURE, "offset must be multiple of 0x20\n");

	if (size == 0)
		img = open(argv[1], O_RDONLY);
	else
		img = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (img == -1)
		err(EXIT_FAILURE, "%s", argv[1]);

	if ((buf = malloc(_BLOCKSIZE)) == NULL)
		err(EXIT_FAILURE, "malloc");

	memset(buf, 0, _BLOCKSIZE);

	memset(&c, 0, sizeof(c));
	c.hdr[0] = 'U';
	c.hdr[1] = 'S';
	c.hdr[2] = 'B';
	c.hdr[3] = 'C';

	do {
		SETCMD_INIT(c);
		BULK_SEND_EP2(&c, sizeof(c));
		res = BULK_RECV_EP1(&r, sizeof(r));
	} while(res < 0);

	usleep(20 * 1000);
#if 0
	SETCMD_UNKNOWN(c);
	BULK_SEND_EP2(&c, sizeof(c));
	BULK_RECV_EP1(&r, sizeof(r));

	usleep(40 * 1000);
#endif
	if (size == 0) {
		while (read(img, buf, _BLOCKSIZE) > 0) {
			if (verbose)
				fprintf(stderr, "writing offset 0x%08x\n", off);

			SETCMD_WRITE(c, off);
			BULK_SEND_EP2(&c, sizeof(c));

			BULK_SEND_EP2(buf, _BLOCKSIZE);

			if (BULK_RECV_EP1(&r, sizeof(r)) < 0)
				fprintf(stderr, "error writing offset 0x%08x\n", off);

			off += 0x20;
			memset(buf, 0, _BLOCKSIZE);
		}
	} else {
		while (size > 0) {
			if (verbose)
				fprintf(stderr, "reading offset 0x%08x\n", off);

			SETCMD_READ(c, off);
			BULK_SEND_EP2(&c, sizeof(c));

			BULK_RECV_EP1(buf, _BLOCKSIZE);

			if (BULK_RECV_EP1(&r, sizeof(r)) < 0)
				fprintf(stderr, "error reading offset 0x%08x\n", off);

			write(img, buf, _BLOCKSIZE);

			off += 0x20;
			size -= 0x20;
		}
	}

	if (boot) {
		fprintf(stderr, "booting\n");

		usleep(20 * 1000);

		SETCMD_BOOT(c);
		BULK_SEND_EP2(&c, sizeof(c));
		BULK_RECV_EP1(&r, sizeof(r));
	}

	free(buf);
	close(img);

	return EXIT_SUCCESS;
}
