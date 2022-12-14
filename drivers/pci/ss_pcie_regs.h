// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 */

#ifndef S32GEN1_SS_PCIE_REGS_H
#define S32GEN1_SS_PCIE_REGS_H

#define BUILD_BIT_VALUE(field, x) (((x) & (1)) << field##_BIT)
#define BUILD_MASK_VALUE(field, x) (((x) & (field##_MASK)) << field##_LSB)

/* Status and Command Register. */

#define PCIE_CTRL_TYPE1_STATUS_COMMAND_REG		0x4U

/* Class Code and Revision ID Register. */

#define PCIE_CTRL_TYPE1_CLASS_CODE_REV_ID_REG		0x8U

/* Field definitions for TYPE1_STATUS_COMMAND_REG */

#define PCIE_IO_EN_BIT             (0)
#define PCIE_IO_EN                 BIT(PCIE_IO_EN_BIT)

#define PCIE_MSE_BIT               (1)
#define PCIE_MSE                   BIT(PCIE_MSE_BIT)

#define PCIE_BME_BIT               (2)
#define PCIE_BME                   BIT(PCIE_BME_BIT)

#define PCIE_PERREN_BIT            (6)
#define PCIE_PERREN                BIT(PCIE_PERREN_BIT)

#define PCIE_SERREN_BIT            (8)
#define PCIE_SERREN                BIT(PCIE_SERREN_BIT)

#define PCIE_INT_EN_BIT            (10)
#define PCIE_INT_EN                BIT(PCIE_INT_EN_BIT)

/* Instance PCIE_SPCIE_CAP_HEADER */

#define PCIE_SPCIE_CAP_HEADER_BASEADDRESS		0x148U

/* Lane Equalization Control Register for lanes 1 and 0. */

#define PCIE_SPCIE_CAP_SPCIE_CAP_0CH_REG		0xCU

/* Instance PCIE_CAP */

#define PCIE_CAP_BASEADDRESS		0x0070U

/* Device Control and Status Register. */

#define PCIE_CAP_DEVICE_CONTROL_DEVICE_STATUS \
	(PCIE_CAP_BASEADDRESS + 0x8U)

/* Link Capabilities Register */
#define PCIE_CAP_LINK_CAPABILITIES_REG	(PCIE_CAP_BASEADDRESS + 0xCU)

/* Link Control 2 and Status 2 Register. */

#define PCIE_CAP_LINK_CONTROL2_LINK_STATUS2_REG \
	(PCIE_CAP_BASEADDRESS + 0x30U)

/* Field definitions for DEVICE_CONTROL_DEVICE_STATUS */

#define PCIE_CAP_CORR_ERR_REPORT_EN_BIT  (0)
#define PCIE_CAP_CORR_ERR_REPORT_EN \
		BIT(PCIE_CAP_CORR_ERR_REPORT_EN_BIT)

#define PCIE_CAP_NON_FATAL_ERR_REPORT_EN_BIT  (1)
#define PCIE_CAP_NON_FATAL_ERR_REPORT_EN \
		BIT(PCIE_CAP_NON_FATAL_ERR_REPORT_EN_BIT)

#define PCIE_CAP_FATAL_ERR_REPORT_EN_BIT  (2)
#define PCIE_CAP_FATAL_ERR_REPORT_EN \
		BIT(PCIE_CAP_FATAL_ERR_REPORT_EN_BIT)

#define PCIE_CAP_UNSUPPORT_REQ_REP_EN_BIT  (3)
#define PCIE_CAP_UNSUPPORT_REQ_REP_EN \
		BIT(PCIE_CAP_UNSUPPORT_REQ_REP_EN_BIT)

#define PCIE_CAP_EN_REL_ORDER_BIT  (4)
#define PCIE_CAP_EN_REL_ORDER      BIT(PCIE_CAP_EN_REL_ORDER_BIT)

#define PCIE_CAP_MAX_PAYLOAD_SIZE_CS_LSB  (5)
#define PCIE_CAP_MAX_PAYLOAD_SIZE_CS_MASK (0x00000007)
#define PCIE_CAP_MAX_PAYLOAD_SIZE_CS ((PCIE_CAP_MAX_PAYLOAD_SIZE_CS_MASK) << \
		(PCIE_CAP_MAX_PAYLOAD_SIZE_CS_LSB))

