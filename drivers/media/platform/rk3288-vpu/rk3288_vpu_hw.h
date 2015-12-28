/*
 * Rockchip RK3288 VPU codec driver
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

#ifndef RK3288_VPU_HW_H_
#define RK3288_VPU_HW_H_

#include <media/videobuf2-core.h>

#define RK3288_HEADER_SIZE		1280
#define RK3288_HW_PARAMS_SIZE		5487
#define RK3288_RET_PARAMS_SIZE		488

struct rk3288_vpu_dev;
struct rk3288_vpu_ctx;
struct rk3288_vpu_buf;

struct rk3288_vpu_h264d_priv_tbl;

/**
 * enum rk3288_vpu_enc_fmt - source format ID for hardware registers.
 */
enum rk3288_vpu_enc_fmt {
	RK3288_VPU_ENC_FMT_YUV420P = 0,
	RK3288_VPU_ENC_FMT_YUV420SP = 1,
	RK3288_VPU_ENC_FMT_YUYV422 = 2,
	RK3288_VPU_ENC_FMT_UYVY422 = 3,
};

/**
 * struct rk3288_vp8e_reg_params - low level encoding parameters
 * TODO: Create abstract structures for more generic controls or just
 *       remove unused fields.
 */
struct rk3288_vp8e_reg_params {
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

struct r3288_h264e_reg_params {
	u32 mbsInCol;
	u32 mbsInRow;
	u32 qp;
	u32 qpMin;
	u32 qpMax;
	u32 constrainedIntraPrediction;
	u32 frameCodingType;
	u32 codingType;
	u32 pixelsOnRow;
	u32 xFill;
	u32 yFill;
	u32 ppsId;
	u32 idrPicId;
	u32 frameNum;
	u32 picInitQp;
	s32 sliceAlphaOffset;
	s32 sliceBetaOffset;
	u32 filterDisable;
	u32 transform8x8Mode;
	u32 enableCabac;
	u32 cabacInitIdc;
	s32 chromaQpIndexOffset;
	u32 sliceSizeMbRows;
	u32 inputImageFormat;
	u32 inputImageRotation;
	u32 outputStrmBase;
	u32 outputStrmSize;
	u32 firstFreeBit;
	u32 strmStartMSB;
	u32 strmStartLSB;
	u32 rlcBase;
	u32 rlcLimitSpace;
	union {
		u32 nal;
		u32 vp;
		u32 gob;
	} sizeTblBase;
	u32 cpDistanceMbs;
	u32 cpTarget[10];
	s32 targetError[7];
	s32 deltaQp[7];
	u32 rlcCount;
	u32 qpSum;
	u32 h264StrmMode;   /* 0 - byte stream, 1 - NAL units */
	u32 sizeTblPresent;
	u32 inputLumaBaseOffset;
	u32 inputChromaBaseOffset;
	u32 h264Inter4x4Disabled;
	u32 disableQuarterPixelMv;
	u32 vsNextLumaBase;
	u32 vsMode;
	u32 vpSize;
	u32 vpMbBits;
	u32 intraDcVlcThr;
	u32 asicCfgReg;
	u32 intra16Favor;
	u32 interFavor;
	u32 skipPenalty;
	s32 madQpDelta;
	u32 madThreshold;
	u32 madCount;
	u32 riceEnable;
	u32 riceReadBase;
	u32 riceWriteBase;
	u32 cabacCtxBase;
	u32 colorConversionCoeffA;
	u32 colorConversionCoeffB;
	u32 colorConversionCoeffC;
	u32 colorConversionCoeffE;
	u32 colorConversionCoeffF;
	u32 rMaskMsb;
	u32 gMaskMsb;
	u32 bMaskMsb;

