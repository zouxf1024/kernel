/*
 * Crypto acceleration support for Rockchip RK3288
 *
 * Copyright (c) 2015, Fuzhou Rockchip Electronics Co., Ltd
 *
 * Author: Zain Wang <zain.wang@rock-chips.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * Some ideas are from marvell/cesa.c and s5p-sss.c driver.
 */
#include "rk3288_crypto.h"

/*
 * IC can not process zero message hash,
 * so we put the fixed hash out when met zero message.
 */

static int zero_message_process(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	int rk_digest_size = crypto_ahash_digestsize(tfm);

	switch (rk_digest_size) {
	case SHA1_DIGEST_SIZE:
		memcpy(req->result, sha1_zero_message_hash, rk_digest_size);
		break;
	case SHA256_DIGEST_SIZE:
		memcpy(req->result, sha256_zero_message_hash, rk_digest_size);
		break;
	case MD5_DIGEST_SIZE:
		memcpy(req->result, md5_zero_message_hash, rk_digest_size);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void rk_ahash_crypto_complete(struct rk_crypto_info *dev, int err)
{
	if (dev->ahash_req->base.complete)
		dev->ahash_req->base.complete(&dev->ahash_req->base, err);
}

static void rk_ahash_hw_init(struct rk_crypto_info *dev)
{
	int reg_status = 0;

	reg_status = CRYPTO_READ(dev, RK_CRYPTO_CTRL) |
		     RK_CRYPTO_HASH_FLUSH | _SBF(0xffff, 16);
	CRYPTO_WRITE(dev, RK_CRYPTO_CTRL, reg_status);

	reg_status = CRYPTO_READ(dev, RK_CRYPTO_CTRL);
	reg_status &= (~RK_CRYPTO_HASH_FLUSH);
	reg_status |= _SBF(0xffff, 16);
	CRYPTO_WRITE(dev, RK_CRYPTO_CTRL, reg_status);

	memset_io(dev->reg + RK_CRYPTO_HASH_DOUT_0, 0, 32);
}

static void rk_ahash_reg_init(struct rk_crypto_info *dev)
{
	rk_ahash_hw_init(dev);

	CRYPTO_WRITE(dev, RK_CRYPTO_INTENA, RK_CRYPTO_HRDMA_ERR_ENA |
					    RK_CRYPTO_HRDMA_DONE_ENA);

	CRYPTO_WRITE(dev, RK_CRYPTO_INTSTS, RK_CRYPTO_HRDMA_ERR_INT |
					    RK_CRYPTO_HRDMA_DONE_INT);

	CRYPTO_WRITE(dev, RK_CRYPTO_HASH_CTRL, dev->mode |
					       RK_CRYPTO_HASH_SWAP_DO);

	CRYPTO_WRITE(dev, RK_CRYPTO_CONF, RK_CRYPTO_BYTESWAP_HRFIFO |
					  RK_CRYPTO_BYTESWAP_BRFIFO |
					  RK_CRYPTO_BYTESWAP_BTFIFO);
}

static int rk_ahash_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct rk_ahash_ctx *tctx = crypto_tfm_ctx(req->base.tfm);
	struct rk_crypto_info *dev = NULL;

	dev = tctx->dev;
	dev->left_bytes = 0;
	dev->aligned = 0;
	dev->ahash_req = req;
	dev->mode = 0;
	dev->align_size = 4;
	dev->sg_dst = NULL;

	tctx->first_op = 1;

	switch (crypto_ahash_digestsize(tfm)) {
	case SHA1_DIGEST_SIZE:
		dev->mode = RK_CRYPTO_HASH_SHA1;
		break;
	case SHA256_DIGEST_SIZE:
		dev->mode = RK_CRYPTO_HASH_SHA256;
		break;
	case MD5_DIGEST_SIZE:
		dev->mode = RK_CRYPTO_HASH_MD5;
		break;
	default:
		return -EINVAL;
	}

	rk_ahash_reg_init(dev);
	return 0;
}

static int rk_ahash_update(struct ahash_request *req)
{
	struct rk_ahash_ctx *tctx = crypto_tfm_ctx(req->base.tfm);
	struct rk_crypto_info *dev = tctx->dev;
	unsigned long flags;
	int ret;

	dev->total = req->nbytes;
	dev->left_bytes = req->nbytes;
	dev->sg_src = req->src;
	dev->first = req->src;
	dev->nents = sg_nents(req->src);

	/* IC can calculate 0 message hash, so it should finish update here */
	if (!dev->total)
		return 0;

	if (tctx->first_op) {
		tctx->first_op = 0;
		CRYPTO_WRITE(dev, RK_CRYPTO_HASH_MSG_LEN, dev->total);
	} else {
		/*
		 * IC must know the length of total data at first,
		 * multiple updatings cannot support this variable.
		 */
		dev_warn(dev->dev, "Cannot carry multiple updatings!\n");
		return 0;
	}
	spin_lock_irqsave(&dev->lock, flags);
	ret = crypto_enqueue_request(&dev->queue, &req->base);
	spin_unlock_irqrestore(&dev->lock, flags);

	tasklet_schedule(&dev->crypto_tasklet);

	return ret;
}

static int rk_ahash_final(struct ahash_request *req)
{
	struct rk_ahash_ctx *tctx = crypto_tfm_ctx(req->base.tfm);
	struct rk_crypto_info *dev = tctx->dev;
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);

	/* If met 0 message, put the fixed hash out.*/
	if (!dev->total)
		return zero_message_process(req);

	/*
	 * IC should process the result again after last dma interrupt.
	 * And the last processing is very quick so than it may entry
	 * interrupt before finishing last interrupt.
	 * So I don't use interrupt finished hash.
	 */
	while (!CRYPTO_READ(dev, RK_CRYPTO_HASH_STS))
		usleep_range(50, 100);

	memcpy_fromio(req->result, dev->reg + RK_CRYPTO_HASH_DOUT_0,
		      crypto_ahash_digestsize(tfm));
	return 0;
}

