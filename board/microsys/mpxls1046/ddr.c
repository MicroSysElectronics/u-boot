/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 * Copyright 2018 MicroSys Electronics GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fsl_ddr_sdram.h>
#include <fsl_ddr_dimm_params.h>
#include <asm/global_data.h>
#include <asm/arch-fsl-layerscape/clock.h>
#include "ddr.h"
#ifdef CONFIG_FSL_DEEP_SLEEP
#include <fsl_sleep.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

void fsl_ddr_board_options(memctl_options_t *popts,
			   dimm_params_t *pdimm,
			   unsigned int ctrl_num)
{
	const struct board_specific_parameters *pbsp, *pbsp_highest = NULL;
	ulong ddr_freq;

	if (ctrl_num > 1) {
		printf("Not supported controller number %d\n", ctrl_num);
		return;
	}
	if (!pdimm->n_ranks)
		return;

	pbsp = udimms[0];

	/* Get clk_adjust, wrlvl_start, wrlvl_ctl, according to the board ddr
	 * freqency and n_banks specified in board_specific_parameters table.
	 */
	ddr_freq = get_ddr_freq(0) / 1000000;
	while (pbsp->datarate_mhz_high) {
		if (pbsp->n_ranks == pdimm->n_ranks) {
			if (ddr_freq <= pbsp->datarate_mhz_high) {
				popts->clk_adjust = pbsp->clk_adjust;
				popts->wrlvl_start = pbsp->wrlvl_start;
				popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
				popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;

				goto found;
			}
			pbsp_highest = pbsp;
		}
		pbsp++;
	}

	if (pbsp_highest) {
		printf("Error: board specific timing not found for %lu MT/s\n",
		       ddr_freq);
		printf("Trying to use the highest speed (%u) parameters\n",
		       pbsp_highest->datarate_mhz_high);
		popts->clk_adjust = pbsp_highest->clk_adjust;
		popts->wrlvl_start = pbsp_highest->wrlvl_start;
		popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
		popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
	} else {
		panic("DIMM is not supported by this board");
	}
found:
	debug("Found timing match: n_ranks %d, data rate %d, rank_gb %d\n",
	      pbsp->n_ranks, pbsp->datarate_mhz_high, pbsp->rank_gb);

	popts->data_bus_width = 0;	/* 64-bit data bus */
	popts->otf_burst_chop_en = 0;
	popts->burst_length = DDR_BL8;
	popts->bstopre = 0;		/* enable auto precharge */

	/*
	 * Factors to consider for half-strength driver enable:
	 *	- number of DIMMs installed
	 */
	popts->half_strength_driver_enable = 0;
	/*
	 * Write leveling override
	 */
	popts->wrlvl_override = 1;
	popts->wrlvl_sample = 0xf;

	/*
	 * Rtt and Rtt_WR override
	 */
	popts->rtt_override = 0;

	/* Enable ZQ calibration */
	popts->zq_en = 1;

	popts->ddr_cdr1 = DDR_CDR1_DHC_EN | DDR_CDR1_ODT(DDR_CDR_ODT_60ohm);
	popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_60ohm) |
			  DDR_CDR2_VREF_TRAIN_EN | DDR_CDR2_VREF_RANGE_2;

	/* optimize cpo for erratum A-009942 */
	popts->cpo_sample = 0x4b;
}

#define MEM_2G (1UL << 31)
#define MEM_4G (1UL << 32)


