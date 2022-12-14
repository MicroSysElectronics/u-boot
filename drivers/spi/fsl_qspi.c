// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2013-2015 Freescale Semiconductor, Inc.
 * Copyright 2020-2022 NXP
 *
 * Freescale Quad Serial Peripheral Interface (QSPI) driver
 */
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <spi-mem.h>
#include <spi.h>
#include <wait_bit.h>
#include <watchdog.h>
#include <asm/io.h>
#include <linux/iopoll.h>
#include <linux/sizes.h>
#include "fsl_qspi.h"

#define OFFSET_BITS_MASK	((FSL_QSPI_FLASH_SIZE  > SZ_16M) ? \
					GENMASK(27, 0) :  GENMASK(23, 0))

/* SEQID */
#define SEQID_WRSR		0
#define SEQID_WREN		1
#define SEQID_FAST_READ		2
#define SEQID_RDSR		3
#define SEQID_SE		4
#define SEQID_CHIP_ERASE	5
#define SEQID_PP		6
#define SEQID_RDID		7
#define SEQID_BE_4K		8
#ifdef CONFIG_SPI_FLASH_BAR
#define SEQID_BRRD		9
#define SEQID_BRWR		10
#define SEQID_RDEAR		11
#define SEQID_WREAR		12
#endif
#define SEQID_WRAR		13
#define SEQID_RDAR		14

/* QSPI CMD */
#define QSPI_CMD_WRSR		0x01	/* Write status register */
#define QSPI_CMD_PP		0x02	/* Page program (up to 256 bytes) */
#define QSPI_CMD_RDSR		0x05	/* Read status register */
#define QSPI_CMD_WREN		0x06	/* Write enable */
#define QSPI_CMD_FAST_READ	0x0b	/* Read data bytes (high frequency) */
#define QSPI_CMD_BE_4K		0x20    /* 4K erase */
#define QSPI_CMD_CHIP_ERASE	0xc7	/* Erase whole flash chip */
#define QSPI_CMD_SE		0xd8	/* Sector erase (usually 64KiB) */
#define QSPI_CMD_RDID		0x9f	/* Read JEDEC ID */

/* Used for Micron, winbond and Macronix flashes */
#define	QSPI_CMD_WREAR		0xc5	/* EAR register write */
#define	QSPI_CMD_RDEAR		0xc8	/* EAR reigster read */

/* Used for Spansion flashes only. */
#define	QSPI_CMD_BRRD		0x16	/* Bank register read */
#define	QSPI_CMD_BRWR		0x17	/* Bank register write */

/* Used for Spansion S25FS-S family flash only. */
#define QSPI_CMD_RDAR		0x65	/* Read any device register */
#define QSPI_CMD_WRAR		0x71	/* Write any device register */

/* 4-byte address QSPI CMD - used on Spansion and some Macronix flashes */
#define QSPI_CMD_FAST_READ_4B	0x0c    /* Read data bytes (high frequency) */
#define QSPI_CMD_PP_4B		0x12    /* Page program (up to 256 bytes) */
#define QSPI_CMD_SE_4B		0xdc    /* Sector erase (usually 64KiB) */

/* default SCK frequency, unit: HZ */
#define FSL_QSPI_DEFAULT_SCK_FREQ	50000000

/* Controller needs driver to swap endian */
#define QUADSPI_QUIRK_SWAP_ENDIAN	BIT(0)

enum fsl_qspi_devtype {
	FSL_QUADSPI_VYBRID,
	FSL_QUADSPI_IMX6SX,
	FSL_QUADSPI_IMX6UL_7D,
	FSL_QUADSPI_IMX7ULP,
	FSL_QUADSPI_S32CC,
};

struct fsl_qspi_devtype_data {
	enum fsl_qspi_devtype devtype;
	u32 rxfifo;
	u32 txfifo;
	u32 ahb_buf_size;
	u32 driver_data;
};

#define QSPI_CMD_SIZE 5

#define LUTS_PER_CONFIG 4

/**
 * struct fsl_qspi_platdata - platform data for Freescale QSPI
 *
 * @flags: Flags for QSPI QSPI_FLAG_...
 * @speed_hz: Default SCK frequency
 * @reg_base: Base address of QSPI registers
 * @amba_base: Base address of QSPI memory mapping
 * @amba_total_size: size of QSPI memory mapping
 * @flash_num: Number of active slave devices
 * @num_chipselect: Number of QSPI chipselect signals
 */
struct fsl_qspi_platdata {
	u32 flags;
	u32 speed_hz;
	fdt_addr_t reg_base;
	fdt_addr_t amba_base;
	fdt_size_t amba_total_size;
	u32 flash_num;
	u32 num_chipselect;
};

static const struct fsl_qspi_devtype_data vybrid_data = {
	.devtype = FSL_QUADSPI_VYBRID,
	.rxfifo = 128,
	.txfifo = 64,
	.ahb_buf_size = 1024,
	.driver_data = QUADSPI_QUIRK_SWAP_ENDIAN,
};

static const struct fsl_qspi_devtype_data imx6sx_data = {
	.devtype = FSL_QUADSPI_IMX6SX,
	.rxfifo = 128,
	.txfifo = 512,
	.ahb_buf_size = 1024,
	.driver_data = 0,
};

static const struct fsl_qspi_devtype_data imx6ul_7d_data = {
	.devtype = FSL_QUADSPI_IMX6UL_7D,
	.rxfifo = 128,
	.txfifo = 512,
	.ahb_buf_size = 1024,
	.driver_data = 0,
};

static const struct fsl_qspi_devtype_data imx7ulp_data = {
	.devtype = FSL_QUADSPI_IMX7ULP,
	.rxfifo = 64,
	.txfifo = 64,
	.ahb_buf_size = 128,
	.driver_data = 0,
};

