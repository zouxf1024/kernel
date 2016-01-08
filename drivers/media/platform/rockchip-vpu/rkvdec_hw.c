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

#include "rockchip_vpu_common.h"

#include "rkvdec_regs.h"

/*
 * Interrupt handlers.
 */

int rkvdec_irq(int irq, struct rockchip_vpu_dev *vpu)
{
	/*
	u32 status = vepu_read(vpu, VEPU_REG_INTERRUPT);

	vpu_debug(5, "enc status %x\n", status);

	vepu_write(vpu, 0, VEPU_REG_INTERRUPT);

	if (status & VEPU_REG_INTERRUPT_BIT) {
		if (!(status & VEPU_REG_INTERRUPT_FRAME_READY)) {
			panic("invalid interrupt\n");
		}
		vepu_write(vpu, 0, VEPU_REG_AXI_CTRL);
		return 0;
	}
	*/

	return -1;
}

/*
 * Initialization/clean-up.
 */

void rkvdec_reset(struct rockchip_vpu_ctx *ctx)
{
	/*
	struct rockchip_vpu_dev *vpu = ctx->dev;

	vepu_write(vpu, VEPU_REG_INTERRUPT_DIS_BIT, VEPU_REG_INTERRUPT);
	vepu_write(vpu, 0, VEPU_REG_ENCODE_START);
	vepu_write(vpu, 0, VEPU_REG_AXI_CTRL);
	*/
}
