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
#ifndef __EEPROM_H__
#define __EEPROM_H__

#include "zed_iic.h"

#define IIC_EEPROM_SLAVE_ADDRESS                    0x50

#define EEPROM_ADDRESS_BOARD_NAME		0x00
#define EEPROM_ADDRESS_BOARD_VERSION	0x08
#define EEPROM_ADDRESS_BOARD_REVISION	0x09
#define EEPROM_ADDRESS_BOARD_SERIALNUM	0x0A
#define EEPROM_ADDRESS_BOARD_DATE_YEAR	0x0E
#define EEPROM_ADDRESS_BOARD_DATE_MONTH	0x10
#define EEPROM_ADDRESS_BOARD_DATE_DAY	0x12

//#define EEPROM_ADDRESS_TX_IF_MAX		0x20
//#define EEPROM_ADDRESS_TX_IF_MIN		0x24
//#define EEPROM_ADDRESS_RX_IF_MAX		0x28
//#define EEPROM_ADDRESS_RX_IF_MIN		0x2C
#define EEPROM_ADDRESS_TX_IF_MIN		0x20
#define EEPROM_ADDRESS_TX_IF_MAX		0x24
#define EEPROM_ADDRESS_RX_IF_MIN		0x28
#define EEPROM_ADDRESS_RX_IF_MAX		0x2C

struct struct_iic_eeprom_demo_t
{
   int32u bVerbose;

   ////////////////////////////////
   // RTC I2C related context
   ////////////////////////////////
   int32u uBaseAddr_IIC_RTC;

   zed_iic_t eeprom_iic;
};

typedef struct struct_iic_eeprom_demo_t iic_eeprom_demo_t;
iic_eeprom_demo_t cots_eeprom;
int eeprom_OK;

extern int zed_iic_eeprom_init(iic_eeprom_demo_t *pEeprom);
extern int write_eeprom(iic_eeprom_demo_t *pEeprom, int8u addr, int8u data);
extern int read_eeprom(iic_eeprom_demo_t *pEeprom, int8u addr, unsigned char* data);

#endif // __EEPROM_H__
