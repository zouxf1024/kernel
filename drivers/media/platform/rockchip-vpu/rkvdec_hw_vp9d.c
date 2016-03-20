/*
 * Rockchip VPU codec driver
 *
 * Copyright (C) 2016 Rockchip Electronics Co., Ltd.
 *	Alpha Lin <Alpha.Lin@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "rockchip_vpu_common.h"

#include "rkvdec_regs.h"
#include "rockchip_vpu_hw.h"

#define RKV_VP9D_PROBE_SIZE		4864
#define RKV_VP9D_COUNT_SIZE		13208
#define RKV_VP9D_MAX_SEGMAP_SIZE	73728
/* Data structure describing auxilliary buffer format. */
struct rkvdec_vp9d_priv_tbl {
	u8 prob_table[RKV_VP9D_PROBE_SIZE];
	u8 segmap[RKV_VP9D_MAX_SEGMAP_SIZE];
	u8 segmap_last[RKV_VP9D_MAX_SEGMAP_SIZE];
};

#define PARTITION_CONTEXTS		16
#define PARTITION_TYPES			4

#define MAX_SEGMENTS			8
#define SEG_TREE_PROBS			(MAX_SEGMENTS-1)
#define PREDICTION_PROBS		3
#define SKIP_CONTEXTS			3
#define TX_SIZE_CONTEXTS		2
#define TX_SIZES			4
#define INTRA_INTER_CONTEXTS		4
#define PLANE_TYPES			2
#define COEF_BANDS			6
#define COEFF_CONTEXTS			6
#define UNCONSTRAINED_NODES		3
#define INTRA_MODES			10
#define INTER_PROB_SIZE_ALIGN_TO_128	151
#define INTRA_PROB_SIZE_ALIGN_TO_128	149
#define BLOCK_SIZE_GROUPS		4
#define COMP_INTER_CONTEXTS		5
#define REF_CONTEXTS			5
#define INTER_MODE_CONTEXTS		7
#define SWITCHABLE_FILTERS		3 /* number of switchable filters */
/*#define SWITCHABLE_FILTER_CONTEXTS	(SWITCHABLE_FILTERS + 1)*/
#define INTER_MODES			4
#define MV_JOINTS			4
#define MV_CLASSES			11
/* bits at integer precision for class 0 */
#define CLASS0_BITS			1
/*#define CLASS0_SIZE			(1 << CLASS0_BITS)*/
/*#define MV_OFFSET_BITS		(MV_CLASSES + CLASS0_BITS - 2)*/
#define MV_FP_SIZE			4

