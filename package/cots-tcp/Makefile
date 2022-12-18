#----------------------------------------------------------------------------
# SPDX short identifier: BSD-3-Clause
# Copyright (c) 2022, Avnet
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions a
#
# * Redistributions of source code must retain the above copyright not
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   this list of conditions and the following disclaimer in the docume
#   and/or other materials provided with the distribution
#
# * Neither the name of Avnet, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived fr
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR P
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQU
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GO
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) H
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT L
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT O
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#----------------------------------------------------------------------------
APP = cots-tcp
TOOLCHAIN_PATH = "/mathworks/hub/share/apps/RTT/ARM_CortexA/LinaroToolchain/6.3.1-2017.02/Linux/aarch64/bin/aarch64-linux-gnu-gcc"

# Add any other object files to this list below
#APP_OBJS = cots-tcp.o
SRCS=$(wildcard *.c)
APP_OBJS = $(SRCS:.c=.o)
CC = $(TOOLCHAIN_PATH) 
all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	$(RM) $(APP) *.o *.elf
