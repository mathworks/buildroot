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
// Module Name:         zed_iic_axi.c
// Project Name:        ZED-IIC
// Target Devices:      Zynq
// Avnet Boards:        ZedBoard
//
// Tool versions:       ISE 14.6
//
// Description:         IIC Hardware Abstraction Layer
//                      => AXI_IIC implementation
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
//            Oct 04, 2022: Updated license
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "zed_iic.h"

#if defined(XPAR_XIIC_NUM_INSTANCES)

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
   
/*
 * The page size determines how much data should be written at a time.
 * The write function should be called with this as a maximum byte count.
 */
#define PAGE_SIZE   20

#define I2C_SLAVE_FORCE 0x0706
#define I2C_SLAVE    0x0703    /* Change slave address            */
#define I2C_FUNCS    0x0705    /* Get the adapter functionality */
#define I2C_RDWR    0x0707    /* Combined R/W transfer (one stop only)*/


////////////////////////////////////////////////////////////////////////
// Context Data
////////////////////////////////////////////////////////////////////////

struct struct_zed_iic_axi_t
{
	int Fdiic;
};
typedef struct struct_zed_iic_axi_t zed_iic_axi_t;

////////////////////////////////////////////////////////////////////////
// I2C Functions
////////////////////////////////////////////////////////////////////////

// Forward declarations
int zed_iic_axi_GpoRead ( zed_iic_t *pIIC, uint32_t *pGpioData );
int zed_iic_axi_GpoWrite( zed_iic_t *pIIC, uint32_t GpioData );
int zed_iic_axi_IicWrite( zed_iic_t *pIIC, uint8_t ChipAddress, uint8_t RegAddress,
                                           uint8_t *pBuffer, uint8_t ByteCount);
int zed_iic_axi_IicRead ( zed_iic_t *pIIC, uint8_t ChipAddress, uint8_t RegAddress,
                                           uint8_t *pBuffer, uint8_t ByteCount);
int zed_iic_axi_IicEWrite( zed_iic_t *pIIC, uint8_t ChipAddress, uint16_t RegAddress,
                                            uint8_t *pBuffer, uint8_t ByteCount);
int zed_iic_axi_IicERead ( zed_iic_t *pIIC, uint8_t ChipAddress, uint16_t RegAddress,
                                            uint8_t *pBuffer, uint8_t ByteCount);

/******************************************************************************
* This function initializes the IIC controller.
*
* @param    CoreAddress contains the address of the IIC core.
*
* @return   If successfull, returns 1.  Otherwise, returns 0.
*
* @note     None.
*
******************************************************************************/
int zed_iic_axi_init( zed_iic_t *pIIC, char szName[], uint32_t CoreAddress )
{
   extern const char * I2C_FILE_NAME;
   zed_iic_axi_t *pContext = (zed_iic_axi_t *) (pIIC->ContextBuffer);
   if ( sizeof(zed_iic_axi_t) > ZED_IIC_CONTEXT_BUFFER_SIZE )
   {
      printf("ZED_IIC_CONTEXT_BUFFER_SIZE is not large enough for fic_iic_xps_t structure (increase to %ld)\n\r", sizeof(zed_iic_axi_t));
      return 0;
   }

   pContext->Fdiic = open(I2C_FILE_NAME, O_RDWR);
   assert(pContext->Fdiic >= 0);

   /*
    * Initialize the IIC structure
	*/ 
   pIIC->uVersion = 1;
   strcpy( pIIC->szName, szName );
   pIIC->pContext = (void *)pContext;
   pIIC->fpIicRead   = &zed_iic_axi_IicRead;
   pIIC->fpIicERead  = &zed_iic_axi_IicERead;
   pIIC->fpIicWrite  = &zed_iic_axi_IicWrite;

   return 1;
}

