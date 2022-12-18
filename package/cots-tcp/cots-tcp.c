//----------------------------------------------------------------------------
// SPDX short identifier: BSD-3-Clause
// Copyright (c) 2022, Avnet
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions a
//
// * Redistributions of source code must retain the above copyright not
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   this list of conditions and the following disclaimer in the docume
//   and/or other materials provided with the distribution
//
// * Neither the name of Avnet, Inc. nor the names of its
//   contributors may be used to endorse or promote products derived fr
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR P
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQU
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GO
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) H
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT L
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT O
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

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
#include "lmx_pll.h"
#include "cots-tcp.h"
#include "eeprom.h"
#include "expander.h"
#include "zed-spi.h"

#define PS_VERSION_MSB 2
#define PS_VERSION_LSB 27
#define INITCFG_JSON_FILENAME_DEFAULT "dtrx2_init.cfg"

/* --- Start Functionality for JSON messages --- */
#include "cJSON.h"
#define MAX_OTAVA_CMD_LENGTH 512
char json_cmd_string[MAX_OTAVA_CMD_LENGTH];
#define UNDKNOWN_STATUS_STRING "-1"
/* --- End Functionality for JSON messages --- */

/* --- Start Functionality for TCP control --- */
#include <sys/socket.h>
#include <netinet/in.h>

int tcpServerInit(struct sockaddr_in * otava_address, unsigned tcpPort);
int waitClientConnect(int socket_fd, struct sockaddr_in * otava_address);
void closeSocket(int new_socket);
int getTcpCmdString(int new_socket, char ** cmd_type, char ** cmd_value);
int ReadEeprom_Multiple(unsigned char ucEeprom_addr, unsigned char * ucEeprom_value, int len);

//#define MAX_OTAVA_CMD_LENGTH 256

int new_socket;
static struct sockaddr_in otava_address;
static char * cmd_type = NULL;
static char * cmd_value = NULL;
static int tcp_mode = 0;
static int blink_LED = 1;
char stringtosend[512];
char host_cmd[32];
char host_setting[32];
char host_setting1[32];
char host_setting2[32];
int  host_value_int;
int  host_value_int1;
int  host_value_int2;
unsigned long host_value_long;
unsigned char * host_value_ucptr;
int  host_value_len;
float  host_value_float = -1;
float  host_value_float2 = -1;
float  host_value_float3 = -1;
char host_value_string[32];
int  host_value_string_len;
char host_value_string_year[3];
char host_value_string_month[3];
char host_value_string_day[3];
int host_addr;
/* --- End Functionality for TCP control --- */
int spi_fd;
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define false 0
#define true 1
#define BUFFER_SIZE_MAX		128
// The following variables are used to read and write to the  Spi device, they are global to avoid having large buffers on the stack.
unsigned char ReadBuffer[BUFFER_SIZE_MAX];
unsigned char WriteBuffer[BUFFER_SIZE_MAX];
unsigned char data_tx_array[BUFFER_SIZE_MAX];
unsigned char data_rx_array[BUFFER_SIZE_MAX];
unsigned char ucPmod0TestpinMask;
unsigned char ucPmod1TestpinMask;
unsigned char ucPmod0Status;
unsigned char ucPmod1Status;

//For Otava:
#define CMD_GET_VERSION			0x01
#define CMD_GET_FEEDBACK		0x02
#define CMD_SET_COMMS_LED		0x03
#define CMD_SET_TX_DSAS			0x04
#define CMD_SET_CH1_RX_DSA		0x05
#define CMD_SET_CH2_RX_DSA		0x06
#define CMD_SET_RFSA3713		0x07
#define CMD_ACCESS_TX_PLL		0x08
#define CMD_ACCESS_RX_PLL		0x09
#define CMD_SET_5V_TX_EN		0x0A
#define CMD_SET_3V3_TX_EN		0x0B
#define CMD_SET_5V_RX_EN		0x0C
#define CMD_SET_3V3_RX_EN		0x0D
#define CMD_SET_TX_CH1_AMP_EN	0x0E
#define CMD_SET_TX_CH2_AMP_EN	0x0F
#define CMD_SET_RX_CH1_AMP_EN	0x10
#define CMD_SET_RX_CH2_AMP_EN	0x11
#define CMD_SET_RX_CH1_BIAS_EN	0x12
#define CMD_SET_RX_CH2_BIAS_EN	0x13
#define CMD_SET_PMOD0_PINS		0x14
#define CMD_SET_PMOD1_PINS		0x15
#define CMD_SET_PMOD0_SOFT_EN	0x16
#define CMD_SET_PMOD1_SOFT_EN	0x17

#define CHIP_ID					0x20

#define Ch1TxAttenuator_DEFAULT		127
#define Ch1RxAttenuator_DEFAULT		0
#define Ch1DpdAttenuator_DEFAULT	127
#define Ch1TxPaEnable_DEFAULT		1
#define Ch1TxLnaDisable_DEFAULT		0
#define Ch1RxLna0Bypass_DEFAULT		0
#define Ch1RxLna1Bypass_DEFAULT		0
#define Ch1RxLna0Disable_DEFAULT	0
#define Ch1RxLna0Enable_DEFAULT		1
#define Ch1RxLna1Disable_DEFAULT	0

#define Ch2TxAttenuator_DEFAULT		127
#define Ch2RxAttenuator_DEFAULT		0
#define Ch2DpdAttenuator_DEFAULT	127
#define Ch2TxPaEnable_DEFAULT		1
#define Ch2TxLnaDisable_DEFAULT		0
#define Ch2RxLna0Bypass_DEFAULT		0
#define Ch2RxLna1Bypass_DEFAULT		0
#define Ch2RxLna0Disable_DEFAULT	0
#define Ch2RxLna0Enable_DEFAULT		1
#define Ch2RxLna1Disable_DEFAULT	0

unsigned char PlVersionMSB, PlVersionLSB;
int iattenuator_addr;
int iattenuator_data;
int iTxAttenuator_value;
int iRxCh1Attenuator_value;
int iRxCh2Attenuator_value;
int iIfRxCh1Attenuator_value;
int iIfRxCh2Attenuator_value;
int iPLL_value;
int ucPLL_register;
int iTx5VEnable;
int iTx3V3Enable;
int iRx5VEnable;
int iRx3V3Enable;
int iTxCh1AmpEnable;
int iTxCh2AmpEnable;
int iRxCh1AmpEnable;
int iRxCh2AmpEnable;
int iRxCh1BiasEnable;
int iRxCh2BiasEnable;
int iSoftwareLedState;
float fAttenuator_dB;
int iconfigfile_error;
int iTcpConnected;

int bTxAttenuators_set = false;
int bRxCh1Attenuator_set = false;
int bRxCh2Attenuator_set = false;
int bIfRxCh1Attenuator_set = false;
int bIfRxCh2Attenuator_set = false;
int bTxPLL_set = false;
int bRxPLL_set = false;
int bTx5VEnable_set = false;
int bTx3V3Enable_set = false;
int bRx5VEnable_set = false;
int bRx3V3Enable_set = false;
int bTxCh1AmpEnable_set = false;
int bTxCh2AmpEnable_set = false;
int bRxCh1AmpEnable_set = false;
int bRxCh2AmpEnable_set = false;
int bRxCh1BiasEnable_set = false;
int bRxCh2BiasEnable_set = false;
int bSoftwareLed_set = false;

/* Global shared with expander.c */
const char * I2C_FILE_NAME = "/dev/i2c-22";  // Default for Petalinux 2020.2 ZCU208 DTRX bsp

// These are used to allow a cmd line option for the file path search
static unsigned useCmdLinePath = 0;
static const char * rootCfgPath = "";

void string_to_host( char* stringtosend)
{
	int ilen = strlen(stringtosend);
	int n = write(new_socket,stringtosend,ilen);
				     if (n < 0) printf("ERROR writing to socket");
} //string_to_host()

int tcp_printf(const char *fmt, ...)
{
	if (iTcpConnected == false)
	{
		return false;
	}
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    int rc = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    string_to_host(buffer);
    return rc;
} //tcp_printf

int WriteCommsLedViaSpi(int fd, int ivalue)
{
    int index = 0;

	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = CMD_SET_COMMS_LED;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 1; //Length
   	WriteBuffer[index++] = ivalue;
   	WriteBuffer[index++] = 0xFF; //Processing byte
   	transfer(fd, WriteBuffer, ReadBuffer, index);
   	//transfer_mybuf(fd, 8); //Use this if you want a verbose display
	return true;
} //WriteCommsLedViaSpi()

void flash_comms_LED(int fd, int micro_seconds)
{
	extern int tcp_mode;
	extern int blink_LED;

	/* If tcp remote control is on, speedup
	    consecutive commands */
	if (tcp_mode == 0 || blink_LED == 1) {
		WriteCommsLedViaSpi(fd, 1);
		usleep(micro_seconds); //microseconds
		WriteCommsLedViaSpi(fd, 0);
	}
} //flash_comms_LED()

int WritePllDataViaSpi(int fd, unsigned char command, int len, unsigned char* pll_array)
{
    int index = 0;
    int array_index = 0;

    WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = command;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 5; //Length
   	WriteBuffer[index++] = 0xAA; //1'st dummy
   	WriteBuffer[index++] = 0xBB; //2nd dummy
    for (array_index = 0; array_index < len; array_index++) {
    	WriteBuffer[index++] = (unsigned char)(pll_array[array_index]) & (0xFF);
    }
   	transfer(fd, WriteBuffer, ReadBuffer, index);
   	//transfer_mybuf(fd, 8); //Use this if you want a verbose display
	return true;
} //WritePllDataViaSpi

static int Lmx2595UpdateFreq(int fd, unsigned char command, unsigned int r[LMX2594_A_count]) {
  int Index = 0;
  int val;
  unsigned char tx_array[3];
  /*
   * 1. Apply power to device.
   * 2. Program RESET = 1 to reset registers.
   * 3. Program RESET = 0 to remove reset.
   * 4. Program registers as shown in the register map in REVERSE order from
   * highest to lowest.
   * 5. Program register R0 one additional time with FCAL_EN = 1 to ensure that
   * the VCO calibration runs from a
   * stable state.
   */
  val = 0x2;
  tx_array[2] = (unsigned char)val & (0xFF);
  tx_array[1] = (unsigned char)val & (0xFF);
  tx_array[0] = (unsigned char)(val >> 16) & (0xFF);
  val = tx_array[0] | (tx_array[1] << 8) | (tx_array[2] << 16);
  WritePllDataViaSpi(fd, command, 3, &tx_array[0]);
  usleep(10000);
  val = 0x0;
  tx_array[2] = (unsigned char)val & (0xFF);
  tx_array[1] = (unsigned char)val & (0xFF);
  tx_array[0] = (unsigned char)(val >> 16) & (0xFF);
  val = tx_array[0] | (tx_array[1] << 8) | (tx_array[2] << 16);
  WritePllDataViaSpi(fd, command, 3, &tx_array[0]);
  usleep(10000);
  for (Index = 0; Index < LMX2594_A_count; Index++) {
    tx_array[2] = (unsigned char)(r[Index]) & (0xFF);
    tx_array[1] = (unsigned char)(r[Index] >> 8) & (0xFF);
    tx_array[0] = (unsigned char)(r[Index] >> 16) & (0xFF);
    val = tx_array[0] | (tx_array[1] << 8) | (tx_array[2] << 16);
    WritePllDataViaSpi(fd, command, 3, &tx_array[0]);
    usleep(10000);
  }
  /* FCAL_EN = 1 */
  tx_array[2] = (unsigned char)(r[112]) & (0xFF);
  tx_array[1] = (unsigned char)(r[112] >> 8) & (0xFF);
  tx_array[0] = (unsigned char)(r[112] >> 16) & (0xFF);
  val = tx_array[0] | (tx_array[1] << 8) | (tx_array[2] << 16);
  WritePllDataViaSpi(fd, command, 3, &tx_array[0]);
  printf("LMX configured \n");
  return true;
} //Lmx2595UpdateFreq()

int LMX2595ClockConfig(int fd, unsigned char command, unsigned int LMX2595_CKin[DAC_MAX][LMX2594_A_count]) {
  int ret;
  ret = Lmx2595UpdateFreq(fd, command, LMX2595_CKin[0]);
  return ret;
} //LMX2595ClockConfig()

int WritePllSetViaSpi(int fd, unsigned char command, int freq)
{
    /* configure LMX clock */
    int ret = LMX2595ClockConfig(fd, command, LMX2592_A);
	flash_comms_LED(fd, 100000); //100 ms
	return true;
} //WritePllSetViaSpi()

int WritePllRegisterViaSpi(int fd, unsigned char command, unsigned char ucaddress, int ivalue, int must_flash_LED)
{
    unsigned char tx_array[3];
    tx_array[2] = (unsigned char)(ivalue) & (0xFF);
    tx_array[1] = (unsigned char)(ivalue >> 8) & (0xFF);
    tx_array[0] = ucaddress;
    WritePllDataViaSpi(fd, command, 3, &tx_array[0]);
    if (must_flash_LED)
    {
        flash_comms_LED(fd, 80000); //80 ms
    }
	return true;
} //WritePllRegisterViaSpi()

#define PLL_REG0_DEFAULT		0x2498 //LMX2595 default register
#define PLL_LOCKED_BIT_ENABLE	0x04 //bit to allow 0 = readback on MUXOUT pin instead of 1 = "locked" bit, which is the default
int ReadPllRegisterViaSpi(int fd, unsigned char command, unsigned char ucaddress, int must_flash_LED)
{
    if (must_flash_LED)
    {
    	WritePllRegisterViaSpi(fd, command, 0, PLL_REG0_DEFAULT, false); //disable LOCKED bit on muxout pin
    }
	unsigned char tx_array[3];
    tx_array[2] = 0xBB; //Dummy byte
    tx_array[1] = 0xAA; //Dummy byte
    tx_array[0] = 0x80 | ucaddress; //MSB set for read mode
    WritePllDataViaSpi(fd, command, 3, &tx_array[0]);
   	unsigned char ucValueMSB, ucValueLSB;
   	//Address  = ReadBuffer[6 + 0];
   	ucValueMSB = ReadBuffer[6 + 1];
   	ucValueLSB = ReadBuffer[6 + 2];
    unsigned int iValue = (ucValueMSB << 8) + ucValueLSB;
    if (must_flash_LED)
    {
        WritePllRegisterViaSpi(fd, command, 0, (PLL_REG0_DEFAULT | PLL_LOCKED_BIT_ENABLE), false); //re-enable LOCKED bit on muxout pin
    	flash_comms_LED(fd, 80000); //80 ms
    }
   	return iValue;
} //ReadPllRegisterViaSpi

int WriteAndVerifyPllRegisterViaSpi(int fd, unsigned char command, unsigned char ucaddress, int ivalue)
{
    unsigned char tx_array[3];
    tx_array[2] = (unsigned char)(ivalue) & (0xFF);
    tx_array[1] = (unsigned char)(ivalue >> 8) & (0xFF);
    tx_array[0] = ucaddress;
    WritePllDataViaSpi(fd, command, 3, &tx_array[0]);

    int iVerify_value = ReadPllRegisterViaSpi(fd, command, ucaddress, true);

    if (iVerify_value != ivalue)
    {
    	return false;
    }
    else
    {
    	return true;
    }
} //WriteAndVerifyPllRegisterViaSpi()

int CheckPllAlive(int fd, unsigned char command)
{
    int iPllVerifyOK = WriteAndVerifyPllRegisterViaSpi(fd, command, 0, PLL_REG0_DEFAULT);
    if (iPllVerifyOK == false)
    {
    	return false;
    }
    else
    {
    	return true;
    }
} //CheckPllAlive()

int WriteSerialAttenuatorViaSpi(int fd, unsigned char command, int ivalue)
{
    int index = 0;

	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = command;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 3; //Length
   	WriteBuffer[index++] = ivalue & 0x00FF; //LSB
   	WriteBuffer[index++] = (ivalue & 0xFF00) >> 8; //MSB
   	WriteBuffer[index++] = 0x01; //Start!
   	WriteBuffer[index++] = 0xFF; //Processing byte
   	transfer(fd, WriteBuffer, ReadBuffer, index);
   	//transfer_mybuf(fd, 8); //Use this if you want a verbose display
	flash_comms_LED(fd, 100000); //100 ms
	return true;
} //WriteSerialAttenuatorViaSpi

int WriteParallelAttenuatorViaSpi(int fd, unsigned char command, int ivalue)
{
    int index = 0;

	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = command;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 1; //Length
   	WriteBuffer[index++] = ivalue;
   	WriteBuffer[index++] = 0xFF; //Processing byte
   	transfer(fd, WriteBuffer, ReadBuffer, index);
   	//transfer_mybuf(fd, 8); //Use this if you want a verbose display
	flash_comms_LED(fd, 100000); //100 ms
	return true;
} //WriteParallelAttenuatorViaSpi

int SetPmodTestPin(int fd, unsigned char ucpmod_number, unsigned char bit_number, unsigned char value)
{
    unsigned char mask_command;
    unsigned char status_command;
    unsigned char mask_data;
    unsigned char status_data;
    unsigned char and_mask;
    unsigned char mask_changed = false;

    unsigned char ucPmod0TestpinMask_old = ucPmod0TestpinMask;
    unsigned char ucPmod1TestpinMask_old = ucPmod1TestpinMask;

	int index;

	if (ucpmod_number == 0)
	{
		ucPmod0TestpinMask = ucPmod0TestpinMask | (1 << bit_number);
		mask_data = ucPmod0TestpinMask;
		if (ucPmod0TestpinMask_old != ucPmod0TestpinMask)
		{
			mask_changed = true;
		}
		mask_command = CMD_SET_PMOD0_SOFT_EN;
		status_command = CMD_SET_PMOD0_PINS;
		if (value)
		{
			ucPmod0Status = ucPmod0Status | (1 << bit_number);
		}
		else
		{
			and_mask = 0xFF ^ (1 << bit_number); //xor
			ucPmod0Status = ucPmod0Status & and_mask;
		}
		status_data = ucPmod0Status;
	}
	else if (ucpmod_number == 1)
	{
		ucPmod1TestpinMask = ucPmod1TestpinMask | (1 << bit_number);
		mask_data = ucPmod1TestpinMask;
		if (ucPmod1TestpinMask_old != ucPmod1TestpinMask)
		{
			mask_changed = true;
		}
		mask_command = CMD_SET_PMOD1_SOFT_EN;
		status_command = CMD_SET_PMOD1_PINS;
		if (value)
		{
			ucPmod1Status = ucPmod1Status | (1 << bit_number);
		}
		else
		{
			and_mask = 0xFF ^ (1 << bit_number); //xor
			ucPmod1Status = ucPmod1Status & and_mask;
		}
		status_data = ucPmod1Status;
	}
	else
	{
		printf("Error invalid PMOD");
	}

	//Send the bit mask if it changed:
	if(mask_changed)
	{
		index = 0;
		WriteBuffer[index++] = CHIP_ID;
	  	WriteBuffer[index++] = mask_command;
		WriteBuffer[index++] = 0; //Start address LSB
		WriteBuffer[index++] = 1; //Length
	   	WriteBuffer[index++] = mask_data;
	   	WriteBuffer[index++] = 0xFF; //Processing byte
	   	transfer(fd, WriteBuffer, ReadBuffer, index);
	}

	//Send the status:
	index = 0;
	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = status_command;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 1; //Length
   	WriteBuffer[index++] = status_data;
   	WriteBuffer[index++] = 0xFF; //Processing byte
   	transfer(fd, WriteBuffer, ReadBuffer, index);

   	return true;
} //SetPmodTestPin

int WriteBitViaSpi(int fd, unsigned char command, int ivalue)
{
    int index = 0;

	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = command;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 1; //Length
   	WriteBuffer[index++] = ivalue;
   	WriteBuffer[index++] = 0xFF; //Processing byte
   	transfer(fd, WriteBuffer, ReadBuffer, index);
   	//transfer_mybuf(fd, 8); //Use this if you want a verbose display
	flash_comms_LED(fd, 100000); //100 ms
	return true;
} //WriteBitViaSpi

int WriteRegViaSpiNoflash(int fd, unsigned char command, int ivalue)
{
    int index = 0;

	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = command;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 1; //Length
   	WriteBuffer[index++] = ivalue;
   	WriteBuffer[index++] = 0xFF; //Processing byte
   	transfer(fd, WriteBuffer, ReadBuffer, index);
	return true;
} //WriteRegViaSpiNoflash