#define PCIE_CAP_MAX_READ_REQ_SIZE_LSB (12)
#define PCIE_CAP_MAX_READ_REQ_SIZE_MASK (0x00000007)
#define PCIE_CAP_MAX_READ_REQ_SIZE ((PCIE_CAP_MAX_READ_REQ_SIZE_MASK) << \
		(PCIE_CAP_MAX_READ_REQ_SIZE_LSB))

/* Field definitions for PCIE_CAP_LINK_CAPABILITIES_REG */

#define PCIE_MAX_LINK_SPEED_MASK	0xf
#define PCIE_MAX_LINK_SPEED_LSB		0
#define PCIE_MAX_LINK_SPEED ((PCIE_MAX_LINK_SPEED_MASK) << \
		(PCIE_MAX_LINK_SPEED_LSB))
#define PCIE_MAX_LINK_WIDTH_MASK	0x3f0
#define PCIE_MAX_LINK_WIDTH_LSB		4
#define PCIE_MAX_LINK_WIDTH ((PCIE_MAX_LINK_WIDTH_MASK) << \
		(PCIE_MAX_LINK_WIDTH_LSB))

/* Field definitions for LINK_CONTROL2_LINK_STATUS2_REG */

#define PCIE_CAP_TARGET_LINK_SPEED_LSB (0)
#define PCIE_CAP_TARGET_LINK_SPEED_MASK (0x0000000F)
#define PCIE_CAP_TARGET_LINK_SPEED ((PCIE_CAP_TARGET_LINK_SPEED_MASK) << \
		(PCIE_CAP_TARGET_LINK_SPEED_LSB))

/* Symbol Timer Register and Filter Mask 1 Register (SYMBOL_TIMER_FILTER_1) */

#define PCIE_SYMBOL_TIMER_FILTER_1		(0x71cU)

/* Instance PCIE_BAR (Shadow Mask Registers) */

#define PCIE_DBI_BAR_OFFSET		0x20000U
#define PCIE_BAR_BASEADDRESS		(PCIE_DBI_BAR_OFFSET)

#define PCIE_BAR0_MASK		(PCIE_BAR_BASEADDRESS + 0x10U)
#define PCIE_BAR1_MASK		(PCIE_BAR_BASEADDRESS + 0x14U)
#define PCIE_BAR2_MASK		(PCIE_BAR_BASEADDRESS + 0x18U)
#define PCIE_BAR3_MASK		(PCIE_BAR_BASEADDRESS + 0x1CU)
#define PCIE_BAR4_MASK		(PCIE_BAR_BASEADDRESS + 0x20U)
#define PCIE_BAR5_MASK		(PCIE_BAR_BASEADDRESS + 0x24U)

/* Instance PCIE_IATU */

#define PCIE_DBI_IATU_OFFSET	0x60000U
#define PCIE_IATU_BASEADDRESS		(PCIE_DBI_IATU_OFFSET)

/* iATU Region Control 1 Register. */

#define PCIE_IATU_REGION_CTRL_1_OUTBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x0U)

/* iATU Region Control 2 Register. */

#define PCIE_IATU_REGION_CTRL_2_OUTBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x4U)

/* iATU Lower Base Address Register. */

#define PCIE_IATU_LWR_BASE_ADDR_OUTBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x8U)

/* iATU Upper Base Address Register. */

#define PCIE_IATU_UPPER_BASE_ADDR_OUTBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0xCU)

/* iATU Limit Address Register. */

#define PCIE_IATU_LIMIT_ADDR_OUTBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x10U)

/* iATU Lower Target Address Register. */

#define PCIE_IATU_LWR_TARGET_ADDR_OUTBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x14U)

/* iATU Upper Target Address Register. */

#define PCIE_IATU_UPPER_TARGET_ADDR_OUTBOUND_0 \
		(PCIE_IATU_BASEADDRESS + 0x18U)

/* iATU Region Control 1 Register. */

#define PCIE_IATU_REGION_CTRL_1_INBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x100U)

/* iATU Region Control 2 Register. */

#define PCIE_IATU_REGION_CTRL_2_INBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x104U)

/* iATU Lower Base Address Register. */

#define PCIE_IATU_LWR_BASE_ADDR_INBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x108U)

/* iATU Upper Base Address Register. */

#define PCIE_IATU_UPPER_BASE_ADDR_INBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x10CU)

