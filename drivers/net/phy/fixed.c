// SPDX-License-Identifier: GPL-2.0+
/*
 * Fixed-Link phy
 *
 * Copyright 2017 Bernecker & Rainer Industrieelektronik GmbH
 */

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <phy.h>
#include <dm.h>
#include <fdt_support.h>

DECLARE_GLOBAL_DATA_PTR;

int fixedphy_probe(struct phy_device *phydev)
{
	/* fixed-link phy must not be reset by core phy code */
	phydev->flags |= PHY_FLAG_BROKEN_RESET;

	return 0;
}

int fixedphy_config(struct phy_device *phydev)
{
	ofnode node = phy_get_ofnode(phydev);
	struct fixed_link *priv;
	u32 val;

	if (!ofnode_valid(node))
		return -EINVAL;

	/* check for mandatory properties within fixed-link node */
	val = ofnode_read_u32_default(node, "speed", 0);
	if (val != SPEED_10 && val != SPEED_100 && val != SPEED_1000) {
		printf("ERROR: no/invalid speed given in fixed-link node!");
		return -EINVAL;
	}

	priv = malloc(sizeof(*priv));
	if (!priv)
		return -ENOMEM;
	memset(priv, 0, sizeof(*priv));

	phydev->priv = priv;

	priv->link_speed = val;
	priv->duplex = ofnode_read_bool(node, "full-duplex");
	priv->pause = ofnode_read_bool(node, "pause");
	priv->asym_pause = ofnode_read_bool(node, "asym-pause");

	return 0;
}

int fixedphy_startup(struct phy_device *phydev)
{
	struct fixed_link *priv = phydev->priv;

	phydev->asym_pause = priv->asym_pause;
	phydev->pause = priv->pause;
	phydev->duplex = priv->duplex;
	phydev->speed = priv->link_speed;
	phydev->link = 1;

	return 0;
}

int fixedphy_shutdown(struct phy_device *phydev)
{
	return 0;
}

static struct phy_driver fixedphy_driver = {
	.uid		= PHY_FIXED_ID,
	.mask		= 0xffffffff,
	.name		= "Fixed PHY",
	.features	= PHY_GBIT_FEATURES | SUPPORTED_MII,
	.probe		= fixedphy_probe,
	.config		= fixedphy_config,
	.startup	= fixedphy_startup,
	.shutdown	= fixedphy_shutdown,
};

int phy_fixed_init(void)
{
	phy_register(&fixedphy_driver);
	return 0;
}