int GetPlVersionViaSpi(int fd)
{
    int index = 0;

	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = CMD_GET_VERSION;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 2; //Length
   	WriteBuffer[index++] = 0; //Dat0
   	WriteBuffer[index++] = 0; //Dat1
   	WriteBuffer[index++] = 0; //Proc0
   	WriteBuffer[index++] = 0; //Proc1
   	transfer(fd, WriteBuffer, ReadBuffer, index);
   	PlVersionLSB = ReadBuffer[6 + 0];
	PlVersionMSB = ReadBuffer[6 + 1];
	tcp_printf("The IP version of the Attenuator control logic is %d.%d\r\n", PlVersionMSB, PlVersionLSB);
  	printf("The IP version of the Attenuator control logic is %d.%d\r\n", PlVersionMSB, PlVersionLSB);
   	return true;
} //GetPlVersionViaSpi

#if (0)
int ReadFeedbackViaSpi(int fd)
{
    int index = 0;

	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = CMD_GET_FEEDBACK;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 1; //Length
   	WriteBuffer[index++] = 0; //Dat0
   	WriteBuffer[index++] = 0; //Proc0
   	WriteBuffer[index++] = 0; //Proc1
   	transfer(fd, WriteBuffer, ReadBuffer, index);
   	unsigned char FeedbackByte;
   	unsigned char Ch1OverVoltage, Ch2OverVoltage;
   	FeedbackByte = ReadBuffer[6 + 0];
   	Ch1OverVoltage = (FeedbackByte & 0x01) >> 0; //bit 0
   	Ch2OverVoltage = (FeedbackByte & 0x02) >> 1; //bit 1
  	printf("Channel 1 Over-Voltage bit = %d\r\n", Ch1OverVoltage);
  	printf("Channel 2 Over-Voltage bit = %d\r\n", Ch2OverVoltage);
   	return true;
} //ReadFeedbackViaSpi
#endif

unsigned int power(int base, int exponent)
{
	long long result = 1;
	while (exponent != 0)
	{
	   result *= base;
	   --exponent;
	}
	return (result);
	//return (result & 0xFF);
} //power()

int RxAttenuation_dB_to_bits(float fchoice)
{
	int iresult;
	printf("Rx attenuator set to ");
	if (fchoice < 1)
	{
		iresult = 0x51; //State 0 = 0dB
		printf("0 dB\n");
	}
	else if (fchoice < 3)
	{
		iresult = 0x49; //State 4 = 2dB
		printf("2 dB\n");
	}
	else if (fchoice < 5)
	{
		iresult = 0x91; //State 8 = 4dB
		printf("4 dB\n");
	}
	else if (fchoice < 7)
	{
		iresult = 0x89; //State 12 = 6dB
		printf("6 dB\n");
	}
	else if (fchoice < 9)
	{
		iresult = 0x52; //State 16 = 8dB
		printf("8 dB\n");
	}
	else if (fchoice < 11)
	{
		iresult = 0x4A; //State 20 = 10dB
		printf("10 dB\n");
	}
	else if (fchoice < 13)
	{
		iresult = 0x92; //State 24 = 12dB
		printf("12 dB\n");
	}
	else if (fchoice <= 15.5)
	{
		iresult = 0x8A; //State 28 = 14dB
		printf("14 dB\n");
	}
	else
	{
		iresult = -1;
		printf("Out of range.........\n");
		fflush(stdout); //     may need memory barriers i.e. caution with data being cached
		sleep(2); //display for a moment
	}
	//fflush(stdout); // Prints to screen or whatever your standard out is
	return(iresult);
} //RxAttenuation_dB_to_bits()

int TxAttenuation_dB_to_bits(float fchoice)
{
	int iresult;
	printf("Tx attenuator set to ");
	//On the ZCU111 only 6 of the 8 bits are used (2dB resolution), while on the ZCU208 all 8 bits are used (0.5dB resolution)
	if (PlVersionMSB < 3)
	{ //We have a ZCU111
		if (fchoice < 1)
		{
			iresult = 0x51; //State 0 = 0dB
			printf("0 dB\n");
		}
		else if (fchoice < 3)
		{
			iresult = 0x49; //State 4 = 2dB
			printf("2 dB\n");
		}
		else if (fchoice < 5)
		{
			iresult = 0x91; //State 8 = 4dB
			printf("4 dB\n");
		}
		else if (fchoice < 7)
		{
			iresult = 0x89; //State 12 = 6dB
			printf("6 dB\n");
		}
		else if (fchoice < 9)
		{
			iresult = 0x52; //State 16 = 8dB
			printf("8 dB\n");
		}
		else if (fchoice < 11)
		{
			iresult = 0x4A; //State 20 = 10dB
			printf("10 dB\n");
		}
		else if (fchoice < 13)
		{
			iresult = 0x92; //State 24 = 12dB
			printf("12 dB\n");
		}
		else if (fchoice <= 15.5)
		{
			iresult = 0x8A; //State 28 = 14dB
			printf("14 dB\n");
		}
		else
		{
			iresult = -1;
			printf("Out of range.........\n");
			fflush(stdout); //     may need memory barriers i.e. caution with data being cached
			sleep(2); //display for a moment
		}
	} //We have a ZCU111
	else
	{ //We have a ZCU208
		int state = (int)(fchoice * 2);
		switch (state)
		{
		   case 0:
	        {
			   iresult = 0x51; //State 0 = 0.0dB
			   printf("0 dB\n");
			   break;
	        }
		   case 1:
		   {
			   iresult = 0x59; //State 1 = 0.5dB
			   printf("0.5 dB\n");
			   break;
		   }
		   case 2:
		   {
			   iresult = 0x71; //State 2 = 1.0dB
			   printf("1.0 dB\n");
			   break;
		   }
		   case 3:
		   {
			   iresult = 0x75; //State 3 = 1.5dB
			   printf("1.5 dB\n");
			   break;
		   }
		   case 4:
		   {
			   iresult = 0x49; //State 4 = 2.0dB
			   printf("2 dB\n");
			   break;
		   }
		   case 5:
		   {
			   iresult = 0x4D; //State 5 = 2.5dB
			   printf("2.5 dB\n");
			   break;
		   }
		   case 6:
		   {
			   iresult = 0x69; //State 6 = 3.0dB
			   printf("3.0 dB\n");
			   break;
		   }
		   case 7:
		   {
			   iresult = 0x6D; //State 7 = 3.5dB
			   printf("3.5 dB\n");
			   break;
		   }
		   case 8:
		   {
			   iresult = 0x91; //State 8 = 4.0dB
			   printf("4.0 dB\n");
			   break;
		   }
		   case 9:
		   {
			   iresult = 0x95; //State 9 = 4.5dB
			   printf("4.5 dB\n");
			   break;
		   }
		   case 10:
		   {
			   iresult = 0xB1; //State 10 = 5.0dB
			   printf("5.0 dB\n");
			   break;
		   }
		   case 11:
		   {
			   iresult = 0xB5; //State 11 = 5.5dB
			   printf("5.5 dB\n");
			   break;
		   }
		   case 12:
		   {
			   iresult = 0x89; //State 12 = 6.0dB
			   printf("6.0 dB\n");
			   break;
		   }
		   case 13:
		   {
			   iresult = 0x8D; //State 13 = 6.5dB
			   printf("6.5 dB\n");
			   break;
		   }
		   case 14:
		   {
			   iresult = 0xA9; //State 14 = 7.0dB
			   printf("7.0 dB\n");
			   break;
		   }
		   case 15:
		   {
			   iresult = 0xAD; //State 15 = 7.5dB
			   printf("7.5 dB\n");
			   break;
		   }
		   case 16:
		   {
			   iresult = 0x52; //State 16 = 8.0dB
			   printf("8.0 dB\n");
			   break;
		   }
		   case 17:
		   {
			   iresult = 0x56; //State 17 = 8.5dB
			   printf("8.5 dB\n");
			   break;
		   }
		   case 18:
		   {
			   iresult = 0x72; //State 18 = 9.0dB
			   printf("9.0 dB\n");
			   break;
		   }
		   case 19:
		   {
			   iresult = 0x76; //State 19 = 9.5dB
			   printf("9.5 dB\n");
			   break;
		   }
		   case 20:
		   {
			   iresult = 0x4A; //State 20 = 10.0dB
			   printf("10.0 dB\n");
			   break;
		   }
		   case 21:
		   {
			   iresult = 0x4E; //State 21 = 10.5dB
			   printf("10.5 dB\n");
			   break;
		   }
		   case 22:
		   {
			   iresult = 0x6A; //State 22 = 11.0dB
			   printf("11.0 dB\n");
			   break;
		   }
		   case 23:
		   {
			   iresult = 0x6E; //State 23 = 11.5dB
			   printf("11.5 dB\n");
			   break;
		   }
		   case 24:
		   {
			   iresult = 0x92; //State 24 = 12.0dB
			   printf("12.0 dB\n");
			   break;
		   }
		   case 25:
		   {
			   iresult = 0x96; //State 25 = 12.5dB
			   printf("12.5 dB\n");
			   break;
		   }
		   case 26:
		   {
			   iresult = 0xB2; //State 26 = 13.0dB
			   printf("13.0 dB\n");
			   break;
		   }
		   case 27:
		   {
			   iresult = 0xB6; //State 27 = 13.5dB
			   printf("13.5 dB\n");
			   break;
		   }
		   case 28:
		   {
			   iresult = 0x8A; //State 28 = 14.0dB
			   printf("14.0 dB\n");
			   break;
		   }
		   case 29:
		   {
			   iresult = 0x8E; //State 29 = 14.5dB
			   printf("14.5 dB\n");
			   break;
		   }
		   case 30:
		   {
			   iresult = 0xAA; //State 30 = 15.0dB
			   printf("15.0 dB\n");
			   break;
		   }
		   case 31:
		   {
			   iresult = 0xAE; //State 31 = 15.5dB
			   printf("15.5 dB\n");
			   break;
		   }
	       default:
	       {
	    	   iresult = -1;
	    	   printf("Out of range.........\n");
	    	   fflush(stdout); //     may need memory barriers i.e. caution with data being cached
	    	   sleep(2); //display for a moment
	    	   break;
	       }
		} //switch (state)
	} //We have a ZCU208
	//fflush(stdout); // Prints to screen or whatever your standard out is
	return(iresult);
} //TxAttenuation_dB_to_bits()

float attenuation_bits_to_dB(int ichoice)
{
	float iresultdB;
	switch (ichoice)
	{
	   case 0x51:
	   {
		   iresultdB = 0.0; //dB
		   break;
        }
	   case 0x59:
	   {
		   iresultdB = 0.5; //dB
		   break;
	   }
	   case 0x71:
	   {
		   iresultdB = 1.0; //dB
		   break;
	   }
	   case 0x75:
	   {
		   iresultdB = 1.5; //dB
		   break;
	   }
	   case 0x49:
	   {
		   iresultdB = 2.0; //dB
		   break;
	   }
	   case 0x4D:
	   {
		   iresultdB = 2.5; //dB
		   break;
	   }
	   case 0x69:
	   {
		   iresultdB = 3.0; //dB
		   break;
	   }
	   case 0x6D:
	   {
		   iresultdB = 3.5; //dB
		   break;
	   }
	   case 0x91:
	   {
		   iresultdB = 4.0; //dB
		   break;
	   }
	   case 0x95:
	   {
		   iresultdB = 4.5; //dB
		   break;
	   }
	   case 0xB1:
	   {
		   iresultdB = 5.0; //dB
		   break;
	   }
	   case 0xB5:
	   {
		   iresultdB = 5.5; //dB
		   break;
	   }
	   case 0x89:
	   {
		   iresultdB = 6.0; //dB
		   break;
	   }
	   case 0x8D:
	   {
		   iresultdB = 6.5; //dB
		   break;
	   }
	   case 0xA9:
	   {
		   iresultdB = 7.0; //dB
		   break;
	   }
	   case 0xAD:
	   {
		   iresultdB = 7.5; //dB
		   break;
	   }
	   case 0x52:
	   {
		   iresultdB = 8.0; //dB
		   break;
	   }
	   case 0x56:
	   {
		   iresultdB = 8.5; //dB
		   break;
	   }
	   case 0x72:
	   {
		   iresultdB = 9.0; //dB
		   break;
	   }
	   case 0x76:
	   {
		   iresultdB = 9.5; //dB
		   break;
	   }
	   case 0x4A:
	   {
		   iresultdB = 10.0; //dB
		   break;
	   }
	   case 0x4E:
	   {
		   iresultdB = 10.5; //dB
		   break;
	   }
	   case 0x6A:
	   {
		   iresultdB = 11.0; //dB
		   break;
	   }
	   case 0x6E:
	   {
		   iresultdB = 11.5; //dB
		   break;
	   }
	   case 0x92:
	   {
		   iresultdB = 12.0; //dB
		   break;
	   }
	   case 0x96:
	   {
		   iresultdB = 12.5; //dB
		   break;
	   }
	   case 0xB2:
	   {
		   iresultdB = 13.0; //dB
		   break;
	   }
	   case 0xB6:
	   {
		   iresultdB = 13.5; //dB
		   break;
	   }
	   case 0x8A:
	   {
		   iresultdB = 14.0; //dB
		   break;
	   }
	   case 0x8E:
	   {
		   iresultdB = 14.5; //dB
		   break;
	   }
	   case 0xAA:
	   {
		   iresultdB = 15.0; //dB
		   break;
	   }
	   case 0xAE:
	   {
		   iresultdB = 15.5; //dB
		   break;
	   }
       default:
       {
    	   iresultdB = -1;
    	   printf("Out of range.........\n");
    	   fflush(stdout); //     may need memory barriers i.e. caution with data being cached
    	   sleep(2); //display for a moment
    	   break;
       }
	} //switch (ichoice)
#if (0)
	if (ichoice == 0x51)
	{
		iresultdB = 0; //dB
	}
	else if (ichoice == 0x49)
	{
		iresultdB = 2; //dB
	}
	else if (ichoice == 0x91)
	{
		iresultdB = 4; //dB
	}
	else if (ichoice == 0x89)
	{
		iresultdB = 6; //dB
	}
	else if (ichoice == 0x52)
	{
		iresultdB = 8; //dB
	}
	else if (ichoice == 0x4A)
	{
		iresultdB = 10; //dB
	}
	else if (ichoice == 0x92)
	{
		iresultdB = 12; //dB
	}
	else if (ichoice == 0x8A)
	{
		iresultdB = 14; //dB
	}
	else
	{
		iresultdB = -1;
		printf("Out of range.........\n");
		fflush(stdout); //     may need memory barriers i.e. caution with data being cached
		sleep(2); //display for a moment
	}
#endif
	return(iresultdB);
} //attenuation_bits_to_dB()

int get_RFSA3713_attenuation_dB(int num_valid_bits)
{
    unsigned long bit_mask = power(2,num_valid_bits) - 1;
	char choice[20];
	int iresult, ichoice;
	scanf("%s", &choice[0]);  //- deprecated
	float fchoice = (atof(choice));
	printf("You entered %g => ", fchoice);
	ichoice = (int)(fchoice * 4); //0.25 dB for each LSB
	iresult =  ichoice & bit_mask;
	return(iresult);
} //get_RFSA3713_attenuation_dB()

int find_file_starting_with(char* directory, char* filenamestart, char* extension, char* filename_found)
{
  int inum_files_found = 0;
  DIR *d;
  struct dirent *dir;
  d = opendir(directory);
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
    	if(strstr(dir->d_name, extension) != NULL)
    	{
        	if(strstr(dir->d_name, filenamestart) != NULL)
        	{
        		sprintf(filename_found, "%s", dir->d_name);
        		inum_files_found++;
        	}
    	}
    }
    closedir(d);
  }
  return(inum_files_found);
} //find_file_starting_with()

#define PATH_FILENAME_ZCU111		"/run/media/mmcblk0p1/pll-configs/"
#define PATH_FILENAME_ZCU208		"/media/sd-mmcblk0p1/pll-configs/"
#define FILENAME_ZCU111_PLL_REGISTERS	"LMX2595_12p6GHz_RefDiv2.txt"
#define FILENAME_ZCU208_TX_PLL_REGISTERS	"TX_LMX2595_11p75GHz_out_Refin_122p88MHz_Div2.txt"
#define FILENAME_ZCU208_RX_PLL_REGISTERS	"RX_LMX2595_12p15GHz_out_Refin_122p88MHz_Div2.txt"

static int LoadPllFile(int fd, int get_user_input, unsigned int echo, unsigned char command)
{
	int ifilestatus = 0;
    FILE *fptr;
    int linenumber = 0;
    char line[256]; /* or other suitable maximum line size */
	char pathname[500];
	char userfilename[530];
	char filename_found[530];
	char PLLfilename[1030];  // This could contain both pathname + filename_found or userfilename + pathname

	if (useCmdLinePath == 0) {
		if (PlVersionMSB < 3)
		{ //We have a ZCU111
			sprintf(pathname, PATH_FILENAME_ZCU111);
		}
		else
		{ //We have a ZCU208
			sprintf(pathname, PATH_FILENAME_ZCU208);
		}
	} else {
		int nCfgPath = strlen(rootCfgPath);
		// '=' to leave room for \0
		if (nCfgPath >= sizeof(pathname)/sizeof(pathname[0])) {
			printf("\nError: PLL config file path name is to long!\n");
			exit(-1);
		} else {
			printf("New PLL config path name length is: %d\n", nCfgPath);
		}
		sprintf(pathname, "%s", rootCfgPath);
	}

	if (get_user_input)
	{
		//printf("Enter the file name to load (or just enter 'x' for HexRegisterValues.txt): \n");
		printf("Enter the file name to load (or just enter 'x' for LMX2595_12p6GHz_RefDiv2.txt): \n");
		scanf("%s", &userfilename[0]);  //- deprecated
		if (userfilename[0] == 'x')
		{
			if (PlVersionMSB < 3)
			{ //We have a ZCU111
				ifilestatus = find_file_starting_with(pathname, "LMX2595", ".txt", filename_found);
			}
			else if (command == CMD_ACCESS_TX_PLL)
			{
				ifilestatus = find_file_starting_with(pathname, "TX_LMX2595", ".txt", filename_found);
			}
			else
			{
				ifilestatus = find_file_starting_with(pathname, "RX_LMX2595", ".txt", filename_found);
			}
			if (ifilestatus == 0)
			{
				printf("ERROR: PLL file not found in the %s folder.\n", pathname);
		        return(-1);
			}
			else if (ifilestatus > 1)
			{
				printf("ERROR: Found %d PLL files in the %s folder.  There should be only one.\n", ifilestatus, pathname);
		        return(-1);
			}
			else if (ifilestatus == 1)
			{
				sprintf(PLLfilename, "%s%s", pathname, filename_found);
			}
		}
		else
		{
			// This is assuming ZCU111 board here.  It is a little bug but leaving it as it was before
			//  mods to enable dynamic path names.  This assumes this feature is not used for ZCU208.
			//  Will log it as minor issue in github, should be issue #1
			if (useCmdLinePath == 0) {
				sprintf(PLLfilename, "/run/media/mmcblk0p1/%s", userfilename);
			} else {
				int nCfgPath = strlen(rootCfgPath);
				// '=' to leave room for \0
				if (nCfgPath >= sizeof(PLLfilename)/sizeof(PLLfilename[0])) {
					printf("\nError: PLLfilename config file path name is to long!\n");
					exit(-1);
				} else {
					printf("New PLLfilename config path name length is: %d\n", nCfgPath);
				}
				sprintf(PLLfilename, "%s%s", rootCfgPath, userfilename);
			}
		}
	}
	else
	{ //no user entry, as in auto initialization
		if (PlVersionMSB < 3)
		{ //We have a ZCU111
			ifilestatus = find_file_starting_with(pathname, "LMX2595", ".txt", filename_found);
		}
		else if (command == CMD_ACCESS_TX_PLL)
		{
			ifilestatus = find_file_starting_with(pathname, "TX_LMX2595", ".txt", filename_found);
		}
		else
		{
			ifilestatus = find_file_starting_with(pathname, "RX_LMX2595", ".txt", filename_found);
		}
		if (ifilestatus == 0)
		{
			printf("ERROR: PLL file not found in the %s folder.\n", pathname);
	        return(-1);
		}
		else if (ifilestatus > 1)
		{
			printf("ERROR: Found %d PLL files in the %s folder.  There should be only one.\n", ifilestatus, pathname);
	        return(-1);
		}
		else if (ifilestatus == 1)
		{
			sprintf(PLLfilename, "%s%s", pathname, filename_found);
		}
	}
	printf("Opening file %s\n", PLLfilename);
    if ((fptr = fopen(PLLfilename, "r")) == NULL)
    {
        printf("Cannot open that file name!\n");
        return(-1);
    }
    //Reset PLL:
    //RESET = 1 (R0 bit 1 = 1)
    WritePllRegisterViaSpi(fd, command, 0, 2, false);
    //RESET = 0 (R0 bit 1 = 0)
    WritePllRegisterViaSpi(fd, command, 0, 0, false);
    //Write all registers in file:
    while (fgets(line, sizeof line, fptr) != NULL) /* read a line */
    {
        //printf("Line from the file:%s", line);
        char *hexstringinfile = strstr(line, "0x");
        if (echo)
        {
            printf("From line %i : %s", linenumber, hexstringinfile);
        }
        int number = (int)strtol(hexstringinfile, NULL, 0);
        //printf("The string int is : 0x%06X\n", number);
        unsigned char tx_array[3];
        tx_array[2] = (unsigned char)(number) & (0xFF);
        tx_array[1] = (unsigned char)(number >> 8) & (0xFF);
        tx_array[0] = (unsigned char)(number >> 16) & (0xFF);
        WritePllDataViaSpi(fd, command, 3, &tx_array[0]);
        linenumber++;
    }
    fclose(fptr);
    //Wait 10 ms:
	usleep(10000); //microseconds
    //Program register R0 one additional time with FCAL_EN = 1:
    WritePllRegisterViaSpi(fd, command, 0, (PLL_REG0_DEFAULT | PLL_LOCKED_BIT_ENABLE), true);
	fflush(stdout); // Prints to screen or whatever your standard out is
    return 0;
} //LoadPllFile()