/* iATU Limit Address Register. */

#define PCIE_IATU_LIMIT_ADDR_INBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x110U)

/* iATU Lower Target Address Register. */

#define PCIE_IATU_LWR_TARGET_ADDR_INBOUND_0 \
	(PCIE_IATU_BASEADDRESS + 0x114U)

/* iATU Upper Target Address Register. */

#define PCIE_IATU_UPPER_TARGET_ADDR_INBOUND_0 \
		(PCIE_IATU_BASEADDRESS + 0x118U)


/* Field definitions for IATU_REGION_CTRL_2_OUTBOUND_0 */

#define PCIE_CFG_SHIFT_MODE_BIT    (28)
#define PCIE_CFG_SHIFT_MODE        BIT(PCIE_CFG_SHIFT_MODE_BIT)

#define PCIE_REGION_EN_BIT         (31)
#define PCIE_REGION_EN             BIT(PCIE_REGION_EN_BIT)

/* Field definitions for IATU_REGION_CTRL_2_INBOUND_0 */

#define PCIE_BAR_NUM_LSB           (8)
#define PCIE_BAR_NUM_MASK          (0x00000007)
#define PCIE_BAR_NUM               ((PCIE_BAR_NUM_MASK) << (PCIE_BAR_NUM_LSB))

#define PCIE_MATCH_MODE_BIT        (30)
#define PCIE_MATCH_MODE            BIT(PCIE_MATCH_MODE_BIT)


/* Instance PCIE_PORT_LOGIC */

#define PCIE_PORT_LOGIC_BASEADDRESS		(0x0700U)

/* Port Force Link Register. */

#define PCIE_PORT_LOGIC_PORT_FORCE \
	(PCIE_PORT_LOGIC_BASEADDRESS + 0x8U)

/* Port Link Control Register. */

#define PCIE_PORT_LOGIC_PORT_LINK_CTRL \
	(PCIE_PORT_LOGIC_BASEADDRESS + 0x10U)

/* Timer Control and Max Function Number Register. */

#define PCIE_PORT_LOGIC_TIMER_CTRL_MAX_FUNC_NUM \
		(PCIE_PORT_LOGIC_BASEADDRESS + 0x18U)

/* Link Width and Speed Change Control Register. */

#define PCIE_PORT_LOGIC_GEN2_CTRL \
		(PCIE_PORT_LOGIC_BASEADDRESS + 0x10CU)

/* Gen3 Control Register. */

#define PCIE_PORT_LOGIC_GEN3_RELATED \
	(PCIE_PORT_LOGIC_BASEADDRESS + 0x190U)

/* Gen3 EQ Control Register. */

#define PCIE_PORT_LOGIC_GEN3_EQ_CONTROL \
		(PCIE_PORT_LOGIC_BASEADDRESS + 0x1A8U)

/* DBI Read-Only Write Enable Register. */

#define PCIE_PORT_LOGIC_MISC_CONTROL_1 \
		(PCIE_PORT_LOGIC_BASEADDRESS + 0x1BCU)

/* ACE Cache Coherency Control Register 3 */

#define PCIE_PORT_LOGIC_COHERENCY_CONTROL_3 \
		(PCIE_PORT_LOGIC_BASEADDRESS + 0x1E8U)

/* Field definitions for PORT_FORCE */

#define PCIE_LINK_NUM_LSB          (0)
#define PCIE_LINK_NUM_MASK         (0x000000FF)
#define PCIE_LINK_NUM              ((PCIE_LINK_NUM_MASK) << \
		(PCIE_LINK_NUM_LSB))

#define PCIE_FORCED_LTSSM_LSB      (8)
#define PCIE_FORCED_LTSSM_MASK     (0x0000000F)
#define PCIE_FORCED_LTSSM          ((PCIE_FORCED_LTSSM_MASK) << \
		(PCIE_FORCED_LTSSM_LSB))

#define PCIE_FORCE_EN_BIT          (15)
#define PCIE_FORCE_EN              BIT(PCIE_FORCE_EN_BIT)

#define PCIE_LINK_STATE_LSB        (16)
#define PCIE_LINK_STATE_MASK       (0x0000003F)
#define PCIE_LINK_STATE            ((PCIE_LINK_STATE_MASK) << \
		(PCIE_LINK_STATE_LSB))