/******************************************************************************
* This function writes a buffer of bytes to the IIC chip.
*
* @param    ChipAddress contains the address of the chip.
* @param    RegAddress contains the address of the register to write to.
* @param    pBuffer contains the address of the data to write.
* @param    ByteCount contains the number of bytes in the buffer to be written.
*           Note that this should not exceed the page size as noted by the 
*           constant PAGE_SIZE.
*
* @return   The number of bytes written, a value less than that which was
*           specified as an input indicates an error.
*
* @note     None.
*
******************************************************************************/
int zed_iic_axi_IicWrite(zed_iic_t *pIIC, uint8_t ChipAddress, uint8_t RegAddress,
                                          uint8_t *pBuffer, uint8_t ByteCount)
{
  uint8_t WriteBuffer[PAGE_SIZE + 1];
  uint8_t Index;
  zed_iic_axi_t *pContext = (zed_iic_axi_t *)(pIIC->pContext);

  ioctl(pContext->Fdiic, I2C_SLAVE_FORCE, ChipAddress);

  /*
   * A temporary write buffer must be used which contains both the address
   * and the data to be written, put the address in first 
   */
  WriteBuffer[0] = RegAddress;

  /*
   * Put the data in the write buffer following the address.
   */
  for (Index = 0; Index < ByteCount; Index++)
  {
    WriteBuffer[Index + 1] = pBuffer[Index];
  }

  /*
   * Write data at the specified address.
   */
  return write(pContext->Fdiic, WriteBuffer, ByteCount+1);
}


/******************************************************************************
* This function reads a number of bytes from an IIC chip into a
* specified buffer.
*
* @param    ChipAddress contains the address of the IIC core.
* @param    RegAddress contains the 8-bit long address of the register
*           to read from.
* @param    pBuffer contains the address of the data buffer to be filled.
* @param    ByteCount contains the number of bytes in the buffer to be read.
*           This value is constrained by the page size of the device such
*           that up to 64K may be read in one call.
*
* @return   The number of bytes read. A value less than the specified input
*           value indicates an error.
*
* @note     None.
*
******************************************************************************/
int zed_iic_axi_IicRead(zed_iic_t *pIIC, uint8_t ChipAddress, uint8_t RegAddress,
                                         uint8_t *pBuffer, uint8_t ByteCount)
{
  uint8_t address[1];

  zed_iic_axi_t *pContext = (zed_iic_axi_t *)(pIIC->pContext);

  ioctl(pContext->Fdiic, I2C_SLAVE_FORCE, ChipAddress);

  address[0] = RegAddress;
  write(pContext->Fdiic, address, 1);
  
  // Receive the data.
  return read(pContext->Fdiic, pBuffer, ByteCount);
}

/******************************************************************************
* This function reads a number of bytes from an IIC chip into a
* specified buffer.
*
* @param    ChipAddress contains the address of the IIC core.
* @param    RegAddress contains the 16-bit long address of the register
*           to read from.
* @param    pBuffer contains the address of the data buffer to be filled.
* @param    ByteCount contains the number of bytes in the buffer to be read.
*           This value is constrained by the page size of the device such
*           that up to 64K may be read in one call.
*
* @return   The number of bytes read. A value less than the specified input
*           value indicates an error.
*
* @note     None.
*
******************************************************************************/
int zed_iic_axi_IicERead(zed_iic_t *pIIC, uint8_t ChipAddress, uint16_t RegAddress,
                         uint8_t *pBuffer, uint8_t ByteCount)
{
  uint8_t address[2];

  zed_iic_axi_t *pContext = (zed_iic_axi_t *)(pIIC->pContext);

  ioctl(pContext->Fdiic, I2C_SLAVE_FORCE, ChipAddress);

  address[0] = (uint8_t) ((RegAddress >> 8) & 0x00FF);
  address[1] = (uint8_t) (RegAddress & 0x00FF);
  write(pContext->Fdiic, address, 2);

  // Receive the data.
  return read(pContext->Fdiic, pBuffer, ByteCount);
}

#endif // defined(XPAR_XIIC_NUM_INSTANCES)
