/* -*-C-*- */
/* SPDX-License-Identifier:    GPL-2.0+ */
/*
 * Copyright (C) 2020-2022 MicroSys Electronics GmbH
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

#include "mpxs32g274a.h"

#include <i2c.h>
#include <s32-cc/serdes_hwconfig.h>
#include <asm/gpio.h>
#include <dm/uclass.h>
#include <hwconfig.h>
#include <net.h>

#include "board_common.h"
#include "mpxs32g274a.h"

#include "pfeng/pfeng.h"

#define DIP_SEL_SDHC_BIT 3

#if CONFIG_IS_ENABLED(MICROSYS_CRXS32G2) || CONFIG_IS_ENABLED(MICROSYS_CRXS32G3)
static serdes_t serdes_mode = SERDES_M2;
#endif

void setup_iomux_uart(void)
{
#if (CONFIG_FSL_LINFLEX_MODULE == 0)

	/* Muxing for linflex0 */
//	setup_iomux_uart0();

#elif (CONFIG_FSL_LINFLEX_MODULE == 1)
	/* Muxing for linflex1 */

	/* set PC08 - MSCR[40] - for UART1 TXD */
	writel(SIUL2_MSCR_S32G_G1_PORT_CTRL_UART1_TXD,
			SIUL2_0_MSCRn(SIUL2_PC08_MSCR_S32_G1_UART1));

	/* set PC04 - MSCR[36] - for UART1 RXD */
	writel(SIUL2_MSCR_S32G_G1_PORT_CTRL_UART_RXD,
			SIUL2_0_MSCRn(SIUL2_PC04_MSCR_S32_G1_UART1));

	/* set PC04 - MSCR[736]/IMCR[224] - for UART1 RXD */
	writel(SIUL2_IMCR_S32G_G1_UART1_RXD_to_pad,
			SIUL2_1_IMCRn(SIUL2_PC04_IMCR_S32_G1_UART1));
#else
#error "Unsupported UART pinmuxing configuration"
#endif
}

static const char* bmode_str(const uint8_t bmode)
{
	switch (bmode) {
	case 0: return "Serial boot/XOSC diff";
	case 1: return "RCON";
	case 2: return "Serial boot/XOSC/bypass";
	default: return "n/a";
	}
}

static void print_dips(const uint8_t sw)
{
	printf("  PCIe0/SGMII CLK:  %dMHz\n", sw&BIT(0) ? 100 : 125);
	printf("  PCIe1/SGMII CLK:  %dMHz\n", sw&BIT(1) ? 100 : 125);
	printf("  RCON EEPROM WP:   %s\n", sw&BIT(2) ? "yes" : "no");
	printf("  SEL SDHC:         %s\n", sw&BIT(DIP_SEL_SDHC_BIT) ? "eMMC" : "SDHC");
	printf("  BOOT MODE:        %s\n", bmode_str((sw>>4)&0x3));
}

#if CONFIG_IS_ENABLED(MICROSYS_CRXS32G2) || CONFIG_IS_ENABLED(MICROSYS_CRXS32G3)

serdes_t get_serdes_sel(void)
{
	uchar reg = 0xff;

	struct udevice *dev = NULL;
	if (i2c_get_chip_for_busnum(1, 0x44, 1, &dev)==0) {
		dm_i2c_read(dev, 0x5, &reg, 1);
	}

	return (reg & 1) ? SERDES_M2 : SERDES_2G5;
}

static int set_serdes_sel(const serdes_t serdes_mode)
{
	uchar reg = 0xff;

	struct udevice *dev = NULL;
	if (i2c_get_chip_for_busnum(1, 0x44, 1, &dev)==0) {
		dm_i2c_read(dev, 0x5, &reg, 1);
		if (serdes_mode == SERDES_2G5)
			reg &= ~1;
		else
			reg |= 1;
		dm_i2c_write(dev, 0x5, &reg, 1);
	}

	return 0;
}

static void check_kconfig(const serdes_t serdes_mode)
{
	switch (serdes_mode) {
	case SERDES_2G5:
#if CONFIG_IS_ENABLED(MICROSYS_CRXS32G2)
env_set("kconfig", "#conf-s32g274asbc2_2g5");
#elif CONFIG_IS_ENABLED(MICROSYS_CRXS32G3)
env_set("kconfig", "#conf-s32g274asbc3_2g5");
#endif
env_set("sja1110_cfg", "sja1110.firmware_name=sja1110_uc_2g5.bin");
break;
	default:
#if CONFIG_IS_ENABLED(MICROSYS_CRXS32G2)
		env_set("kconfig", "#conf-s32g274asbc2_m2");
#elif CONFIG_IS_ENABLED(MICROSYS_CRXS32G3)
		env_set("kconfig", "#conf-s32g274asbc3_m2");
#endif
		env_set("sja1110_cfg", "sja1110.firmware_name=sja1110_uc_m2.bin");
		break;
	}
}