#define PCIE_DO_DESKEW_FOR_SRIS_BIT    (23)
#define PCIE_DO_DESKEW_FOR_SRIS        BIT(PCIE_DO_DESKEW_FOR_SRIS_BIT)

/* Field definitions for PORT_LINK_CTRL */

#define PCIE_RESET_ASSERT_BIT      (3)
#define PCIE_RESET_ASSERT          BIT(PCIE_RESET_ASSERT_BIT)

#define PCIE_DLL_LINK_EN_BIT       (5)
#define PCIE_DLL_LINK_EN           BIT(PCIE_DLL_LINK_EN_BIT)

#define PCIE_LINK_DISABLE_BIT      (6)
#define PCIE_LINK_DISABLE          BIT(PCIE_LINK_DISABLE_BIT)

#define PCIE_FAST_LINK_MODE_BIT    (7)
#define PCIE_FAST_LINK_MODE        BIT(PCIE_FAST_LINK_MODE_BIT)

#define PCIE_LINK_RATE_LSB         (8)
#define PCIE_LINK_RATE_MASK        (0x0000000F)
#define PCIE_LINK_RATE             ((PCIE_LINK_RATE_MASK) << \
		(PCIE_LINK_RATE_LSB))

#define PCIE_LINK_CAPABLE_LSB      (16)
#define PCIE_LINK_CAPABLE_MASK     (0x0000003F)
#define PCIE_LINK_CAPABLE          ((PCIE_LINK_CAPABLE_MASK) << \
		(PCIE_LINK_CAPABLE_LSB))

/* Field definitions for TIMER_CTRL_MAX_FUNC_NUM */

#define PCIE_MAX_FUNC_NUM_LSB      (0)
#define PCIE_MAX_FUNC_NUM_MASK     (0x000000FF)
#define PCIE_MAX_FUNC_NUM          ((PCIE_MAX_FUNC_NUM_MASK) << \
		(PCIE_MAX_FUNC_NUM_LSB))

#define PCIE_FAST_LINK_SCALING_FACTOR_LSB (29)
#define PCIE_FAST_LINK_SCALING_FACTOR_MASK (0x00000003)
#define PCIE_FAST_LINK_SCALING_FACTOR (PCIE_FAST_LINK_SCALING_FACTOR_MASK << \
		(PCIE_FAST_LINK_SCALING_FACTOR_LSB))

/* Field definitions for GEN2_CTRL */

#define PCIE_NUM_OF_LANES_LSB      (8)
#define PCIE_NUM_OF_LANES_MASK     (0x0000001F)
#define PCIE_NUM_OF_LANES          ((PCIE_NUM_OF_LANES_MASK) << \
		(PCIE_NUM_OF_LANES_LSB))

#define PCIE_DIRECT_SPEED_CHANGE_BIT  (17)
#define PCIE_DIRECT_SPEED_CHANGE      BIT(PCIE_DIRECT_SPEED_CHANGE_BIT)

#define PCIE_SRIS_MODE_BIT			(8)
#define PCIE_SRIS_MODE_MASK			BIT(PCIE_SRIS_MODE_BIT)

/* Field definitions for GEN3_RELATED */

#define PCIE_EQ_PHASE_2_3_BIT      (9)
#define PCIE_EQ_PHASE_2_3          BIT(PCIE_EQ_PHASE_2_3_BIT)

#define PCIE_EQ_EIEOS_CNT_BIT      (10)
#define PCIE_EQ_EIEOS_CNT          BIT(PCIE_DSP_EQ_EIEOS_CNT_BIT)

#define PCIE_EQ_REDO_BIT           (11)
#define PCIE_EQ_REDO               BIT(PCIE_DSP_EQ_REDO_BIT)

#define PCIE_RXEQ_PH01_EN_BIT      (12)
#define PCIE_RXEQ_PH01_EN          BIT(PCIE_RXEQ_PH01_EN_BIT)

/* Field definitions for GEN3_EQ_CONTROL */

#define PCIE_GEN3_EQ_FB_MODE_LSB   (0)
#define PCIE_GEN3_EQ_FB_MODE_MASK  (0x0000000F)
#define PCIE_GEN3_EQ_FB_MODE       ((PCIE_GEN3_EQ_FB_MODE_MASK) << \
		(PCIE_GEN3_EQ_FB_MODE_LSB))