static const struct fsl_qspi_devtype_data s32cc_data = {
	.devtype = FSL_QUADSPI_S32CC,
	.rxfifo = 128,
	.txfifo = 256,
	.ahb_buf_size = 1024,
	.driver_data = 0,
};

static const struct fsl_qspi_devtype_data s32g3_data = {
	.devtype = FSL_QUADSPI_S32CC,
	.rxfifo = 128,
	.txfifo = 256,
	.ahb_buf_size = 1024,
	.driver_data = 0,
};

u32 qspi_read32(u32 flags, u32 *addr)
{
	return flags & QSPI_FLAG_REGMAP_ENDIAN_BIG ?
		in_be32(addr) : in_le32(addr);
}

void qspi_write32(u32 flags, u32 *addr, u32 val)
{
	flags & QSPI_FLAG_REGMAP_ENDIAN_BIG ?
		out_be32(addr, val) : out_le32(addr, val);
}

int is_s32g3_qspi(struct fsl_qspi_priv *priv)
{
	return priv->devtype_data == &s32g3_data;
}

static inline int is_controller_busy(const struct fsl_qspi_priv *priv)
{
	u32 val;
	u32 mask = QSPI_SR_BUSY_MASK | QSPI_SR_AHB_ACC_MASK |
		   QSPI_SR_IP_ACC_MASK;

	if (priv->flags & QSPI_FLAG_REGMAP_ENDIAN_BIG)
		mask = (u32)cpu_to_be32(mask);

	return readl_poll_timeout(&priv->regs->sr, val, !(val & mask), 1000);
}

/* QSPI support swapping the flash read/write data
 * in hardware for LS102xA, but not for VF610 */
static inline u32 qspi_endian_xchg(struct fsl_qspi_priv *priv, u32 data)
{
	if (priv->devtype_data->driver_data & QUADSPI_QUIRK_SWAP_ENDIAN)
		return swab32(data);
	else
		return data;
}

#ifndef CONFIG_NXP_S32CC
static void qspi_set_lut(struct fsl_qspi_priv *priv)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 lut_base;
	u32 oprnd0, oprnd1;

	/* Unlock the LUT */
	qspi_write32(priv->flags, &regs->lutkey, LUT_KEY_VALUE);
	qspi_write32(priv->flags, &regs->lckcr, QSPI_LCKCR_UNLOCK);

	/* Write Enable */
	lut_base = SEQID_WREN * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_WREN) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	qspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Fast Read */
	lut_base = SEQID_FAST_READ * LUTS_PER_CONFIG;
#ifndef CONFIG_SPI_FLASH_BAR
	qspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(QSPI_CMD_FAST_READ) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#else
	if (FSL_QSPI_FLASH_SIZE  <= SZ_16M)
		qspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(QSPI_CMD_FAST_READ) | PAD0(LUT_PAD1) |
			     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
			     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	else
		qspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(QSPI_CMD_FAST_READ_4B) |
			     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) |
			     OPRND1(ADDR32BIT) | PAD1(LUT_PAD1) |
			     INSTR1(LUT_ADDR));
#endif
	qspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(8) | PAD0(LUT_PAD1) | INSTR0(LUT_DUMMY) |
		     OPRND1(priv->devtype_data->rxfifo) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_READ));
	qspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read Status */
	lut_base = SEQID_RDSR * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_RDSR) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	qspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Erase a sector */
	lut_base = SEQID_SE * LUTS_PER_CONFIG;

	if (FSL_QSPI_FLASH_SIZE  <= SZ_16M) {
		oprnd0 = QSPI_CMD_SE;
		oprnd1 = ADDR24BIT;
	} else {
		oprnd0 = QSPI_CMD_SE_4B;
		oprnd1 = ADDR32BIT;
	}

#ifdef CONFIG_SPI_FLASH_BAR
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(oprnd0) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(oprnd1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));

#else
	qspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(oprnd0) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(oprnd1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#endif
	qspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Erase the whole chip */
	lut_base = SEQID_CHIP_ERASE * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(QSPI_CMD_CHIP_ERASE) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	qspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Page Program */
	lut_base = SEQID_PP * LUTS_PER_CONFIG;
#ifndef CONFIG_SPI_FLASH_BAR
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_PP) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#else
	if (FSL_QSPI_FLASH_SIZE  <= SZ_16M)
		qspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(QSPI_CMD_PP) | PAD0(LUT_PAD1) |
			     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
			     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	else
		qspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(QSPI_CMD_PP_4B) | PAD0(LUT_PAD1) |
			     INSTR0(LUT_CMD) | OPRND1(ADDR32BIT) |
			     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#endif
	/* Use IDATSZ in IPCR to determine the size and here set 0. */
	qspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(0) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_WRITE));
	qspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* READ ID */
	lut_base = SEQID_RDID * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_RDID) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(8) |
		PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	qspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* SUB SECTOR 4K ERASE */
	lut_base = SEQID_BE_4K * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_BE_4K) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	qspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	qspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

#ifdef CONFIG_SPI_FLASH_BAR
	/*
	 * BRRD BRWR RDEAR WREAR are all supported, because it is hard to
	 * dynamically check whether to set BRRD BRWR or RDEAR WREAR during
	 * initialization.
	 */
	lut_base = SEQID_BRRD * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_BRRD) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_READ));

	lut_base = SEQID_BRWR * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_BRWR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_WRITE));

	lut_base = SEQID_RDEAR * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_RDEAR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_READ));

	lut_base = SEQID_WREAR * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(QSPI_CMD_WREAR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_WRITE));
