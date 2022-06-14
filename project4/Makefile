# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux RomFS filesystem routines.
#

obj-m += romfs.o
romfs-objs += super.o storage.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules