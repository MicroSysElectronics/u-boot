/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019-2022 NXP
 */

#ifndef DWC_ETH_QOS_DM_H
#define DWC_ETH_QOS_DM_H

#include <net.h>

struct eqos_config;

struct eqos_pdata {
	struct eth_pdata eth;
	struct eqos_config *config;
};

/* Vendor specific driver configs */

#if CONFIG_IS_ENABLED(DWC_ETH_QOS_TEGRA)
extern struct eqos_config eqos_tegra186_config;
#endif
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_STM32)
extern struct eqos_config eqos_stm32_config;
#endif
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)
extern struct eqos_config eqos_s32cc_config;

enum {
	S32CCGMAC_MODE_UNINITED = 0,
	S32CCGMAC_MODE_DISABLE,
	S32CCGMAC_MODE_ENABLE,
};

u32 s32ccgmac_cfg_get_mode(int cardnum);
phy_interface_t s32ccgmac_cfg_get_interface(int cardnum);
const char *s32ccgmac_cfg_get_ifmode_str_by_num(int cardum);
#endif

/* Supported implementations */

#if CONFIG_IS_ENABLED(OF_CONTROL)
static const struct udevice_id eqos_ids[] = {
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_TEGRA)
	{
		.compatible = "nvidia,tegra186-eqos",
		.data = (ulong)&eqos_tegra186_config
	},
#endif /* CONFIG_DWC_ETH_QOS_TEGRA */
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_STM32)
	{
		.compatible = "snps,dwmac-4.20a",
		.data = (ulong)&eqos_stm32_config
	},
#endif /* CONFIG_DWC_ETH_QOS_STM32 */
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)
	{
		.compatible = "nxp,s32cc-dwmac",
		.data = (ulong)&eqos_s32cc_config
	},
#endif /* CONFIG_DWC_ETH_QOS_S32CC */

	{ }
};
#endif /* CONFIG_IS_ENABLED(OF_CONTROL) */

#endif /* DWC_ETH_QOS_DM_H */