static const u8 vp9_kf_y_mode_prob[INTRA_MODES][INTRA_MODES][INTRA_MODES - 1] = {
	{
		// above = dc
		{ 137,  30,  42, 148, 151, 207,  70,  52,  91 },// left = dc
		{  92,  45, 102, 136, 116, 180,  74,  90, 100 },// left = v
		{  73,  32,  19, 187, 222, 215,  46,  34, 100 },// left = h
		{  91,  30,  32, 116, 121, 186,  93,  86,  94 },// left = d45
		{  72,  35,  36, 149,  68, 206,  68,  63, 105 },// left = d135
		{  73,  31,  28, 138,  57, 124,  55, 122, 151 },// left = d117
		{  67,  23,  21, 140, 126, 197,  40,  37, 171 },// left = d153
		{  86,  27,  28, 128, 154, 212,  45,  43,  53 },// left = d207
		{  74,  32,  27, 107,  86, 160,  63, 134, 102 },// left = d63
		{  59,  67,  44, 140, 161, 202,  78,  67, 119 } // left = tm
	}, {  // above = v
		{  63,  36, 126, 146, 123, 158,  60,  90,  96 },// left = dc
		{  43,  46, 168, 134, 107, 128,  69, 142,  92 },// left = v
		{  44,  29,  68, 159, 201, 177,  50,  57,  77 },// left = h
		{  58,  38,  76, 114,  97, 172,  78, 133,  92 },// left = d45
		{  46,  41,  76, 140,  63, 184,  69, 112,  57 },// left = d135
		{  38,  32,  85, 140,  46, 112,  54, 151, 133 },// left = d117
		{  39,  27,  61, 131, 110, 175,  44,  75, 136 },// left = d153
		{  52,  30,  74, 113, 130, 175,  51,  64,  58 },// left = d207
		{  47,  35,  80, 100,  74, 143,  64, 163,  74 },// left = d63
		{  36,  61, 116, 114, 128, 162,  80, 125,  82 } // left = tm
	}, {  // above = h
		{  82,  26,  26, 171, 208, 204,  44,  32, 105 },// left = dc
		{  55,  44,  68, 166, 179, 192,  57,  57, 108 },// left = v
		{  42,  26,  11, 199, 241, 228,  23,  15,  85 },// left = h
		{  68,  42,  19, 131, 160, 199,  55,  52,  83 },// left = d45
		{  58,  50,  25, 139, 115, 232,  39,  52, 118 },// left = d135
		{  50,  35,  33, 153, 104, 162,  64,  59, 131 },// left = d117
		{  44,  24,  16, 150, 177, 202,  33,  19, 156 },// left = d153
		{  55,  27,  12, 153, 203, 218,  26,  27,  49 },// left = d207
		{  53,  49,  21, 110, 116, 168,  59,  80,  76 },// left = d63
		{  38,  72,  19, 168, 203, 212,  50,  50, 107 } // left = tm
	}, {  // above = d45
		{ 103,  26,  36, 129, 132, 201,  83,  80,  93 },// left = dc
		{  59,  38,  83, 112, 103, 162,  98, 136,  90 },// left = v
		{  62,  30,  23, 158, 200, 207,  59,  57,  50 },// left = h
		{  67,  30,  29,  84,  86, 191, 102,  91,  59 },// left = d45
		{  60,  32,  33, 112,  71, 220,  64,  89, 104 },// left = d135
		{  53,  26,  34, 130,  56, 149,  84, 120, 103 },// left = d117
		{  53,  21,  23, 133, 109, 210,  56,  77, 172 },// left = d153
		{  77,  19,  29, 112, 142, 228,  55,  66,  36 },// left = d207
		{  61,  29,  29,  93,  97, 165,  83, 175, 162 },// left = d63
		{  47,  47,  43, 114, 137, 181, 100,  99,  95 } // left = tm
	}, {  // above = d135
		{  69,  23,  29, 128,  83, 199,  46,  44, 101 },// left = dc
		{  53,  40,  55, 139,  69, 183,  61,  80, 110 },// left = v
		{  40,  29,  19, 161, 180, 207,  43,  24,  91 },// left = h
		{  60,  34,  19, 105,  61, 198,  53,  64,  89 },// left = d45
		{  52,  31,  22, 158,  40, 209,  58,  62,  89 },// left = d135
		{  44,  31,  29, 147,  46, 158,  56, 102, 198 },// left = d117
		{  35,  19,  12, 135,  87, 209,  41,  45, 167 },// left = d153
		{  55,  25,  21, 118,  95, 215,  38,  39,  66 },// left = d207
		{  51,  38,  25, 113,  58, 164,  70,  93,  97 },// left = d63
		{  47,  54,  34, 146, 108, 203,  72, 103, 151 } // left = tm
	}, {  // above = d117
		{  64,  19,  37, 156,  66, 138,  49,  95, 133 },// left = dc
		{  46,  27,  80, 150,  55, 124,  55, 121, 135 },// left = v
		{  36,  23,  27, 165, 149, 166,  54,  64, 118 },// left = h
		{  53,  21,  36, 131,  63, 163,  60, 109,  81 },// left = d45
		{  40,  26,  35, 154,  40, 185,  51,  97, 123 },// left = d135
		{  35,  19,  34, 179,  19,  97,  48, 129, 124 },// left = d117
		{  36,  20,  26, 136,  62, 164,  33,  77, 154 },// left = d153
		{  45,  18,  32, 130,  90, 157,  40,  79,  91 },// left = d207
		{  45,  26,  28, 129,  45, 129,  49, 147, 123 },// left = d63
		{  38,  44,  51, 136,  74, 162,  57,  97, 121 } // left = tm
	}, {  // above = d153
		{  75,  17,  22, 136, 138, 185,  32,  34, 166 },// left = dc
		{  56,  39,  58, 133, 117, 173,  48,  53, 187 },// left = v
		{  35,  21,  12, 161, 212, 207,  20,  23, 145 },// left = h
		{  56,  29,  19, 117, 109, 181,  55,  68, 112 },// left = d45
		{  47,  29,  17, 153,  64, 220,  59,  51, 114 },// left = d135
		{  46,  16,  24, 136,  76, 147,  41,  64, 172 },// left = d117
		{  34,  17,  11, 108, 152, 187,  13,  15, 209 },// left = d153
		{  51,  24,  14, 115, 133, 209,  32,  26, 104 },// left = d207
		{  55,  30,  18, 122,  79, 179,  44,  88, 116 },// left = d63
		{  37,  49,  25, 129, 168, 164,  41,  54, 148 } // left = tm
	}, {  // above = d207
		{  82,  22,  32, 127, 143, 213,  39,  41,  70 },// left = dc
		{  62,  44,  61, 123, 105, 189,  48,  57,  64 },// left = v
		{  47,  25,  17, 175, 222, 220,  24,  30,  86 },// left = h
		{  68,  36,  17, 106, 102, 206,  59,  74,  74 },// left = d45
		{  57,  39,  23, 151,  68, 216,  55,  63,  58 },// left = d135
		{  49,  30,  35, 141,  70, 168,  82,  40, 115 },// left = d117
		{  51,  25,  15, 136, 129, 202,  38,  35, 139 },// left = d153
		{  68,  26,  16, 111, 141, 215,  29,  28,  28 },// left = d207
		{  59,  39,  19, 114,  75, 180,  77, 104,  42 },// left = d63
		{  40,  61,  26, 126, 152, 206,  61,  59,  93 } // left = tm
	}, {  // above = d63
		{  78,  23,  39, 111, 117, 170,  74, 124,  94 },// left = dc
		{  48,  34,  86, 101,  92, 146,  78, 179, 134 },// left = v
		{  47,  22,  24, 138, 187, 178,  68,  69,  59 },// left = h
		{  56,  25,  33, 105, 112, 187,  95, 177, 129 },// left = d45
		{  48,  31,  27, 114,  63, 183,  82, 116,  56 },// left = d135
		{  43,  28,  37, 121,  63, 123,  61, 192, 169 },// left = d117
		{  42,  17,  24, 109,  97, 177,  56,  76, 122 },// left = d153
		{  58,  18,  28, 105, 139, 182,  70,  92,  63 },// left = d207
		{  46,  23,  32,  74,  86, 150,  67, 183,  88 },// left = d63
		{  36,  38,  48,  92, 122, 165,  88, 137,  91 } // left = tm
	}, {  // above = tm
		{  65,  70,  60, 155, 159, 199,  61,  60,  81 },// left = dc
		{  44,  78, 115, 132, 119, 173,  71, 112,  93 },// left = v
		{  39,  38,  21, 184, 227, 206,  42,  32,  64 },// left = h
		{  58,  47,  36, 124, 137, 193,  80,  82,  78 },// left = d45
		{  49,  50,  35, 144,  95, 205,  63,  78,  59 },// left = d135
		{  41,  53,  52, 148,  71, 142,  65, 128,  51 },// left = d117
		{  40,  36,  28, 143, 143, 202,  40,  55, 137 },// left = d153
		{  52,  34,  29, 129, 183, 227,  42,  35,  43 },// left = d207
		{  42,  44,  44, 104, 105, 164,  64, 130,  80 },// left = d63
		{  43,  81,  53, 140, 169, 204,  68,  84,  72 } // left = tm
	}
};