static int ReadEntirePll(int fd, unsigned char command, int include_address)
{
	int ucPLL_register;
	int iPLL_value;
	//for (ucPLL_register = 0; ucPLL_register < LMX2594_A_count; ucPLL_register++) {
	ucPLL_register = (LMX2594_A_count-1);
	WritePllRegisterViaSpi(fd, command, 0, PLL_REG0_DEFAULT, true); //disable LOCKED bit on muxout pin
	while (ucPLL_register >= 0)
	{
		iPLL_value = ReadPllRegisterViaSpi(fd, command, ucPLL_register, false);
	    //printf("Register %i = 0x%04X\n", ucPLL_register, iPLL_value);
	    if (include_address)
	    {
	    	tcp_printf("R%i\t0x%02X%04X\n", ucPLL_register, ucPLL_register, iPLL_value);
	    }
	    else
	    {
	    	tcp_printf("R%i\t0x%04X\n", ucPLL_register, iPLL_value);
	    }
		fflush(stdout); // Prints to screen or whatever your standard out is
		ucPLL_register--;
	}
    WritePllRegisterViaSpi(fd, command, 0, (PLL_REG0_DEFAULT | PLL_LOCKED_BIT_ENABLE), false); //re-enable LOCKED bit on muxout pin
    return 0;
} //ReadEntirePll()

double Fvco_actual;
double TxFvco_actual = -1.0;
double RxFvco_actual = -1.0;
#define Fpd_DEFAULT 122880000 //Hz
unsigned long ulFpfd;
//#define DEN_DEFAULT 768
#define DEN_DEFAULT 1000
unsigned long ulRxPllDemominator;
unsigned long ulTxPllDemominator;
float fTxPllFreqRF_GHz = -1.0; //Just to store here so the host can recall it if necessary
float fTxPllFreqIF_GHz = -1.0; //Just to store here so the host can recall it if necessary
float fRxPllFreqRF_GHz = -1.0; //Just to store here so the host can recall it if necessary
float fRxPllFreqIF_GHz = -1.0; //Just to store here so the host can recall it if necessary
float fRxPllFreqFpfd_MHz = -1.0; //Just to store here so the host can recall it if necessary
float fTxPllFreqFpfd_MHz = -1.0; //Just to store here so the host can recall it if necessary

static int SetPllFrequency(int fd, unsigned char command, double Fvco)
{
	unsigned char ucPLL_register;
	tcp_printf("Fvco : %g\n", Fvco);
	//From Cecile:
	//Basically, Fvco = (RF_out � IF)/2 = (28-2.8)/2 = 12.6GHz out of the PLL.
	//After that changing either the VCO frequency or the output power level or the output buffer en/disable is done by changing these registers only : Reg 34, 36, 42, 43,  0, 44, 45, and using the following:
	//Fvco = Fpfd. (N + NUM/DEN) where DEN can remain fixed for the resolution we want. Here DEN=1000.
	//Then N = INT(Fvco/Fpfd) and NUM =  DEN x (Fvco/Fpfd � N)
	//For any VCO frequency > 15GHz, we�ll have to use the doubler (reg 27) and disable the OUTB, which unfortunately doesn�t have that option. A bit of a bummer, but we don�t have much other choice at this point.
	//I�m ok if we write these registers manually after the first chip initialization, especially during the debug phase. But if it�s not too much to add a script to automatically set them based on a set of selectable options (like FVCO, OUTA EN/DIS, OUTB EN/DIS, OUT PWR), that would help speed up the test phase.
	double doubleN = (Fvco/ulFpfd);
	unsigned long intN = (int)(doubleN);
	unsigned long ulPllDemominator;
	if (command == CMD_ACCESS_TX_PLL)
	{
		ulPllDemominator = ulTxPllDemominator;
	}
	else
	{
		ulPllDemominator = ulRxPllDemominator;
	}
	double doubleNUM = ulPllDemominator * (doubleN - intN);
	unsigned long NUM = (unsigned long)(doubleNUM);
	tcp_printf("fixed numbers: Fpfd = %i Hz, DEN = %i\n", ulFpfd, ulPllDemominator);
	tcp_printf("doubleN = (Fvco/Fpd) = %g , intN = (int)(doubleN) = %i\n", doubleN, intN);
	tcp_printf("NUM = DEN * (doubleN - intN) = %i\n", NUM);
    //Update intN registers
    unsigned int INTN_MSW = (unsigned int)((intN & 0x00070000) >> 16);
    unsigned int INTN_LSW = (unsigned int)((intN & 0x0000FFFF) >> 0);
    ucPLL_register = 34; //PLL_N[18:16]
    WritePllRegisterViaSpi(fd, command, ucPLL_register, INTN_MSW, false);
    ucPLL_register = 36; //PLL_N[15:0]
    WritePllRegisterViaSpi(fd, command, ucPLL_register, INTN_LSW, false);
    unsigned int NUM_MSW = (unsigned int)((NUM & 0xFFFF0000) >> 16);
    unsigned int NUM_LSW = (unsigned int)((NUM & 0x0000FFFF) >> 0);
    tcp_printf("NUM MSW = 0x%04X, NUM_LSW = 0x%04X\n", NUM_MSW, NUM_LSW);
    unsigned int iPLL_DEN_MSW = (unsigned int)((ulPllDemominator & 0xFFFF0000) >> 16);
    unsigned int iPLL_DEN_LSW = (unsigned int)((ulPllDemominator & 0x0000FFFF) >> 0);
    //Calculate the actual Fvco (which will be different after the rounding:
    Fvco_actual = ulFpfd * ((double)intN + ((double)NUM/(double)ulPllDemominator));
    //Write the PLL_DEN fields:
    ucPLL_register = 38; //PLL_DEN[31:16]
    WritePllRegisterViaSpi(fd, command, ucPLL_register, iPLL_DEN_MSW, false);
    ucPLL_register = 39; //PLL_DEN[15:0]
    WritePllRegisterViaSpi(fd, command, ucPLL_register, iPLL_DEN_LSW, false);
    //Update NUM registers
    ucPLL_register = 42; //PLL_NUM[31:16]
    WritePllRegisterViaSpi(fd, command, ucPLL_register, NUM_MSW, false);
    ucPLL_register = 43; //PLL_NUM[15:0]
    WritePllRegisterViaSpi(fd, command, ucPLL_register, NUM_LSW, false);
    //Write CAL bit to 1 again:
    WritePllRegisterViaSpi(fd, command, 0, (PLL_REG0_DEFAULT | PLL_LOCKED_BIT_ENABLE), true);
	return true;
} //SetPllFrequency()

static int ParsePllParameterLine(int fd, unsigned char command, char * instring)
{
	int iOldPLL_value;
	int iNewPLL_value;
	unsigned char ucPLL_register;
	int iValueFromFile;
	if (strstr(instring, "#"))
	{
		//This line has been commented out, ignore it
	}
	else if (strstr(instring, "Fpfd"))
	{
		char *numtringinfile = strstr(instring, "= ");
		ulFpfd = strtoul(numtringinfile+2, NULL, 10);
	}
	else if (strstr(instring, "PLL_DEN"))
	{
		char *numtringinfile = strstr(instring, "= ");
		ulTxPllDemominator = strtoul(numtringinfile+2, NULL, 10);
		ulRxPllDemominator = strtoul(numtringinfile+2, NULL, 10);
	}
	else if (strstr(instring, "Fvco"))
	{
		char *numtringinfile = strstr(instring, "= ");
	    //int number=(int)strtol(numtringinfile+2, NULL, 0);
		double Fvco=strtof(numtringinfile+2, NULL);
	    SetPllFrequency(fd, command, Fvco);
	}

	else if (strstr(instring, "OUTA_PD"))
	{
		char *numtringinfile = strstr(instring, "= ");
		iValueFromFile=(int)strtol(numtringinfile+2, NULL, 0);
	    printf("OUTA_PD : %i\n", iValueFromFile);
	    ucPLL_register = 44; //R44[6]
	    iOldPLL_value = ReadPllRegisterViaSpi(fd, command, ucPLL_register, true);
	    iNewPLL_value = (iOldPLL_value & 0xFFBF) | (iValueFromFile << 6); //Replace bit 6
	    WritePllRegisterViaSpi(fd, command, ucPLL_register, iNewPLL_value, true);
	}
	else if (strstr(instring, "OUTB_PD"))
	{
		char *numtringinfile = strstr(instring, "= ");
		iValueFromFile=(int)strtol(numtringinfile+2, NULL, 0);
	    printf("OUTB_PD : %i\n", iValueFromFile);
	    ucPLL_register = 44; //R44[7]
	    iOldPLL_value = ReadPllRegisterViaSpi(fd, command, ucPLL_register, true);
	    iNewPLL_value = (iOldPLL_value & 0xFF7F) | (iValueFromFile << 7); //Replace bit 7
	    WritePllRegisterViaSpi(fd, command, ucPLL_register, iNewPLL_value, true);
	}
	else if (strstr(instring, "OUTA_PWR"))
	{
		char *numtringinfile = strstr(instring, "= ");
		iValueFromFile=(int)strtol(numtringinfile+2, NULL, 0);
	    printf("OUTA_PWR : %i\n", iValueFromFile);
	    ucPLL_register = 44; //R44[13:8]
	    iOldPLL_value = ReadPllRegisterViaSpi(fd, command, ucPLL_register, true);
	    iNewPLL_value = (iOldPLL_value & 0xC0FF) | (iValueFromFile << 8); //Replace bits 8 to 13
	    WritePllRegisterViaSpi(fd, command, ucPLL_register, iNewPLL_value, true);
	}
	else if (strstr(instring, "OUTB_PWR"))
	{
		char *numtringinfile = strstr(instring, "= ");
		iValueFromFile=(int)strtol(numtringinfile+2, NULL, 0);
	    printf("OUTB_PWR : %i\n", iValueFromFile);
	    ucPLL_register = 45; //R45[5:0]
	    iOldPLL_value = ReadPllRegisterViaSpi(fd, command, ucPLL_register, true);
	    iNewPLL_value = (iOldPLL_value & 0xFFC0) | (iValueFromFile << 0); //Replace bits 0 to 5
	    WritePllRegisterViaSpi(fd, command, ucPLL_register, iNewPLL_value, true);
	}
	else if (strstr(instring, "WRITE")) //write to a PLL register directly
	{
		//Use the PLL file format where the address is included e.g. (any text before the 0x is ignored):
		//R105	0x690021
        char *hexstringinfile = strstr(instring, "0x");
        int number = (int)strtol(hexstringinfile, NULL, 0);
        unsigned char tx_array[3];
        tx_array[2] = (unsigned char)(number) & (0xFF);
        tx_array[1] = (unsigned char)(number >> 8) & (0xFF);
        tx_array[0] = (unsigned char)(number >> 16) & (0xFF);
        WritePllDataViaSpi(fd, command, 3, &tx_array[0]);
	}
	return true;
} //ParsePllParameterLine()

#define FILENAME_PLL_PARAMETERS	"pll_params.txt"
static int LoadPllParams(int fd, unsigned char command)
{
    FILE *fptr;
    int linenumber = 0;
    char line[256]; /* or other suitable maximum line size */
	char pathname[500];
	char paramsfilename[530];
	if (useCmdLinePath == 0) {
		if (PlVersionMSB > 2)
		{ //We have a ZCU208
			sprintf(pathname, PATH_FILENAME_ZCU208);
		}
		else
		{ //We have a ZCU111
			sprintf(pathname, PATH_FILENAME_ZCU111);
		}
	} else {
		int nCfgPath = strlen(rootCfgPath);
		// '=' to leave room for \0
		if (nCfgPath >= sizeof(pathname)/sizeof(pathname[0])) {
			printf("\nError: PLL config file path name is to long!\n");
			exit(-1);
		} else {
			printf("New PLL config path name length is: %d\n", nCfgPath);
		}
		sprintf(pathname, "%s", rootCfgPath);
	}
	sprintf(paramsfilename, "%s%s", pathname, FILENAME_PLL_PARAMETERS);
	printf("Opening file %s\n", paramsfilename);
    if ((fptr = fopen(paramsfilename, "r")) == NULL)
    {
        printf("Cannot open that file name!\n");
        return(-1);
    }
    while (fgets(line, sizeof line, fptr) != NULL) /* read a line */
    {
        //printf("Line from the file:%s", line);
    	ParsePllParameterLine(fd, command, line);
        linenumber++;
    }
    fclose(fptr);
	fflush(stdout); // Prints to screen or whatever your standard out is
    return 0;
} //LoadPllParams()

static int ReadBackPllStatus(int fd, unsigned char command)
{
    int iReg110value = ReadPllRegisterViaSpi(fd, command, 110, true);
	printf("Read 0x%04X from PLL register 110\n", iReg110value);
	int iReg111value = ReadPllRegisterViaSpi(fd, command, 111, true);
	printf("Read 0x%04X from PLL register 111\n", iReg111value);
	int iReg112value = ReadPllRegisterViaSpi(fd, command, 112, true);
	printf("Read 0x%04X from PLL register 112\n", iReg112value);

	int irb_LD_VTUNE = (iReg110value & 0x0600) >> 9; //R110[10:9]
	//printf("rb_LD_VTUNE = %d\n", irb_LD_VTUNE);
	//Readback of Vtune lock detect
	//0: Unlocked (Vtune low)
	//1: Invalid State
	//2: Locked
	//3: Unlocked (Vtune High)
	switch (irb_LD_VTUNE)
	{
	   case 0:
	        {
	        	printf("Vtune lock detect = 0 => Unlocked (Vtune low)\n");
	            break;
	        }
	   case 1:
	        {
	        	printf("Vtune lock detect = 1 => Invalid State\n");
	            break;
	        }
	   case 2:
	        {
	        	printf("Vtune lock detect = 2 => Locked\n");
	            break;
	        }
	   case 3:
	        {
	        	printf("Vtune lock detect = 3 => Unlocked (Vtune High)\n");
	        	break;
	        }
       default:
       	   {
	        	printf("Vtune error\n");
	        	break;
       	   }
	}

	int irb_VCO_SEL = (iReg110value & 0x00E0) >> 5; //R110[7:5]
	//printf("rb_VCO_SEL = %d\n", irb_VCO_SEL);
	//Reads back the actual VCO that the calibration has selected.
	//0: Invalid
	//1: VCO1
	//...
	//7: VCO7
	switch (irb_VCO_SEL)
	{
	   case 0:
	        {
	        	printf("VCO that the calibration has selected = 0 => Invalid\n");
	            break;
	        }
	   case 1:
	        {
	        	printf("VCO that the calibration has selected = 1 => VCO1\n");
	            break;
	        }
	   case 2:
	        {
	        	printf("VCO that the calibration has selected = 2 => VCO2\n");
	            break;
	        }
	   case 3:
	        {
	        	printf("VCO that the calibration has selected = 3 => VCO3\n");
	        	break;
	        }
	   case 4:
	        {
	        	printf("VCO that the calibration has selected = 4 => VCO4\n");
	        	break;
	        }
	   case 5:
	        {
	        	printf("VCO that the calibration has selected = 5 => VCO5\n");
	        	break;
	        }
	   case 6:
	        {
	        	printf("VCO that the calibration has selected = 6 => VCO6\n");
	        	break;
	        }
	   case 7:
	        {
	        	printf("VCO that the calibration has selected = 7 => VCO7\n");
	        	break;
	        }
       default:
       	   {
	        	printf("rb_VCO_SEL error\n");
	        	break;
       	   }
	}

	int irb_VCO_CAPCTRL = (iReg111value & 0x00FF) >> 0; //R111[7:0]
	//printf("rb_VCO_CAPCTRL = %d\n", irb_VCO_CAPCTRL);
	//Reads back the actual CAPCTRL capcode value the VCO calibration has chosen.
	printf("CAPCTRL capcode value the VCO calibration has chosen = %d\n", irb_VCO_CAPCTRL);

	int irb_VCO_DACISET = (iReg112value & 0x01FF) >> 0; //R112[8:0]
	//printf("rb_VCO_DACISET = %d\n", irb_VCO_DACISET);
	//Reads back the actual amplitude (DACISET) value that the VCO calibration has chosen.
	printf("The actual amplitude (DACISET) value that the VCO calibration has chosen = %d\n", irb_VCO_CAPCTRL);

    return 0;
} //ReadBackPllStatus()

static int ReadBackPllStatusForTcp(int fd, unsigned char command)
{
    int iReg110value = ReadPllRegisterViaSpi(fd, command, 110, true);
    tcp_printf("Read 0x%04X from PLL register 110\n", iReg110value);
	int iReg111value = ReadPllRegisterViaSpi(fd, command, 111, true);
	tcp_printf("Read 0x%04X from PLL register 111\n", iReg111value);
	int iReg112value = ReadPllRegisterViaSpi(fd, command, 112, true);
	tcp_printf("Read 0x%04X from PLL register 112\n", iReg112value);

	int irb_LD_VTUNE = (iReg110value & 0x0600) >> 9; //R110[10:9]
	//printf("rb_LD_VTUNE = %d\n", irb_LD_VTUNE);
	//Readback of Vtune lock detect
	//0: Unlocked (Vtune low)
	//1: Invalid State
	//2: Locked
	//3: Unlocked (Vtune High)
	switch (irb_LD_VTUNE)
	{
	   case 0:
	        {
	        	tcp_printf("Vtune lock detect = 0 => Unlocked (Vtune low)\n");
	            break;
	        }
	   case 1:
	        {
	        	tcp_printf("Vtune lock detect = 1 => Invalid State\n");
	            break;
	        }
	   case 2:
	        {
	        	tcp_printf("Vtune lock detect = 2 => Locked\n");
	            break;
	        }
	   case 3:
	        {
	        	tcp_printf("Vtune lock detect = 3 => Unlocked (Vtune High)\n");
	        	break;
	        }
       default:
       	   {
       		tcp_printf("Vtune error\n");
	        	break;
       	   }
	}

	int irb_VCO_SEL = (iReg110value & 0x00E0) >> 5; //R110[7:5]
	//printf("rb_VCO_SEL = %d\n", irb_VCO_SEL);
	//Reads back the actual VCO that the calibration has selected.
	//0: Invalid
	//1: VCO1
	//...
	//7: VCO7
	switch (irb_VCO_SEL)
	{
	   case 0:
	        {
	        	tcp_printf("VCO that the calibration has selected = 0 => Invalid\n");
	            break;
	        }
	   case 1:
	        {
	        	tcp_printf("VCO that the calibration has selected = 1 => VCO1\n");
	            break;
	        }
	   case 2:
	        {
	        	tcp_printf("VCO that the calibration has selected = 2 => VCO2\n");
	            break;
	        }
	   case 3:
	        {
	        	tcp_printf("VCO that the calibration has selected = 3 => VCO3\n");
	        	break;
	        }
	   case 4:
	        {
	        	tcp_printf("VCO that the calibration has selected = 4 => VCO4\n");
	        	break;
	        }
	   case 5:
	        {
	        	tcp_printf("VCO that the calibration has selected = 5 => VCO5\n");
	        	break;
	        }
	   case 6:
	        {
	        	tcp_printf("VCO that the calibration has selected = 6 => VCO6\n");
	        	break;
	        }
	   case 7:
	        {
	        	tcp_printf("VCO that the calibration has selected = 7 => VCO7\n");
	        	break;
	        }
       default:
       	   {
       		   	tcp_printf("rb_VCO_SEL error\n");
	        	break;
       	   }
	}

	int irb_VCO_CAPCTRL = (iReg111value & 0x00FF) >> 0; //R111[7:0]
	//printf("rb_VCO_CAPCTRL = %d\n", irb_VCO_CAPCTRL);
	//Reads back the actual CAPCTRL capcode value the VCO calibration has chosen.
	tcp_printf("CAPCTRL capcode value the VCO calibration has chosen = %d\n", irb_VCO_CAPCTRL);

	int irb_VCO_DACISET = (iReg112value & 0x01FF) >> 0; //R112[8:0]
	//printf("rb_VCO_DACISET = %d\n", irb_VCO_DACISET);
	//Reads back the actual amplitude (DACISET) value that the VCO calibration has chosen.
	tcp_printf("The actual amplitude (DACISET) value that the VCO calibration has chosen = %d\n", irb_VCO_CAPCTRL);

    return 0;
} //ReadBackPllStatusForTcp()

