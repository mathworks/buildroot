#ifndef __ZED_SPI_H__
#define __ZED_SPI_H__

/*
 * SPI testing utility (using spidev driver) based on https://github.com/torvalds/linux/blob/master/tools/spi/spidev_test.c
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

void pabort(const char *s);
void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len);
int SPI_Init();
int SPI_Close(int fd);

static char * spi_device = "/dev/spidev1.0";   // Default for PetaLinux

#endif // __ZED_SPI_H__