#endif

	/*
	 * Read any device register.
	 * Used for Spansion S25FS-S family flash only.
	 */
	lut_base = SEQID_RDAR * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(QSPI_CMD_RDAR) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	qspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(8) | PAD0(LUT_PAD1) | INSTR0(LUT_DUMMY) |
		     OPRND1(1) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_READ));

	/*
	 * Write any device register.
	 * Used for Spansion S25FS-S family flash only.
	 */
	lut_base = SEQID_WRAR * LUTS_PER_CONFIG;
	qspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(QSPI_CMD_WRAR) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	qspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(1) | PAD0(LUT_PAD1) | INSTR0(LUT_WRITE));

	/* Lock the LUT */
	qspi_write32(priv->flags, &regs->lutkey, LUT_KEY_VALUE);
	qspi_write32(priv->flags, &regs->lckcr, QSPI_LCKCR_LOCK);

	return;
}
#endif

#if defined(CONFIG_SYS_FSL_QSPI_AHB)
/*
 * If we have changed the content of the flash by writing or erasing,
 * we need to invalidate the AHB buffer. If we do not do so, we may read out
 * the wrong data. The spec tells us reset the AHB domain and Serial Flash
 * domain at the same time.
 */
static void qspi_ahb_invalid(struct fsl_qspi_priv *priv)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 reg;

	reg = qspi_read32(priv->flags, &regs->mcr);
	reg |= QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK;
	qspi_write32(priv->flags, &regs->mcr, reg);

	/*
	 * The minimum delay : 1 AHB + 2 SFCK clocks.
	 * Delay 1 us is enough.
	 */
	udelay(1);

	reg &= ~(QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK);
	qspi_write32(priv->flags, &regs->mcr, reg);
}

static void enable_write(struct fsl_qspi_priv *priv)
{
	u32 status_reg;
	struct fsl_qspi_regs *regs = priv->regs;

	qspi_write32(priv->flags, &regs->rbct, QSPI_RBCT_RXBRD_USEIPS);

	status_reg = 0;
	while ((status_reg & FLASH_STATUS_WEL) != FLASH_STATUS_WEL) {
		WATCHDOG_RESET();

		qspi_write32(priv->flags, &regs->ipcr,
			     (SEQID_WREN << QSPI_IPCR_SEQID_SHIFT) | 0);
		while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
			;

		qspi_write32(priv->flags, &regs->ipcr,
			     (SEQID_RDSR << QSPI_IPCR_SEQID_SHIFT) | 1);
		while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
			;

		while (!(qspi_read32(priv->flags, &regs->rbsr) &
			QSPI_RBSR_RDBFL_MASK))
			;

		status_reg = qspi_read32(priv->flags, &regs->rbdr[0]);
		status_reg = qspi_endian_xchg(priv, status_reg);

		qspi_write32(priv->flags, &regs->mcr,
			     qspi_read32(priv->flags, &regs->mcr) |
			     QSPI_MCR_CLR_RXF_MASK);
	}
}

/* Read out the data from the AHB buffer. */
static void qspi_ahb_read(struct fsl_qspi_priv *priv, u8 *rxbuf, int len)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 mcr_reg;
	void *rx_addr;

	mcr_reg = qspi_read32(priv->flags, &regs->mcr);

	qspi_write32(priv->flags, &regs->mcr,
		     QSPI_MCR_CLR_RXF_MASK | QSPI_MCR_CLR_TXF_MASK |
		     mcr_reg);

	qspi_write32(priv->flags, &regs->bfgencr,
		     SEQID_FAST_READ << QSPI_BFGENCR_SEQID_SHIFT);

	rx_addr = (void *)(uintptr_t)(priv->cur_amba_base + priv->sf_addr);
	/* Read out the data directly from the AHB buffer. */
	memcpy_fromio(rxbuf, rx_addr, len);

	qspi_write32(priv->flags, &regs->mcr, mcr_reg);
}

static void qspi_enable_ddr_mode(struct fsl_qspi_priv *priv)
{
#ifndef CONFIG_NXP_S32CC
	u32 reg;
	u32 reg2;
	struct fsl_qspi_regs *regs = priv->regs;

	reg = qspi_read32(priv->flags, &regs->mcr);
	/* Disable the module */
	qspi_write32(priv->flags, &regs->mcr, reg | QSPI_MCR_MDIS_MASK);

	/* Set the Sampling Register for DDR */
	reg2 = qspi_read32(priv->flags, &regs->smpr);
	reg2 &= ~QSPI_SMPR_DDRSMP_MASK;
	reg2 |= (2 << QSPI_SMPR_DDRSMP_SHIFT);
	qspi_write32(priv->flags, &regs->smpr, reg2);

	/* Enable bit 29 for imx6sx */
	reg |= BIT(29);

	/* Enable the module again (enable the DDR too) */
	reg |= QSPI_MCR_DDR_EN_MASK;

	qspi_write32(priv->flags, &regs->mcr, reg);

	/* Enable the TDH to 1 for some platforms like imx6ul, imx7d, etc
	 * These two bits are reserved on other platforms
	 */
	reg = qspi_read32(priv->flags, &regs->flshcr);
	reg &= ~(BIT(17));
	reg |= BIT(16);
	qspi_write32(priv->flags, &regs->flshcr, reg);
#endif
}

/*
 * There are two different ways to read out the data from the flash:
 *  the "IP Command Read" and the "AHB Command Read".
 *
 * The IC guy suggests we use the "AHB Command Read" which is faster
 * then the "IP Command Read". (What's more is that there is a bug in
 * the "IP Command Read" in the Vybrid.)
 *
 * After we set up the registers for the "AHB Command Read", we can use
 * the memcpy to read the data directly. A "missed" access to the buffer
 * causes the controller to clear the buffer, and use the sequence pointed
 * by the QUADSPI_BFGENCR[SEQID] to initiate a read from the flash.
 */