static int ReportPllPower(unsigned char command)
{
	unsigned char OutAEnable, OutBEnable;
	unsigned int PowerA,  PowerB;

	if (command == CMD_ACCESS_TX_PLL)
	{
		if (bTxPLL_set ==false)
		{
		    tcp_printf("{\"rsp\": \"TxPllPower\",\"setting1\": \"%s\",\"value_int1\": \"%s\",\"setting2\": \"%s\",\"value_int2\": \"%s\"}", UNDKNOWN_STATUS_STRING, UNDKNOWN_STATUS_STRING, UNDKNOWN_STATUS_STRING, UNDKNOWN_STATUS_STRING);
		    return -1;
		}
	}
	else
	{
		if (bRxPLL_set ==false)
		{
		    tcp_printf("{\"rsp\": \"RxPllPower\",\"setting1\": \"%s\",\"value_int1\": \"%s\",\"setting2\": \"%s\",\"value_int2\": \"%s\"}", UNDKNOWN_STATUS_STRING, UNDKNOWN_STATUS_STRING, UNDKNOWN_STATUS_STRING, UNDKNOWN_STATUS_STRING);
		    return -1;
		}
	}

	ucPLL_register = 44; //R44
    iPLL_value = ReadPllRegisterViaSpi(spi_fd, command, ucPLL_register, true);
    if (((iPLL_value & 0x80) >> 7) > 0) //OUTB_PD = Bit 7
	{
	    OutBEnable = false;
	}
	else
	{
	    OutBEnable = true;
	}

    if (((iPLL_value & 0x40) >> 6) > 0) //OUTA_PD = Bit 6
	{
	    OutAEnable = false;
	}
	else
	{
	    OutAEnable = true;
	}

    PowerA = (iPLL_value & 0x3F00) >> 8; //Bits 8 to 13

    ucPLL_register = 45; //R45
    iPLL_value = ReadPllRegisterViaSpi(spi_fd, command, ucPLL_register, true);
    PowerB = iPLL_value & 0x3F; //Bits 0 to 5

    printf("Read OUTA_EN : %i\n", OutAEnable);
    printf("Read OUTB_EN : %i\n", OutBEnable);
    printf("Read OUTA_PWR : %i\n", PowerA);
    printf("Read OUTB_PWR : %i\n", PowerB);

	if (command == CMD_ACCESS_TX_PLL)
	{
	    tcp_printf("{\"rsp\": \"TxPllPower\",\"setting1\": \"%d\",\"value_int1\": \"%d\",\"setting2\": \"%d\",\"value_int2\": \"%d\"}", OutAEnable, PowerA, OutBEnable, PowerB);
	}
	else
	{
	    tcp_printf("{\"rsp\": \"RxPllPower\",\"setting1\": \"%d\",\"value_int1\": \"%d\",\"setting2\": \"%d\",\"value_int2\": \"%d\"}", OutAEnable, PowerA, OutBEnable, PowerB);
	}
    return 0;
} //ReportPllPower()

#define USE_PLL_MUXOUT_FOR_LOCK_DETECT
static int CheckPllLocked(int fd, unsigned char command)
{
#ifdef USE_PLL_MUXOUT_FOR_LOCK_DETECT
	//Use the state of the PLL MUXOUT pin (also used as SPI SDO pin) to determine locked status
	//Use the Feedback command to read back the status of the two PLL's:
    int index = 0;
	WriteBuffer[index++] = CHIP_ID;
  	WriteBuffer[index++] = CMD_GET_FEEDBACK;
	WriteBuffer[index++] = 0; //Start address LSB
	WriteBuffer[index++] = 1; //Length
   	WriteBuffer[index++] = 0; //Dat0
   	WriteBuffer[index++] = 0; //Proc0
   	WriteBuffer[index++] = 0; //Proc1
   	transfer(fd, WriteBuffer, ReadBuffer, index);
   	unsigned char FeedbackByte;
   	unsigned char LockedBit;
   	FeedbackByte = ReadBuffer[6 + 0];
  	printf("Feedback Byte = %d\r\n", FeedbackByte);
   	if (command == CMD_ACCESS_RX_PLL)
   	{
   	   	LockedBit = (FeedbackByte & 0x01) >> 0; //bit 0 = PLL_MUXOUT_RXX
   	  	printf("Rx PLL Locked bit = %d\r\n", LockedBit);
   	}
   	else
   	{
   	   	LockedBit = (FeedbackByte & 0x02) >> 1; //bit 1 = PLL_MUXOUT_TXX
   	  	printf("Tx PLL Locked bit = %d\r\n", LockedBit);
   	}
	if (LockedBit == 1)  //If the MUXOUT pin is high
	{
		return true;
	}
	else
	{
		return false;
	}
#else
	//Use PLL register 110 to determine locked status
    int iReg110value = ReadPllRegisterViaSpi(fd, command, 110, true);
	//tcp_printf("iReg110value = %d\n", iReg110value);

	int irb_LD_VTUNE = (iReg110value & 0x0600) >> 9; //R110[10:9]
	//printf("rb_LD_VTUNE = %d\n", irb_LD_VTUNE);
	//Readback of Vtune lock detect
	//0: Unlocked (Vtune low)
	//1: Invalid State
	//2: Locked
	//3: Unlocked (Vtune High)
	tcp_printf("iReg110value = 0x%02X, irb_LD_VTUNE = %d\n", iReg110value, irb_LD_VTUNE);
	//tcp_printf("irb_LD_VTUNE = %d\n", irb_LD_VTUNE);
	if (irb_LD_VTUNE == 2)
	{
		return true;
	}
	else
	{
		return false;
	}
#endif //USE_PLL_MUXOUT_FOR_LOCK_DETECT
} //CheckPllLocked()

int get_input_value(int num_valid_bits)
{
	extern int tcp_mode;
	extern char* cmd_value;

	unsigned long bit_mask = power(2,num_valid_bits) - 1;
	char choice[20];
	int ichoice;

	if (tcp_mode == 0) {
		scanf("%s", &choice[0]);
		if (strstr(choice, "0x"))
		{//this is hex
			//int ichoice2 = (int)strtoul(choice, NULL, 16);
			char *hexstring = strstr(choice, "0x");
			int ihexchoice = (int)strtol(hexstring, NULL, 0);
			ichoice = ihexchoice & bit_mask;
			printf("Hex input entered %s => 0x%02X\n", choice, ichoice);
		}
		else
		{ //not hex
			ichoice = (atoi(choice)) & bit_mask;
			printf("You entered %s => 0x%02X\n", choice, ichoice);
		}
	}
	else { //TCP mode
		if (cmd_value != NULL) {
			if (strstr(cmd_value, "0x"))
			{//this is hex
				//int ichoice2 = (int)strtoul(choice, NULL, 16);
				char *hexstring = strstr(cmd_value, "0x");
				int ihexchoice = (int)strtol(hexstring, NULL, 0);
				ichoice = ihexchoice & bit_mask;
				printf("Hex input entered %s => 0x%02X\n", cmd_value, ichoice);
			}
			else
			{ //not hex
				ichoice = (atoi(cmd_value)) & bit_mask;
				printf("You entered %s => 0x%02X\n", cmd_value, ichoice);
			}
		}
		else
			printf("NULL cmd_value error\n");
	}
	//fflush(stdout); // Prints to screen or whatever your standard out is
	return(ichoice);
} //get_input_value()

void display_pll_menu(void)
{
	printf("--------------------------------------------------------------------------------\n");
	printf("Choose whether you would want to:\n");
	printf("a => Program a new LMX2592 configuration file from TICS Pro\n");
	printf("b => Program a fixed (hard-coded) frequency set of 113 registers\n");
	printf("c => Implement parameters in pll_params.txt\n");
	printf("d => Read back PLL status registers\n");
	printf("e => Change the LO frequency for Fpfd = 122.88MHz\n");
	printf("f => Change the LO frequency for Fpfd = 61.44MHz\n");
	printf("g => Write to a specific register\n");
	printf("h => Read from a specific register\n");
	printf("i => Display all the registers in the PLL\n");
	printf("x => Exit\n");

	fflush(stdout); // Prints to screen or whatever your standard out is
} //display_pll_menu()


#define CHOICE_POWER_TX_PATH			'a'
#define CHOICE_SET_TX_ATTENUATORS		'b'
#define CHOICE_TX_5V_EN					'c'
#define CHOICE_TX_3V3_EN				'd'
#define CHOICE_TXCH1_IF_AMP_EN			'e'
#define CHOICE_TXCH2_IF_AMP_EN			'f'
#define CHOICE_SET_TX_PLL				'g'
#define CHOICE_POWER_RX_PATH			'h'
#define CHOICE_SET_RXCH1_ATTENUATOR		'i'
#define CHOICE_SET_RXCH2_ATTENUATOR		'j'
#define CHOICE_SET_RXCH1_RFSA3713		'k'
#define CHOICE_SET_RXCH2_RFSA3713		'l'
#define CHOICE_RX_5V_EN					'm'
#define CHOICE_RX_3V3_EN				'n'
#define CHOICE_RXCH1_IF_AMP_EN			'o'
#define CHOICE_RXCH2_IF_AMP_EN			'p'
#define CHOICE_RXCH1_BIAS_EN			'q'
#define CHOICE_RXCH2_BIAS_EN			'r'
#define CHOICE_SET_RX_PLL				's'
#define CHOICE_POWER_DOWN_PATHS			't'
#define CHOICE_READBACK_SHADOWS			'u'
#define CHOICE_SET_SOFTWARE_LED			'v'
#define CHOICE_I2C_EEPROM				'w'
#define CHOICE_EXIT						'x'

void display_main_menu(void)
{
	tcp_printf("--------------------------------------------------------------------------------\r\n");
	tcp_printf(" Choose a parameter to set:\r\n");
	tcp_printf("................................................................................\r\n");
	tcp_printf(" %c => Power up the Tx path with defaults\r\n", CHOICE_POWER_TX_PATH);
	tcp_printf(" %c => Set Tx TGL2223-SM attenuators\n", CHOICE_SET_TX_ATTENUATORS);
	tcp_printf(" %c => Tx 5V enable/disable\n", CHOICE_TX_5V_EN);
	tcp_printf(" %c => Tx 3.3V enable/disable\n", CHOICE_TX_3V3_EN);
	tcp_printf(" %c => Tx Ch1 IF Amp enable/disable\n", CHOICE_TXCH1_IF_AMP_EN);
	tcp_printf(" %c => Tx Ch2 IF Amp enable/disable\n", CHOICE_TXCH2_IF_AMP_EN);
	tcp_printf(" %c => Set Tx PLL LMX2595 via SPI\n", CHOICE_SET_TX_PLL);
	tcp_printf("................................................................................\n");
	tcp_printf(" %c => Power up the Rx path with defaults\n", CHOICE_POWER_RX_PATH);
	tcp_printf(" %c => Set Rx RF Ch1 TGL2223-SM attenuator\n", CHOICE_SET_RXCH1_ATTENUATOR);
	tcp_printf(" %c => Set Rx RF Ch2 TGL2223-SM attenuator\n", CHOICE_SET_RXCH2_ATTENUATOR);
	tcp_printf(" %c => Set Rx IF Ch1 RFSA3713 attenuator\n", CHOICE_SET_RXCH1_RFSA3713);
	tcp_printf(" %c => Set Rx IF Ch2 RFSA3713 attenuator\n", CHOICE_SET_RXCH2_RFSA3713);
	tcp_printf(" %c => Rx 5V enable/disable\n", CHOICE_RX_5V_EN);
	tcp_printf(" %c => Rx 3.3V enable/disable\n", CHOICE_RX_3V3_EN);
	tcp_printf(" %c => Rx Ch1 IF Amp enable/disable\n", CHOICE_RXCH1_IF_AMP_EN);
	tcp_printf(" %c => Rx Ch2 IF Amp enable/disable\n", CHOICE_RXCH2_IF_AMP_EN);
	tcp_printf(" %c => Rx Ch1 bias enable/disable\n", CHOICE_RXCH1_BIAS_EN);
	tcp_printf(" %c => Rx Ch2 bias enable/disable\n", CHOICE_RXCH2_BIAS_EN);
	tcp_printf(" %c => Set Rx PLL LMX2595 via SPI\n", CHOICE_SET_RX_PLL);
	tcp_printf("................................................................................\n");
	tcp_printf(" %c => Power down the Tx path, then the Rx path\n", CHOICE_POWER_DOWN_PATHS);
	tcp_printf(" %c => Read back all values written, shadowed in software\n", CHOICE_READBACK_SHADOWS);
	tcp_printf(" %c => Software LED on/off\n", CHOICE_SET_SOFTWARE_LED);
	tcp_printf(" %c => I2C for EEPROM\n", CHOICE_I2C_EEPROM);
	tcp_printf(" %c => Exit\n", CHOICE_EXIT);
	tcp_printf("................................................................................\n");
	fflush(stdout); // Prints to screen or whatever your standard out is
} //display_main_menu()

int initializeSpiDevice(void)
{
	static int fd = -1; //make sure this can only be run once
	if (fd == -1)
	{
		fd = open(spi_device, O_RDWR);
		if (fd < 0)
			pabort("can't open device");
		//Set the SPI mode:  (if this is commented out, CPOL=0, CPHA=0 (clock low by default, data latched on clock rising edge).
		//mode |= SPI_CPOL; //Clock High when inactive
		//mode |= SPI_CPHA; //Data is valid on clock trailing edge
		fd = SPI_Init();
		GetPlVersionViaSpi(fd);
		WriteRegViaSpiNoflash(fd, CMD_SET_PMOD0_SOFT_EN, 0x00);
		WriteRegViaSpiNoflash(fd, CMD_SET_PMOD1_SOFT_EN, 0x00);
		WriteRegViaSpiNoflash(fd, CMD_SET_PMOD0_PINS, 0x00);
		WriteRegViaSpiNoflash(fd, CMD_SET_PMOD1_PINS, 0x00);
	}
	return (fd);
} //initializeSpiDevice

//create a monitor with a list of supported resolutions
//NOTE: Returns a heap allocated string, you are required to free it after use.
char *create_json_response(void)
{
    char *string = NULL;
    cJSON *email = NULL;
    cJSON *active = NULL;

    cJSON *monitor = cJSON_CreateObject();
    if (monitor == NULL)
    {
        goto end;
    }

    email = cJSON_CreateString("stefan@avnet.com");
    if (email == NULL)
    {
        goto end;
    }
    /* after creation was successful, immediately add it to the monitor,
     * thereby transferring ownership of the pointer to it */
    cJSON_AddItemToObject(monitor, "email", email);

    active = cJSON_CreateString("true");
    if (active == NULL)
    {
        goto end;
    }
    /* after creation was successful, immediately add it to the monitor,
     * thereby transferring ownership of the pointer to it */
    cJSON_AddItemToObject(monitor, "active", active);

    string = cJSON_Print(monitor);
    if (string == NULL)
    {
        fprintf(stderr, "Failed to print monitor.\n");
    }

end:
    cJSON_Delete(monitor);
    return string;
}

void ReportVersions(void)
{
	tcp_printf("{\"rsp\": \"VersionReport\",\"PsVersion\": \"%d.%d\",\"PlVersion\": \"%d.%d\"}", PS_VERSION_MSB, PS_VERSION_LSB, PlVersionMSB, PlVersionLSB);
} //ReportVersions

void ReportTxPllStatus(void)
{
	if (bTxPLL_set)
	{
		if (CheckPllAlive(spi_fd, CMD_ACCESS_TX_PLL))
		{
			if (CheckPllLocked(spi_fd, CMD_ACCESS_TX_PLL))
			{
				tcp_printf("{\"rsp\": \"TxPllStatus\",\"status\": \"%s\"}", "Locked");
			}
			else
			{
				//tcp_printf("{\"rsp\": \"TxPllStatus\",\"status\": \"%s\"}", "Not Locked");
				//Check it again:
				if (CheckPllLocked(spi_fd, CMD_ACCESS_TX_PLL))
				{
					tcp_printf("{\"rsp\": \"TxPllStatus\",\"status\": \"%s\"}", "Locked");
				}
				else
				{
					tcp_printf("{\"rsp\": \"TxPllStatus\",\"status\": \"%s\"}", "Not Locked");
				}
			}
		}
		else
		{
			tcp_printf("{\"rsp\": \"TxPllStatus\",\"status\": \"%s\"}", "Not Alive");
		}
	}
	else
	{
		tcp_printf("{\"rsp\": \"TxPllStatus\",\"status\": \"%s\"}", "Unknown");
	}
} //ReportTxPllStatus

void ReportRxPllStatus(void)
{
	if (bRxPLL_set)
	{
		if (CheckPllAlive(spi_fd, CMD_ACCESS_RX_PLL))
		{
			if (CheckPllLocked(spi_fd, CMD_ACCESS_RX_PLL))
			{
				tcp_printf("{\"rsp\": \"RxPllStatus\",\"status\": \"%s\"}", "Locked");
			}
			else
			{
				//tcp_printf("{\"rsp\": \"RxPllStatus\",\"status\": \"%s\"}", "Not Locked");
				//Check it again:
				if (CheckPllLocked(spi_fd, CMD_ACCESS_RX_PLL))
				{
					tcp_printf("{\"rsp\": \"RxPllStatus\",\"status\": \"%s\"}", "Locked");
				}
				else
				{
					tcp_printf("{\"rsp\": \"RxPllStatus\",\"status\": \"%s\"}", "Not Locked");
				}
			}
		}
		else
		{
			tcp_printf("{\"rsp\": \"RxPllStatus\",\"status\": \"%s\"}", "Not Alive");
		}
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxPllStatus\",\"status\": \"%s\"}", "Unknown");
	}
} //ReportRxPllStatus

