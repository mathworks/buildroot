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

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <dirent.h>
#include <stdarg.h>
#include "eeprom.h"
#include "cots-tcp.h"


int zed_iic_eeprom_init(iic_eeprom_demo_t *pEeprom)
{
	//cots_eeprom.uBaseAddr_IIC_RTC        = XPAR_AXI_IIC_0_BASEADDR;
    int ret;

   /* IIC initialization for communicating with the EEPROM slave hardware.
    */
   printf("I2C EEPROM Initialization ...\n\r");
   ret = zed_iic_axi_init(&(pEeprom->eeprom_iic),"EEPROM I2C", pEeprom->uBaseAddr_IIC_RTC);
   if (ret)
   {
	  printf("Opened AXI IIC EEPROM device driver\n\r");
   }
   else
   {
      printf("ERROR : Failed to open AXI IIC device driver\n\r");
      return -1;
   }

   return 0;
}

int write_eeprom(iic_eeprom_demo_t *pEeprom, int8u addr, int8u data)
{
	int ret;
	int8u ChipAddress = IIC_EEPROM_SLAVE_ADDRESS;
	int8u RegAddress  = addr;
	int8u WriteData[10];
	WriteData[0] = data;
	int8u ByteCount   = 1;
	ret = pEeprom->eeprom_iic.fpIicWrite(&(pEeprom->eeprom_iic), ChipAddress, RegAddress, WriteData, ByteCount);

	if (ret != (ByteCount + 1))
	{
		printf("ERROR : Failed to Write to EEPROM\n\r");
		return -1;
	}
	/* Wait while write completes. */
	usleep(50000);
	return 0;
} //write_eeprom()

int read_eeprom(iic_eeprom_demo_t *pEeprom, int8u addr, unsigned char* data)
{
	int ret;
	int8u ChipAddress = IIC_EEPROM_SLAVE_ADDRESS;
	int8u RegAddress  = addr;
	int8u ReadData[10];
	int8u ByteCount   = 1;
	ret = pEeprom->eeprom_iic.fpIicRead(&(pEeprom->eeprom_iic), ChipAddress, RegAddress, ReadData, ByteCount + 1);

	if (!ret)
	{
		printf("ERROR : Failed to Read from IIC EEPROM\n\r");
		return -1;
	}
	printf("Read from EEPROM address %d : 0x%02X\n\r", RegAddress, ReadData[0]);
	data[0] = ReadData[0];
	return 0;
} //read_eeprom()