void qspi_init_ahb_read(struct fsl_qspi_priv *priv)
{
	struct fsl_qspi_regs *regs = priv->regs;

	/* AHB configuration for access buffer 0/1/2 .*/
	qspi_write32(priv->flags, &regs->buf0cr, QSPI_BUFXCR_INVALID_MSTRID);
	qspi_write32(priv->flags, &regs->buf1cr, QSPI_BUFXCR_INVALID_MSTRID);
	qspi_write32(priv->flags, &regs->buf2cr, QSPI_BUFXCR_INVALID_MSTRID);
	qspi_write32(priv->flags, &regs->buf3cr, QSPI_BUF3CR_ALLMST_MASK |
		     ((priv->devtype_data->ahb_buf_size >> 3) << QSPI_BUF3CR_ADATSZ_SHIFT));

	/* We only use the buffer3 */
	qspi_write32(priv->flags, &regs->buf0ind, 0);
	qspi_write32(priv->flags, &regs->buf1ind, 0);
	qspi_write32(priv->flags, &regs->buf2ind, 0);

	/*
	 * Set the default lut sequence for AHB Read.
	 * Parallel mode is disabled.
	 */
	qspi_write32(priv->flags, &regs->bfgencr,
		     SEQID_FAST_READ << QSPI_BFGENCR_SEQID_SHIFT);

	/*Enable DDR Mode*/
	qspi_enable_ddr_mode(priv);
}
#endif

#ifdef CONFIG_SPI_FLASH_BAR
/* Bank register read/write, EAR register read/write */
static void qspi_op_rdbank(struct fsl_qspi_priv *priv, u8 *rxbuf, u32 len)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 reg, mcr_reg, data, seqid;

	mcr_reg = qspi_read32(priv->flags, &regs->mcr);

	qspi_write32(priv->flags, &regs->mcr,
		     QSPI_MCR_CLR_RXF_MASK | QSPI_MCR_CLR_TXF_MASK |
		     mcr_reg);
	qspi_write32(priv->flags, &regs->rbct, QSPI_RBCT_RXBRD_USEIPS);

	qspi_write32(priv->flags, &regs->sfar, priv->cur_amba_base);

	if (priv->cur_seqid == QSPI_CMD_BRRD)
		seqid = SEQID_BRRD;
	else
		seqid = SEQID_RDEAR;

	qspi_write32(priv->flags, &regs->ipcr,
		     (seqid << QSPI_IPCR_SEQID_SHIFT) | len);

	/* Wait previous command complete */
	while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
		;

	while (1) {
		WATCHDOG_RESET();

		reg = qspi_read32(priv->flags, &regs->rbsr);
		if (reg & QSPI_RBSR_RDBFL_MASK) {
			data = qspi_read32(priv->flags, &regs->rbdr[0]);
			data = qspi_endian_xchg(priv, data);
			memcpy(rxbuf, &data, len);
			qspi_write32(priv->flags, &regs->mcr,
				     qspi_read32(priv->flags, &regs->mcr) |
				     QSPI_MCR_CLR_RXF_MASK);
			break;
		}
	}

	qspi_write32(priv->flags, &regs->mcr, mcr_reg);
}
#endif

static void qspi_op_rdid(struct fsl_qspi_priv *priv, u32 *rxbuf, u32 len)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 mcr_reg, rbsr_reg, data, size = 0;
	int i;

	mcr_reg = qspi_read32(priv->flags, &regs->mcr);

	qspi_write32(priv->flags, &regs->mcr,
		     QSPI_MCR_CLR_RXF_MASK | QSPI_MCR_CLR_TXF_MASK |
		     mcr_reg);

	qspi_write32(priv->flags, &regs->rbct, QSPI_RBCT_RXBRD_USEIPS);

	qspi_write32(priv->flags, &regs->sfar, priv->cur_amba_base);

	qspi_write32(priv->flags, &regs->ipcr,
		     (SEQID_RDID << QSPI_IPCR_SEQID_SHIFT) | 0);

	while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
		;

	i = 0;
	while ((priv->devtype_data->rxfifo >= len) && (len > 0)) {
		WATCHDOG_RESET();

		rbsr_reg = qspi_read32(priv->flags, &regs->rbsr);
		if (rbsr_reg & QSPI_RBSR_RDBFL_MASK) {
			data = qspi_read32(priv->flags, &regs->rbdr[i]);
			data = qspi_endian_xchg(priv, data);
			size = (len < 4) ? len : 4;
			memcpy(rxbuf, &data, size);
			len -= size;
			rxbuf++;
			i++;
		} else {
			break;
		}
	}

	qspi_write32(priv->flags, &regs->mcr, mcr_reg);
}

/* If not use AHB read, read data from ip interface */
static void qspi_op_read(struct fsl_qspi_priv *priv, u32 *rxbuf, u32 len)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 mcr_reg, data;
	int i, size;
	u32 to_or_from;
	u32 seqid;

	if (priv->cur_seqid == QSPI_CMD_RDAR)
		seqid = SEQID_RDAR;
	else
		seqid = SEQID_FAST_READ;

	mcr_reg = qspi_read32(priv->flags, &regs->mcr);

	qspi_write32(priv->flags, &regs->mcr,
		     QSPI_MCR_CLR_RXF_MASK | QSPI_MCR_CLR_TXF_MASK |
		     mcr_reg);
	qspi_write32(priv->flags, &regs->rbct, QSPI_RBCT_RXBRD_USEIPS);

	to_or_from = priv->sf_addr + priv->cur_amba_base;

	while (len > 0) {
		WATCHDOG_RESET();

		qspi_write32(priv->flags, &regs->sfar, to_or_from);

		size = (len > priv->devtype_data->rxfifo) ?
			priv->devtype_data->rxfifo : len;

		qspi_write32(priv->flags, &regs->ipcr,
			     (seqid << QSPI_IPCR_SEQID_SHIFT) |
			     size);
		while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
			;

		to_or_from += size;
		len -= size;

		i = 0;
		while ((priv->devtype_data->rxfifo >= size) && (size > 0)) {
			data = qspi_read32(priv->flags, &regs->rbdr[i]);
			data = qspi_endian_xchg(priv, data);
			if (size < 4)
				memcpy(rxbuf, &data, size);
			else
				memcpy(rxbuf, &data, 4);
			rxbuf++;
			size -= 4;
			i++;
		}
		qspi_write32(priv->flags, &regs->mcr,
			     qspi_read32(priv->flags, &regs->mcr) |
			     QSPI_MCR_CLR_RXF_MASK);
	}

	qspi_write32(priv->flags, &regs->mcr, mcr_reg);
}