void ReportStatus(void)
{
	ReportVersions();
	if (bTx5VEnable_set)
	{
		tcp_printf("{\"rsp\": \"Tx5vStatus\",\"value_int\": \"%d\"}", iTx5VEnable);
	}
	else
	{
		tcp_printf("{\"rsp\": \"Tx5vStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bTx3V3Enable_set)
	{
		tcp_printf("{\"rsp\": \"Tx3v3Status\",\"value_int\": \"%d\"}", iTx3V3Enable);
	}
	else
	{
		tcp_printf("{\"rsp\": \"Tx3v3Status\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bRx5VEnable_set)
	{
		tcp_printf("{\"rsp\": \"Rx5vStatus\",\"value_int\": \"%d\"}", iRx5VEnable);
	}
	else
	{
		tcp_printf("{\"rsp\": \"Rx5vStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bRx3V3Enable_set)
	{
		tcp_printf("{\"rsp\": \"Rx3v3Status\",\"value_int\": \"%d\"}", iRx3V3Enable);
	}
	else
	{
		tcp_printf("{\"rsp\": \"Rx3v3Status\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bTxCh1AmpEnable_set)
	{
		tcp_printf("{\"rsp\": \"TxCh1IfAmpStatus\",\"value_int\": \"%d\"}", !iTxCh1AmpEnable); //Note that here a 1 means enabled...
	}
	else
	{
		tcp_printf("{\"rsp\": \"TxCh1IfAmpStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bTxCh2AmpEnable_set)
	{
		tcp_printf("{\"rsp\": \"TxCh2IfAmpStatus\",\"value_int\": \"%d\"}", !iTxCh2AmpEnable); //Note that here a 1 means enabled...
	}
	else
	{
		tcp_printf("{\"rsp\": \"TxCh2IfAmpStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bRxCh1AmpEnable_set)
	{
		tcp_printf("{\"rsp\": \"RxCh1IfAmpStatus\",\"value_int\": \"%d\"}", !iRxCh1AmpEnable); //Note that here a 1 means enabled...
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxCh1IfAmpStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bRxCh2AmpEnable_set)
	{
		tcp_printf("{\"rsp\": \"RxCh2IfAmpStatus\",\"value_int\": \"%d\"}", !iRxCh2AmpEnable); //Note that here a 1 means enabled...
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxCh2IfAmpStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bTxAttenuators_set)
	{
		fAttenuator_dB = attenuation_bits_to_dB(iTxAttenuator_value);
		tcp_printf("{\"rsp\": \"TxAttenuatorsValue\",\"value_float\": \"%.1f\"}", fAttenuator_dB);
	}
	else
	{
		tcp_printf("{\"rsp\": \"TxAttenuatorsValue\",\"value_float\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bRxCh1Attenuator_set)
	{
		fAttenuator_dB = attenuation_bits_to_dB(iRxCh1Attenuator_value);
		tcp_printf("{\"rsp\": \"RxRfCh1AttenuatorValue\",\"value_float\": \"%.1f\"}", fAttenuator_dB);
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxRfCh1AttenuatorValue\",\"value_float\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	//sleep(1000); //a break
	if (bRxCh2Attenuator_set)
	{
		fAttenuator_dB = attenuation_bits_to_dB(iRxCh2Attenuator_value);
		tcp_printf("{\"rsp\": \"RxRfCh2AttenuatorValue\",\"value_float\": \"%.1f\"}", fAttenuator_dB);
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxRfCh2AttenuatorValue\",\"value_float\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bIfRxCh1Attenuator_set)
	{
		fAttenuator_dB = ((float)iIfRxCh1Attenuator_value)/((float)4);
		tcp_printf("{\"rsp\": \"RxIfCh1AttenuatorValue\",\"value_float\": \"%.2f\"}", fAttenuator_dB);
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxIfCh1AttenuatorValue\",\"value_float\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bIfRxCh2Attenuator_set)
	{
		fAttenuator_dB = ((float)iIfRxCh2Attenuator_value)/((float)4);
		tcp_printf("{\"rsp\": \"RxIfCh2AttenuatorValue\",\"value_float\": \"%.2f\"}", fAttenuator_dB);
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxIfCh2AttenuatorValue\",\"value_float\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bRxCh1BiasEnable_set)
	{
		tcp_printf("{\"rsp\": \"RxCh1BiasStatus\",\"value_int\": \"%d\"}", iRxCh1BiasEnable);
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxCh1BiasStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bRxCh2BiasEnable_set)
	{
		tcp_printf("{\"rsp\": \"RxCh2BiasStatus\",\"value_int\": \"%d\"}", iRxCh2BiasEnable);
	}
	else
	{
		tcp_printf("{\"rsp\": \"RxCh2BiasStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	if (bSoftwareLed_set)
	{
		tcp_printf("{\"rsp\": \"CommsLedStatus\",\"value_int\": \"%d\"}", iSoftwareLedState);
	}
	else
	{
		tcp_printf("{\"rsp\": \"CommsLedStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	}
	ReportTxPllStatus();
	ReportRxPllStatus();
	ReportPllPower(CMD_ACCESS_TX_PLL);
	ReportPllPower(CMD_ACCESS_RX_PLL);
	if (iconfigfile_error < 0)
	{ //Config file not found
		tcp_printf("{\"rsp\": \"ConfigFileErrorStatus\",\"value_int\": \"%s\"}", UNDKNOWN_STATUS_STRING);
	} //Config file not found
	else
	{
		tcp_printf("{\"rsp\": \"ConfigFileErrorStatus\",\"value_int\": \"%d\"}", iconfigfile_error);
	}
	tcp_printf("{\"rsp\": \"TxPllFreqStatus\",\"value_float\": \"%.3f\",\"value_int\": \"%d\",\"value_float2\": \"%.3f\",\"value_float3\": \"%.3f\",\"value_float4\": \"%.3f\"}", fTxPllFreqFpfd_MHz, ulTxPllDemominator, fTxPllFreqRF_GHz, fTxPllFreqIF_GHz, TxFvco_actual);
	tcp_printf("{\"rsp\": \"RxPllFreqStatus\",\"value_float\": \"%.3f\",\"value_int\": \"%d\",\"value_float2\": \"%.3f\",\"value_float3\": \"%.3f\",\"value_float4\": \"%.3f\"}", fRxPllFreqFpfd_MHz, ulRxPllDemominator, fRxPllFreqRF_GHz, fRxPllFreqIF_GHz, RxFvco_actual);
} //ReportStatus

void PowerTxPath(int ipower_on)
{
	if (ipower_on)
	{ // Power on
		tcp_printf("Powering up the Tx path: \n");
		// Turn ON power supply LDOs : apply positive voltage to EN_5V_TXX and EN_3V3_TXX
		// +5V_TX, +3V3_TX, -5V_TX, -3V6_TX, -2V are now ON
		iTx5VEnable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_5V_TX_EN, iTx5VEnable);
		bexpander_EN_5V_TX = iTx5VEnable;
		update_expander();
		bTx5VEnable_set = true;
		tcp_printf("Wrote to EN_5V_TX: %1d\n", iTx5VEnable);
		tcp_printf("{\"rsp\": \"Tx5vStatus\",\"value_int\": \"%d\"}", iTx5VEnable);
		iTx3V3Enable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_3V3_TX_EN, iTx3V3Enable);
		bexpander_EN_3V3_TX = iTx3V3Enable;
		update_expander();
		bTx3V3Enable_set = true;
		tcp_printf("Wrote to EN_3V3_TX: %1d\n", iTx3V3Enable);
		tcp_printf("{\"rsp\": \"Tx3v3Status\",\"value_int\": \"%d\"}", iTx3V3Enable);
		// Set TX path DSAs to max attenuation (-14dB)
		// B0 = 0, B1 = 1, B3 = 1, B4 = 0, B6 = 0, B7 =1 [need to store all 8 attenuation states in memory]
		iTxAttenuator_value = TxAttenuation_dB_to_bits(15.5); //max
		WriteParallelAttenuatorViaSpi(spi_fd, CMD_SET_TX_DSAS, iTxAttenuator_value);
		expander_tx_DSA_value = iTxAttenuator_value;
		update_expander();
		bTxAttenuators_set = true;
		tcp_printf("Wrote to the Tx Attenuators parallel interface: 0x%02X\n", iTxAttenuator_value);
		fAttenuator_dB = attenuation_bits_to_dB(iTxAttenuator_value);
		tcp_printf("Tx attenuation = 0x%02X = %g dB\n", iTxAttenuator_value, fAttenuator_dB);
		tcp_printf("{\"rsp\": \"TxAttenuatorsValue\",\"value_float\": \"%.1f\"}", fAttenuator_dB);
		//********** TEST *****************************
		usleep(1000); //a break
		//********** TEST *****************************
		// Program the TX PLL to set frequency and output power level [check existing drivers for LMX2594]
		// Output buffer should be set ON post calibration
		LoadPllFile(spi_fd, false, false, CMD_ACCESS_TX_PLL);
		bTxPLL_set = true;
		tcp_printf("Wrote file to Tx PLL\n");
		//usleep(1000); //microseconds to give TCP message time to go out before the next one
		// Check PLL MUXOUT status for VCO lock
		//********** TEST *****************************
		usleep(1000); //a break
		//********** TEST *****************************
		ReadBackPllStatus(spi_fd, CMD_ACCESS_TX_PLL);
		bTxPLL_set = true;
		tcp_printf("Finished reading back status from Tx PLL\n");
		// Enable IF AMPS: Apply 0V to TX1_AMP_ENX and TX2_AMP_ENX
		iTxCh1AmpEnable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_TX_CH1_AMP_EN, iTxCh1AmpEnable);
		bTxCh1AmpEnable_set = true;
		tcp_printf("Wrote to TX1_AMP_EN: %1d\n", iTxCh1AmpEnable);
		tcp_printf("{\"rsp\": \"TxCh1IfAmpStatus\",\"value_int\": \"%d\"}", !iTxCh1AmpEnable); //Note that here a 1 means enabled...
		iTxCh2AmpEnable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_TX_CH2_AMP_EN, iTxCh2AmpEnable);
		bTxCh2AmpEnable_set = true;
		tcp_printf("Wrote to TX2_AMP_EN: %1d\n", iTxCh2AmpEnable);
		tcp_printf("{\"rsp\": \"TxCh2IfAmpStatus\",\"value_int\": \"%d\"}", !iTxCh2AmpEnable); //Note that here a 1 means enabled...
		tcp_printf("Tx path should now be powered\n");
		ReportTxPllStatus();
		ReportPllPower(CMD_ACCESS_TX_PLL);
	} //Power on
	else
	{ // Power off
		tcp_printf("Powering down the Tx path: \n");
		// Disable IF AMPS: Apply positive voltage to TX1_AMP_ENX and TX2_AMP_ENX
		iTxCh1AmpEnable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_TX_CH1_AMP_EN, iTxCh1AmpEnable);
		bTxCh1AmpEnable_set = true;
		tcp_printf("Wrote to TX1_AMP_EN: %1d\n", iTxCh1AmpEnable);
		tcp_printf("{\"rsp\": \"TxCh1IfAmpStatus\",\"value_int\": \"%d\"}", !iTxCh1AmpEnable); //Note that here a 1 means enabled...
		iTxCh2AmpEnable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_TX_CH2_AMP_EN, iTxCh2AmpEnable);
		bTxCh2AmpEnable_set = true;
		tcp_printf("Wrote to TX2_AMP_EN: %1d\n", iTxCh2AmpEnable);
		tcp_printf("{\"rsp\": \"TxCh2IfAmpStatus\",\"value_int\": \"%d\"}", !iTxCh2AmpEnable); //Note that here a 1 means enabled...
		// Turn OFF power supply LDOs : apply 0V to EN_5V_TXX and EN_3V3_TXX
		// +5V_TX, +3V3_TX, -5V_TX, -3V6_TX, -2V are now OFF
		iTx5VEnable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_5V_TX_EN, iTx5VEnable);
		bexpander_EN_5V_TX = iTx5VEnable;
		update_expander();
		bTx5VEnable_set = true;
		tcp_printf("Wrote to EN_5V_TX: %1d\n", iTx5VEnable);
		tcp_printf("{\"rsp\": \"Tx5vStatus\",\"value_int\": \"%d\"}", iTx5VEnable);
		iTx3V3Enable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_3V3_TX_EN, iTx3V3Enable);
		bexpander_EN_3V3_TX = iTx3V3Enable;
		update_expander();
		bTx3V3Enable_set = true;
		tcp_printf("Wrote to EN_3V3_TX: %1d\n", iTx3V3Enable);
		tcp_printf("{\"rsp\": \"Tx3v3Status\",\"value_int\": \"%d\"}", iTx3V3Enable);
		tcp_printf("Tx path should now be powered down\n");
		ReportTxPllStatus();
		ReportPllPower(CMD_ACCESS_TX_PLL);
	} // Power off
} //PowerTxPath

void PowerRxPath(int ipower_on)
{
	if (ipower_on)
	{ // Power on
		tcp_printf("Powering up the Rx path: \n");
		// Turn ON power supply LDOs : apply positive voltage to EN_5V_RXX and EN_3V3_RXX
		// +5V_RX, +3V3_RX, -5V_RX, -3V6_RX  are now ON
		iRx5VEnable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_5V_RX_EN, iRx5VEnable);
		bexpander_EN_5V_RX = iRx5VEnable;
		update_expander();
		bRx5VEnable_set = true;
		tcp_printf("Wrote to EN_5V_RX: %1d\n", iRx5VEnable);
		tcp_printf("{\"rsp\": \"Rx5vStatus\",\"value_int\": \"%d\"}", iRx5VEnable);
		iRx3V3Enable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_3V3_RX_EN, iRx3V3Enable);
		bexpander_EN_3V3_RX = iRx3V3Enable;
		update_expander();
		bRx3V3Enable_set = true;
		tcp_printf("Wrote to EN_3V3_RX: %1d\n", iRx3V3Enable);
		tcp_printf("{\"rsp\": \"Rx3v3Status\",\"value_int\": \"%d\"}", iRx3V3Enable);
		// Set RX path DSA to max attenuation (-14dB)
		// B0 = 0, B1 = 1, B3 = 1, B4 = 0, B6 = 0, B7 =1 [need to store all 8 attenuation states in memory]
		iRxCh1Attenuator_value = RxAttenuation_dB_to_bits(15.5); //max
		WriteParallelAttenuatorViaSpi(spi_fd, CMD_SET_CH1_RX_DSA, iRxCh1Attenuator_value);
		bRxCh1Attenuator_set = true;
		tcp_printf("Wrote to the Rx Ch1 Attenuator parallel interface: 0x%02X\n", iRxCh1Attenuator_value);
		fAttenuator_dB = attenuation_bits_to_dB(iRxCh1Attenuator_value);
		tcp_printf("{\"rsp\": \"RxRfCh1AttenuatorValue\",\"value_float\": \"%.1f\"}", fAttenuator_dB);
		iRxCh2Attenuator_value = RxAttenuation_dB_to_bits(15.5); //max
		WriteParallelAttenuatorViaSpi(spi_fd, CMD_SET_CH2_RX_DSA, iRxCh2Attenuator_value);
		bRxCh2Attenuator_set = true;
		tcp_printf("Wrote to the Rx Ch2 Attenuator parallel interface: 0x%02X\n", iRxCh2Attenuator_value);
		fAttenuator_dB = attenuation_bits_to_dB(iRxCh2Attenuator_value);
		tcp_printf("{\"rsp\": \"RxRfCh2AttenuatorValue\",\"value_float\": \"%.1f\"}", fAttenuator_dB);
		//********** TEST *****************************
		usleep(1000); //a break
		//********** TEST *****************************
		// Program the RX PLL to set frequency and output power level [check existing drivers for LMX2594]
		// Output buffer should be set ON post calibration
		LoadPllFile(spi_fd, false, false, CMD_ACCESS_RX_PLL);
		bRxPLL_set = true;
		tcp_printf("Wrote file to Rx PLL\n");
		//********** TEST *****************************
		usleep(1000); //a break
		//********** TEST *****************************
		// Check PLL MUXOUT status for VCO lock
		ReadBackPllStatus(spi_fd, CMD_ACCESS_RX_PLL);
		bRxPLL_set = true;
		tcp_printf("Finished reading back status from Rx PLL\n");
		// Enable IF AMPS: Apply low voltage to RX1_AMP_ENX and RX2_AMP_ENX
		iRxCh1AmpEnable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_RX_CH1_AMP_EN, iRxCh1AmpEnable);
		bRxCh1AmpEnable_set = true;
		tcp_printf("Wrote to RX1_AMP_EN: %1d\n", iRxCh1AmpEnable);
		tcp_printf("{\"rsp\": \"RxCh1IfAmpStatus\",\"value_int\": \"%d\"}", !iRxCh1AmpEnable); //Note that here a 1 means enabled...
		iRxCh2AmpEnable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_RX_CH2_AMP_EN, iRxCh2AmpEnable);
		bRxCh2AmpEnable_set = true;
		tcp_printf("Wrote to RX2_AMP_EN: %1d\n", iRxCh2AmpEnable);
		tcp_printf("{\"rsp\": \"RxCh2IfAmpStatus\",\"value_int\": \"%d\"}", !iRxCh2AmpEnable); //Note that here a 1 means enabled...
		// Enable RF AMPS: Apply positive voltage to RX1_BIAS_ENX and RX2_BIAS_ENX
		iRxCh1BiasEnable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_RX_CH1_BIAS_EN, iRxCh1BiasEnable);
		bRxCh1BiasEnable_set = true;
		tcp_printf("Wrote to RX1_BIAS_EN: %1d\n", iRxCh1BiasEnable);
		tcp_printf("{\"rsp\": \"RxCh1BiasStatus\",\"value_int\": \"%d\"}", iRxCh1BiasEnable);
		iRxCh2BiasEnable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_RX_CH2_BIAS_EN, iRxCh2BiasEnable);
		bRxCh2BiasEnable_set = true;
		tcp_printf("Wrote to RX2_BIAS_EN: %1d\n", iRxCh2BiasEnable);
		tcp_printf("{\"rsp\": \"RxCh2BiasStatus\",\"value_int\": \"%d\"}", iRxCh2BiasEnable);
		iattenuator_addr = 0x00; //As per hardware address bits wiring
		iIfRxCh1Attenuator_value = 127; //only 7 bits are used - Maximum Attenuation
		iattenuator_data = (iattenuator_addr << 8) + iIfRxCh1Attenuator_value;
		WriteSerialAttenuatorViaSpi(spi_fd, CMD_SET_RFSA3713, iattenuator_data);
		bIfRxCh1Attenuator_set = true;
		tcp_printf("Wrote to the Serial Addressable Mode interface: 0x%04X\n", iattenuator_data);
		fAttenuator_dB = ((float)iIfRxCh1Attenuator_value)/((float)4);
		tcp_printf("IF Rx Ch1 RFSA3713 = %d => 0x%02X => %g dB\n", iIfRxCh1Attenuator_value, iIfRxCh1Attenuator_value, fAttenuator_dB);
		tcp_printf("{\"rsp\": \"RxIfCh1AttenuatorValue\",\"value_float\": \"%.2f\"}", fAttenuator_dB);
		iattenuator_addr = 0x01; //As per hardware address bits wiring
		iIfRxCh2Attenuator_value = 127; //only 7 bits are used - Maximum Attenuation
		iattenuator_data = (iattenuator_addr << 8) + iIfRxCh2Attenuator_value;
		WriteSerialAttenuatorViaSpi(spi_fd, CMD_SET_RFSA3713, iattenuator_data);
		bIfRxCh2Attenuator_set = true;
		tcp_printf("Wrote to the Serial Addressable Mode interface: 0x%04X\n", iattenuator_data);
		fAttenuator_dB = ((float)iIfRxCh2Attenuator_value)/((float)4);
		tcp_printf("IF Rx Ch2 RFSA3713 = %d => 0x%02X => %g dB\n", iIfRxCh2Attenuator_value, iIfRxCh2Attenuator_value, fAttenuator_dB);
		tcp_printf("{\"rsp\": \"RxIfCh2AttenuatorValue\",\"value_float\": \"%.2f\"}", fAttenuator_dB);
		tcp_printf("Rx path should now be powered\n");
		ReportRxPllStatus();
		ReportPllPower(CMD_ACCESS_RX_PLL);
	} //Power on
	else
	{ // Power off
		tcp_printf("Powering down the Rx path: \n");
		// Disable RF AMPS: Apply 0V to RX1_BIAS_ENX and RX2_BIAS_ENX
		iRxCh1BiasEnable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_RX_CH1_BIAS_EN, iRxCh1BiasEnable);
		bRxCh1BiasEnable_set = true;
		tcp_printf("Wrote to RX1_BIAS_EN: %1d\n", iRxCh1BiasEnable);
		tcp_printf("{\"rsp\": \"RxCh1BiasStatus\",\"value_int\": \"%d\"}", iRxCh1BiasEnable);
		iRxCh2BiasEnable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_RX_CH2_BIAS_EN, iRxCh2BiasEnable);
		bRxCh2BiasEnable_set = true;
		tcp_printf("Wrote to RX2_BIAS_EN: %1d\n", iRxCh1BiasEnable);
		tcp_printf("{\"rsp\": \"RxCh2BiasStatus\",\"value_int\": \"%d\"}", iRxCh2BiasEnable);
		// Disable IF AMPS: Apply high voltage to RX1_AMP_ENX and RX2_AMP_ENX
		iRxCh1AmpEnable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_RX_CH1_AMP_EN, iRxCh1AmpEnable);
		bRxCh1AmpEnable_set = true;
		tcp_printf("Wrote to RX1_AMP_EN: %1d\n", iRxCh1AmpEnable);
		tcp_printf("{\"rsp\": \"RxCh1IfAmpStatus\",\"value_int\": \"%d\"}", !iRxCh1AmpEnable); //Note that here a 1 means enabled...
		iRxCh2AmpEnable = 1;
		WriteBitViaSpi(spi_fd, CMD_SET_RX_CH2_AMP_EN, iRxCh2AmpEnable);
		bRxCh2AmpEnable_set = true;
		tcp_printf("Wrote to RX2_AMP_EN: %1d\n", iRxCh2AmpEnable);
		tcp_printf("{\"rsp\": \"RxCh2IfAmpStatus\",\"value_int\": \"%d\"}", !iRxCh2AmpEnable); //Note that here a 1 means enabled...
		// Turn OFF power supply LDOs : apply positive voltage to EN_5V_RXX and EN_3V3_RXX
		// +5V_RX, +3V3_RX, -5V_RX, -3V6_RX  are now OFF
		iRx5VEnable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_5V_RX_EN, iRx5VEnable);
		bexpander_EN_5V_RX = iRx5VEnable;
		update_expander();
		bRx5VEnable_set = true;
		tcp_printf("Wrote to EN_5V_RX: %1d\n", iRx5VEnable);
		tcp_printf("{\"rsp\": \"Rx5vStatus\",\"value_int\": \"%d\"}", iRx5VEnable);
		iRx3V3Enable = 0;
		WriteBitViaSpi(spi_fd, CMD_SET_3V3_RX_EN, iRx3V3Enable);
		bexpander_EN_3V3_RX = iRx3V3Enable;
		update_expander();
		bRx3V3Enable_set = true;
		tcp_printf("Wrote to EN_3V3_RX: %1d\n", iRx3V3Enable);
		tcp_printf("{\"rsp\": \"Rx3v3Status\",\"value_int\": \"%d\"}", iRx3V3Enable);
		tcp_printf("Rx path should now be powered down\n");
		ReportRxPllStatus();
		ReportPllPower(CMD_ACCESS_RX_PLL);
	} // Power off
} //PowerRxPath

void PowerDownAll(void)
{
	PowerTxPath(0);
	PowerRxPath(0);
	tcp_printf("Both Tx and Rx paths should now be powered down\n");
} //PowerDownAll

void ChangePllFrequency(char cPll, float Fpfd, double Fvco)
{
	tcp_printf("FpFd = %gMHz.  The new LO frequency in Hz = %g\n", Fpfd, Fvco);
	//tcp_printf("FpFd = %gMHz.  The new LO frequency in Hz = %.0f\n", Fpfd, Fvco);
	if (Fpfd == (float)122.88)
	{ //Change the LO frequency for Fpfd = 122.88MHz
		ulFpfd = 122880000;
		if (cPll == 'T') //Tx PLL
		{
		    SetPllFrequency(spi_fd, CMD_ACCESS_TX_PLL, Fvco);
			bTxPLL_set = true;
			tcp_printf("Updated the Tx LO frequency in the PLL\n");
		}
		else //Rx PLL
		{
		    SetPllFrequency(spi_fd, CMD_ACCESS_RX_PLL, Fvco);
			bRxPLL_set = true;
			tcp_printf("Updated the Rx LO frequency in the PLL\n");
		}
	}
	else if (Fpfd == (float)61.44)
	{ //Change the LO frequency for Fpfd = 61.44MHz
		ulFpfd = 61440000;
		if (cPll == 'T') //Tx PLL
		{
		    SetPllFrequency(spi_fd, CMD_ACCESS_TX_PLL, Fvco);
			bTxPLL_set = true;
			tcp_printf("Updated the Tx LO frequency in the PLL\n");
		}
		else //Rx PLL
		{
		    SetPllFrequency(spi_fd, CMD_ACCESS_RX_PLL, Fvco);
			bRxPLL_set = true;
			tcp_printf("Updated the Rx LO frequency in the PLL\n");
		}
	}
	if (cPll == 'T') //Tx PLL
	{
		tcp_printf("{\"rsp\": \"TxPllFrequency\",\"value_int\": \"%d\",\"value_float\": \"%.1f\"}", ulTxPllDemominator, Fvco_actual);
		TxFvco_actual = Fvco_actual;
		ReportTxPllStatus();
	}
	else //Rx PLL
	{
		tcp_printf("{\"rsp\": \"RxPllFrequency\",\"value_int\": \"%d\",\"value_float\": \"%.1f\"}", ulRxPllDemominator, Fvco_actual);
		RxFvco_actual = Fvco_actual;
		ReportRxPllStatus();
	}
} //ChangePllFrequency

void ChangePllPower(char cPll, unsigned char OutAEnable, int PowerA, unsigned char OutBEnable, int PowerB)
{
	unsigned char command;
	unsigned int iOldPLL_value;
	unsigned int iNewPLL_value;
	if (cPll == 'T') //Tx PLL
	{
		command = CMD_ACCESS_TX_PLL;
	}
	else //Rx PLL
	{
		command = CMD_ACCESS_RX_PLL;
	}

    printf("OUTA_EN : %i\n", OutAEnable);
    printf("OUTB_EN : %i\n", OutBEnable);
    ucPLL_register = 44; //R44[6] and [7]
    iOldPLL_value = ReadPllRegisterViaSpi(spi_fd, command, ucPLL_register, true);
    iNewPLL_value = (iOldPLL_value & 0xFF3F) | (OutAEnable << 6) | (OutBEnable << 7); //Replace bits 6 and 7
    printf("OUTA_PWR : %i\n", PowerA);
    //ucPLL_register = 44; //R44[13:8]
    iNewPLL_value = (iNewPLL_value & 0xC0FF) | (PowerA << 8); //Replace bits 8 to 13
    WritePllRegisterViaSpi(spi_fd, command, ucPLL_register, iNewPLL_value, true);

    printf("OUTB_PWR : %i\n", PowerB);
    ucPLL_register = 45; //R45[5:0]
    iOldPLL_value = ReadPllRegisterViaSpi(spi_fd, command, ucPLL_register, true);
    iNewPLL_value = (iOldPLL_value & 0xFFC0) | (PowerB << 0); //Replace bits 0 to 5
    WritePllRegisterViaSpi(spi_fd, command, ucPLL_register, iNewPLL_value, true);

	ReportPllPower(command);
	if (cPll == 'T') //Tx PLL
	{
		tcp_printf("Updated the Tx power in the PLL\n");
	}
	else //Rx PLL
	{
		tcp_printf("Updated the Rx power in the PLL\n");
	}
} //ChangePllPower

char eeprom_version_string[32];
int GetPllPowerFromFreq(unsigned char ucOUTX_enabled, double dfrequency_achievedHz)
{
unsigned char ucBoardIsRevB;
double dfrequency_achievedGHz = dfrequency_achievedHz / (double)1000000000;
int ipower;
long ulEepromSN = 0;
int bEeprom_error = false;

	if (ucOUTX_enabled == 0) //enabled
	{
   		//Read the EEPROM to decide whether we have a RevB board
		//From Matlab code:
		//if str2double(app.EEPROM_DTRx2.boardSerialNum{1}.value_long) > 15 && str2double(app.EEPROM_DTRx2.boardVersion{1}.value_string) == 2
		// app.EEPROM_DTRx2.boardRevString = 'RevB';
		// success = true;
		//else
		// app.EEPROM_DTRx2.boardRevString = 'RevA';
		// success = true;
		//end
   		host_value_ucptr = (unsigned char*)&ulEepromSN; // not endian safe
		if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_SERIALNUM, host_value_ucptr, 3) == 0)
		{
			//tcp_printf("{\"rsp\": \"EepromBoardSerialNum\",\"value_long\": \"%d\"}", host_value_long);
		}
		else
		{ //EEPROM error
			bEeprom_error = true;
		} //EEPROM error
   		host_value_ucptr = eeprom_version_string;
		if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_VERSION, host_value_ucptr, 1) ==0)
		{
			//tcp_printf("{\"rsp\": \"EepromBoardVersion\",\"value_string\": \"%s\"}", host_value_string);
		}
		else
		{ //EEPROM error
			bEeprom_error = true;
		} //EEPROM error

		if ((bEeprom_error == false) && (ulEepromSN > 15) && ((strcmp(eeprom_version_string, "2") == 0))) //Value of 2 indicates RevB card
		{
			ucBoardIsRevB = true;
		}
		else
		{
			ucBoardIsRevB = false; //Rev A assumed
		}

		if (ucBoardIsRevB)
		{ //RevB
			if (dfrequency_achievedGHz <= 12.0)
			{
				ipower = 20;
			}
			else if (dfrequency_achievedGHz < 12.65)
			{
				ipower = 5;
			}
			else if (dfrequency_achievedGHz > 13.65)
			{
				ipower = 15;
			}
			else
			{
				double dpower = (double)10 * dfrequency_achievedGHz - (double)121.5;
				ipower = (int)(dpower + (double)0.5); //Round to closest integer by adding 0.5 and truncating
			}
		} //RevB
		else
		{ //not RevB (assumed RevA)
			if (dfrequency_achievedGHz < 12.1)
			{
				ipower = 20;
			}
			else if (dfrequency_achievedGHz <= 12.9)
			{
				ipower = 5;
			}
			else
			{
				ipower = 15;
			}
		} //not RevB (assumed RevA)
	} //ucOUTX_enabled
	else
	{ //ucOUTX_enabled == 1 (not enabled)
		ipower = 0;
	} //ucOUTX_enabled == 1 (not enabled)
    return ipower;
} //GetPllPowerFromFreq

