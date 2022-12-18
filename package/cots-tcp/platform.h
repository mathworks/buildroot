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
// Create Date:         Jun 24, 2013
// Design Name:         PicoZed FMC2 FMC Loopback Test
// Module Name:         platform.h
// Project Name:        PicoZed FMC2 FMC Loopback Test
// Target Devices:      Xilinx Zynq-7000
// Hardware Boards:     PicoZed, PicoZed FMC2 Carrier
//
// Tool versions:       Xilinx Vivado 2015.2
//
// Description:         Programmable logic hardware platform definitions file.
//
// Dependencies:
//
// Revision:            Jun 24, 2013: 1.00 Initial version
//                      Oct 04, 2022: Update license
//
//----------------------------------------------------------------------------


#ifndef PLATFORM_H_
#define PLATFORM_H_

/*
 * Define the AXI locations for the PL AXI I2C blocks used for the I2C slave
 * tests.  These come from xparameters.h in the standalone BSP or from the
 * AXI address defined in the hardware platform and these may need to get
 * updated every time this test gets ported to a new platform.
 */
#define XPAR_XIIC_NUM_INSTANCES            1
#define XPAR_AXI_IIC_0_BASEADDR            0x41600000

#define CONFIG_DEFAULT_MMAP_MIN_ADDR   4096

#endif // PLATFORM_H_