#endif

static uchar eeprom_dip = 0xff;

static int set_eeprom_dip(const uchar reg)
{
	struct udevice *dev = NULL;
	if (i2c_get_chip_for_busnum(0, 0x4d, 1, &dev)==0) {
		dm_i2c_write(dev, 0, &reg, 1);
		eeprom_dip = reg;
	}

	return 0;
}

uchar get_eeprom_dip(const int verbose)
{
	if (eeprom_dip == 0xff) {
		struct udevice *dev = NULL;
		if (i2c_get_chip_for_busnum(0, 0x4d, 1, &dev)==0) {
			dm_i2c_read(dev, 0, &eeprom_dip, 1);
			if (verbose) {
				printf("DIP EEPROM[%d]\n", 0);
				print_dips(eeprom_dip);
			}
		}
	}

	return eeprom_dip;
}

static uint8_t get_board_rev(void)
{
	struct udevice *dev;
	static uchar reg = 0xff;

	if ((reg==0xff) && (i2c_get_chip_for_busnum(0, 0x43, 1, &dev)==0)) {
		if (dm_i2c_read(dev, 0x0f, &reg, 1)==0) {
			reg = (((reg&BIT(7))>>7) | ((reg&BIT(6))>>5) | ((reg&BIT(5))>>3)) + 1;
		}
	}

	return reg;
}

bool s32_serdes_is_external_clk_in_hwconfig(int id)
{
	return true;
}

unsigned long s32_serdes_get_clock_fmhz_from_hwconfig(int id)
{
	unsigned long fmhz = MHZ_100;

	uchar reg = get_eeprom_dip(1);

	fmhz = (reg & BIT(id)) ? MHZ_100 : MHZ_125;

#if CONFIG_IS_ENABLED(CARRIER_CRXS32G3) \
		|| CONFIG_IS_ENABLED(CARRIER_CRXS32G2) \
		|| CONFIG_IS_ENABLED(CARRIER_CRXS32G)

	if (id == 0) {

		const unsigned long current_fmhz = fmhz;

		if (fmhz != MHZ_100) {
			fmhz = MHZ_100;
			reg |= BIT(id);
			set_eeprom_dip(reg);
			printf("SerDes%d clocking has changed from %dMHz to %dMHz!\n",
					id,
					current_fmhz==MHZ_100 ? 100:125,
							fmhz==MHZ_100 ? 100:125);
			puts("Performing necessary reset ...\n");
			do_reset(NULL, 0, 0, NULL);
		}
	}

#endif

#if CONFIG_IS_ENABLED(CARRIER_CRXS32G)

	if (id == 1) {

		const unsigned long current_fmhz = fmhz;

		if (fmhz != MHZ_125) {
			fmhz = MHZ_125;
			reg &= ~BIT(id);
			set_eeprom_dip(reg);
			printf("SerDes%d clocking has changed from %dMHz to %dMHz!\n",
					id,
					current_fmhz==MHZ_100 ? 100:125,
							fmhz==MHZ_100 ? 100:125);
			puts("Performing necessary reset ...\n");
			do_reset(NULL, 0, 0, NULL);
		}
	}
#endif

	printf("PCIe%d clock %dMHz\n", id, fmhz==MHZ_100 ? 100 : 125);

	return fmhz;
}

enum serdes_dev_type s32_serdes_get_mode_from_hwconfig(int id)
{
	char pcie_name[10];
	sprintf(pcie_name, "pcie%d", id);
	enum serdes_dev_type devtype = SERDES_INVALID;

	if (hwconfig_subarg_cmp(pcie_name, "mode", "rc"))
		devtype = PCIE_RC;
	if (hwconfig_subarg_cmp(pcie_name, "mode", "ep"))
		devtype = PCIE_EP;
	if (hwconfig_subarg_cmp(pcie_name, "mode", "sgmii"))
		devtype = SGMII;
	if (hwconfig_subarg_cmp(pcie_name, "mode", "rc&sgmii"))
		devtype = PCIE_RC | SGMII;
	if (hwconfig_subarg_cmp(pcie_name, "mode", "ep&sgmii"))
		devtype = PCIE_EP | SGMII;

#if CONFIG_IS_ENABLED(MICROSYS_CRXS32G2) || CONFIG_IS_ENABLED(MICROSYS_CRXS32G3)