static int rk_ahash_finup(struct ahash_request *req)
{
	struct rk_ahash_ctx *tctx = crypto_tfm_ctx(req->base.tfm);
	int err;

	/*
	 * finup should process one updating and final.
	 * and we have to wait for updating in finup so that we can
	 * fetching result by calling rk_ahash_final in finup.
	 */
	tctx->flag_finup = 1;
	err = rk_ahash_update(req);
	if (err == -EINPROGRESS || err == -EBUSY)
		while (tctx->flag_finup)
			/*
			 * waiting time is relative with the last date len,
			 * so here cannot get a fixed time.
			 * 50-100 makes system not call here frequently wasting
			 * efficiency, and make it response quickly when dma
			 * complete.
			 */
			usleep_range(50, 100);

	return rk_ahash_final(req);
}

static int rk_ahash_digest(struct ahash_request *req)
{
	return rk_ahash_init(req) ? -EINVAL : rk_ahash_finup(req);
}

static void crypto_ahash_dma_start(struct rk_crypto_info *dev)
{
	CRYPTO_WRITE(dev, RK_CRYPTO_HRDMAS, dev->addr_in);
	CRYPTO_WRITE(dev, RK_CRYPTO_HRDMAL, (dev->count + 3) / 4);
	CRYPTO_WRITE(dev, RK_CRYPTO_CTRL, RK_CRYPTO_HASH_START |
					  (RK_CRYPTO_HASH_START << 16));
}

static int rk_ahash_set_data_start(struct rk_crypto_info *dev)
{
	int err;

	err = dev->load_data(dev, dev->sg_src, NULL);
	if (!err)
		crypto_ahash_dma_start(dev);
	return err;
}

static int rk_ahash_start(struct rk_crypto_info *dev)
{
	return rk_ahash_set_data_start(dev);
}

/*
 * return:
 * true: some err was occurred
 * fault: no err, please continue
 */
static int rk_ahash_crypto_rx(struct rk_crypto_info *dev)
{
	int err = 0;
	struct rk_ahash_ctx *tctx = crypto_tfm_ctx(dev->ahash_req->base.tfm);

	dev->unload_data(dev);
	if (dev->left_bytes) {
		if (dev->aligned) {
			if (sg_is_last(dev->sg_src)) {
				dev_warn(dev->dev, "[%s:%d], Lack of data\n",
					 __func__, __LINE__);
				err = -ENOMEM;
				goto out_rx;
			}
			dev->sg_src = sg_next(dev->sg_src);
		}
		err = rk_ahash_set_data_start(dev);
	} else {
		tctx->flag_finup = 0;
		dev->complete(dev, 0);
	}

out_rx:
	return err;
}

