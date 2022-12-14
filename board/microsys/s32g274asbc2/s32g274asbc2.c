// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2018-2020 NXP
 * Copyright (C) 2020-2021 MicroSys Electronics GmbH
 */
//#include <asm/arch/soc.h>
#include <common.h>
#include <board_common.h>
#include <miiphy.h>
#include <phy.h>
#include <i2c.h>
#include <reset.h>
#include <clk.h>
#include "dwc_eth_qos.h"
#include "mpxs32g274a.h"

#define GMAC_PHYADDR 0

extern int fix_eeprom_mac_addresses(void);

int last_stage_init(void)
{
	struct udevice *eth;
	struct phy_device *phy;
	struct mii_dev *bus;
	struct eqos_priv *eqos;

	fix_pfe_enetaddr();
	fix_eeprom_mac_addresses();

	// GMAC0:
	eth = eth_get_dev_by_name("eth_eqos");
	bus = miiphy_get_dev_by_name("pfeng_emac_2");
	if (eth && bus) {
		phy = phy_connect(bus, GMAC_PHYADDR, eth,
				PHY_INTERFACE_MODE_SGMII);
		if (phy)
			phy_config(phy);

		/*
		 * Connect PHY device explicitly to private data structure of the
		 * GMAC interface:
		 */
		eqos = dev_get_priv(eth);
		eqos->phy = phy;
		eqos->phy_addr = GMAC_PHYADDR;
		eqos->mii = bus;
	}

	// PFE1:
	eth = eth_get_dev_by_name("eth_pfeng");
	bus = miiphy_get_dev_by_name("pfeng_emac_1");
	if (eth && bus) {
		phy = phy_connect(bus, 0x03, eth, PHY_INTERFACE_MODE_RGMII);
		if (phy) phy_config(phy);
	}

	// PFE2:
	eth = eth_get_dev_by_name("eth_pfeng");
	bus = miiphy_get_dev_by_name("pfeng_emac_2");
	if (eth && bus) {
		phy = phy_connect(bus, 0x01, eth, PHY_INTERFACE_MODE_RGMII);
		if (phy) phy_config(phy);
	}

	return 0;
}
