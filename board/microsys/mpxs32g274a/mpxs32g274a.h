/* -*-C-*- */
/* SPDX-License-Identifier:    GPL-2.0+ */
/*
 * Copyright (C) 2020 MicroSys Electronics GmbH
 * Author: Kay Potthoff <kay.potthoff@microsys.de>
 *
 */

/*!
 * \addtogroup <group> <title>
 * @{
 *
 * \file
 * <description>
 */

#ifndef MPXS32G274A_H
#define MPXS32G274A_H

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Just another dirty hack ...
 * Please check drivers/net/phy/dp83867.c
 */
struct dp83867_private {
	u32 rx_id_delay;
	u32 tx_id_delay;
	int fifo_depth;
	int io_impedance;
	bool rxctrl_strap_quirk;
	int port_mirroring;
	bool set_clk_output;
	unsigned int clk_output_sel;
	bool sgmii_ref_clk_en;
	bool sgmii_an_enabled;
};

#if CONFIG_IS_ENABLED(MICROSYS_CRXS32G2) || CONFIG_IS_ENABLED(MICROSYS_CRXS32G3)

typedef enum {
    SERDES_M2,
    SERDES_2G5
} serdes_t;

extern serdes_t get_serdes_sel(void);

#endif

extern uchar get_eeprom_dip(const int verbose);
extern int fix_pfe_enetaddr(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MPXS32G274A_H */

/*!@}*/

/* *INDENT-OFF* */
/******************************************************************************
 * Local Variables:
 * mode: C
 * c-indent-level: 4
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 * kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
 * vim: set expandtab filetype=c:
 * vi: set et tabstop=4 shiftwidth=4: */
