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
#ifndef __EXPANDER_H__
#define __EXPANDER_H__

#include "zed_iic.h"

#define IIC_EXPANDER_SLAVE_ADDRESS				0x20

#define EXPANDER_COMMAND_OUTPUT_PORT0			0x02
#define EXPANDER_COMMAND_CONFIGURATION_PORT0	0x06

#define u8	unsigned char
#define u16 unsigned short

int expander_OK;
u8 expander_tx_DSA_value;
u8 bexpander_EN_5V_TX;
u8 bexpander_EN_5V_RX;
u8 bexpander_EN_3V3_TX;
u8 bexpander_EN_3V3_RX;
u8 bexpander_TESTPIN_27;
u8 bexpander_TESTPIN_28;
u8 bexpander_TESTPIN_29;
u8 bexpander_TESTPIN_30;

extern int expander_init();
extern int update_expander();

#endif // __EXPANDER_H__