static const u8 vp9_kf_partition_probs[PARTITION_CONTEXTS][PARTITION_TYPES - 1] = {
	/* 8x8 -> 4x4 */
	{ 158,  97,  94 },	/* a/l both not split   */
	{  93,  24,  99 },	/* a split, l not split */
	{  85, 119,  44 },	/* l split, a not split */
	{  62,  59,  67 },	/* a/l both split       */
	/* 16x16 -> 8x8 */
	{ 149,  53,  53 },	/* a/l both not split   */
	{  94,  20,  48 },	/* a split, l not split */
	{  83,  53,  24 },	/* l split, a not split */
	{  52,  18,  18 },	/* a/l both split       */
	/* 32x32 -> 16x16 */
	{ 150,  40,  39 },	/* a/l both not split   */
	{  78,  12,  26 },	/* a split, l not split */
	{  67,  33,  11 },	/* l split, a not split */
	{  24,   7,   5 },	/* a/l both split       */
	/* 64x64 -> 32x32 */
	{ 174,  35,  49 },	/* a/l both not split   */
	{  68,  11,  27 },	/* a split, l not split */
	{  57,  15,   9 },	/* l split, a not split */
	{  12,   3,   3 },	/* a/l both split       */
};

static const u8 vp9_kf_uv_mode_prob[INTRA_MODES][INTRA_MODES - 1] = {
	{ 144,  11,  54, 157, 195, 130,  46,  58, 108 },  /* y = dc   */
	{ 118,  15, 123, 148, 131, 101,  44,  93, 131 },  /* y = v    */
	{ 113,  12,  23, 188, 226, 142,  26,  32, 125 },  /* y = h    */
	{ 120,  11,  50, 123, 163, 135,  64,  77, 103 },  /* y = d45  */
	{ 113,   9,  36, 155, 111, 157,  32,  44, 161 },  /* y = d135 */
	{ 116,   9,  55, 176,  76,  96,  37,  61, 149 },  /* y = d117 */
	{ 115,   9,  28, 141, 161, 167,  21,  25, 193 },  /* y = d153 */
	{ 120,  12,  32, 145, 195, 142,  32,  38,  86 },  /* y = d207 */
	{ 116,  12,  64, 120, 140, 125,  49, 115, 121 },  /* y = d63  */
	{ 102,  19,  66, 162, 182, 122,  35,  59, 128 }   /* y = tm   */
};

int rkvdec_vp9d_init(struct rockchip_vpu_ctx *ctx)
{
	struct rockchip_vpu_dev *vpu = ctx->dev;
	int ret;

	ret = rockchip_vpu_aux_buf_alloc(vpu, &ctx->hw.vp9d.priv_tbl,
					 sizeof(struct rkvdec_vp9d_priv_tbl));
	if (ret) {
		vpu_err("allocate vp9d priv_tbl failed\n");
		return ret;
	}

	ret = rockchip_vpu_aux_buf_alloc(vpu, &ctx->hw.vp9d.priv_dst,
					 RKV_VP9D_COUNT_SIZE);
	if (ret) {
		vpu_err("allocate vp9d priv_dst failed\n");
		return ret;
	}

	memset(&ctx->hw.vp9d.last_info, 0, sizeof(ctx->hw.vp9d.last_info));
	ctx->hw.vp9d.mv_base_addr = 0;

	return 0;
}

void rkvdec_vp9d_exit(struct rockchip_vpu_ctx *ctx)
{
	rockchip_vpu_aux_buf_free(ctx->dev, &ctx->hw.vp9d.priv_tbl);
	rockchip_vpu_aux_buf_free(ctx->dev, &ctx->hw.vp9d.priv_dst);
}

