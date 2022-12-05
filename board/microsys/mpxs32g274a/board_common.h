/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018-2020 NXP
 * Copyright (C) 2020-2021 MicroSys Electronics GmbH
 */

#ifndef __MPXS32G274A_COMMON_H__
#define __MPXS32G274A_COMMON_H__

#include <config.h>

void setup_iomux_i2c(void);
void setup_iomux_sdhc(void);
void setup_iomux_uart(void);
void setup_iomux_uart1(void);
void setup_iomux_uart2(void);

#ifdef CONFIG_FSL_DSPI
void setup_iomux_dspi(void);
#endif

#if defined(CONFIG_NXP_S32G2)
void setup_iomux_uart0(void);
#endif

#if CONFIG_IS_ENABLED(NETDEVICES)
void ft_enet_fixup(void *fdt);
u32 s32ccgmac_cfg_get_mode(void);
#endif

#if defined(CONFIG_SAF1508BET_USB_PHY)
void setup_iomux_usb(void);
#endif

#endif /* __MPXS32G274A_COMMON_H__ */