static void qspi_op_write(struct fsl_qspi_priv *priv, u8 *txbuf, u32 len)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 mcr_reg, data, seqid;
	int i, size, tx_size;
	u32 to_or_from = 0;
	u32 tbsr, trctr, trbfl;

	mcr_reg = qspi_read32(priv->flags, &regs->mcr);

	qspi_write32(priv->flags, &regs->mcr,
		     QSPI_MCR_CLR_RXF_MASK | QSPI_MCR_CLR_TXF_MASK |
		     mcr_reg);

	enable_write(priv);

	/* Default is page programming */
	seqid = SEQID_PP;
	if (priv->cur_seqid == QSPI_CMD_WRAR)
		seqid = SEQID_WRAR;
#ifdef CONFIG_SPI_FLASH_BAR
	if (priv->cur_seqid == QSPI_CMD_BRWR)
		seqid = SEQID_BRWR;
	else if (priv->cur_seqid == QSPI_CMD_WREAR)
		seqid = SEQID_WREAR;
#endif

	to_or_from = priv->sf_addr + priv->cur_amba_base;

	/* Controller isn't busy */
	while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
		;

	/* TX buffer is empty */
	while (qspi_read32(priv->flags, &regs->tbsr) & 0xFFU)
		;

	qspi_write32(priv->flags, &regs->sfar, to_or_from);

	tx_size = (len > priv->devtype_data->txfifo) ?
		priv->devtype_data->txfifo : len;

	size = tx_size / 32;
	/*
	 * There must be atleast 128bit data
	 * available in TX FIFO for any pop operation
	 */
	if (tx_size % 16)
		size++;

	for (i = 0; i < size * 4; i++) {
		memcpy(&data, txbuf, 4);
		data = qspi_endian_xchg(priv, data);
		qspi_write32(priv->flags, &regs->tbdr, data);
		txbuf += 4;
	}

	qspi_write32(priv->flags, &regs->ipcr,
		     (seqid << QSPI_IPCR_SEQID_SHIFT) | tx_size);
	while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
		;

	/* Wait until all bytes are transmitted */
	do {
		tbsr = qspi_read32(priv->flags, &regs->tbsr);
		trctr = QSPI_TBSR_TRCTR(tbsr);
		trbfl = QSPI_TBSR_TRBFL(tbsr);
	} while ((trctr != tx_size / 4) || trbfl);
	qspi_write32(priv->flags, &regs->mcr, mcr_reg);
}

static void qspi_op_rdsr(struct fsl_qspi_priv *priv, void *rxbuf, u32 len)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 mcr_reg, reg, data;

	mcr_reg = qspi_read32(priv->flags, &regs->mcr);

	qspi_write32(priv->flags, &regs->mcr,
		     QSPI_MCR_CLR_RXF_MASK | QSPI_MCR_CLR_TXF_MASK |
		     mcr_reg);
	qspi_write32(priv->flags, &regs->rbct, QSPI_RBCT_RXBRD_USEIPS);

	qspi_write32(priv->flags, &regs->sfar, priv->cur_amba_base);

	qspi_write32(priv->flags, &regs->ipcr,
		     (SEQID_RDSR << QSPI_IPCR_SEQID_SHIFT) | 0);
	while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
		;

	while (1) {
		WATCHDOG_RESET();

		reg = qspi_read32(priv->flags, &regs->rbsr);
		if (reg & QSPI_RBSR_RDBFL_MASK) {
			data = qspi_read32(priv->flags, &regs->rbdr[0]);
			data = qspi_endian_xchg(priv, data);
			memcpy(rxbuf, &data, len);
			qspi_write32(priv->flags, &regs->mcr,
				     qspi_read32(priv->flags, &regs->mcr) |
				     QSPI_MCR_CLR_RXF_MASK);
			break;
		}
	}

	qspi_write32(priv->flags, &regs->mcr, mcr_reg);
}

static void qspi_op_erase(struct fsl_qspi_priv *priv)
{
	struct fsl_qspi_regs *regs = priv->regs;
	u32 mcr_reg;
	u32 to_or_from = 0;

	mcr_reg = qspi_read32(priv->flags, &regs->mcr);

	qspi_write32(priv->flags, &regs->mcr,
		     QSPI_MCR_CLR_RXF_MASK | QSPI_MCR_CLR_TXF_MASK |
		     mcr_reg);

	enable_write(priv);
	qspi_write32(priv->flags, &regs->rbct, QSPI_RBCT_RXBRD_USEIPS);

	to_or_from = priv->sf_addr + priv->cur_amba_base;
	qspi_write32(priv->flags, &regs->sfar, to_or_from);

	qspi_write32(priv->flags, &regs->ipcr,
		     (SEQID_WREN << QSPI_IPCR_SEQID_SHIFT) | 0);
	while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
		;

	if (priv->cur_seqid == QSPI_CMD_SE) {
		qspi_write32(priv->flags, &regs->ipcr,
			     (SEQID_SE << QSPI_IPCR_SEQID_SHIFT) | 0);
	} else if (priv->cur_seqid == QSPI_CMD_BE_4K) {
		qspi_write32(priv->flags, &regs->ipcr,
			     (SEQID_BE_4K << QSPI_IPCR_SEQID_SHIFT) | 0);
	}
	while (qspi_read32(priv->flags, &regs->sr) & QSPI_SR_BUSY_MASK)
		;

	qspi_write32(priv->flags, &regs->mcr, mcr_reg);
}

