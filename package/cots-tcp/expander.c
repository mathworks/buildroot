/*
 * This is for the NXP PCAL6416A I2C expander on the Otava COTS Transceiver, Rev B
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
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <dirent.h>
#include <stdarg.h>
#include "expander.h"
#include "cots-tcp.h"

int i2c_file;
u8 i2c_device_addr = IIC_EXPANDER_SLAVE_ADDRESS;

static int send_expander_command(int file,
                            unsigned char addr,
                            unsigned char reg,
                            unsigned char data0,
                            unsigned char data1) {

    unsigned char outbuf[3];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;

    /* The first byte indicates which register we'll write */
    outbuf[0] = reg;
    outbuf[1] = data0;
    outbuf[2] = data1;

    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(file, I2C_RDWR, &packets) < 0) {
        perror("Unable to send Expander I2C data");
        return -1;
    }

    return 0;
}

int write_expander(u8 command, u8 value_port0, u8 value_port1)
{
	if (send_expander_command(i2c_file, i2c_device_addr,command, value_port0, value_port1) < 0)
	{
		return -1;
	}
	return 0;
} //write_expander()

int expander_init()
{
    extern const char * I2C_FILE_NAME;
    // Open a connection to the I2C userspace control file.
    if ((i2c_file = open(I2C_FILE_NAME, O_RDWR)) < 0)
    {
    	perror("Unable to open I2C control file");
    	return -1;
    }

    //Write defaults to output registers before enabling them:
    u8 expander_port0_default = 0xAE; //This is the value for max attenuation, i.e. 15.5 dB
	u8 expander_port1_default = 0; //All power rail enables = 0 -> disabled
    if (write_expander(EXPANDER_COMMAND_OUTPUT_PORT0, expander_port0_default, expander_port1_default) < 0)
    {
    	perror("Expander initialization failed");
    	return -1;
    }

    //Set all 16 I/O's as outputs:
    if (write_expander(EXPANDER_COMMAND_CONFIGURATION_PORT0, 0x00, 0x00) < 0) //1 for input (default), 0 for output
    {
    	perror("Expander initialization failed");
    	return -1;
    }
    return 0;
} //expander_init

//Write the latest port shadow status to the I/O expander chip:
int update_expander()
{
	u8 expander_port0_value;
	u8 expander_port1_value;

	if (!expander_OK)
	{ //The expander IC did not ACK the initialize message, so it is not present
		return -1;
	}
	expander_port0_value = expander_tx_DSA_value;
	expander_port1_value =
			((bexpander_EN_5V_TX & 0x01) << 0) |
			((bexpander_EN_5V_RX & 0x01) << 1) |
			((bexpander_EN_3V3_TX & 0x01) << 2) |
			((bexpander_EN_3V3_RX & 0x01) << 3) |
			((bexpander_TESTPIN_27 & 0x01) << 4) |
			((bexpander_TESTPIN_28 & 0x01) << 5) |
			((bexpander_TESTPIN_29 & 0x01) << 6) |
			((bexpander_TESTPIN_30 & 0x01) << 7);

	//Write to the expander:
    if (write_expander(EXPANDER_COMMAND_OUTPUT_PORT0, expander_port0_value, expander_port1_value) < 0)
    {
    	return -1;
    }
   return 0;
}