static int rk_cra_hash_init(struct crypto_tfm *tfm)
{
	struct rk_ahash_ctx *tctx = crypto_tfm_ctx(tfm);
	struct rk_crypto_tmp *algt;
	struct ahash_alg *alg = __crypto_ahash_alg(tfm->__crt_alg);

	algt = container_of(alg, struct rk_crypto_tmp, alg.hash);

	tctx->dev = algt->dev;
	tctx->dev->addr_vir = (void *)__get_free_page(GFP_KERNEL);
	if (!tctx->dev->addr_vir) {
		dev_err(tctx->dev->dev, "failed to kmalloc for addr_vir\n");
		return -ENOMEM;
	}
	tctx->dev->start = rk_ahash_start;
	tctx->dev->update = rk_ahash_crypto_rx;
	tctx->dev->complete = rk_ahash_crypto_complete;
	return tctx->dev->enable_clk(tctx->dev);
}

static void rk_cra_hash_exit(struct crypto_tfm *tfm)
{
	struct rk_ahash_ctx *tctx = crypto_tfm_ctx(tfm);

	free_page((unsigned long)tctx->dev->addr_vir);
	return tctx->dev->disable_clk(tctx->dev);
}

struct rk_crypto_tmp rk_ahash_sha1 = {
	.type = ALG_TYPE_HASH,
	.alg.hash = {
		.init = rk_ahash_init,
		.update = rk_ahash_update,
		.final = rk_ahash_final,
		.finup = rk_ahash_finup,
		.digest = rk_ahash_digest,
		.halg = {
			 .digestsize = SHA1_DIGEST_SIZE,
			 .statesize = sizeof(struct sha1_state),
			 .base = {
				  .cra_name = "sha1",
				  .cra_driver_name = "rk-sha1",
				  .cra_priority = 300,
				  .cra_flags = CRYPTO_ALG_ASYNC,
				  .cra_blocksize = SHA1_BLOCK_SIZE,
				  .cra_ctxsize = sizeof(struct rk_ahash_ctx),
				  .cra_alignmask = 3,
				  .cra_init = rk_cra_hash_init,
				  .cra_exit = rk_cra_hash_exit,
				  .cra_module = THIS_MODULE,
				  }
			 }
	}
};

struct rk_crypto_tmp rk_ahash_sha256 = {
	.type = ALG_TYPE_HASH,
	.alg.hash = {
	.init = rk_ahash_init,
	.update = rk_ahash_update,
	.final = rk_ahash_final,
	.finup = rk_ahash_finup,
	.digest = rk_ahash_digest,
		.halg = {
			 .digestsize = SHA256_DIGEST_SIZE,
			 .statesize = sizeof(struct sha256_state),
			 .base = {
				  .cra_name = "sha256",
				  .cra_driver_name = "rk-sha256",
				  .cra_priority = 300,
				  .cra_flags = CRYPTO_ALG_ASYNC,
				  .cra_blocksize = SHA256_BLOCK_SIZE,
				  .cra_ctxsize = sizeof(struct rk_ahash_ctx),
				  .cra_alignmask = 0,
				  .cra_init = rk_cra_hash_init,
				  .cra_exit = rk_cra_hash_exit,
				  .cra_module = THIS_MODULE,
				  }
			 }
	}
};

struct rk_crypto_tmp rk_ahash_md5 = {
	.type = ALG_TYPE_HASH,
	.alg.hash = {
		.init = rk_ahash_init,
		.update = rk_ahash_update,
		.final = rk_ahash_final,
		.finup = rk_ahash_finup,
		.digest = rk_ahash_digest,
		.halg = {
			 .digestsize = MD5_DIGEST_SIZE,
			 .statesize = sizeof(struct md5_state),
			 .base = {
				  .cra_name = "md5",
				  .cra_driver_name = "rk-md5",
				  .cra_priority = 300,
				  .cra_flags = CRYPTO_ALG_ASYNC,
				  .cra_blocksize = SHA1_BLOCK_SIZE,
				  .cra_ctxsize = sizeof(struct rk_ahash_ctx),
				  .cra_alignmask = 0,
				  .cra_init = rk_cra_hash_init,
				  .cra_exit = rk_cra_hash_exit,
				  .cra_module = THIS_MODULE,
				  }
			}
	}
};
