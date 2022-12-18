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
#ifndef TYPES_H
#define TYPES_H

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TEST_FAILURE
#define TEST_FAILURE (-1)
#endif

#ifndef TEST_SUCCESS
#define TEST_SUCCESS (0)
#endif

// This file may be included for embedded programs as well as for
// programs running on the host PC.  For the latter case, BOOL is
// already otherwise defined.
#ifndef HOST_COMPILE
typedef unsigned char   BOOL;
#endif // HOST_COMPILE

typedef unsigned char   int8u;
typedef unsigned char   uint8_t;

typedef char            int8s;
//typedef char            int8_t;

typedef unsigned short  int16u;
typedef unsigned short  uint16_t;

typedef short           int16s;
typedef short           int16_t;

//typedef unsigned long   int32u;
//typedef long            int32s;

// On a Zynq Linux system it is okay to declare 32-bit unsigned int
// as "unsigned int" type?
typedef unsigned int    int32u;
typedef unsigned int    uint32_t;

typedef int             int32s;
typedef int             int32_t;

typedef struct _int64u {
	int32u lo_addr;
	int32u hi_addr;
} int64u;

typedef struct uint64_t {
	int32u lo_addr;
	int32u hi_addr;
} new_uint64_t;

#endif // TYPES_H