int ChangePllPowerAuto(char cPll, unsigned char OutAEnable, unsigned char OutBEnable)
{
	int PowerA, PowerB;
	//First check if the frequency has already been set:
	if (cPll == 'T') //Tx PLL
	{
		Fvco_actual = TxFvco_actual;
	}
	else //Rx PLL
	{
		Fvco_actual = RxFvco_actual;
	}
	if (Fvco_actual == -1.0)
	{ //It has not been set yet
	    printf("**** Error! Fvco_actual must be set before the PLL power can be auto-changed ****\n");
	    return false;
	} //It has not been set yet

	//Calculate the power from the frequency
	PowerA = GetPllPowerFromFreq(OutAEnable, Fvco_actual);
	PowerB = GetPllPowerFromFreq(OutBEnable, Fvco_actual);
	ChangePllPower(cPll, OutAEnable, PowerA, OutBEnable, PowerB);
    return true;
} //ChangePllPowerAuto

void SetCommsLed(int isetting)
{
	iSoftwareLedState = isetting;
	WriteCommsLedViaSpi(spi_fd, iSoftwareLedState);
	bSoftwareLed_set = true;
	tcp_printf("{\"rsp\": \"CommsLedStatus\",\"value_int\": \"%d\"}", iSoftwareLedState);
} //ChangePllPower

void WritePllRegister(char cPll, unsigned char ucPLL_register, unsigned int iPLL_value)
{
	if (cPll == 'T') //Tx PLL
	{
		WritePllRegisterViaSpi(spi_fd, CMD_ACCESS_TX_PLL, ucPLL_register, iPLL_value, true);
		bTxPLL_set = true;
		tcp_printf("Wrote 0x%04X to Tx PLL register #%d\n", iPLL_value, ucPLL_register);
	}
	else
	{
		WritePllRegisterViaSpi(spi_fd, CMD_ACCESS_RX_PLL, ucPLL_register, iPLL_value, true);
		bRxPLL_set = true;
		tcp_printf("Wrote 0x%04X to Rx PLL register #%d\n", iPLL_value, ucPLL_register);
	}
	ReportTxPllStatus();
	ReportRxPllStatus();
} //WritePllRegister

void ReadPllRegister(char cPll, unsigned char ucPLL_register)
{
	int iPLL_value;
	if (cPll == 'T') //Tx PLL
	{
		iPLL_value = ReadPllRegisterViaSpi(spi_fd, CMD_ACCESS_TX_PLL, ucPLL_register, true);
		bTxPLL_set = true;
		tcp_printf("Read 0x%04X from Tx PLL register #%d\n", iPLL_value, ucPLL_register);
	}
	else
	{
		iPLL_value = ReadPllRegisterViaSpi(spi_fd, CMD_ACCESS_RX_PLL, ucPLL_register, true);
		bRxPLL_set = true;
		tcp_printf("Read 0x%04X from Rx PLL register #%d\n", iPLL_value, ucPLL_register);
	}
} //ReadPllRegister

void PllReadAllRegisters(char cPll)
{
	if (cPll == 'T') //Tx PLL
	{
		tcp_printf("Reading back all registers in the Tx PLL:\n");
		ReadEntirePll(spi_fd, CMD_ACCESS_TX_PLL, true);
		bTxPLL_set = true;
	}
	else
	{
		tcp_printf("Reading back all registers in the Rx PLL:\n");
		ReadEntirePll(spi_fd, CMD_ACCESS_RX_PLL, true);
		bRxPLL_set = true;
	}
} //PllReadAllRegisters

void PllGetStatus(char cPll)
{
	if (cPll == 'T') //Tx PLL
	{
		ReadBackPllStatusForTcp(spi_fd, CMD_ACCESS_TX_PLL);
		bTxPLL_set = true;
		tcp_printf("Finished reading back status from Tx PLL\n");
	}
	else
	{
		ReadBackPllStatusForTcp(spi_fd, CMD_ACCESS_RX_PLL);
		bRxPLL_set = true;
		tcp_printf("Finished reading back status from Rx PLL\n");
	}
} //PllGetStatus

void SetTxAttenuators(float fAttenuation)
{
	iTxAttenuator_value = TxAttenuation_dB_to_bits(fAttenuation);
	if (iTxAttenuator_value >= 0)
	{
		WriteParallelAttenuatorViaSpi(spi_fd, CMD_SET_TX_DSAS, iTxAttenuator_value);
		expander_tx_DSA_value = iTxAttenuator_value;
		update_expander();
		bTxAttenuators_set = true;
		tcp_printf("Wrote to the Tx Attenuators parallel interface: 0x%02X\n", iTxAttenuator_value);
		tcp_printf("Note that bits 2 and 5 are 0 and do not go to the board.  They are tied low.\n");
		//Report the settings back to the host:
		fAttenuator_dB = attenuation_bits_to_dB(iTxAttenuator_value);
		tcp_printf("{\"rsp\": \"TxAttenuatorsValue\",\"value_float\": \"%g\"}", fAttenuator_dB);
	}
} //SetTxAttenuators

void SetRxRfCh1Attenuator(float fAttenuation)
{
	iRxCh1Attenuator_value = RxAttenuation_dB_to_bits(fAttenuation);
	if (iRxCh1Attenuator_value >= 0)
	{
		WriteParallelAttenuatorViaSpi(spi_fd, CMD_SET_CH1_RX_DSA, iRxCh1Attenuator_value);
		bRxCh1Attenuator_set = true;
		tcp_printf("Wrote to the Rx Channel 1 parallel interface: 0x%02X\n", iRxCh1Attenuator_value);
		tcp_printf("Note that bits 2 and 5 are 0 and do not go to the board.  They are tied low.\n");
		//Report the settings back to the host:
		fAttenuator_dB = attenuation_bits_to_dB(iRxCh1Attenuator_value);
		tcp_printf("{\"rsp\": \"RxRfCh1AttenuatorValue\",\"value_float\": \"%.1f\"}", fAttenuator_dB);
	}
} //SetRxRfCh1Attenuator

void SetRxRfCh2Attenuator(float fAttenuation)
{
	iRxCh2Attenuator_value = RxAttenuation_dB_to_bits(fAttenuation);
	if (iRxCh2Attenuator_value >= 0)
	{
		WriteParallelAttenuatorViaSpi(spi_fd, CMD_SET_CH2_RX_DSA, iRxCh2Attenuator_value);
		bRxCh2Attenuator_set = true;
		tcp_printf("Wrote to the Rx Channel 2 parallel interface: 0x%02X\n", iRxCh2Attenuator_value);
		tcp_printf("Note that bits 2 and 5 are 0 and do not go to the board.  They are tied low.\n");
		//Report the settings back to the host:
		fAttenuator_dB = attenuation_bits_to_dB(iRxCh2Attenuator_value);
		tcp_printf("{\"rsp\": \"RxRfCh2AttenuatorValue\",\"value_float\": \"%.1f\"}", fAttenuator_dB);
	}
} //SetRxRfCh1Attenuator

void SetRxIfCh1Attenuator(float fAttenuation)
{
	iIfRxCh1Attenuator_value = (int)(fAttenuation * 4); //0.25 dB for each LSB
	iattenuator_addr = 0x00; //As per hardware address bits wiring
	iattenuator_data = (iattenuator_addr << 8) + iIfRxCh1Attenuator_value;
	WriteSerialAttenuatorViaSpi(spi_fd, CMD_SET_RFSA3713, iattenuator_data);
	bIfRxCh1Attenuator_set = true;
	tcp_printf("Wrote to the Serial Addressable Mode interface: 0x%04X\n", iattenuator_data);
	tcp_printf("Rx Ch1 RFSA3713 attenuator set to: 0x%04X\n", iIfRxCh1Attenuator_value);
	//Report the settings back to the host:
	fAttenuator_dB = ((float)iIfRxCh1Attenuator_value)/((float)4);
	tcp_printf("IF Rx Ch1 RFSA3713 = %d => 0x%02X => %g dB\n", iIfRxCh1Attenuator_value, iIfRxCh1Attenuator_value, fAttenuator_dB);
	tcp_printf("{\"rsp\": \"RxIfCh1AttenuatorValue\",\"value_float\": \"%.2f\"}", fAttenuator_dB);
} //SetRxIfCh1Attenuator

void SetRxIfCh2Attenuator(float fAttenuation)
{
	iIfRxCh2Attenuator_value = (int)(fAttenuation * 4); //0.25 dB for each LSB
	iattenuator_addr = 0x01; //As per hardware address bits wiring
	iattenuator_data = (iattenuator_addr << 8) + iIfRxCh2Attenuator_value;
	WriteSerialAttenuatorViaSpi(spi_fd, CMD_SET_RFSA3713, iattenuator_data);
	bIfRxCh2Attenuator_set = true;
	tcp_printf("Wrote to the Serial Addressable Mode interface: 0x%04X\n", iattenuator_data);
	tcp_printf("Rx Ch2 RFSA3713 attenuator set to: 0x%04X\n", iIfRxCh2Attenuator_value);
	//Report the settings back to the host:
	fAttenuator_dB = ((float)iIfRxCh2Attenuator_value)/((float)4);
	tcp_printf("IF Rx Ch2 RFSA3713 = %d => 0x%02X => %g dB\n", iIfRxCh2Attenuator_value, iIfRxCh2Attenuator_value, fAttenuator_dB);
	tcp_printf("{\"rsp\": \"RxIfCh2AttenuatorValue\",\"value_float\": \"%.2f\"}", fAttenuator_dB);
} //SetRxIfCh2Attenuator

void SetTx5v(unsigned char bEnable)
{
	bexpander_EN_5V_TX = bEnable;
	update_expander();
	iTx5VEnable = bEnable;
	WriteBitViaSpi(spi_fd, CMD_SET_5V_TX_EN, iTx5VEnable);
	bTx5VEnable_set = true;
	tcp_printf("Wrote to EN_5V_TX: %1d\n", iTx5VEnable);
	tcp_printf("{\"rsp\": \"Tx5vStatus\",\"value_int\": \"%d\"}", iTx5VEnable);
} //SetTx5v

void SetTx3v3(unsigned char bEnable)
{
	bexpander_EN_3V3_TX = bEnable;
	update_expander();
	iTx3V3Enable = bEnable;
	WriteBitViaSpi(spi_fd, CMD_SET_5V_TX_EN, iTx3V3Enable);
	bTx3V3Enable_set = true;
	tcp_printf("Wrote to EN_3V3_TX: %1d\n", iTx3V3Enable);
	tcp_printf("{\"rsp\": \"Tx3v3Status\",\"value_int\": \"%d\"}", iTx3V3Enable);
	ReportTxPllStatus();
} //SetTx3v3

void SetRx5v(unsigned char bEnable)
{
	bexpander_EN_5V_RX = bEnable;
	update_expander();
	iRx5VEnable = bEnable;
	WriteBitViaSpi(spi_fd, CMD_SET_5V_RX_EN, iRx5VEnable);
	bRx5VEnable_set = true;
	tcp_printf("Wrote to EN_5V_RX: %1d\n", iRx5VEnable);
	tcp_printf("{\"rsp\": \"Rx5vStatus\",\"value_int\": \"%d\"}", iRx5VEnable);
} //SetRx5v

void SetRx3v3(unsigned char bEnable)
{
	bexpander_EN_3V3_RX = bEnable;
	update_expander();
	iRx3V3Enable = bEnable;
	WriteBitViaSpi(spi_fd, CMD_SET_3V3_RX_EN, iRx3V3Enable);
	bRx3V3Enable_set = true;
	tcp_printf("Wrote to EN_3V3_RX: %1d\n", iRx3V3Enable);
	tcp_printf("{\"rsp\": \"Rx3v3Status\",\"value_int\": \"%d\"}", iRx3V3Enable);
	ReportRxPllStatus();
} //SetRx3v3

void SetTxCh1IfAmp(unsigned char bEnable)
{
	iTxCh1AmpEnable = !bEnable; //0 to enable
	WriteBitViaSpi(spi_fd, CMD_SET_TX_CH1_AMP_EN, iTxCh1AmpEnable);
	bTxCh1AmpEnable_set = true;
	tcp_printf("Wrote to TX1_AMP_EN: %1d\n", iTxCh1AmpEnable);
	tcp_printf("{\"rsp\": \"TxCh1IfAmpStatus\",\"value_int\": \"%d\"}", !iTxCh1AmpEnable); //Note that here a 1 means enabled...
} //SetTxCh1IfAmp

void SetTxCh2IfAmp(unsigned char bEnable)
{
	iTxCh2AmpEnable = !bEnable; //0 to enable
	WriteBitViaSpi(spi_fd, CMD_SET_TX_CH2_AMP_EN, iTxCh2AmpEnable);
	bTxCh2AmpEnable_set = true;
	tcp_printf("Wrote to TX2_AMP_EN: %1d\n", iTxCh2AmpEnable);
	tcp_printf("{\"rsp\": \"TxCh2IfAmpStatus\",\"value_int\": \"%d\"}", !iTxCh2AmpEnable); //Note that here a 1 means enabled...
} //SetTxCh2IfAmp