#ifdef CONFIG_SYS_DDR_RAW_TIMING
dimm_params_t ddr_raw_timing = {
	.n_ranks = 1,
#ifdef CONFIG_MPXLS1046_MEM_4G
	/* DDR model number: MT40A512M16HX-093E */
	.rank_density = MEM_4G,
	.capacity = MEM_4G,
#else
	/* DDR model number: MT40A512M8HX-093E */
	.rank_density = MEM_2G,
	.capacity = MEM_2G,
#endif
	.primary_sdram_width = 64,
	.ec_sdram_width = 8,
	.data_width = 72,
	.registered_dimm = 0,
	.mirrored_dimm = 0,
#ifdef CONFIG_MPXLS1046_MEM_4G
	.n_row_addr = 16,
	.n_col_addr = 10,
	.bank_addr_bits = 0,
	.bank_group_bits = 1,
	.edc_config = 2,
	.burst_lengths_bitmask = 0x0c,

	.tckmin_x_ps = 937,
	.tckmax_ps = 1070,
	.caslat_x = 0x000DFA00,
	.taa_ps = 14060,
	.trcd_ps = 14060,
	.trp_ps = 14060,
	.tras_ps = 33000,
	.trc_ps = 47060,
	.trfc1_ps = 350000,
	.trfc2_ps = 260000,
	.trfc4_ps = 160000,
	.tfaw_ps = 30000,
	.trrds_ps = 5300,
	.trrdl_ps = 6400,
	.tccdl_ps = 5355,
	.refresh_rate_ps = 3900000,
#else
	.n_row_addr = 15,
	.n_col_addr = 10,
	.bank_addr_bits = 0,
	.bank_group_bits = 1,
	.edc_config = 2,
	.burst_lengths_bitmask = 0x0c,

	.tckmin_x_ps = 1250,
	.tckmax_ps = 1499,
	.caslat_x = 0x000DFA00,
	.taa_ps = 13750,
	.trcd_ps = 13750,
	.trp_ps = 13750,
	.tras_ps = 35000,
	.trc_ps = 48750,
	.trfc1_ps = 260000,
	.trfc2_ps = 160000,
	.trfc4_ps = 110000,
	.tfaw_ps = 35000,
	.trrds_ps = 6000,
	.trrdl_ps = 7500,
	.tccdl_ps = 6250,
	.refresh_rate_ps = 3900000,
#endif


	.dq_mapping[0] = 0x07,
	.dq_mapping[1] = 0x2B,
	.dq_mapping[2] = 0x0B,
	.dq_mapping[3] = 0x36,
	.dq_mapping[4] = 0x0C,
	.dq_mapping[5] = 0x2C,
	.dq_mapping[6] = 0x0C,
	.dq_mapping[7] = 0x35,
	.dq_mapping[8] = 0x0B,
	.dq_mapping[9] = 0x2B,
	.dq_mapping[10] = 0x17,
	.dq_mapping[11] = 0x2C,
	.dq_mapping[12] = 0x09,
	.dq_mapping[13] = 0x2C,
	.dq_mapping[14] = 0x15,
	.dq_mapping[15] = 0x36,
	.dq_mapping[16] = 0x02,
	.dq_mapping[17] = 0x21,
	.dq_mapping_ors = 0,
};

int fsl_ddr_get_dimm_params(dimm_params_t *pdimm,
			    unsigned int controller_number,
			    unsigned int dimm_number)
{
	static const char dimm_model[] = "Fixed DDR on board";

	if (((controller_number == 0) && (dimm_number == 0)) ||
	    ((controller_number == 1) && (dimm_number == 0))) {
		memcpy(pdimm, &ddr_raw_timing, sizeof(dimm_params_t));
		memset(pdimm->mpart, 0, sizeof(pdimm->mpart));
		memcpy(pdimm->mpart, dimm_model, sizeof(dimm_model) - 1);
	}

	return 0;
}
#endif

#ifdef CONFIG_TFABOOT
int fsl_initdram(void)
{
	gd->ram_size = tfa_get_dram_size();

	if (!gd->ram_size)
		gd->ram_size = fsl_ddr_sdram_size();

	return 0;
}
#else
int fsl_initdram(void)
{
	phys_size_t dram_size;

#if defined(CONFIG_SPL) && !defined(CONFIG_SPL_BUILD)
	gd->ram_size = fsl_ddr_sdram_size();

	return 0;
#else
	puts("Initializing DDR...\n");

	dram_size = fsl_ddr_sdram();
#endif

	erratum_a008850_post();

	gd->ram_size = dram_size;

	return 0;
}
#endif