int qspi_xfer(struct fsl_qspi_priv *priv, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	u32 bytes = DIV_ROUND_UP(bitlen, 8);
	static u32 wr_sfaddr;
	u32 txbuf = 0;

	WATCHDOG_RESET();

	if (dout) {
		if (flags & SPI_XFER_BEGIN) {
			priv->cur_seqid = *(u8 *)dout;
			if (FSL_QSPI_FLASH_SIZE > SZ_16M && 4 < bytes) {
				/* Remove first byte of the command */
				memcpy(&txbuf, dout, 4);
				/* Address is on 3 bytes */
				txbuf &= GENMASK(31, 8);
			} else {
				memcpy(&txbuf, dout, sizeof(txbuf));
			}
		}

		if (flags == SPI_XFER_END) {
			priv->sf_addr = wr_sfaddr;

			qspi_op_write(priv, (u8 *)dout, bytes);
			return 0;
		}

		if (priv->cur_seqid == QSPI_CMD_FAST_READ ||
		    priv->cur_seqid == QSPI_CMD_RDAR) {
			wr_sfaddr = swab32(txbuf) & OFFSET_BITS_MASK;

		} else if ((priv->cur_seqid == QSPI_CMD_SE) ||
			   (priv->cur_seqid == QSPI_CMD_BE_4K)) {
			wr_sfaddr = swab32(txbuf) & OFFSET_BITS_MASK;
			priv->sf_addr = wr_sfaddr;
			qspi_op_erase(priv);
		} else if (priv->cur_seqid == QSPI_CMD_PP ||
			   priv->cur_seqid == QSPI_CMD_WRAR) {
			wr_sfaddr = swab32(txbuf) & OFFSET_BITS_MASK;
		} else if ((priv->cur_seqid == QSPI_CMD_BRWR) ||
			 (priv->cur_seqid == QSPI_CMD_WREAR)) {
#ifdef CONFIG_SPI_FLASH_BAR
			wr_sfaddr = 0;
#endif
		}
	}

	if (din) {
		if (priv->cur_seqid == QSPI_CMD_FAST_READ) {
#ifdef CONFIG_SYS_FSL_QSPI_AHB
			priv->sf_addr = wr_sfaddr;
			qspi_ahb_read(priv, din, bytes);
#else
			qspi_op_read(priv, din, bytes);
#endif
			wr_sfaddr = 0;
		} else if (priv->cur_seqid == QSPI_CMD_RDAR) {
			qspi_op_read(priv, din, bytes);
		} else if (priv->cur_seqid == QSPI_CMD_RDID) {
			qspi_op_rdid(priv, din, bytes);
		} else if (priv->cur_seqid == QSPI_CMD_RDSR) {
			qspi_op_rdsr(priv, din, bytes);
#ifdef CONFIG_SPI_FLASH_BAR
		} else if ((priv->cur_seqid == QSPI_CMD_BRRD) ||
			 (priv->cur_seqid == QSPI_CMD_RDEAR)) {
			priv->sf_addr = 0;

			qspi_op_rdbank(priv, din, bytes);
#endif
		}
	}

#ifdef CONFIG_SYS_FSL_QSPI_AHB
	if ((priv->cur_seqid == QSPI_CMD_SE) ||
	    (priv->cur_seqid == QSPI_CMD_PP) ||
	    (priv->cur_seqid == QSPI_CMD_BE_4K) ||
	    (priv->cur_seqid == QSPI_CMD_WREAR) ||
	    (priv->cur_seqid == QSPI_CMD_FAST_READ) ||
	    (priv->cur_seqid == QSPI_CMD_BRWR)) {
		qspi_ahb_invalid(priv);
	}
#endif

	return 0;
}

void qspi_module_disable(struct fsl_qspi_priv *priv, u8 disable)
{
	u32 mcr_val;
	mcr_val = qspi_read32(priv->flags, &priv->regs->mcr);
	if (disable)
		mcr_val |= QSPI_MCR_MDIS_MASK;
	else
		mcr_val &= ~QSPI_MCR_MDIS_MASK;
	qspi_write32(priv->flags, &priv->regs->mcr, mcr_val);
}

void qspi_cfg_smpr(struct fsl_qspi_priv *priv, u32 clear_bits, u32 set_bits)
{
	u32 smpr_val;

	smpr_val = qspi_read32(priv->flags, &priv->regs->smpr);
	smpr_val &= ~clear_bits;
	smpr_val |= set_bits;
	qspi_write32(priv->flags, &priv->regs->smpr, smpr_val);
}

static int fsl_qspi_child_pre_probe(struct udevice *dev)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);
	struct fsl_qspi_priv *priv = dev_get_priv(dev_get_parent(dev));

	slave->max_write_size = priv->devtype_data->txfifo + QSPI_CMD_SIZE;

	return 0;
}

static __maybe_unused ulong fsl_qspi_clk_get_rate(struct udevice *bus)
{
	struct fsl_qspi_priv *priv = dev_get_priv(bus);
	struct clk clk_qspi_en;
	int ret;

	ret = clk_get_by_name(bus, "qspi_en", &clk_qspi_en);
	if (ret)
		return ret;

	ret = clk_get_by_name(bus, "qspi", &priv->clk_qspi);
	if (ret)
		return ret;

	ret = clk_enable(&clk_qspi_en);
	if (ret)
		return ret;

	ret = clk_enable(&priv->clk_qspi);
	if (ret) {
		clk_disable(&clk_qspi_en);
		return ret;
	}

	return clk_get_rate(&priv->clk_qspi);
}