#define PCIE_GEN3_EQ_PSET_REQ_VEC_LSB (8)
#define PCIE_GEN3_EQ_PSET_REQ_VEC_MASK (0x0000FFFF)
#define PCIE_GEN3_EQ_PSET_REQ_VEC  ((PCIE_GEN3_EQ_PSET_REQ_VEC_MASK) << \
		(PCIE_GEN3_EQ_PSET_REQ_VEC_LSB))

/* Field definitions for MISC_CONTROL_1 */

#define PCIE_DBI_RO_WR_EN_BIT      (0)
#define PCIE_DBI_RO_WR_EN          BIT(PCIE_DBI_RO_WR_EN_BIT)

/* Field definitions for COHERENCY_CONTROL_3 */

#define PCIE_CFG_MSTR_ARDOMAIN_MODE_LSB (0)
#define PCIE_CFG_MSTR_ARDOMAIN_MODE_MASK (0x00000003)
#define PCIE_CFG_MSTR_ARDOMAIN_MODE ((PCIE_CFG_MSTR_ARDOMAIN_MODE_MASK) << \
		(PCIE_CFG_MSTR_ARDOMAIN_MODE_LSB))

#define PCIE_CFG_MSTR_ARCACHE_MODE_LSB (3)
#define PCIE_CFG_MSTR_ARCACHE_MODE_MASK (0x0000000F)
#define PCIE_CFG_MSTR_ARCACHE_MODE ((PCIE_CFG_MSTR_ARCACHE_MODE_MASK) << \
		(PCIE_CFG_MSTR_ARCACHE_MODE_LSB))

#define PCIE_CFG_MSTR_AWDOMAIN_MODE_LSB (8)
#define PCIE_CFG_MSTR_AWDOMAIN_MODE_MASK (0x00000003)
#define PCIE_CFG_MSTR_AWDOMAIN_MODE ((PCIE_CFG_MSTR_AWDOMAIN_MODE_MASK) << \
		(PCIE_CFG_MSTR_AWDOMAIN_MODE_LSB))

#define PCIE_CFG_MSTR_AWCACHE_MODE_LSB (11)
#define PCIE_CFG_MSTR_AWCACHE_MODE_MASK (0x0000000F)
#define PCIE_CFG_MSTR_AWCACHE_MODE ((PCIE_CFG_MSTR_AWCACHE_MODE_MASK) << \
		(PCIE_CFG_MSTR_AWCACHE_MODE_LSB))

#define PCIE_CFG_MSTR_ARDOMAIN_VALUE_LSB (16)
#define PCIE_CFG_MSTR_ARDOMAIN_VALUE_MASK (0x00000003)
#define PCIE_CFG_MSTR_ARDOMAIN_VALUE ((PCIE_CFG_MSTR_ARDOMAIN_VALUE_MASK) << \
		(PCIE_CFG_MSTR_ARDOMAIN_VALUE_LSB))

#define PCIE_CFG_MSTR_ARCACHE_VALUE_LSB (19)
#define PCIE_CFG_MSTR_ARCACHE_VALUE_MASK (0x0000000F)
#define PCIE_CFG_MSTR_ARCACHE_VALUE ((PCIE_CFG_MSTR_ARCACHE_VALUE_MASK) << \
		(PCIE_CFG_MSTR_ARCACHE_VALUE_LSB))

#define PCIE_CFG_MSTR_AWDOMAIN_VALUE_LSB (24)
#define PCIE_CFG_MSTR_AWDOMAIN_VALUE_MASK (0x00000003)
#define PCIE_CFG_MSTR_AWDOMAIN_VALUE ((PCIE_CFG_MSTR_AWDOMAIN_VALUE_MASK) << \
		(PCIE_CFG_MSTR_AWDOMAIN_VALUE_LSB))

#define PCIE_CFG_MSTR_AWCACHE_VALUE_LSB (27)
#define PCIE_CFG_MSTR_AWCACHE_VALUE_MASK (0x0000000F)
#define PCIE_CFG_MSTR_AWCACHE_VALUE ((PCIE_CFG_MSTR_AWCACHE_VALUE_MASK) << \
		(PCIE_CFG_MSTR_AWCACHE_VALUE_LSB))

#endif  /* S32GEN1_SS_PCIE_REGS_H */
