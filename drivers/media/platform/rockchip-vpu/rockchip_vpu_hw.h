/*
 * Rockchip VPU codec driver
 *
 * Copyright (C) 2014 Google, Inc.
 *	Tomasz Figa <tfiga@chromium.org>
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

#ifndef ROCKCHIP_VPU_HW_H_
#define ROCKCHIP_VPU_HW_H_

#include <media/videobuf2-core.h>

#define ROCKCHIP_HEADER_SIZE		1280
#define ROCKCHIP_HW_PARAMS_SIZE		5487
#define ROCKCHIP_RET_PARAMS_SIZE	488

struct rockchip_vpu_dev;
struct rockchip_vpu_ctx;
struct rockchip_vpu_buf;

/**
 * enum rockchip_vpu_type - vpu type.
 * @RK_VPU_NONE:	No vpu type. Used for RAW video formats.
 * @RK3288_VPU:		Vpu on rk3288 soc.
 * @RK3228_VPU:		Vpu on rk3228 soc.
 * @RKVDEC:		Rkvdec.
 */
enum rockchip_vpu_type {
	RK_VPU_NONE	= -1,
	RK3288_VPU,
	RK3228_VPU,
	RKVDEC,
};

#define ROCKCHIP_VPU_MATCHES(type1, type2) \
	(type1 == RK_VPU_NONE || type2 == RK_VPU_NONE || type1 == type2)

/**
 * enum rockchip_vpu_enc_fmt - source format ID for hardware registers.
 */
enum rockchip_vpu_enc_fmt {
	ROCKCHIP_VPU_ENC_FMT_YUV420P = 0,
	ROCKCHIP_VPU_ENC_FMT_YUV420SP = 1,
	ROCKCHIP_VPU_ENC_FMT_YUYV422 = 2,
	ROCKCHIP_VPU_ENC_FMT_UYVY422 = 3,
};

/**
 * struct rockchip_vp8e_reg_params - low level encoding parameters
 * TODO: Create abstract structures for more generic controls or just
 *       remove unused fields.
 */
struct rockchip_vp8e_reg_params {
	u32 unused_00[5];
	u32 hdr_len;
	u32 unused_18[8];
	u32 enc_ctrl;
	u32 unused_3c;
	u32 enc_ctrl0;
	u32 enc_ctrl1;
	u32 enc_ctrl2;
	u32 enc_ctrl3;
	u32 enc_ctrl5;
	u32 enc_ctrl4;
	u32 str_hdr_rem_msb;
	u32 str_hdr_rem_lsb;
	u32 unused_60;
	u32 mad_ctrl;
	u32 unused_68;
	u32 qp_val[8];
	u32 bool_enc;
	u32 vp8_ctrl0;
	u32 rlc_ctrl;
	u32 mb_ctrl;
	u32 unused_9c[14];
	u32 rgb_yuv_coeff[2];
	u32 rgb_mask_msb;
	u32 intra_area_ctrl;
	u32 cir_intra_ctrl;
	u32 unused_e8[2];
	u32 first_roi_area;
	u32 second_roi_area;
	u32 mvc_ctrl;
	u32 unused_fc;
	u32 intra_penalty[7];
	u32 unused_11c;
	u32 seg_qp[24];
	u32 dmv_4p_1p_penalty[32];
	u32 dmv_qpel_penalty[32];
	u32 vp8_ctrl1;
	u32 bit_cost_golden;
	u32 loop_flt_delta[2];
};

/**
 * struct rockchip_vp8e_reg_params - low level encoding parameters
 * TODO: Create abstract structures for more generic controls or just
 *       remove unused fields.
 */
struct rockchip_vp8e_reg_params_new {
	u32 is_intra;
	u32 frm_hdr_size;

	u32 qp;

	s32 mv_prob[2][19];
	s32 intra_prob;

	u32 bool_enc_value;
	u32 bool_enc_value_bits;
	u32 bool_enc_range;

	u32 filterDisable;
	u32 filter_sharpness;
	u32 filter_level;

	s32 intra_frm_delta;
	s32 last_frm_delta;
	s32 golden_frm_delta;
	s32 altref_frm_delta;

	s32 bpred_mode_delta;
	s32 zero_mode_delta;
	s32 newmv_mode_delta;
	s32 splitmv_mode_delta;
};

/**
 * struct rockchip_vpu_aux_buf - auxiliary DMA buffer for hardware data
 * @cpu:	CPU pointer to the buffer.
 * @dma:	DMA address of the buffer.
 * @size:	Size of the buffer.
 */
struct rockchip_vpu_aux_buf {
	void *cpu;
	dma_addr_t dma;
	size_t size;
};

/**
 * struct rockchip_vpu_vp8e_hw_ctx - Context private data specific to codec mode.
 * @ctrl_buf:		VP8 control buffer.
 * @ext_buf:		VP8 ext data buffer.
 * @mv_buf:		VP8 motion vector buffer.
 * @ref_rec_ptr:	Bit flag for swapping ref and rec buffers every frame.
 */
struct rockchip_vpu_vp8e_hw_ctx {
	struct rockchip_vpu_aux_buf ctrl_buf;
	struct rockchip_vpu_aux_buf ext_buf;
	struct rockchip_vpu_aux_buf mv_buf;
	u8 ref_rec_ptr:1;
};

/**
 * struct rockchip_vpu_vp8d_hw_ctx - Context private data of VP8 decoder.
 * @segment_map:	Segment map buffer.
 * @prob_tbl:		Probability table buffer.
 */
struct rockchip_vpu_vp8d_hw_ctx {
	struct rockchip_vpu_aux_buf segment_map;
	struct rockchip_vpu_aux_buf prob_tbl;
};

