/*
 * mtd - simple memory technology device manipulation tool
 *
 * Copyright (C) 2005      Waldemar Brodkorb <wbx@dass-it.de>,
 * Copyright (C) 2005-2009 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License v2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The code is based on the linux-mtd examples.
 */

#define _GNU_SOURCE
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <mtd/mtd-user.h>
#include "mtd.h"

int mtdsize = 0;
int erasesize = 0;
char * mtd_device="/dev/mtd3";

int mtd_open(const char *mtd, bool block)
{
	FILE *fp;
	char dev[PATH_MAX];
	int i;
	int ret;
	int flags = O_RDWR | O_SYNC;
	char name[PATH_MAX];

	snprintf(name, sizeof(name), "\"%s\"", mtd);
	if ((fp = fopen("/proc/mtd", "r"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, name)) {
				snprintf(dev, sizeof(dev), "/dev/mtd%s/%d", (block ? "block" : ""), i);
				if ((ret=open(dev, flags))<0) {
					snprintf(dev, sizeof(dev), "/dev/mtd%s%d", (block ? "block" : ""), i);
					ret=open(dev, flags);
				}
				fclose(fp);
				return ret;
			}
		}
		fclose(fp);
	}

	return open(mtd, flags);
}

int mtd_check_open(const char *mtd)
{
	struct mtd_info_user mtdInfo;
	int fd;

	fd = mtd_open(mtd, false);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		return -1;
	}

	if(ioctl(fd, MEMGETINFO, &mtdInfo)) {
		fprintf(stderr, "Could not get MTD device info from %s\n", mtd);
		close(fd);
		return -1;
	}
	mtdsize = mtdInfo.size;
	erasesize = mtdInfo.erasesize;

	return fd;
}

int mtd_erase_block(int fd, int offset)
{
	struct erase_info_user mtdEraseInfo;

	mtdEraseInfo.start = offset;
	mtdEraseInfo.length = erasesize;
	ioctl(fd, MEMUNLOCK, &mtdEraseInfo);
	if (ioctl (fd, MEMERASE, &mtdEraseInfo) < 0)
	  return -1;

	return 0;
}

static int mtd_check(const char *mtd) {
	int fd;

	fd = mtd_check_open(mtd);
	if (fd < 0)
	  return 0;

	close(fd);
	return 1;
}

static int
mtd_unlock(const char *mtd)
{
	struct erase_info_user mtdLockInfo;
	int fd;

	fd = mtd_check_open(mtd);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		return -1;
	}

	fprintf(stderr, "Unlocking %s ...\n", mtd);

	mtdLockInfo.start = 0;
	mtdLockInfo.length = mtdsize;
	ioctl(fd, MEMUNLOCK, &mtdLockInfo);
	close(fd);

	return 0;
}

static int
mtd_dump(const char *mtd, int offset, int size,char* data)
{
	int fd;

	if (!size)
		return -1;
	fprintf(stderr, "Dumping %s ...\n", mtd);

	fd = mtd_check_open(mtd);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		return -1;
	}

	if (offset)
	  lseek(fd, offset, SEEK_SET);

	int rlen = read(fd, data, size);

	close(fd);
	return rlen;
}


static void
indicate_writing(const char *mtd)
{
	fprintf(stderr, "\nWriting to %s ... ", mtd);

	fprintf(stderr, " [ ]");
}

static int
mtd_write(const char* data, int len, const char *mtd, size_t offset)
{
	int fd, result;

	fd = mtd_check_open(mtd);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		return -1;
	}
	char *buf = malloc(erasesize);
	int rlen = read(fd, buf, erasesize);
	if(rlen < 0)
	  return -1;

	indicate_writing(mtd);

	lseek(fd, 0, SEEK_SET);

	if (mtd_erase_block(fd, 0) < 0) {
		fprintf(stderr, "Failed to erase block\n");
		close(fd);
		return -1;
	}

	fprintf(stderr, "\b\b\b[w]");

	memcpy(buf + offset, data, len);

	if ((result = write(fd, buf, erasesize)) < erasesize) {
		if (result < 0) {
			fprintf(stderr, "Error writing image.\n");
			close(fd);
			return -1;
		} else {
			fprintf(stderr, "Insufficient space.\n");
			close(fd);
			return result;
		}
	}

	fprintf(stderr, "\b\b\b\b    ");

	fprintf(stderr, "\n");

	close(fd);
	return result;
}

/* CMD_READ ret read length
* CMD_WRITE ret write length
 */
int mtd_operation(int cmd, char* data, int len, size_t offset)
{
	char *device = mtd_device;
	int ret = 0;


	if (!mtd_check(device)) {
		fprintf(stderr, "Can't open device for writing!\n");
		return -1;
	}

	sync();

	switch (cmd) {
		case CMD_READ:
			ret = mtd_dump(device, offset, len, data);
			break;
		case CMD_WRITE:
			mtd_unlock(device);
			ret = mtd_write(data, len,device, offset);
			break;
	}

	sync();

	return ret;
}