void SetRxCh1IfAmp(unsigned char bEnable)
{
	iRxCh1AmpEnable = !bEnable; //0 to enable
	WriteBitViaSpi(spi_fd, CMD_SET_RX_CH1_AMP_EN, iRxCh1AmpEnable);
	bRxCh1AmpEnable_set = true;
	tcp_printf("Wrote to RX1_AMP_EN: %1d\n", iRxCh1AmpEnable);
	tcp_printf("{\"rsp\": \"RxCh1IfAmpStatus\",\"value_int\": \"%d\"}", !iRxCh1AmpEnable); //Note that here a 1 means enabled...
} //SetRxCh1IfAmp

void SetRxCh2IfAmp(unsigned char bEnable)
{
	iRxCh2AmpEnable = !bEnable; //0 to enable
	WriteBitViaSpi(spi_fd, CMD_SET_RX_CH2_AMP_EN, iRxCh2AmpEnable);
	bRxCh2AmpEnable_set = true;
	tcp_printf("Wrote to RX2_AMP_EN: %1d\n", iRxCh2AmpEnable);
	tcp_printf("{\"rsp\": \"RxCh2IfAmpStatus\",\"value_int\": \"%d\"}", !iRxCh2AmpEnable); //Note that here a 1 means enabled...
} //SetRxCh2IfAmp

void SetRxCh1Bias(unsigned char bEnable)
{
	iRxCh1BiasEnable = bEnable;
	WriteBitViaSpi(spi_fd, CMD_SET_RX_CH1_BIAS_EN, iRxCh1BiasEnable);
	bRxCh1BiasEnable_set = true;
	tcp_printf("Wrote to RX1_BIAS_EN: %1d\n", iRxCh1BiasEnable);
	tcp_printf("{\"rsp\": \"RxCh1BiasStatus\",\"value_int\": \"%d\"}", iRxCh1BiasEnable);
} //SetRxCh1Bias

void SetRxCh2Bias(unsigned char bEnable)
{
	iRxCh2BiasEnable = bEnable;
	WriteBitViaSpi(spi_fd, CMD_SET_RX_CH2_BIAS_EN, iRxCh2BiasEnable);
	bRxCh2BiasEnable_set = true;
	tcp_printf("Wrote to RX2_BIAS_EN: %1d\n", iRxCh2BiasEnable);
	tcp_printf("{\"rsp\": \"RxCh2BiasStatus\",\"value_int\": \"%d\"}", iRxCh2BiasEnable);
} //SetRxCh2Bias

void ReadEeprom(unsigned char ucEeprom_addr, int len)
{
	unsigned char addr;
	unsigned char ucEeprom_value;
	if (eeprom_OK)
	{
		for (int i = 0;i < len;i++)
		{
			addr = ucEeprom_addr + i;
			read_eeprom(&cots_eeprom, addr, &ucEeprom_value);
			tcp_printf("From EEPROM addr %d read 0x%02X\n", addr, ucEeprom_value);

		}
	}
	else
	{
		tcp_printf("EEPROM faulty or not present\n");
	}
} //ReadEeprom

void WriteEeprom(unsigned char ucEeprom_addr, unsigned char ucEeprom_value)
{
	if (eeprom_OK)
	{
		write_eeprom(&cots_eeprom, ucEeprom_addr, ucEeprom_value);
		tcp_printf("Wrote to EEPROM Addr #%d, 0x%02X\n", ucEeprom_addr, ucEeprom_value);
	}
	else
	{
		tcp_printf("EEPROM faulty or not present\n");
	}
} //WriteEeprom

int WriteEeprom_Multiple(unsigned char ucEeprom_addr, unsigned char * ucEeprom_value, int len)
{
	unsigned char addr;
	unsigned char value;
	if (eeprom_OK)
	{
		for (int i = 0;i < len;i++)
		{
			addr = ucEeprom_addr + i;
			value = ucEeprom_value[i];
			write_eeprom(&cots_eeprom, addr, value);
			tcp_printf("Wrote to EEPROM Addr #%d, 0x%02X\n", addr, value);
		}
		return 0;
	}
	else
	{
		tcp_printf("EEPROM faulty or not present\n");
		return -1;
	}
} //WriteEeprom_Multiple

int ReadEeprom_Multiple(unsigned char ucEeprom_addr, unsigned char * ucEeprom_value, int len)
{
	unsigned char addr;
	unsigned char value;
	if (eeprom_OK)
	{
		for (int i = 0;i < len;i++)
		{
			addr = ucEeprom_addr + i;
			read_eeprom(&cots_eeprom, addr, &value);
			ucEeprom_value[i] = value;
			tcp_printf("Read from EEPROM Addr #%d, 0x%02X\n", addr, value);
		}
		ucEeprom_value[len] = 0; //null-terminate to remove history of previous reads
		return 0;
	}
	else
	{
		tcp_printf("EEPROM faulty or not present\n");
		return -1;
	}
} //ReadEeprom_Multiple

void EraseEeprom()
{
	unsigned char addr;
	unsigned char ucEeprom_value = 0xFF; //Erase value
	if (eeprom_OK)
	{
		for (int i = 0;i < 128;i++)
		{
			addr = i;
			write_eeprom(&cots_eeprom, addr, ucEeprom_value);
		}
		tcp_printf("EEPROM Erased\n");
	}
	else
	{
		tcp_printf("EEPROM faulty or not present\n");
	}
} //EraseEeprom

int parse_json_message(const char * const monitor)
{
    const cJSON *cmd = NULL;
    const cJSON *setting = NULL;
    const cJSON *setting1 = NULL;
    const cJSON *setting2 = NULL;
    const cJSON *value_int = NULL;
    const cJSON *value_int1 = NULL;
    const cJSON *value_int2 = NULL;
    const cJSON *value_long = NULL;
    const cJSON *value_float = NULL;
    const cJSON *value_float2 = NULL;
    const cJSON *value_float3 = NULL;
    const cJSON *value_len = NULL;
    const cJSON *value_string = NULL;
    const cJSON *value_year = NULL;
    const cJSON *value_month = NULL;
    const cJSON *value_day = NULL;
    const cJSON *addr = NULL;
	char input_string[100];
    int status = 1; //pass by default
    cJSON *message_json = cJSON_Parse(monitor);
    if (message_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        status = 0;
        goto end;
    }

    cmd = cJSON_GetObjectItemCaseSensitive(message_json, "cmd");
    if (cJSON_IsString(cmd) && (cmd->valuestring != NULL))
    {
    	sprintf (host_cmd, "%s", cmd->valuestring); //This will null_terminate the string.  Needed because cJSON strings cannot be used for strcmp...
    }
	setting1 = cJSON_GetObjectItemCaseSensitive(message_json, "setting1");
    if (cJSON_IsString(setting1) && (setting1->valuestring != NULL))
    {
    	sprintf (host_setting1, "%s", setting1->valuestring);
    }
	setting2 = cJSON_GetObjectItemCaseSensitive(message_json, "setting2");
    if (cJSON_IsString(setting2) && (setting2->valuestring != NULL))
    {
    	sprintf (host_setting2, "%s", setting2->valuestring);
    }
	setting = cJSON_GetObjectItemCaseSensitive(message_json, "setting");
    if (cJSON_IsString(setting) && (setting->valuestring != NULL))
    {
    	sprintf (host_setting, "%s", setting->valuestring);
    }
    value_int1 = cJSON_GetObjectItemCaseSensitive(message_json, "value_int1");
    if (cJSON_IsString(value_int1) && (value_int1->valuestring != NULL))
    {
    	sprintf (input_string, "%s", value_int1->valuestring);
   		if (strstr(input_string, "0x"))
   		{//this is hex
   			char *hexstring = strstr(input_string, "0x");
   			host_value_int1 = (int)strtol(hexstring, NULL, 0);
    	}
    	else
    	{ //not hex
    		host_value_int1 = atoi(input_string);
    	}
    }
    value_int2 = cJSON_GetObjectItemCaseSensitive(message_json, "value_int2");
    if (cJSON_IsString(value_int2) && (value_int2->valuestring != NULL))
    {
    	sprintf (input_string, "%s", value_int2->valuestring);
   		if (strstr(input_string, "0x"))
   		{//this is hex
   			char *hexstring = strstr(input_string, "0x");
   			host_value_int2 = (int)strtol(hexstring, NULL, 0);
    	}
    	else
    	{ //not hex
    		host_value_int2 = atoi(input_string);
    	}
    }
    value_int = cJSON_GetObjectItemCaseSensitive(message_json, "value_int");
    if (cJSON_IsString(value_int) && (value_int->valuestring != NULL))
    {
    	sprintf (input_string, "%s", value_int->valuestring);
   		if (strstr(input_string, "0x"))
   		{//this is hex
   			char *hexstring = strstr(input_string, "0x");
   			host_value_int = (int)strtol(hexstring, NULL, 0);
    	}
    	else
    	{ //not hex
    		host_value_int = atoi(input_string);
    	}
    }
    value_long = cJSON_GetObjectItemCaseSensitive(message_json, "value_long");
    if (cJSON_IsString(value_long) && (value_long->valuestring != NULL))
    {
    	sprintf (input_string, "%s", value_long->valuestring);
   		if (strstr(input_string, "0x"))
   		{//this is hex
   			char *hexstring = strstr(input_string, "0x");
   			host_value_long = (int)strtol(hexstring, NULL, 0);
    	}
    	else
    	{ //not hex
    		host_value_long = atoi(input_string);
    	}
    }
    value_year = cJSON_GetObjectItemCaseSensitive(message_json, "value_year");
    if (cJSON_IsString(value_year) && (value_year->valuestring != NULL))
    {
    	sprintf (host_value_string_year, "%s", value_year->valuestring);
    }
    value_month = cJSON_GetObjectItemCaseSensitive(message_json, "value_month");
    if (cJSON_IsString(value_month) && (value_month->valuestring != NULL))
    {
    	sprintf (host_value_string_month, "%s", value_month->valuestring);
    }
    value_day = cJSON_GetObjectItemCaseSensitive(message_json, "value_day");
    if (cJSON_IsString(value_day) && (value_day->valuestring != NULL))
    {
    	sprintf (host_value_string_day, "%s", value_day->valuestring);
    }
    value_string = cJSON_GetObjectItemCaseSensitive(message_json, "value_string");
    if (cJSON_IsString(value_string) && (value_string->valuestring != NULL))
    {
    	sprintf (host_value_string, "%s", value_string->valuestring);
    	host_value_string_len = strlen(host_value_string);
    }
    value_len = cJSON_GetObjectItemCaseSensitive(message_json, "len");
    if (cJSON_IsString(value_len) && (value_len->valuestring != NULL))
    {
    	sprintf (input_string, "%s", value_len->valuestring);
   		if (strstr(input_string, "0x"))
   		{//this is hex
   			char *hexstring = strstr(input_string, "0x");
   			host_value_len = (int)strtol(hexstring, NULL, 0);
    	}
    	else
    	{ //not hex
    		host_value_len = atoi(input_string);
    	}
    }
    value_float = cJSON_GetObjectItemCaseSensitive(message_json, "value_float");
    if (cJSON_IsString(value_float) && (value_float->valuestring != NULL))
    {
    	sprintf (input_string, "%s", value_float->valuestring);
    	host_value_float = (atof(input_string));
    }
    value_float2 = cJSON_GetObjectItemCaseSensitive(message_json, "value_float2");
    if (cJSON_IsString(value_float2) && (value_float2->valuestring != NULL))
    {
    	sprintf (input_string, "%s", value_float2->valuestring);
    	host_value_float2 = (atof(input_string));
    }
    value_float3 = cJSON_GetObjectItemCaseSensitive(message_json, "value_float3");
    if (cJSON_IsString(value_float3) && (value_float3->valuestring != NULL))
    {
    	sprintf (input_string, "%s", value_float3->valuestring);
    	host_value_float3 = (atof(input_string));
    }
    addr = cJSON_GetObjectItemCaseSensitive(message_json, "addr");
    if (cJSON_IsString(addr) && (addr->valuestring != NULL))
    {
    	sprintf (input_string, "%s", addr->valuestring);
   		if (strstr(input_string, "0x"))
   		{//this is hex
   			char *hexstring = strstr(input_string, "0x");
   			host_addr = (int)strtol(hexstring, NULL, 0);
    	}
    	else
    	{ //not hex
    		host_addr = atoi(input_string);
    	}
    }
end:
    cJSON_Delete(message_json);
    return status;
} //parse_json_message