/**
 * struct rockchip_vpu_h264d_hw_ctx - Per context data specific to H264 decoding.
 * @priv_tbl:		Private auxiliary buffer for hardware.
 */
struct rockchip_vpu_h264d_hw_ctx {
	struct rockchip_vpu_aux_buf priv_tbl;
};

struct rockchip_vpu_h264e_hw_ctx {
	struct rockchip_vpu_aux_buf cabac_tbl;
	struct rockchip_vpu_aux_buf ext_buf;
	u8 ref_rec_ptr:1;
};

/**
 * struct rockchip_vpu_hw_ctx - Context private data of hardware code.
 * @codec_ops:		Set of operations associated with current codec mode.
 */
struct rockchip_vpu_hw_ctx {
	const struct rockchip_vpu_codec_ops *codec_ops;

	/* Specific for particular codec modes. */
	union {
		struct rockchip_vpu_vp8e_hw_ctx vp8e;
		struct rockchip_vpu_vp8d_hw_ctx vp8d;
		struct rockchip_vpu_h264d_hw_ctx h264d;
		struct rockchip_vpu_h264e_hw_ctx h264e;
		struct rockchip_vpu_vp9d_hw_ctx vp9d;
		/* Other modes will need different data. */
	};
};

int rockchip_vpu_hw_probe(struct rockchip_vpu_dev *vpu);
void rockchip_vpu_hw_remove(struct rockchip_vpu_dev *vpu);

int rockchip_vpu_init(struct rockchip_vpu_ctx *ctx);
void rockchip_vpu_deinit(struct rockchip_vpu_ctx *ctx);

void rockchip_vpu_run(struct rockchip_vpu_ctx *ctx);

void rockchip_vpu_power_on(struct rockchip_vpu_dev *vpu);

/* Ops for rk3288 vpu */
int rk3288_vepu_irq(int irq, struct rockchip_vpu_dev *vpu);
int rk3288_vdpu_irq(int irq, struct rockchip_vpu_dev *vpu);
void rk3288_vpu_enc_reset(struct rockchip_vpu_ctx *ctx);
void rk3288_vpu_dec_reset(struct rockchip_vpu_ctx *ctx);

/* Run ops for rk3288 H264 decoder */
int rk3288_vpu_h264d_init(struct rockchip_vpu_ctx *ctx);
void rk3288_vpu_h264d_exit(struct rockchip_vpu_ctx *ctx);
void rk3288_vpu_h264d_run(struct rockchip_vpu_ctx *ctx);

/* Run ops for rk3288 VP8 decoder */
int rk3288_vpu_vp8d_init(struct rockchip_vpu_ctx *ctx);
void rk3288_vpu_vp8d_exit(struct rockchip_vpu_ctx *ctx);
void rk3288_vpu_vp8d_run(struct rockchip_vpu_ctx *ctx);

/* Run ops for rk3288 VP8 encoder */
int rk3288_vpu_vp8e_init(struct rockchip_vpu_ctx *ctx);
void rk3288_vpu_vp8e_exit(struct rockchip_vpu_ctx *ctx);
void rk3288_vpu_vp8e_run(struct rockchip_vpu_ctx *ctx);
void rk3288_vpu_vp8e_done(struct rockchip_vpu_ctx *ctx,
			  enum vb2_buffer_state result);

/* Ops for rk3228 vpu */
int rk3228_vepu_irq(int irq, struct rockchip_vpu_dev *vpu);
int rk3228_vdpu_irq(int irq, struct rockchip_vpu_dev *vpu);
void rk3228_vpu_enc_reset(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_dec_reset(struct rockchip_vpu_ctx *ctx);

/* Run ops for rk3228 H264 encoder */
int rk3228_vpu_h264e_init(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_h264e_exit(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_h264e_run(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_h264e_done(struct rockchip_vpu_ctx *ctx,
			   enum vb2_buffer_state result);

/* Run ops for rk3228 VP8 encoder */
int rk3228_vpu_vp8e_init(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_vp8e_exit(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_vp8e_run(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_vp8e_done(struct rockchip_vpu_ctx *ctx,
			   enum vb2_buffer_state result);

/* Run ops for rk3228 VP8 decoder */
int rk3228_vpu_vp8d_init(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_vp8d_exit(struct rockchip_vpu_ctx *ctx);
void rk3228_vpu_vp8d_run(struct rockchip_vpu_ctx *ctx);

/* For rockchip VP8 encoder */
const struct rockchip_vp8e_reg_params *rockchip_vpu_vp8e_get_dummy_params(void);

void rockchip_vpu_vp8e_assemble_bitstream(struct rockchip_vpu_ctx *ctx,
					struct rockchip_vpu_buf *dst_buf);
void rockchip_vpu_h264e_assemble_bitstream(struct rockchip_vpu_ctx *ctx,
					struct rockchip_vpu_buf *dst_buf);

/* Ops for rkvdec */
int rkvdec_irq(int irq, struct rockchip_vpu_dev *vpu);
void rkvdec_reset(struct rockchip_vpu_ctx *ctx);

/* Run ops for rkvdec H264 decoder */
int rkvdec_h264d_init(struct rockchip_vpu_ctx *ctx);
void rkvdec_h264d_exit(struct rockchip_vpu_ctx *ctx);
void rkvdec_h264d_run(struct rockchip_vpu_ctx *ctx);

/* Run ops for rkvdec VP9 decoder */
int rkvdec_vp9d_init(struct rockchip_vpu_ctx *ctx);
void rkvdec_vp9d_exit(struct rockchip_vpu_ctx *ctx);
void rkvdec_vp9d_run(struct rockchip_vpu_ctx *ctx);

#endif /* RK3288_VPU_HW_H_ */