static void rkvdec_vp9d_output_prob(struct rockchip_vpu_ctx *ctx)
{
	const struct v4l2_ctrl_vp9_decode_param *dec_param =
		ctx->run.vp9d.dec_param;
	struct rkvdec_vp9d_priv_tbl *tbl =
		(struct rkvdec_vp9d_priv_tbl *)ctx->hw.vp9d.priv_tbl.cpu;

	struct fifo_s prob;

	s32 i, j, k, m, n;
	bool intra_only = dec_param->key_frame ||
		dec_param->flags & V4L2_VP9_FRAME_HDR_FLAG_INTRA_MB_ONLY;
	u8 partition_probs[PARTITION_CONTEXTS][PARTITION_TYPES - 1];
	u8 uv_mode_prob[INTRA_MODES][INTRA_MODES - 1];

	memset(tbl->prob_table, 0, sizeof(tbl->prob_table));

	if (intra_only) {
		memcpy(partition_probs, vp9_kf_partition_probs,
			sizeof(partition_probs));
		memcpy(uv_mode_prob, vp9_kf_uv_mode_prob,
			sizeof(uv_mode_prob));
	} else {
		memcpy(partition_probs, dec_param->entropy_hdr.partition,
			sizeof(partition_probs));
		memcpy(uv_mode_prob, dec_param->entropy_hdr.uv_mode,
			sizeof(uv_mode_prob));
	}

	fifo_packet_init(&prob, tbl->prob_table, sizeof(tbl->prob_table));
	/* sb info  5 x 128 bit */
	for (i = 0; i < PARTITION_CONTEXTS; i++)
		for (j = 0; j < PARTITION_TYPES - 1; j++)
			fifo_write_bits(&prob, partition_probs[i][j], 8,
					"partition probs");

	for (i = 0; i < PREDICTION_PROBS; i++)
		fifo_write_bits(&prob, dec_param->sgmnt_hdr.pred_probs[i], 8,
				"segment id pred probs");

	for (i = 0; i < SEG_TREE_PROBS; i++)
		fifo_write_bits(&prob, dec_param->sgmnt_hdr.tree_probs[i], 8,
				"segment id probs");

	for (i = 0; i < SKIP_CONTEXTS; i++)
		fifo_write_bits(&prob, dec_param->entropy_hdr.skip[i], 8,
				"skip flag probs");

	for (i = 0; i < TX_SIZE_CONTEXTS; i++)
		for (j = 0; j < TX_SIZES - 1; j++)
			fifo_write_bits(&prob,
					dec_param->entropy_hdr.tx32p[i][j],
					8, "tx_size probs");

	for (i = 0; i < TX_SIZE_CONTEXTS; i++)
		for (j = 0; j < TX_SIZES - 2; j++)
			fifo_write_bits(&prob,
					dec_param->entropy_hdr.tx16p[i][j],
					8, "tx_size probs");

	for (i = 0; i < TX_SIZE_CONTEXTS; i++)
		fifo_write_bits(&prob, dec_param->entropy_hdr.tx8p[i],
				8, "tx_size probs");

	for (i = 0; i < INTRA_INTER_CONTEXTS; i++)
		fifo_write_bits(&prob, dec_param->entropy_hdr.intra[i], 8,
				"intra probs");

	fifo_align_bits(&prob, 128);
	if (intra_only) {
		/* intra probs */
		//intra only //149 x 128 bit ,aligned to 152 x 128 bit
		//coeff releated prob   64 x 128 bit
		for (i = 0; i < TX_SIZES; i++) {
			for (j = 0; j < PLANE_TYPES; j++) {
				u32 byte_count = 0;
				for (k = 0; k < COEF_BANDS; k++) {
					for (m = 0; m < COEFF_CONTEXTS; m++) {
						for (n = 0; n < UNCONSTRAINED_NODES; n++) {
							fifo_write_bits(&prob, dec_param->entropy_hdr.coef[i][j][0][k][m][n], 8, "coeff prob");

							byte_count++;
							if (byte_count == 27) {
								fifo_align_bits(&prob, 128);
								byte_count = 0;
							}
						}
					}
				}
				fifo_align_bits(&prob, 128);
			}
		}

		//intra mode prob  80 x 128 bit
		for (i = 0; i < INTRA_MODES; i++) { //vp9_kf_y_mode_prob
			u32 byte_count = 0;
			for (j = 0; j < INTRA_MODES; j++) {
				for (k = 0; k < INTRA_MODES - 1; k++) {
					fifo_write_bits(&prob, vp9_kf_y_mode_prob[i][j][k], 8, "intra mode probs");
					byte_count++;
					if (byte_count == 27) {
						byte_count = 0;
						fifo_align_bits(&prob, 128);
					}
				}
			}

			if (i < 4) {
				for (m = 0; m < (i < 3 ? 23 : 21); m++)
					fifo_write_bits(&prob, ((u8 *)(&vp9_kf_uv_mode_prob[0][0]))[i * 23 + m], 8, "intra mode probs");
				for (; m < 23; m++)
					fifo_write_bits(&prob, 0, 8, "intra mode probs");
			} else {
				for (m = 0; m < 23; m++) {
					fifo_write_bits(&prob, 0, 8, "intra mode probs");
				}
			}
			fifo_align_bits(&prob, 128);
		}
		//align to 152 x 128 bit
		for (i = 0; i < INTER_PROB_SIZE_ALIGN_TO_128 - INTRA_PROB_SIZE_ALIGN_TO_128; i++) { //aligned to 153 x 256 bit
			fifo_write_bits(&prob, 0, 8, "align");
			fifo_align_bits(&prob, 128);
		}
	} else {
		//inter probs
		//151 x 128 bit ,aligned to 152 x 128 bit
		//inter only
		//intra_y_mode & inter_block info   6 x 128 bit
		for (i = 0; i < BLOCK_SIZE_GROUPS; i++) {//intra_y_mode
			for (j = 0; j < INTRA_MODES - 1; j++) {
				fifo_write_bits(&prob, dec_param->entropy_hdr.y_mode[i][j], 8, "intra y mode probs");
			}
		}

		for (i = 0; i < COMP_INTER_CONTEXTS; i++) //reference_mode prob
			fifo_write_bits(&prob, dec_param->entropy_hdr.comp[i], 8, "ref mode probs");

		for (i = 0; i < REF_CONTEXTS; i++) //comp ref bit
			fifo_write_bits(&prob, dec_param->entropy_hdr.comp_ref[i], 8, "comp ref bit");

		for (i = 0; i < REF_CONTEXTS; i++) //single ref bit
			for (j = 0; j < 2; j++)
				fifo_write_bits(&prob, dec_param->entropy_hdr.single_ref[i][j], 8, "single ref bit");

		for (i = 0; i < INTER_MODE_CONTEXTS; i++) //mv mode bit
			for (j = 0; j < INTER_MODES - 1; j++)
				fifo_write_bits(&prob, dec_param->entropy_hdr.mv_mode[i][j], 8, "mv mode bit");

		for (i = 0; i < SWITCHABLE_FILTER_CONTEXTS; i++) //comp ref bit
			for (j = 0; j < SWITCHABLE_FILTERS - 1; j++)
				fifo_write_bits(&prob, dec_param->entropy_hdr.filter[i][j], 8, "comp ref bit");

		fifo_align_bits(&prob, 128);

		//128 x 128bit
		//coeff releated
		for (i = 0; i < TX_SIZES; i++) {
			for (j = 0; j < PLANE_TYPES; j++) {
				u32 byte_count = 0;
				for (k = 0; k < COEF_BANDS; k++) {
					for (m = 0; m < COEFF_CONTEXTS; m++) {
						for (n = 0; n < UNCONSTRAINED_NODES; n++) {
							fifo_write_bits(&prob, dec_param->entropy_hdr.coef[i][j][0][k][m][n], 8, "coeff probs");
							byte_count++;
							if (byte_count == 27) {
								fifo_align_bits(&prob, 128);
								byte_count = 0;
							}
						}
					}
				}
				fifo_align_bits(&prob, 128);
			}
		}

		for (i = 0; i < TX_SIZES; i++) {
			for (j = 0; j < PLANE_TYPES; j++) {
				u32 byte_count = 0;
				for (k = 0; k < COEF_BANDS; k++) {
					for (m = 0; m < COEFF_CONTEXTS; m++) {
						for (n = 0; n < UNCONSTRAINED_NODES; n++) {
							fifo_write_bits(&prob, dec_param->entropy_hdr.coef[i][j][1][k][m][n], 8, "coeff probs");
							byte_count++;
							if (byte_count == 27) {
								fifo_align_bits(&prob, 128);
								byte_count = 0;
							}
						}
					}
				}
				fifo_align_bits(&prob, 128);
			}
		}

		/* intra uv mode 6 x 128 */
		for (i = 0; i < 3; i++) //intra_uv_mode
			for (j = 0; j < INTRA_MODES - 1; j++)
				fifo_write_bits(&prob, uv_mode_prob[i][j], 8, "intra uv mode probs");
		fifo_align_bits(&prob, 128);

		for (; i < 6; i++) //intra_uv_mode
			for (j = 0; j < INTRA_MODES - 1; j++)
				fifo_write_bits(&prob, uv_mode_prob[i][j], 8, "intra uv mode probs");
		fifo_align_bits(&prob, 128);

		for (; i < 9; i++) //intra_uv_mode
			for (j = 0; j < INTRA_MODES - 1; j++)
				fifo_write_bits(&prob, uv_mode_prob[i][j], 8, "intra uv mode probs");
		fifo_align_bits(&prob, 128);

		for (; i < INTRA_MODES; i++) //intra_uv_mode
			for (j = 0; j < INTRA_MODES - 1; j++)
				fifo_write_bits(&prob, uv_mode_prob[i][j], 8, "intra uv mode probs");
		fifo_align_bits(&prob, 128);

		fifo_write_bits(&prob, 0, 8, "align");
		fifo_align_bits(&prob, 128);

		//mv releated 6 x 128
		for (i = 0; i < MV_JOINTS - 1; i++) //mv_joint_type
			fifo_write_bits(&prob, dec_param->entropy_hdr.mv_joint[i], 8, "mv join type");

		for (i = 0; i < 2; i++) //sign bit
			fifo_write_bits(&prob, dec_param->entropy_hdr.mv_comp[i].sign, 8, "sign bit");

		for (i = 0; i < 2; i++) { //classes bit
			for (j = 0; j < MV_CLASSES - 1; j++) {
				fifo_write_bits(&prob, dec_param->entropy_hdr.mv_comp[i].classes[j], 8, "classes bit");
			}
		}

		for (i = 0; i < 2; i++) //classe0 bit
			fifo_write_bits(&prob, dec_param->entropy_hdr.mv_comp[i].class0, 8, "class0 bit");

		for (i = 0; i < 2; i++) { // bits
			for (j = 0; j < MV_OFFSET_BITS; j++) {
				fifo_write_bits(&prob, dec_param->entropy_hdr.mv_comp[i].bits[j], 8, "mv comp bit");
			}
		}

		for (i = 0; i < 2; i++) { //class0_fp bit
			for (j = 0; j < CLASS0_SIZE; j++) {
				for (k = 0; k < MV_FP_SIZE - 1; k++) {
					fifo_write_bits(&prob, dec_param->entropy_hdr.mv_comp[i].class0_fp[j][k], 8, "class0 fp bit");
				}
			}
		}

		for (i = 0; i < 2; i++) { //comp ref bit
			for (j = 0; j < MV_FP_SIZE - 1; j++) {
				fifo_write_bits(&prob, dec_param->entropy_hdr.mv_comp[i].fp[j], 8, "comp ref bit");
			}
		}

		for (i = 0; i < 2; i++) //class0_hp bit
			fifo_write_bits(&prob, dec_param->entropy_hdr.mv_comp[i].class0_hp, 8, "class0 hp bit");

		for (i = 0; i < 2; i++) //hp bit
			fifo_write_bits(&prob, dec_param->entropy_hdr.mv_comp[i].hp, 8, "hp bit");

		fifo_align_bits(&prob, 128);
	}
}

