/* SPDX-License-Identifier: GPL-2.0 */
/**
 * Copyright 2021-2022 NXP
 */
#ifndef FSL_S32GEN1_PCIE_PHY_SUBMODE_H
#define FSL_S32GEN1_PCIE_PHY_SUBMODE_H

enum pcie_phy_mode {
	PCIE_PHY_MODE_INVALID = -1,
	CRNS = 0, /* Common Reference Clock, No Spread Spectrum */
	CRSS = 1, /* Common Reference Clock, Spread Spectrum */
	SRIS = 2  /* Separate Reference Clock, Spread Spectrum */
};

#endif