static int fsl_qspi_probe(struct udevice *bus)
{
	u32 amba_size_per_chip;
	struct fsl_qspi_platdata *plat = dev_get_platdata(bus);
	struct fsl_qspi_priv *priv = dev_get_priv(bus);
	struct dm_spi_bus *dm_spi_bus;
	int i, ret;
	u32 mcr_val;

	dm_spi_bus = bus->uclass_priv;

#if defined(CONFIG_NXP_S32CC)
	dm_spi_bus->max_hz = fsl_qspi_clk_get_rate(bus);
	if (!dm_spi_bus->max_hz) {
		printf("Invalid clk rate: %u\n", dm_spi_bus->max_hz);
		return -EINVAL;
	}
#else
	dm_spi_bus->max_hz = plat->speed_hz;
#endif

	priv->regs = (struct fsl_qspi_regs *)(uintptr_t)plat->reg_base;
	priv->flags = plat->flags;

	priv->ddr_mode = false;
	priv->num_pads = 1;

#if defined(CONFIG_NXP_S32CC) && (defined(CONFIG_SPI_FLASH_MACRONIX) || \
	defined(CONFIG_SPI_FLASH_STMICRO))
	s32cc_reset_bootrom_settings(priv);
#endif

#if defined(CONFIG_NXP_S32CC)
	priv->speed_hz = dm_spi_bus->max_hz;
#else
	priv->speed_hz = plat->speed_hz;
#endif
	/*
	 * QSPI SFADR width is 32bits, the max dest addr is 4GB-1.
	 * AMBA memory zone should be located on the 0~4GB space
	 * even on a 64bits cpu.
	 */
	priv->amba_base[0] = (u32)plat->amba_base;
	priv->amba_total_size = (u32)plat->amba_total_size;
	priv->flash_num = plat->flash_num;
	priv->num_chipselect = plat->num_chipselect;

	priv->devtype_data = (struct fsl_qspi_devtype_data *)dev_get_driver_data(bus);
	if (!priv->devtype_data) {
		printf("ERROR : No devtype_data found\n");
		return -ENODEV;
	}

	debug("devtype=%d, txfifo=%d, rxfifo=%d, ahb=%d, data=0x%x\n",
		priv->devtype_data->devtype,
		priv->devtype_data->txfifo,
		priv->devtype_data->rxfifo,
		priv->devtype_data->ahb_buf_size,
		priv->devtype_data->driver_data);

	/* make sure controller is not busy anywhere */
	ret = is_controller_busy(priv);

	if (ret) {
		debug("ERROR : The controller is busy\n");
		return ret;
	}

	mcr_val = 0;
	if (IS_ENABLED(CONFIG_NXP_S32CC))
		mcr_val = QSPI_MCR_DQS_EN;

	qspi_write32(priv->flags, &priv->regs->mcr,
		     QSPI_MCR_RESERVED_MASK | QSPI_MCR_MDIS_MASK |
		     QSPI_MCR_END_CFD_LE | mcr_val);

#ifndef CONFIG_NXP_S32CC
	qspi_cfg_smpr(priv, ~(QSPI_SMPR_FSDLY_MASK | QSPI_SMPR_DDRSMP_MASK |
		QSPI_SMPR_FSPHS_MASK | QSPI_SMPR_HSENA_MASK), 0);
#endif

	/*
	 * Assign AMBA memory zone for every chipselect
	 * QuadSPI has two channels, every channel has two chipselects.
	 * If the property 'num-cs' in dts is 2, the AMBA memory will be divided
	 * into two parts and assign to every channel. This indicate that every
	 * channel only has one valid chipselect.
	 * If the property 'num-cs' in dts is 4, the AMBA memory will be divided
	 * into four parts and assign to every chipselect.
	 * Every channel will has two valid chipselects.
	 */
	amba_size_per_chip = priv->amba_total_size >>
			     (priv->num_chipselect >> 1);
	for (i = 1 ; i < priv->num_chipselect ; i++)
		priv->amba_base[i] =
			amba_size_per_chip + priv->amba_base[i - 1];

	/*
	 * Any read access to non-implemented addresses will provide
	 * undefined results.
	 *
	 * In case single die flash devices, TOP_ADDR_MEMA2 and
	 * TOP_ADDR_MEMB2 should be initialized/programmed to
	 * TOP_ADDR_MEMA1 and TOP_ADDR_MEMB1 respectively - in effect,
	 * setting the size of these devices to 0.  This would ensure
	 * that the complete memory map is assigned to only one flash device.
	 */
	qspi_write32(priv->flags, &priv->regs->sfa1ad,
		     priv->amba_base[0] + amba_size_per_chip);
	switch (priv->num_chipselect) {
	case 1:
		break;
	case 2:
		qspi_write32(priv->flags, &priv->regs->sfa2ad,
			     priv->amba_base[1]);
		qspi_write32(priv->flags, &priv->regs->sfb1ad,
			     priv->amba_base[1] + amba_size_per_chip);
		qspi_write32(priv->flags, &priv->regs->sfb2ad,
			     priv->amba_base[1] + amba_size_per_chip);
		break;
	case 4:
		qspi_write32(priv->flags, &priv->regs->sfa2ad,
			     priv->amba_base[2]);
		qspi_write32(priv->flags, &priv->regs->sfb1ad,
			     priv->amba_base[3]);
		qspi_write32(priv->flags, &priv->regs->sfb2ad,
			     priv->amba_base[3] + amba_size_per_chip);
		break;
	default:
		debug("Error: Unsupported chipselect number %u!\n",
		      priv->num_chipselect);
		qspi_module_disable(priv, 1);
		return -EINVAL;
	}

#ifndef CONFIG_NXP_S32CC
	qspi_set_lut(priv);
#else
	s32cc_enable_spi(priv, true);
#endif

#ifdef CONFIG_SYS_FSL_QSPI_AHB
	qspi_init_ahb_read(priv);
#endif

	qspi_write32(priv->flags, &priv->regs->sfacr, 0x0);
	qspi_module_disable(priv, 0);

	return 0;
}