static void rkvdec_vp9d_prepare_table(struct rockchip_vpu_ctx *ctx)
{
	/*
	 * Prepare auxiliary buffer.
	 */
	rkvdec_vp9d_output_prob(ctx);
}

struct vp9d_ref_config {
	u32 reg_frm_size;
	u32 reg_hor_stride;
	u32 reg_y_stride;
	u32 reg_yuv_stride;
	u32 reg_ref_base;
};

enum FRAME_REF_INDEX {
	REF_INDEX_LAST_FRAME,
	REF_INDEX_GOLDEN_FRAME,
	REF_INDEX_ALTREF_FRAME,
	REF_INDEX_NUM
};

static struct vp9d_ref_config ref_config[REF_INDEX_NUM] = {
	{
		.reg_frm_size = RKVDEC_REG_VP9_FRAME_SIZE(0),
		.reg_hor_stride = RKVDEC_VP9_HOR_VIRSTRIDE(0),
		.reg_y_stride = RKVDEC_VP9_LAST_FRAME_YSTRIDE,
		.reg_yuv_stride = RKVDEC_VP9_LAST_FRAME_YUVSTRIDE,
		.reg_ref_base = RKVDEC_REG_VP9_LAST_FRAME_BASE,
	},
	{
		.reg_frm_size = RKVDEC_REG_VP9_FRAME_SIZE(1),
		.reg_hor_stride = RKVDEC_VP9_HOR_VIRSTRIDE(1),
		.reg_y_stride = RKVDEC_VP9_GOLDEN_FRAME_YSTRIDE,
		.reg_yuv_stride = 0,
		.reg_ref_base = RKVDEC_REG_VP9_GOLDEN_FRAME_BASE,
	},
	{
		.reg_frm_size = RKVDEC_REG_VP9_FRAME_SIZE(2),
		.reg_hor_stride = RKVDEC_VP9_HOR_VIRSTRIDE(2),
		.reg_y_stride = RKVDEC_VP9_ALTREF_FRAME_YSTRIDE,
		.reg_yuv_stride = 0,
		.reg_ref_base = RKVDEC_REG_VP9_ALTREF_FRAME_BASE,
	}
};