int processJsonCommand(void)
{
	if (parse_json_message(json_cmd_string))
	{
		if (strcmp(host_cmd, "GetVersions") == 0)
		{
			ReportVersions();
		} //host_cmd = "GetVersions"
		else if (strcmp(host_cmd, "SetCommsLed") == 0)
		{
			if (strcmp(host_setting, "on") == 0)
			{
				SetCommsLed(1);
			} //host_setting = "on"
			else if (strcmp(host_setting, "off") == 0)
			{
				SetCommsLed(0);
			} //host_setting = "off"
		} //host_cmd = "SetCommsLed"
		else if (strcmp(host_cmd, "GetStatus") == 0)
		{
			ReportStatus();
		} //host_cmd = "GetStatus"
		else if (strcmp(host_cmd, "PowerTxPath") == 0)
		{
			if (strcmp(host_setting, "on") == 0)
			{
				PowerTxPath(1);
			} //host_setting = "on"
			else if (strcmp(host_setting, "off") == 0)
			{
				PowerTxPath(0);
			} //host_setting = "off"
		} //host_cmd = "PowerTxPath"
		else if (strcmp(host_cmd, "PowerRxPath") == 0)
		{
			if (strcmp(host_setting, "on") == 0)
			{
				PowerRxPath(1);
			} //host_setting = "on"
			else if (strcmp(host_setting, "off") == 0)
			{
				PowerRxPath(0);
			} //host_setting = "off"
		} //host_cmd = "PowerRxPath"
		else if (strcmp(host_cmd, "PowerDownAll") == 0)
		{
			if (strcmp(host_setting, "off") == 0)
			{
				PowerDownAll();
			} //host_setting = "off"
		} //host_cmd = "PowerDownAll"
		else if (strcmp(host_cmd, "SetTx5vEnable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetTx5v(true);
			} //host_setting = "enable
			else
			{
				SetTx5v(false);
			} //host_setting = "disable"
		} //host_cmd = "SetTx5vEnable"
		else if (strcmp(host_cmd, "SetTx3v3Enable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetTx3v3(true);
			} //host_setting = "enable
			else
			{
				SetTx3v3(false);
			} //host_setting = "disable"
		} //host_cmd = "SetTx3v3Enable"
		else if (strcmp(host_cmd, "SetRx5vEnable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetRx5v(true);
			} //host_setting = "enable
			else
			{
				SetRx5v(false);
			} //host_setting = "disable"
		} //host_cmd = "SetRx5vEnable"
		else if (strcmp(host_cmd, "SetRx3v3Enable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetRx3v3(true);
			} //host_setting = "enable
			else
			{
				SetRx3v3(false);
			} //host_setting = "disable"
		} //host_cmd = "SetTx3v3Enable"
		else if (strcmp(host_cmd, "SetTxCh1IfAmpEnable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetTxCh1IfAmp(true);
			} //host_setting = "enable"
			else
			{
				SetTxCh1IfAmp(false);
			} //host_setting = "disable"
		} //host_cmd = "SetTxCh1IfAmpEnable"
		else if (strcmp(host_cmd, "SetTxCh2IfAmpEnable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetTxCh2IfAmp(true);
			} //host_setting = "enable"
			else
			{
				SetTxCh2IfAmp(false);
			} //host_setting = "disable"
		} //host_cmd = "SetTxCh2IfAmpEnable"
		else if (strcmp(host_cmd, "SetRxCh1IfAmpEnable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetRxCh1IfAmp(true);
			} //host_setting = "enable"
			else
			{
				SetRxCh1IfAmp(false);
			} //host_setting = "disable"
		} //host_cmd = "SetRxCh1IfAmpEnable"
		else if (strcmp(host_cmd, "SetRxCh2IfAmpEnable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetRxCh2IfAmp(true);
			} //host_setting = "enable"
			else
			{
				SetRxCh2IfAmp(false);
			} //host_setting = "disable"
		} //host_cmd = "SetRxCh2IfAmpEnable"
		else if (strcmp(host_cmd, "SetRxCh1BiasEnable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetRxCh1Bias(true);
			} //host_setting = "enable"
			else
			{
				SetRxCh1Bias(false);
			} //host_setting = "disable"
		} //host_cmd = "SetRxCh1BiasEnable"
		else if (strcmp(host_cmd, "SetRxCh2BiasEnable") == 0)
		{
			if (strcmp(host_setting, "enable") == 0)
			{
				SetRxCh2Bias(true);
			} //host_setting = "enable"
			else
			{
				SetRxCh2Bias(false);
			} //host_setting = "disable"
		} //host_cmd = "SetRxCh2BiasEnable"
		else if (strcmp(host_cmd, "SetTxAttenuators") == 0)
		{
			SetTxAttenuators(host_value_float);
		} //host_cmd = "SetTxAttenuators"
		else if (strcmp(host_cmd, "SetRxRfCh1Attenuator") == 0)
		{
			SetRxRfCh1Attenuator(host_value_float);
		} //host_cmd = "SetRxRfCh1Attenuator"
		else if (strcmp(host_cmd, "SetRxRfCh2Attenuator") == 0)
		{
			SetRxRfCh2Attenuator(host_value_float);
		} //host_cmd = "SetRxRfCh2Attenuator"
		else if (strcmp(host_cmd, "SetRxIfCh1Attenuator") == 0)
		{
			SetRxIfCh1Attenuator(host_value_float);
		} //host_cmd = "SetRxIfCh1Attenuator"
		else if (strcmp(host_cmd, "SetRxIfCh2Attenuator") == 0)
		{
			SetRxIfCh2Attenuator(host_value_float);
		} //host_cmd = "SetRxIfCh2Attenuator"
		else if (strcmp(host_cmd, "SetTxPllFreq") == 0)
		{
			ulTxPllDemominator = host_value_int;
			if (strcmp(host_setting, "Fpfd=122.88") == 0)
			{
				fTxPllFreqFpfd_MHz = 122.88;
			} //host_setting = "Fpfd=122.88"
			else
			{
				fTxPllFreqFpfd_MHz = 61.44;
			} //host_setting = "Fpfd=61.44"
			ChangePllFrequency('T', fTxPllFreqFpfd_MHz, (double)host_value_float);
			fTxPllFreqRF_GHz = host_value_float2;
			fTxPllFreqIF_GHz = host_value_float3;
		} //host_cmd = "SetTxPllFreq"
		else if (strcmp(host_cmd, "SetRxPllFreq") == 0)
		{
			ulRxPllDemominator = host_value_int;
			if (strcmp(host_setting, "Fpfd=122.88") == 0)
			{
				fRxPllFreqFpfd_MHz = 122.88;
			} //host_setting = "Fpfd=122.88"
			else
			{
				fRxPllFreqFpfd_MHz = 61.44;
			} //host_setting = "Fpfd=61.44"
			ChangePllFrequency('R', fRxPllFreqFpfd_MHz, (double)host_value_float);
			fRxPllFreqRF_GHz = host_value_float2;
			fRxPllFreqIF_GHz = host_value_float3;
		} //host_cmd = "SetRxPllFreq"
		else if (strcmp(host_cmd, "SetTxPllPower") == 0)
		{
			unsigned char settingA, settingB;
			if (strcmp(host_setting1, "enable") == 0)
			{
				settingA = 0; //do not power down
			} //host_setting1 = "enable"
			else
			{
				settingA = 1; //power down
			} //host_setting1 = "disable"
			if (strcmp(host_setting2, "enable") == 0)
			{
				settingB = 0; //do not power down
			} //host_setting2 = "enable"
			else
			{
				settingB = 1; //power down
			} //host_setting2 = "disable"
			ChangePllPower('T', settingA, host_value_int1, settingB, host_value_int2);
		} //host_cmd = "SetTxPllPower"
		else if (strcmp(host_cmd, "SetRxPllPower") == 0)
		{
			unsigned char settingA, settingB;
			if (strcmp(host_setting1, "enable") == 0)
			{
				settingA = 0; //do not power down
			} //host_setting1 = "enable"
			else
			{
				settingA = 1; //power down
			} //host_setting1 = "disable"
			if (strcmp(host_setting2, "enable") == 0)
			{
				settingB = 0; //do not power down
			} //host_setting2 = "enable"
			else
			{
				settingB = 1; //power down
			} //host_setting2 = "disable"
			ChangePllPower('R', settingA, host_value_int1, settingB, host_value_int2);
		} //host_cmd = "SetRxPllPower"
		else if (strcmp(host_cmd, "SetTxPllPowerAuto") == 0)
		{
			unsigned char settingA, settingB;
			if (strcmp(host_setting1, "enable") == 0)
			{
				settingA = 0; //do not power down
			} //host_setting1 = "enable"
			else
			{
				settingA = 1; //power down
			} //host_setting1 = "disable"
			if (strcmp(host_setting2, "enable") == 0)
			{
				settingB = 0; //do not power down
			} //host_setting2 = "enable"
			else
			{
				settingB = 1; //power down
			} //host_setting2 = "disable"
			ChangePllPowerAuto('T', settingA, settingB);
		} //host_cmd = "SetTxPllPowerAuto"
		else if (strcmp(host_cmd, "SetRxPllPowerAuto") == 0)
		{
			unsigned char settingA, settingB;
			if (strcmp(host_setting1, "enable") == 0)
			{
				settingA = 0; //do not power down
			} //host_setting1 = "enable"
			else
			{
				settingA = 1; //power down
			} //host_setting1 = "disable"
			if (strcmp(host_setting2, "enable") == 0)
			{
				settingB = 0; //do not power down
			} //host_setting2 = "enable"
			else
			{
				settingB = 1; //power down
			} //host_setting2 = "disable"
			ChangePllPowerAuto('R', settingA, settingB);
		} //host_cmd = "SetRxPllPowerAuto"
		else if (strcmp(host_cmd, "TxPllWriteRegister") == 0)
		{
			WritePllRegister('T', (unsigned char)host_addr, (unsigned int)host_value_int);
		} //host_cmd = "TxPllWriteRegister"
		else if (strcmp(host_cmd, "RxPllWriteRegister") == 0)
		{
			WritePllRegister('R', (unsigned char)host_addr, (unsigned int)host_value_int);
		} //host_cmd = "RxPllWriteRegister"
		else if (strcmp(host_cmd, "TxPllReadRegister") == 0)
		{
			ReadPllRegister('T', (unsigned char)host_addr);
		} //host_cmd = "TxPllReadRegister"
		else if (strcmp(host_cmd, "RxPllReadRegister") == 0)
		{
			ReadPllRegister('R', (unsigned char)host_addr);
		} //host_cmd = "RxPllReadRegister"
		else if (strcmp(host_cmd, "TxPllReadAllRegisters") == 0)
		{
			PllReadAllRegisters('T');
		} //host_cmd = "TxPllReadAllRegisters"
		else if (strcmp(host_cmd, "RxPllReadAllRegisters") == 0)
		{
			PllReadAllRegisters('R');
		} //host_cmd = "RxPllReadAllRegisters"

		else if (strcmp(host_cmd, "TxPllGetStatus") == 0)
		{
			PllGetStatus('T');
		} //host_cmd = "TxPllGetStatus"
		else if (strcmp(host_cmd, "RxPllGetStatus") == 0)
		{
			PllGetStatus('R');
		} //host_cmd = "RxPllGetStatus"
		else if (strcmp(host_cmd, "EepromWrite") == 0)
		{
			WriteEeprom((unsigned char)host_addr, (unsigned int)host_value_int);
		} //host_cmd = "EepromWrite"
		else if (strcmp(host_cmd, "EepromRead") == 0)
		{
			ReadEeprom((unsigned char)host_addr, (int)host_value_len);
		} //host_cmd = "EepromRead"
		else if (strcmp(host_cmd, "EepromEraseAll") == 0)
		{
			EraseEeprom();
		} //host_cmd = "EepromEraseAll"
		else if (strcmp(host_cmd, "SetEepromTxIfMax") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			WriteEeprom_Multiple(EEPROM_ADDRESS_TX_IF_MAX, host_value_ucptr, 3);
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_TX_IF_MAX, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromTxIfMax\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "SetEepromTxIfMax"
		else if (strcmp(host_cmd, "GetEepromTxIfMax") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_TX_IF_MAX, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromTxIfMax\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "GetEepromTxIfMax"
		else if (strcmp(host_cmd, "SetEepromTxIfMin") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			WriteEeprom_Multiple(EEPROM_ADDRESS_TX_IF_MIN, host_value_ucptr, 3);
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_TX_IF_MIN, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromTxIfMin\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "SetEepromTxIfMin"
		else if (strcmp(host_cmd, "GetEepromTxIfMin") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_TX_IF_MIN, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromTxIfMin\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "GetEepromTxIfMin"
		else if (strcmp(host_cmd, "SetEepromRxIfMax") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			WriteEeprom_Multiple(EEPROM_ADDRESS_RX_IF_MAX, host_value_ucptr, 3);
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_RX_IF_MAX, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromRxIfMax\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "SetEepromRxIfMax"
		else if (strcmp(host_cmd, "GetEepromRxIfMax") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_RX_IF_MAX, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromRxIfMax\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "GetEepromRxIfMax"
		else if (strcmp(host_cmd, "SetEepromRxIfMin") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			WriteEeprom_Multiple(EEPROM_ADDRESS_RX_IF_MIN, host_value_ucptr, 3);
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_RX_IF_MIN, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromRxIfMin\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "SetEepromRxIfMin"
		else if (strcmp(host_cmd, "GetEepromRxIfMin") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_RX_IF_MIN, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromRxIfMin\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "GetEepromRxIfMin"
		else if (strcmp(host_cmd, "SetEepromBoardName") == 0)
		{
	   		host_value_ucptr = host_value_string;
			WriteEeprom_Multiple(EEPROM_ADDRESS_BOARD_NAME, host_value_ucptr, host_value_string_len);
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_NAME, host_value_ucptr, 8) ==0)
			{
				tcp_printf("{\"rsp\": \"EepromBoardName\",\"value_string\": \"%s\"}", host_value_string);
			}
		} //host_cmd = "SetEepromBoardName"
		else if (strcmp(host_cmd, "GetEepromBoardName") == 0)
		{
	   		host_value_ucptr = host_value_string;
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_NAME, host_value_ucptr, 8) ==0)
			{
				tcp_printf("{\"rsp\": \"EepromBoardName\",\"value_string\": \"%s\"}", host_value_string);
			}
		} //host_cmd = "GetEepromBoardName"
		else if (strcmp(host_cmd, "SetEepromBoardVersion") == 0)
		{
	   		host_value_ucptr = host_value_string;
			WriteEeprom_Multiple(EEPROM_ADDRESS_BOARD_VERSION, host_value_ucptr, host_value_string_len);
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_VERSION, host_value_ucptr, 1) ==0)
			{
				tcp_printf("{\"rsp\": \"EepromBoardVersion\",\"value_string\": \"%s\"}", host_value_string);
			}
		} //host_cmd = "SetEepromBoardVersion"
		else if (strcmp(host_cmd, "GetEepromBoardVersion") == 0)
		{
	   		host_value_ucptr = host_value_string;
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_VERSION, host_value_ucptr, 1) ==0)
			{
				tcp_printf("{\"rsp\": \"EepromBoardVersion\",\"value_string\": \"%s\"}", host_value_string);
			}
		} //host_cmd = "GetEepromBoardVersion"
		else if (strcmp(host_cmd, "SetEepromBoardRevision") == 0)
		{
	   		host_value_ucptr = host_value_string;
			WriteEeprom_Multiple(EEPROM_ADDRESS_BOARD_REVISION, host_value_ucptr, host_value_string_len);
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_REVISION, host_value_ucptr, 1) ==0)
			{
				tcp_printf("{\"rsp\": \"EepromBoardRevision\",\"value_string\": \"%s\"}", host_value_string);
			}
		} //host_cmd = "SetEepromBoardRevision"
		else if (strcmp(host_cmd, "GetEepromBoardRevision") == 0)
		{
	   		host_value_ucptr = host_value_string;
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_REVISION, host_value_ucptr, 1) ==0)
			{
				tcp_printf("{\"rsp\": \"EepromBoardRevision\",\"value_string\": \"%s\"}", host_value_string);
			}
		} //host_cmd = "GetEepromBoardRevision"
		else if (strcmp(host_cmd, "SetEepromBoardSerialNum") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			WriteEeprom_Multiple(EEPROM_ADDRESS_BOARD_SERIALNUM, host_value_ucptr, 3);
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_SERIALNUM, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromBoardSerialNum\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "SetEepromBoardSerialNum"
		else if (strcmp(host_cmd, "GetEepromBoardSerialNum") == 0)
		{
	   		host_value_ucptr = (unsigned char*)&host_value_long; // not endian safe
			if (ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_SERIALNUM, host_value_ucptr, 3) == 0)
			{
				tcp_printf("{\"rsp\": \"EepromBoardSerialNum\",\"value_long\": \"%d\"}", host_value_long);
			}
		} //host_cmd = "SetEepromBoardSerialNum"
		else if (strcmp(host_cmd, "SetEepromBoardDate") == 0)
		{
			host_value_ucptr = host_value_string_year;
	    		host_value_string_len = strlen(host_value_string_year);
			WriteEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_YEAR, host_value_ucptr, host_value_string_len);
			host_value_ucptr = host_value_string_month;
	    		host_value_string_len = strlen(host_value_string_month);
			WriteEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_MONTH, host_value_ucptr, host_value_string_len);
			host_value_ucptr = host_value_string_day;
	    		host_value_string_len = strlen(host_value_string_day);
			WriteEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_DAY, host_value_ucptr, host_value_string_len);
			//Read back the date:
			host_value_ucptr = host_value_string_year;
			ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_YEAR, host_value_ucptr, 2);
			host_value_ucptr = host_value_string_month;
			ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_MONTH, host_value_ucptr, 2);
			host_value_ucptr = host_value_string_day;
			ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_DAY, host_value_ucptr, 2);
			tcp_printf("{\"rsp\": \"EepromBoardDate\",\"value_year\": \"%s\",\"value_month\": \"%s\",\"value_day\": \"%s\"}", host_value_string_year, host_value_string_month, host_value_string_day);
		} //host_cmd = "SetEepromBoardDate"
		else if (strcmp(host_cmd, "GetEepromBoardDate") == 0)
		{
			host_value_ucptr = host_value_string_year;
			ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_YEAR, host_value_ucptr, 2);
			host_value_ucptr = host_value_string_month;
			ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_MONTH, host_value_ucptr, 2);
			host_value_ucptr = host_value_string_day;
			ReadEeprom_Multiple(EEPROM_ADDRESS_BOARD_DATE_DAY, host_value_ucptr, 2);
			tcp_printf("{\"rsp\": \"EepromBoardDate\",\"value_year\": \"%s\",\"value_month\": \"%s\",\"value_day\": \"%s\"}", host_value_string_year, host_value_string_month, host_value_string_day);
		} //host_cmd = "GetEepromBoardDate"
		else
		{ //Unknown JSON command
			printf("Unknown JASON command %s\n", host_cmd);
			return -2;
		} //host_cmd = "GetEepromBoardDate"
		return false; // No problems
	} //parse_json_message is OK
	else
	{ //parse_json_message problem
		printf("Problem with JSON format\n");
		return -1;
	} //parse_json_message problem
} //processJsonCommand

static int LoadJsonConfigFile(void)
{
    FILE *fptr;
    int ilinenumber = 0;
	char pathname[500];
	char cfirstcharinline;
	//char userfilename[50];
	char ConfigFilename[514];
	unsigned char bjson_error = false;
	 //For now, assume we have a We have a ZCU208, not a ZCU111
	if (useCmdLinePath == 0) {
		sprintf(pathname,"%s", PATH_FILENAME_ZCU208);
	} else {
		int nCfgPath = strlen(rootCfgPath);
		// '=' to leave room for \0
		if (nCfgPath >= sizeof(pathname)/sizeof(pathname[0])) {
			printf("\nError: JSON config file path name is to long!\n");
			exit(-1);
		} else {
			printf("New path name length is: %d\n", nCfgPath);
		}
		sprintf(pathname, "%s", rootCfgPath);
	}
	sprintf(ConfigFilename, "%s%s", pathname, INITCFG_JSON_FILENAME_DEFAULT);
	printf("Opening file %s\n", ConfigFilename);
    if ((fptr = fopen(ConfigFilename, "r")) == NULL)
    {
        printf("Cannot open that file name!\n");
        return(-1);
    }
    while ((bjson_error == false) && fgets(json_cmd_string, sizeof json_cmd_string, fptr) != NULL) /* read a line */
    {
    	ilinenumber++;
        printf("Line %i from the Config file: %s", ilinenumber, json_cmd_string);
        cfirstcharinline = json_cmd_string[0];
        if ((cfirstcharinline != '#') && (cfirstcharinline != '\r') && (cfirstcharinline != '\n') && (cfirstcharinline != '\0')) //Comment lines start with '#'.  Empty lines also ignored
        {
        	bjson_error = processJsonCommand();
        }
        printf("\n");
    }
    fclose(fptr);
	fflush(stdout); // Prints to screen or whatever your standard out is
	if (bjson_error !=false)
	{
		return ilinenumber; //Line on which we have a problem/error
	}
	else
	{
		return false; // No problems
	}
} //LoadJsonConfigFile()

void print_usage(void) {
    printf("PS Version: %d.%d\n\n", PS_VERSION_MSB, PS_VERSION_LSB);
	printf("Usage: cots-tcp -p <tcp port number> [-s <SPI device name> -i <I2C device string> -d <path of cfg files>]\n");
	printf("\nThe -p option is not optional, you must supply a TCP/IP port number. -s -i -d arguments are optional.\n");
	printf("If -s is given that device name for the SPI Device used to control the DTRX will be used.\n");
	printf("If -i is given that device name for the I2C Expander Device will be used.\n");
	printf("If -d is given it will be used for the root path to find various config files. Make sure you add final '/'.\n\n");
	printf("Example: cots-tcp -p 8083 -s /dev/spidev1.0 -i /dev/i2c-22 -d /opt/dtrx2/\n\n");
	printf("For ZCU208 if -d is used, off of the base root path these files must exist:\n\n");
	printf("           RX_LMX2595*.txt\n");
	printf("           TX_LMX2595*.txt\n");
	printf("           dtrx2_init.cfg\n\n");
}

unsigned parseCmdLine(int argc, char ** argv)
{
	extern const char * I2C_FILE_NAME;
	unsigned port = 0;

	if (argc == 1) {
		print_usage();
		exit(-1);
	}

	if (argc == 3 || argc == 5 || argc == 7 || argc == 9) {
		for(unsigned i=1; i<argc; i+=2) {
			if (argv[i][1] == 'p') {
				port = atoi(argv[i+1]);
				printf("Setting up to use TCP port: %u\n", port);
			}
			else if (argv[i][1] == 's') {
				spi_device = argv[i+1];
				printf("Setting up to use spi device: %s\n", spi_device);
			}
			else if (argv[i][1] == 'i') {
				I2C_FILE_NAME = argv[i+1];
				printf("Setting up to use I2C expander device name: %s\n", I2C_FILE_NAME);
			}
			else if (argv[i][1] == 'd') {
				useCmdLinePath = 1;
				rootCfgPath = argv[i+1];
				printf("Look for JSON, RX&Tx PLL init cfg files here: %s\n", rootCfgPath);
			}
		}
		return (port);
	}

	printf("Please specify the TCP/IP port using the -p option!!!\n");
	print_usage();
	exit(-1);

	// return will never be reached due to exit()
	return (0);
}

int main(int argc, char **argv)
{
	int ret = 0;
	unsigned otavaTcpPort;

	iTcpConnected = false;
	ucPmod0TestpinMask = 0;
	ucPmod1TestpinMask = 0;
	ucPmod0Status = 0;
	ucPmod1Status = 0;

	ulRxPllDemominator = DEN_DEFAULT;
	ulTxPllDemominator = DEN_DEFAULT;
	ulFpfd = Fpd_DEFAULT;

	// parseCmdLine(..) can modify device names and paths and must
	//  be called before opening the SPI or I2C devices etc.
	otavaTcpPort = parseCmdLine(argc, argv);

	spi_fd = initializeSpiDevice();

	if (zed_iic_eeprom_init(&cots_eeprom) < 0 )
	{
		eeprom_OK = false;
	}
	else
	{
		eeprom_OK = true;
	}

	if (expander_init() < 0 )
	{
		expander_OK = false;
	}
	else
	{
		expander_OK = true;
	}

	//Perform initialization JSON commands from a .cfg file, if it exists:
	iconfigfile_error = LoadJsonConfigFile();
	if (iconfigfile_error < 0)
	{
		printf("Config file not found.\n");
	}
	else if (iconfigfile_error > 0)
	{
		printf("Config file error on line %i.\n", iconfigfile_error);
	}

	if (otavaTcpPort != 0)
		tcp_mode = 1;

	int socket_fd;
	if (tcp_mode == 1)
	{
		socket_fd = tcpServerInit(&otava_address, otavaTcpPort);
		new_socket = waitClientConnect(socket_fd, &otava_address);
	}

	tcp_printf("#####################################################\r\n");
	tcp_printf("Testing Program for the Otava COTS Transceiver\r\n");
	if (!expander_OK)
	{
		tcp_printf("Warning - I/O Expander faulty or not present\r\n");
	}
	if (iconfigfile_error == 0)
	{
		tcp_printf("Commands in Config file were executed\r\n");
	}
	else if (iconfigfile_error < 0)
	{
		tcp_printf("Config file was not found\r\n");
	}
	else if (iconfigfile_error > 0)
	{
		tcp_printf("Config file error on line %i.\r\n", iconfigfile_error);
	}
	tcp_printf("#####################################################\r\n");
	fflush(stdout); // Prints to screen or whatever your standard out is

	//display_main_menu();


	int bLooping = true;

	while (bLooping)
	{
		int sts;
		sts = getTcpCmdString(new_socket, &cmd_type, &cmd_value);
		if (sts > 0)
		{
			processJsonCommand();
		}
		else
		{
			if (sts == 0) /* Client disconnected */
			{
				iTcpConnected = false;
				new_socket = waitClientConnect(socket_fd, &otava_address);
			}
			continue;
		}
	} //bLooping

	SPI_Close(spi_fd);

	if (tcp_mode == 1)
		closeSocket(new_socket);

	return ret;
} //main()

/* initialize command socket */
int tcpServerInit(struct sockaddr_in * otava_address, unsigned tcpPort)
{
	int socket_fd, opt = 1;

	/* Create TCP v4 IP socket file descriptor */
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		printf("Otava socket create failed\n");
		exit(EXIT_FAILURE);
	}

	/* set socket options */
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				&opt, sizeof(opt))) {
		printf("setsockopt failed\n");
		exit(EXIT_FAILURE);
	}

	/* bind socket with port */
	otava_address->sin_family = AF_INET;
	otava_address->sin_addr.s_addr = INADDR_ANY;
	otava_address->sin_port = htons( tcpPort );
	if (bind(socket_fd, (struct sockaddr *)otava_address, sizeof(*otava_address)) < 0) {
		printf("bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Listen socket */
	if (listen(socket_fd, 3) < 0) {
		printf("listen failed\n");
		exit(EXIT_FAILURE);
	}

	printf("Otava tcp server started\n");

	return (socket_fd);
}

int waitClientConnect(int socket_fd, struct sockaddr_in * otava_address)
{
	int addrlen = sizeof(otava_address);
	int new_socket;

	printf("waiting for Otava tcp client to connect...\n");

	new_socket = accept(socket_fd, (struct sockaddr *)otava_address, (socklen_t*)&addrlen);
	if (new_socket < 0) {
		printf("accept failed\n");
		exit(EXIT_FAILURE);
	}

	printf("Otava tcp client connected\n");
	iTcpConnected = true;
	return (new_socket);
}

void closeSocket(int new_socket)
{
	close(new_socket);
}

/* returns 0 if client closed socket, -1 if socket or comms error, -2 if cmd too long for local buffer
    and 1 if cmd fully parsed */
int getTcpCmdString(int new_socket, char ** cmd_type, char ** cmd_value)
{
	unsigned char bJson_started = false;
	static char cmd_buf[MAX_OTAVA_CMD_LENGTH];
	char * buf_ptr;
	char * json_start_ptr;
	int sts;
	int ijson_length = 0;

	memset( (void*)cmd_buf, 0, sizeof(cmd_buf));
	for (buf_ptr = cmd_buf; buf_ptr < (cmd_buf + MAX_OTAVA_CMD_LENGTH); buf_ptr++) {

		sts = recv(new_socket, (void*)buf_ptr, 1, MSG_WAITALL);
		ijson_length++;
		/* Handle socket exceptions */
		if (sts == 0) {
			printf("client closed socket, aborting cmd!\n");
			return (0);
		}
		else if (sts == -1) {
			printf("unknown socket error, aborting cmd!\n");
			return (-1);
		}

		/* JSON strings start with '{'.  Look for the start */
		if (*buf_ptr == '{')
		{
			if (bJson_started == false)
			{
				bJson_started = true;
				json_start_ptr = buf_ptr;
			}
			else
			{ //this is a second start, discard message
				printf("Second '{'Error, second '{' in JSON\n");
				return (-1);
			}
		} //received '{'

		if (*buf_ptr == '}') //end of JSON string
		{
			memcpy(json_cmd_string, json_start_ptr, (buf_ptr - json_start_ptr + 1));
			//Null-terminate JSON string just in case
			json_cmd_string[ijson_length] = 0;
			return (1);
		}
	}

	printf("local cmd_buf length exceeded, aborting cmd\n");
	return (-2);
} //getTcpCmdString
