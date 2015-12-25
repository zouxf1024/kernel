/*
 * Rockchip VPU codec driver
 *
 * Copyright (C) 2014 Rockchip Electronics Co., Ltd.
 *	Alpha Lin <Alpha.Lin@rock-chips.com>
 *	Jeffy Chen <jeffy.chen@rock-chips.com>
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

#include <linux/types.h>
#include <linux/sort.h>

#include "rk3228_vpu_regs.h"
#include "rockchip_vpu_hw.h"


int rk3228_vpu_h264e_init(struct rockchip_vpu_ctx *ctx)
{
	return 0;
}

void rk3228_vpu_h264e_exit(struct rockchip_vpu_ctx *ctx)
{
}

void rk3228_vpu_h264e_run(struct rockchip_vpu_ctx *ctx)
{
}