static void rkvdec_vp9d_config_registers(struct rockchip_vpu_ctx *ctx)
{
	const struct v4l2_ctrl_vp9_decode_param *dec_param =
		ctx->run.vp9d.dec_param;
	struct rockchip_vpu_vp9d_last_info *last_info =
		&ctx->hw.vp9d.last_info;
	struct rockchip_vpu_dev *vpu = ctx->dev;
	dma_addr_t hw_base;
	u32 reg = 0;
	u32 stream_len = 0;
	u32 hor_virstride = 0;
	u32 y_virstride = 0;
	u32 uv_virstride = 0;
	u32 yuv_virstride = 0;
	u32 bit_depth;
	u32 align_h_y;
	u32 align_h_uv;
	s32 i;
	bool intra_only = dec_param->key_frame ||
		dec_param->flags & V4L2_VP9_FRAME_HDR_FLAG_INTRA_MB_ONLY;

	reg = RKVDEC_MODE(2);
	vdpu_write_relaxed(vpu, reg, RKVDEC_REG_SYSCTRL);

	bit_depth = dec_param->bit_depth_luma_minus8 + 8;
	align_h_y = round_up(ctx->dst_fmt.height, 64);
	align_h_uv = round_up(ctx->dst_fmt.height, 64) / 2;

	hor_virstride = round_up(ctx->dst_fmt.width * bit_depth, 128) / 128;
	y_virstride = align_h_y * hor_virstride;

	uv_virstride = align_h_uv * hor_virstride;
	yuv_virstride = y_virstride + uv_virstride;

	reg = RKVDEC_Y_HOR_VIRSTRIDE(hor_virstride)
		| RKVDEC_UV_HOR_VIRSTRIDE(hor_virstride);
	vdpu_write_relaxed(vpu, reg, RKVDEC_REG_PICPAR);

	reg = RKVDEC_Y_VIRSTRIDE(y_virstride);
	vdpu_write_relaxed(vpu, reg, RKVDEC_REG_Y_VIRSTRIDE);

	reg = RKVDEC_YUV_VIRSTRIDE(yuv_virstride);
	vdpu_write_relaxed(vpu, reg, RKVDEC_REG_YUV_VIRSTRIDE);

	stream_len = vb2_get_plane_payload(&ctx->run.src->b.vb2_buf, 0) + 0x80;

	reg = RKVDEC_STRM_LEN(stream_len);
	vdpu_write_relaxed(vpu, reg, RKVDEC_REG_STRM_LEN);

	for (i = 0; i < REF_INDEX_NUM; i++) {
		u32 ref_idx;
		u32 width;
		u32 height;
		struct vb2_buffer *buf;

		switch (i) {
		case REF_INDEX_LAST_FRAME:
			ref_idx = dec_param->last_frame;
			break;
		case REF_INDEX_GOLDEN_FRAME:
			ref_idx = dec_param->golden_frame;
			break;
		case REF_INDEX_ALTREF_FRAME:
			ref_idx = dec_param->alt_frame;
			break;
		default:
			;
		};

		if (ref_idx >= ctx->vq_dst.num_buffers) {
			buf = &ctx->run.dst->b.vb2_buf;
			vdpu_write_relaxed(
				vpu,
				vb2_dma_contig_plane_dma_addr(buf, 0),
				RKVDEC_REG_VP9_LAST_FRAME_BASE);
			continue;
		}

		width = dec_param->ref_frame_coded_width[ref_idx];
		height = dec_param->ref_frame_coded_height[ref_idx];
		buf = ctx->dst_bufs[ref_idx];

		reg = RKVDEC_VP9_FRAMEWIDTH(width)
			| RKVDEC_VP9_FRAMEHEIGHT(height);
		vdpu_write_relaxed(vpu, reg, ref_config[i].reg_frm_size);

		align_h_y = round_up(width * bit_depth, 64) / 128;
		hor_virstride = round_up(width * bit_depth, 128) / 128;

		y_virstride = hor_virstride * align_h_y;
		uv_virstride = hor_virstride * align_h_uv;
		yuv_virstride = y_virstride + uv_virstride;

		reg = RKVDEC_HOR_Y_VIRSTRIDE(hor_virstride)
			| RKVDEC_HOR_UV_VIRSTRIDE(hor_virstride);
		vdpu_write_relaxed(vpu, reg, ref_config[i].reg_hor_stride);

		reg = RKVDEC_VP9_REF_YSTRIDE(y_virstride);
		vdpu_write_relaxed(vpu, reg, ref_config[i].reg_y_stride);

		reg = RKVDEC_VP9_REF_YUVSTRIDE(yuv_virstride);
		if (ref_config[i].reg_yuv_stride)
			vdpu_write_relaxed(vpu, reg,
					   ref_config[i].reg_yuv_stride);

		vdpu_write_relaxed(vpu, vb2_dma_contig_plane_dma_addr(buf, 0),
			ref_config[i].reg_ref_base);
	}

	for (i = 0; i < 8; i++) {
		reg = 0;
		if (last_info->feature_mask[i] & 0x1)
			reg |= RKVDEC_SEGID_FRAME_QP_DELTA_EN;
		reg |= RKVDEC_SEGID_FRAME_QP_DELTA(
			last_info->feature_data[i][0]);
		if (last_info->feature_mask[i] & 0x2)
			reg |= RKVDEC_SEGID_FRAME_LOOPFILTER_VALUE_EN;
		reg |= RKVDEC_SEGID_FRAME_LOOPFILTER_VALUE(
			last_info->feature_data[i][1]);
		if (last_info->feature_mask[i] & 0x4)
			reg |= RKVDEC_SEGID_REFERINFO_EN;
		reg |= RKVDEC_SEGID_REFERINFO(last_info->feature_data[i][2]);
		if (last_info->feature_mask[i] & 0x8)
			reg |= RKVDEC_SEGID_FRAME_SKIP_EN;
		if (i == 0 && last_info->abs_delta)
			reg |= RKVDEC_SEGID_ABS_DELTA;
		vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_SEGID_GRP(i));
	}

	reg = RKVDEC_VP9_TX_MODE(dec_param->txmode)
		| RKVDEC_VP9_FRAME_REF_MODE(dec_param->refmode);
	vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_CPRHEADER_CONFIG);

	if (intra_only) {
		last_info->segmentation_enable = false;
		last_info->intra_only = true;
	} else {
		reg = 0;
		for (i = 0; i < 4; i++)
			reg |= (last_info->ref_deltas[i] & 0x7f) << (7 * i);
		vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_REF_DELTAS_LASTFRAME);

		reg = 0;
		for (i = 0; i < 2; i++)
			reg |= (last_info->mode_deltas[i] & 0x7f) << (7 * i);
		vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_INFO_LASTFRAME);
	}

	reg = 0;
	if (last_info->segmentation_enable)
		reg |= RKVDEC_SEG_EN_LASTFRAME;
	if (last_info->show_frame)
		reg |= RKVDEC_LAST_SHOW_FRAME;
	if (last_info->intra_only)
		reg |= RKVDEC_LAST_INTRA_ONLY;
	if (ctx->dst_fmt.width == last_info->width &&
	    ctx->dst_fmt.height == last_info->height)
		reg |= RKVDEC_LAST_WIDHHEIGHT_EQCUR;
	vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_INFO_LASTFRAME);

	reg = stream_len - dec_param->first_partition_size;
	vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_LASTTILE_SIZE);

	if (!intra_only) {
		/* last frame */
		reg = RKVDEC_VP9_REF_HOR_SCALE(dec_param->mvscale[0][0])
			| RKVDEC_VP9_REF_VER_SCALE(dec_param->mvscale[0][1]);
		vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_REF_SCALE(0));
		/* golden frame */
		reg = RKVDEC_VP9_REF_HOR_SCALE(dec_param->mvscale[1][0])
			| RKVDEC_VP9_REF_VER_SCALE(dec_param->mvscale[1][1]);
		vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_REF_SCALE(1));
		/* altref frame */
		reg = RKVDEC_VP9_REF_HOR_SCALE(dec_param->mvscale[2][0])
			| RKVDEC_VP9_REF_VER_SCALE(dec_param->mvscale[2][1]);
		vdpu_write_relaxed(vpu, reg, RKVDEC_VP9_REF_SCALE(2));
	}

	if (!((dec_param->sgmnt_hdr.flags &
	       V4L2_VP9_SEGMENTATION_FLAG_ENABLED) &&
	      !(dec_param->sgmnt_hdr.flags &
		V4L2_VP9_SEGMENTATION_FLAG_UPDATE_MAP))) {
		if (!intra_only &&
		    !(dec_param->flags &
		      V4L2_VP9_FRAME_HDR_FLAG_ERROR_RESILIENT_MODE)) {
			struct rkvdec_vp9d_priv_tbl *tbl =
				ctx->hw.vp9d.priv_tbl.cpu;

			memcpy(tbl->segmap_last, tbl->segmap,
				sizeof(tbl->segmap));
		}
	}

	if (!intra_only && !(dec_param->flags &
			     V4L2_VP9_FRAME_HDR_FLAG_ERROR_RESILIENT_MODE))
		last_info->mv_base_addr = ctx->hw.vp9d.mv_base_addr;

	hw_base = vb2_dma_contig_plane_dma_addr(&ctx->run.dst->b.vb2_buf, 0);
	vdpu_write_relaxed(vpu, hw_base, RKVDEC_REG_DECOUT_BASE);

	ctx->hw.vp9d.mv_base_addr = hw_base + (yuv_virstride << 4);

	hw_base = vb2_dma_contig_plane_dma_addr(&ctx->run.src->b.vb2_buf, 0);
	vdpu_write_relaxed(vpu, hw_base, RKVDEC_REG_STRM_RLC_BASE);

	hw_base = ctx->hw.vp9d.priv_tbl.dma +
		offsetof(struct rkvdec_vp9d_priv_tbl, prob_table);
	vdpu_write_relaxed(vpu, hw_base, RKVDEC_REG_CABACTBL_PROB_BASE);

	hw_base = ctx->hw.vp9d.priv_dst.dma;
	vdpu_write_relaxed(vpu, hw_base, RKVDEC_REG_VP9COUNT_BASE);

	hw_base = ctx->hw.vp9d.priv_tbl.dma +
		offsetof(struct rkvdec_vp9d_priv_tbl, segmap);
	vdpu_write_relaxed(vpu, hw_base, RKVDEC_REG_VP9_SEGIDCUR_BASE);

	hw_base = ctx->hw.vp9d.priv_tbl.dma +
		offsetof(struct rkvdec_vp9d_priv_tbl, segmap_last);
	vdpu_write_relaxed(vpu, hw_base, RKVDEC_REG_VP9_SEGIDLAST_BASE);

	if (!last_info->mv_base_addr)
		last_info->mv_base_addr = ctx->hw.vp9d.mv_base_addr;

	vdpu_write_relaxed(vpu, last_info->mv_base_addr,
			   RKVDEC_VP9_REF_COLMV_BASE);
}