	u8 dmvPenalty[128];
	u8 dmvQpelPenalty[128];
	u32 splitMvMode;
	u32 diffMvPenalty[3];
	u32 splitPenalty[4];
	u32 zeroMvFavorDiv2;
};

/**
 * struct rk3288_vpu_aux_buf - auxiliary DMA buffer for hardware data
 * @cpu:	CPU pointer to the buffer.
 * @dma:	DMA address of the buffer.
 * @size:	Size of the buffer.
 */
struct rk3288_vpu_aux_buf {
	void *cpu;
	dma_addr_t dma;
	size_t size;
};

/**
 * struct rk3288_vpu_vp8e_hw_ctx - Context private data specific to codec mode.
 * @ctrl_buf:		VP8 control buffer.
 * @ext_buf:		VP8 ext data buffer.
 * @mv_buf:		VP8 motion vector buffer.
 * @ref_rec_ptr:	Bit flag for swapping ref and rec buffers every frame.
 */
struct rk3288_vpu_vp8e_hw_ctx {
	struct rk3288_vpu_aux_buf ctrl_buf;
	struct rk3288_vpu_aux_buf ext_buf;
	struct rk3288_vpu_aux_buf mv_buf;
	u8 ref_rec_ptr:1;
};

/**
 * struct rk3288_vpu_vp8d_hw_ctx - Context private data of VP8 decoder.
 * @segment_map:	Segment map buffer.
 * @prob_tbl:		Probability table buffer.
 */
struct rk3288_vpu_vp8d_hw_ctx {
	struct rk3288_vpu_aux_buf segment_map;
	struct rk3288_vpu_aux_buf prob_tbl;
};

/**
 * struct rk3288_vpu_h264d_hw_ctx - Per context data specific to H264 decoding.
 * @priv_tbl:		Private auxiliary buffer for hardware.
 */
struct rk3288_vpu_h264d_hw_ctx {
	struct rk3288_vpu_aux_buf priv_tbl;
};

struct rk3288_vpu_h264e_hw_ctx {
	struct rk3288_vpu_aux_buf regs;
	struct rk3288_vpu_aux_buf cabac_tbl;
	struct rk3288_vpu_aux_buf ext_buf;
	u8 ref_rec_ptr:1;
};

/**
 * struct rk3288_vpu_hw_ctx - Context private data of hardware code.
 * @codec_ops:		Set of operations associated with current codec mode.
 */
struct rk3288_vpu_hw_ctx {
	const struct rk3288_vpu_codec_ops *codec_ops;

	/* Specific for particular codec modes. */
	union {
		struct rk3288_vpu_vp8e_hw_ctx vp8e;
		struct rk3288_vpu_vp8d_hw_ctx vp8d;
		struct rk3288_vpu_h264d_hw_ctx h264d;
		struct rk3288_vpu_h264e_hw_ctx h264e;
		/* Other modes will need different data. */
	};
};

int rk3288_vpu_hw_probe(struct rk3288_vpu_dev *vpu);
void rk3288_vpu_hw_remove(struct rk3288_vpu_dev *vpu);

int rk3288_vpu_init(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_deinit(struct rk3288_vpu_ctx *ctx);

void rk3288_vpu_run(struct rk3288_vpu_ctx *ctx);

/* Run ops for H264 decoder */
int rk3288_vpu_h264d_init(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_h264d_exit(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_h264d_run(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_power_on(struct rk3288_vpu_dev *vpu);

/* Run ops for H264 encoder */
int rk3288_vpu_h264e_init(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_h264e_exit(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_h264e_run(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_h264e_done(struct rk3288_vpu_ctx *ctx,
			  enum vb2_buffer_state result);

/* Run ops for VP8 decoder */
int rk3288_vpu_vp8d_init(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_vp8d_exit(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_vp8d_run(struct rk3288_vpu_ctx *ctx);

/* Run ops for VP8 encoder */
int rk3288_vpu_vp8e_init(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_vp8e_exit(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_vp8e_run(struct rk3288_vpu_ctx *ctx);
void rk3288_vpu_vp8e_done(struct rk3288_vpu_ctx *ctx,
			 enum vb2_buffer_state result);
const struct rk3288_vp8e_reg_params *rk3288_vpu_vp8e_get_dummy_params(void);

void rk3288_vpu_vp8e_assemble_bitstream(struct rk3288_vpu_ctx *ctx,
					struct rk3288_vpu_buf *dst_buf);

#endif /* RK3288_VPU_HW_H_ */
