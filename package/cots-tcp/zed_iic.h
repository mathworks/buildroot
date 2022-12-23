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
//
// Create Date:         Jul 01, 2013
// Design Name:         ZED-IIC
// Module Name:         zed_iic.h
// Project Name:        ZED-IIC
// Target Devices:      Zynq
// Avnet Boards:        ZedBoard
//
// Tool versions:       ISE 14.6
//
// Description:         IIC Hardware Abstraction Layer
//
// Dependencies:        
//
// Revision:            Jul 01, 2013: 1.00 Initial version
//						Mar 27, 2015: 1.01 Updated to remove
//                                         dependency upon
//                                         xbasic_types.h
//						Mar 17, 2016: 1.02 Updated header disclaimer and
//                                         added support for reading from
//                                         slave devices under Linux.
//                Oct 04, 2022: Updated license
//
//----------------------------------------------------------------------------

#ifndef __ZED_IIC_H__
#define __ZED_IIC_H__

#include <stdio.h>

#include "platform.h"
#include "types.h"

#define ZED_IIC_CONTEXT_BUFFER_SIZE 32

struct struct_zed_iic_t
{
   // software library version
   uint32_t uVersion;

   // instantiation-specific names
   char szName[32];

   // pointer to instantiation-specific data
   void *pContext;

   // context data (must be large enough to contain fmc_iic_axi_t or other implementations)
   unsigned char ContextBuffer[ZED_IIC_CONTEXT_BUFFER_SIZE];

   // function pointers to implementation-specific code
   int (*fpIicRead )(struct struct_zed_iic_t *, uint8_t ChipAddress,
                                                uint8_t RegAddress,
                                                uint8_t *pBuffer,
                                                uint8_t ByteCount );

   int (*fpIicERead )(struct struct_zed_iic_t *, uint8_t ChipAddress,
                                                uint16_t RegAddress,
                                                uint8_t *pBuffer,
                                                uint8_t ByteCount);

   int (*fpIicWrite)(struct struct_zed_iic_t *, uint8_t ChipAddress,
                                                uint8_t RegAddress,
                                                uint8_t *pBuffer,
                                                uint8_t ByteCount );
};
typedef struct struct_zed_iic_t zed_iic_t;

// Initialization routine for AXI_IIC implementation
int zed_iic_axi_init( zed_iic_t *pIIC, char szName[], uint32_t CoreAddress );


#endif // __ZED_IIC_H__