void rkvdec_vp9d_run(struct rockchip_vpu_ctx *ctx)
{
	struct rockchip_vpu_dev *vpu = ctx->dev;

	/* Prepare data in memory. */
	rkvdec_vp9d_prepare_table(ctx);

	rockchip_vpu_power_on(vpu);

	/* Configure hardware registers. */
	rkvdec_vp9d_config_registers(ctx);

	schedule_delayed_work(&vpu->watchdog_work, msecs_to_jiffies(2000));

	/* Start decoding! */
	vdpu_write(vpu,
		   RKVDEC_INTERRUPT_DEC_E | RKVDEC_CONFIG_DEC_CLK_GATE_E,
		   RKVDEC_REG_INTERRUPT);
}

void rkvdec_vp9d_done(struct rockchip_vpu_ctx *ctx,
		      enum vb2_buffer_state result)
{
	struct rockchip_vpu_vp9d_last_info *last_info =
			&ctx->hw.vp9d.last_info;
	const struct v4l2_ctrl_vp9_decode_param *dec_param =
		ctx->run.vp9d.dec_param;
	u32 i;

	last_info->abs_delta = dec_param->sgmnt_hdr.flags &
		V4L2_VP9_SEGMENTATION_FLAG_ABS_DELTA;

	for (i = 0 ; i < 4; i ++)
		last_info->ref_deltas[i] = dec_param->ref_deltas[i];