static int fsl_qspi_ofdata_to_platdata(struct udevice *bus)
{
	fdt_addr_t reg_base = 0U, mem_base = 0U;
	fdt_addr_t mem_size = 0U;
	struct fsl_qspi_platdata *plat = bus->platdata;
	int ret, flash_num = 0;
	ofnode subnode;

	if (dev_read_bool(bus, "big-endian"))
		plat->flags |= QSPI_FLAG_REGMAP_ENDIAN_BIG;

	reg_base = dev_read_addr_name(bus, "QuadSPI");
	if (reg_base == FDT_ADDR_T_NONE) {
		debug("Error: can't get regs base addresses!\n");
		return -ENOMEM;
	}

	mem_base = dev_read_addr_size_name(bus, "QuadSPI-memory", &mem_size);
	if (mem_base == FDT_ADDR_T_NONE) {
		debug("Error: can't get AMBA base addresses(ret = %d)!\n", ret);
		return -ENOMEM;
	}

	/* Count flash numbers */
	dev_for_each_subnode(subnode, bus)
		++flash_num;

	if (flash_num == 0) {
		debug("Error: Missing flashes!\n");
		return -ENODEV;
	}

	plat->speed_hz = dev_read_u32_default(bus, "spi-max-frequency",
					      FSL_QSPI_DEFAULT_SCK_FREQ);
	plat->num_chipselect = dev_read_u32_default(bus, "num-cs",
						    FSL_QSPI_MAX_CHIPSELECT_NUM);
	plat->reg_base = reg_base;
	plat->amba_base = mem_base;
	plat->amba_total_size = mem_size;
	plat->flash_num = flash_num;

	debug("%s: regs=<0x%llx> <0x%llx, 0x%llx>, max-frequency=%d, endianess=%s\n",
	      __func__,
	      (u64)plat->reg_base,
	      (u64)plat->amba_base,
	      (u64)plat->amba_total_size,
	      plat->speed_hz,
	      plat->flags & QSPI_FLAG_REGMAP_ENDIAN_BIG ? "be" : "le"
	      );

	return 0;
}

static int fsl_qspi_xfer(struct udevice *dev, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	struct fsl_qspi_priv *priv;
	struct udevice *bus;

	bus = dev->parent;
	priv = dev_get_priv(bus);

	return qspi_xfer(priv, bitlen, dout, din, flags);
}

static int fsl_qspi_claim_bus(struct udevice *dev)
{
	struct fsl_qspi_priv *priv;
	struct udevice *bus;
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);
	int ret;

	bus = dev->parent;
	priv = dev_get_priv(bus);

	/* make sure controller is not busy anywhere */
	ret = is_controller_busy(priv);

	if (ret) {
		debug("ERROR : The controller is busy\n");
		return ret;
	}

	priv->cur_amba_base = priv->amba_base[slave_plat->cs];

	qspi_module_disable(priv, 0);

	return 0;
}

static int fsl_qspi_release_bus(struct udevice *dev)
{
	struct fsl_qspi_priv *priv;
	struct udevice *bus;

	bus = dev->parent;
	priv = dev_get_priv(bus);

	qspi_module_disable(priv, 1);

	return 0;
}

static int fsl_qspi_set_speed(struct udevice *bus, uint speed)
{
	struct fsl_qspi_priv *priv = dev_get_priv(bus);
	int dev_speed;
	int ret;

	ret = clk_disable(&priv->clk_qspi);
	if (ret)
		return ret;

	debug("%s: requested QSPI frequency %u\n", bus->name, speed);
	dev_speed = clk_set_rate(&priv->clk_qspi, speed);
	if (dev_speed < 0)
		return dev_speed;

	debug("%s: actual QSPI frequency %d\n", bus->name, dev_speed);
	if (speed != dev_speed)
		return -EIO;

	return clk_enable(&priv->clk_qspi);
}

static int fsl_qspi_set_mode(struct udevice *bus, uint mode)
{
	/* Nothing to do */
	return 0;
}

static const struct dm_spi_ops fsl_qspi_ops = {
	.claim_bus	= fsl_qspi_claim_bus,
	.release_bus	= fsl_qspi_release_bus,
	.xfer		= fsl_qspi_xfer,
	.set_speed	= fsl_qspi_set_speed,
	.set_mode	= fsl_qspi_set_mode,
#ifdef CONFIG_NXP_S32CC
	.mem_ops	= &s32cc_mem_ops,
#endif
};

static const struct udevice_id fsl_qspi_ids[] = {
	{ .compatible = "fsl,vf610-qspi", .data = (ulong)&vybrid_data },
	{ .compatible = "fsl,imx6sx-qspi", .data = (ulong)&imx6sx_data },
	{ .compatible = "fsl,imx6ul-qspi", .data = (ulong)&imx6ul_7d_data },
	{ .compatible = "fsl,imx7d-qspi", .data = (ulong)&imx6ul_7d_data },
	{ .compatible = "fsl,imx7ulp-qspi", .data = (ulong)&imx7ulp_data },
	{ .compatible = "nxp,s32g-qspi", .data = (ulong)&s32cc_data },
	{ .compatible = "nxp,s32g3-qspi", .data = (ulong)&s32g3_data },
	{ .compatible = "nxp,s32r45-qspi", .data = (ulong)&s32cc_data },
	{ }
};

U_BOOT_DRIVER(fsl_qspi) = {
	.name	= "fsl_qspi",
	.id	= UCLASS_SPI,
	.of_match = fsl_qspi_ids,
	.ops	= &fsl_qspi_ops,
	.ofdata_to_platdata = fsl_qspi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct fsl_qspi_platdata),
	.priv_auto_alloc_size = sizeof(struct fsl_qspi_priv),
	.probe	= fsl_qspi_probe,
	.child_pre_probe = fsl_qspi_child_pre_probe,
};
