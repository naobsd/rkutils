/*-
 * Copyright (c) 2011 FUKAUMI Naoki.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rkcrc.h"

#define T_CRAMFS	0
#define T_KERNEL	1
#define T_PARAMETER	2
#define T_UPDATE_AF	3
#define T_UPDATE_FW	4
#define T_ANDROID	5
#define T_EXTFS		6

#define _BLOCKSIZE	(16 * 1024)

int
main(int argc, char *argv[])
{
	ssize_t nr;
	uint32_t crc, ocrc, size;
	uint8_t *buf, *end;
	int in, out, type;

	if (argc != 3) {
		fprintf(stderr, "usage: %s infile outfile\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((in = open(argv[1], O_RDONLY)) == -1)
		err(EXIT_FAILURE, "%s", argv[1]);

	if ((buf = malloc(_BLOCKSIZE)) == NULL)
		err(EXIT_FAILURE, "malloc");

	if ((nr = read(in, buf, _BLOCKSIZE)) == -1 || nr == 0)
		err(EXIT_FAILURE, "read");

	if (buf[0] == 0x45 && buf[1] == 0x3d &&
	    buf[2] == 0xcd && buf[3] == 0x28) {
		printf("cramfs");
		type = T_CRAMFS;
	} else if (buf[0] == 'K' && buf[1] == 'R' &&
	    buf[2] == 'N' && buf[3] == 'L') {
		printf("kernel.img");
		type = T_KERNEL;
	} else if (buf[0] == 'P' && buf[1] == 'A' &&
	    buf[2] == 'R' && buf[3] == 'M') {
		printf("parameter");
		type = T_PARAMETER;
	} else if (buf[0] == 'R' && buf[1] == 'K' &&
	    buf[2] == 'A' && buf[3] == 'F') {
		printf("update.img");
		type = T_UPDATE_AF;
	} else if (strncmp((void*)buf, "ANDROID!", 8) == 0) {
		printf("android");
		type = T_ANDROID;
	} else {
		fprintf(stderr, "unknown image (%02x %02x %02x %02x)\n",
		    buf[0], buf[1], buf[2], buf[3]);
		exit(EXIT_FAILURE);
	}

	size = buf[4] | buf[5] << 8 | buf[6] << 16 | buf[7] << 24;
	if (type == T_KERNEL || type == T_PARAMETER)
		size += 8;
	else if (type == T_ANDROID) {
		unsigned pgsz, pages;
		pgsz = buf[36] | buf[37] << 8 | buf[38] << 16 | buf[39] << 24;
		pages = 1 /* header */ +
		    ((buf[8] | buf[9] << 8 | buf[10] << 16 |
		        buf[11] << 24) + pgsz - 1) / pgsz + /* kernel */
		    ((buf[16] | buf[17] << 8 | buf[18] << 16 |
		        buf[19] << 24) + pgsz - 1) / pgsz + /* ramdisk */
		    ((buf[24] | buf[25] << 8 | buf[26] << 16 |
		        buf[27] << 24) + pgsz - 1) / pgsz; /* second */
		size = pages * pgsz;
	}

	printf(" found (%u bytes)\n", size);

	if ((out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
		err(EXIT_FAILURE, "%s", argv[2]);

	printf("dumping...\n");

	if (size < (uint32_t)nr)
		nr = size;

	crc = 0;
	if (type == T_CRAMFS || type == T_UPDATE_AF)
		RKCRC(crc, buf, nr);
	else
		RKCRC(crc, &buf[8], nr - 8);
	if (type != T_PARAMETER)
		write(out, buf, nr);
	else
		write(out, &buf[8], nr - 8);
	size -= nr;

	while (size > 0) {
		if ((nr = read(in, buf, _BLOCKSIZE)) == -1 || nr == 0)
			break;

		if (size < (uint32_t)nr)
			nr = size;

		RKCRC(crc, buf, nr);
		write(out, buf, nr);
		size -= nr;
	}

	if (nr == _BLOCKSIZE) {
		if ((nr = read(in, buf, _BLOCKSIZE)) == -1 || nr == 0)
			goto end;
		nr = 0;
	}

	if (type != T_ANDROID) {
		ocrc = buf[nr] | buf[nr + 1] << 8 |
		    buf[nr + 2] << 16 | buf[nr + 3] << 24;

		if (crc != ocrc)
			goto end;

		printf("crc found (4 bytes, 0x%08x)\n", ocrc);
		if (type != T_PARAMETER)
			write(out, &buf[nr], 4);

		nr += 4;
	}

	if (type == T_KERNEL) {
		if (nr != _BLOCKSIZE) {
			if ((end = memchr(&buf[nr], '\0', _BLOCKSIZE - nr))
			    == NULL)
				write(out, &buf[nr], _BLOCKSIZE - nr);
			else {
				write(out, &buf[nr], end - &buf[nr]);
				goto end;
			}
		}

		while ((nr = read(in, buf, _BLOCKSIZE)) != -1 && nr != 0) {
			if ((end = memchr(buf, '\0', nr)) == NULL)
				write(out, buf, nr);
			else {
				write(out, buf, end - buf);
				goto end;
			}
		}
	}

 end:
	printf("done\n");

	close(out);
	free(buf);
	close(in);

	return EXIT_SUCCESS;
}