	for (i = 0 ; i < 2; i ++)
		last_info->mode_deltas[i] = dec_param->mode_deltas[i];

	for (i = 0; i < 8; i++) {
		last_info->feature_data[i][0] =
			dec_param->sgmnt_hdr.feature_data[i][0];
		last_info->feature_data[i][1] =
			dec_param->sgmnt_hdr.feature_data[i][1];
		last_info->feature_data[i][2] =
			dec_param->sgmnt_hdr.feature_data[i][2];
		last_info->feature_data[i][3] =
			dec_param->sgmnt_hdr.feature_data[i][3];
		last_info->feature_mask[i] =
			dec_param->sgmnt_hdr.feature_mask[i];
	}

	last_info->segmentation_enable = dec_param->sgmnt_hdr.flags &
		V4L2_VP9_SEGMENTATION_FLAG_ENABLED;
	last_info->show_frame = dec_param->flags &
		V4L2_VP9_FRAME_HDR_FLAG_SHOW_FRAME;
	last_info->width = ctx->dst_fmt.width;
	last_info->height = ctx->dst_fmt.height;
	last_info->intra_only = (dec_param->key_frame ||
				 dec_param->flags &
				 V4L2_VP9_FRAME_HDR_FLAG_INTRA_MB_ONLY);

	rockchip_vpu_run_done(ctx, result);
}
