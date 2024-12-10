/*
 * This file is used to do spi-flash speed test
 *
 * Copyright (c) 2017, Shanghai Siflower Communication Tenology Co.,Ltd.
 *     Qi Zhang <qi.zhang@siflower.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>

/*
 * size: in unit of byte
 *
 * speed = size / (end - start)
 * return speed in unit of KB/s
 */
static double calculate_speed(size_t size, struct timeval start, struct timeval end)
{
	unsigned int delta_us;
	double speed;

	delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
	speed = ((double)size / 1024) / ((double)delta_us / 1000000);

	return speed;
}

int main(int argc, char *argv[])
{
	size_t size, ret;
	int file, i;
	unsigned char *buffer;
	struct timeval start, end;
	double write_speed, read_speed;

	if (argc < 2) {
		printf("please pass in total space avaible in unit of MB\n");
		return -1;
	}

	/* use half of the free space */
	size = (atoi(argv[1]) / 2) * 1024 * 1024;
	buffer = malloc(size);
	if (!buffer) {
		printf("alloc memory failed!\n");
		return -1;
	}
	for (i = 0; i < size; i++)
		buffer[i] = rand() & 0xff;

	/* write speed test */
	file = open(argv[2], O_RDWR | O_CREAT,
		    S_IRUSR | S_IWUSR | S_IXUSR);
	if (file == -1) {
		printf("open %s failed!\n", argv[2]);
		return -1;
	}

	gettimeofday(&start, NULL);
	ret = write(file, buffer, size);
	gettimeofday(&end, NULL);
	if (ret < 0) {
		printf("write failed!\n");
		return ret;
	}
	write_speed = calculate_speed(ret, start, end);
	printf("flash write speed = %fKB/s\n", write_speed);
	close(file);

	/* read speed test */
	file = open(argv[3], O_RDONLY);
	if (file == -1) {
		printf("open %s failed!\n", argv[3]);
		return -1;
	}

	gettimeofday(&start, NULL);
	ret = read(file, buffer, size);
	gettimeofday(&end, NULL);
	if (ret < 0) {
		printf("write failed!\n");
		return ret;
	}
	read_speed = calculate_speed(ret, start, end);
	printf("flash read speed = %fKB/s\n", read_speed);
	close(file);

	return 0;
}