	if (id == 0) {
		if (devtype == SGMII)
			devtype |= PCIE_RC;
		else
			devtype |= SGMII;
	}

	if (id == 1) {
		if (hwconfig_subarg_cmp("pcie1", "xpcs_mode", "2G5")) {
			serdes_mode = SERDES_2G5;
			devtype = SGMII;
		}
		else {
			serdes_mode = SERDES_M2;
			switch (devtype) {
			case PCIE_RC:
			case PCIE_EP:
				devtype |= SGMII;
				break;
			default:
				devtype = PCIE_RC | SGMII;
				break;
			}
		}

		check_kconfig(serdes_mode);
		set_serdes_sel(serdes_mode);

		{
			bool changed = false;
			uchar dip = get_eeprom_dip(0);
			const unsigned long fmhz
				= (dip & BIT(id)) ? MHZ_100 : MHZ_125;

			if ((serdes_mode == SERDES_M2)
				&& (fmhz != MHZ_100)) {
				dip |= BIT(id);
				changed = true;
			}
			else if ((serdes_mode == SERDES_2G5)
					&& (fmhz != MHZ_125)) {
				dip &= ~BIT(id);
				changed = true;
			}

			if (changed) {
				set_eeprom_dip(dip);
				printf("SerDes%d clocking has changed from %dMHz to %dMHz!\n",
						id,
						fmhz==MHZ_100 ? 100:125,
						fmhz==MHZ_100 ? 125:100);
				puts("Performing necessary reset ...\n");
				do_reset(NULL, 0, 0, NULL);
			}
		}
	}
#elif CONFIG_IS_ENABLED(CARRIER_CRXS32G)

	if (id == 0) {
		if (devtype & SGMII)
			devtype &= ~SGMII;
	}

	if (id == 1) {
		if (devtype & PCIE_RC)
			devtype &= ~PCIE_RC;

		if (devtype & PCIE_EP)
			devtype &= ~PCIE_EP;
	}
#endif

	//printf("devtype = 0x%02x\n", devtype);

	return devtype;
}

enum serdes_xpcs_mode s32_serdes_get_xpcs_cfg_from_hwconfig(int id)
{
	char pcie_name[10];

	sprintf(pcie_name, "pcie%d", id);
	/* Set default mode to invalid to force configuration */
	enum serdes_xpcs_mode xpcs_mode = SGMII_INAVALID;

	if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "0"))
		xpcs_mode = SGMII_XPCS0;
	if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "1"))
		xpcs_mode = SGMII_XPCS1;
	if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "both"))
		xpcs_mode = SGMII_XPCS0_XPCS1;
	if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "2G5"))
		xpcs_mode = SGMII_XPCS0_2G5;

#if CONFIG_IS_ENABLED(MICROSYS_CRXS32G2) || CONFIG_IS_ENABLED(MICROSYS_CRXS32G3)

	//    if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "M2"))
	//        xpcs_mode = SGMII_XPCS0_XPCS1;

	if (id == 0) {
		xpcs_mode = SGMII_XPCS0;
	}

	if (id == 1) {
		const serdes_t serdes_mode = get_serdes_sel();
		if (serdes_mode == SERDES_M2)
			xpcs_mode = SGMII_XPCS0;
		else
			xpcs_mode = SGMII_XPCS0_2G5;
	}
#endif

	return xpcs_mode;
}

int fix_pfe_enetaddr(void)
{
	int pfe_index;
	uchar ea[ARP_HLEN];

	for (pfe_index = 0; pfe_index < PFENG_EMACS_COUNT; pfe_index++) {
		if (eth_env_get_enetaddr_by_index("eth", pfe_index+1, ea)) {
			eth_env_set_enetaddr_by_index("pfe", pfe_index, ea);
		}
	}

	return 0;
}

int board_early_init_r(void)
{
	printf("Board: Rev. %d\n", get_board_rev());
#ifdef CONFIG_QSPI_BOOT
	puts("Boot:  QSPI\n");
#endif
#ifdef CONFIG_SD_BOOT
	{
		uchar dip = get_eeprom_dip(0);

		if (dip & BIT(DIP_SEL_SDHC_BIT))
			puts("Boot:  eMMC\n");
		else
			puts("Boot:  SD\n");
	}
#endif
	return 0;
}

int misc_init_f(void)
{
	return 0;
}

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
