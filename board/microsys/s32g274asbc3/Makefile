# SPDX-License-Identifier:	GPL-2.0+
#
# Copyright (C) 2022 MicroSys Electronics GmbH
#

SRC = $(realpath $(srctree))

FSL_COMMON_PATH = $(SRC)/board/freescale/common
NXP_BOARD_PATH  = $(SRC)/board/nxp

obj-y += s32g274asbc3.o
obj-y += ../mpxs32g274a/mpxs32g274a.o
obj-y += ../../../board/nxp/s32-cc/board_common.o
obj-$(CONFIG_ID_EEPROM) += ../../../board/freescale/common/sys_eeprom.o

#ccflags-y += -Iarch/$(ARCH)/cpu/armv8/s32
#ccflags-y += -I$(NXP_S32_PATH)
#ccflags-y += -Idrivers/clk/s32/include
#ccflags-y += -Idrivers/net/s32 -Idrivers/net -Idrivers/net/pfeng/hw_abi
ccflags-y += -I$(FSL_COMMON_PATH) -I$(SRC)/board/microsys/mpxs32g274a
ccflags-y += -I$(SRC)/drivers/net -I$(SRC)/drivers/net/pfeng/hw_abi
