/*
 * Support for Cryptographic Engine in FPGA card using PCIe interface
 * that can be found on the following platform: Armada. 
 *
 * Author: Duy H.Ho <duyhungho.work@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/scatterlist.h>
#include <linux/genalloc.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/genalloc.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/mbus.h>
#include <linux/workqueue.h>
/* get system time */
#include <linux/jiffies.h> 
#include <linux/timekeeping.h>
#include <crypto/aes.h>
#include <crypto/gcm.h>
#include <crypto/des.h>
#include <crypto/aead.h>
#include <crypto/internal/aead.h>
#include <crypto/sha.h>
#include "bkcrypto.h"
#include "xdma_region.h"
#include "xdma_crypto.h"
#include "crypto_testcases.h"
#include "common.h"
#include "xdma_crypto.h"

// #define BKCRYPTO_DEBUG

/* Limit of the crypto queue before reaching the backlog */
#define BKCRYPTO_DEFAULT_MAX_QLEN 128


// global variable for device
struct bkcrypto_dev *mydevice_glo;
struct inbound region_in;
struct outbound region_out;
spinlock_t submit_lock;

int bkcrypto_check_errors(struct bkcrypto_dev *mydevice, struct bkcrypto_context *ctx);
void set_xfer_bkcryptocontext(struct crypto_async_request *base, struct xfer_req *req_xfer);
static int handle_crypto_xfer_callback(struct xfer_req *data, int res);


int bkcrypto_compare_icv(u32 *tag_out, u32 *tag_in)
{
	int ret;
	int success = 0;// 4 : success ; <4: Fail
	int i;
	for (i = 0; i < 4; i++)
	{
		if(tag_out[i] == tag_in[i])
			success ++;
#ifdef BKCRYPTO_DEBUG
		pr_err("Chunk:%d tag_in: %8.0x tag_out:%8.0x\n",i, tag_in[i],tag_out[i]);
		pr_err("tag in:%x - Chunk:%d",tag_in[i],i);
		pr_err("success value in loop:%d\n", success);
#endif
	}
	if (success < 4)
	{
		ret = -EBADMSG;//authentication fail
		goto back;
	}
	ret = 0;//successful
back:
	return ret;
}

void print_sg_content(struct scatterlist *sg)
{
    unsigned int length;
    unsigned int i;
    char *buf;

	if (!sg){
		pr_err("Print sg failed, sg NULL \n");
		return;
	}

    length = sg->length;
    buf = sg_virt (sg);
	   pr_err("print sg buffer with length %d\n", length);

    for (i = 0; i < length; i += 16)
    {
        pr_err("Print virual sg: address = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i ,
            *((u32 *)(&buf[i + 12])), *((u32 *)(&buf[i + 8])), 
            *((u32 *)(&buf[i + 4])), *((u32 *)(&buf[i])));
    }
}

static struct bkcrypto_alg_template *bkcrypto_algs[] = {
	&bkcrypto_alg_gcm_aes,
	&bkcrypto_alg_rfc4106_gcm_aes
};

void set_xfer_bkcryptocontext(struct crypto_async_request *base, struct xfer_req *req_xfer)
{
	struct aead_request *req; 
	struct bkcrypto_cipher_op ctx ;
	req= aead_request_cast(base);
	ctx = *(struct bkcrypto_cipher_op *)crypto_tfm_ctx(req->base.tfm);
	req_xfer->ctx.ctx_op = ctx;
	req_xfer->base = base;	
}
static inline void bkcrypto_handle_result(struct crypto_async_request *base, int ret)
{
	struct bkcrypto_req_operation *opr_ctx;
	bool should_complete=true;
	opr_ctx = crypto_tfm_ctx(base->tfm);
	if (should_complete) 
	{
			local_bh_disable();
			base->complete(base, ret);
			local_bh_enable();
	}
}

struct crypto_async_request *bkcrypto_dequeue_req_locked(struct bkcrypto_dev *mydevice,
			   struct crypto_async_request **backlog)
{
	struct crypto_async_request *base;//asyn_req
	*backlog = crypto_get_backlog(&mydevice->queue);
	base = crypto_dequeue_request(&mydevice->queue);
	if (!base)
		return NULL;
	return base;
}

int bkcrypto_dequeue_req(struct bkcrypto_dev *mydevice)
{
	struct crypto_async_request *base = NULL;
	struct xfer_req *req_xfer;
	struct aead_request *aead_req ;
	u8 *buff;
	int res;
	u32 i;
	__le32 *key;
	__le32 key_tmp[8];
	size_t len;
	int src_nents;
	int case_nents=0;
	
	base = mydevice->req; // for no crypto-queue

	if (!base){
		pr_err("no current req\n");
		return -EBUSY;
	}
	/* Allocate xfer_req */
	req_xfer = alloc_xfer_req();

	if (!req_xfer)
	{
		return -ENOMEM;
	}

	/* Step 1: Initaiate fields of xfer_req*/
	aead_req = aead_request_cast(base);
	src_nents = sg_nents(aead_req->src);
	set_callback(req_xfer, &handle_crypto_xfer_callback);
	set_xfer_bkcryptocontext(base, req_xfer);

	len = (size_t)(aead_req->cryptlen + aead_req->assoclen + 16);
	set_sg_in(req_xfer, aead_req->src);	
	set_sg_out(req_xfer, aead_req->src);
	
	if(src_nents == 1)
	{
		buff = sg_virt(aead_req->src);
		case_nents = 1;
		// sg_init_one(&req_xfer->sg, buff , aead_req->src->length);

	}
	else if (src_nents > 1)
	{
		buff = kzalloc(len, GFP_KERNEL);
		len = sg_pcopy_to_buffer(aead_req->src,src_nents,buff,len,0);
		case_nents = 2;
	}
	else if (src_nents < 1)
	{
		return -EINVAL;
	}
	/* Step 2: Setting transfer description*/
	// INFO
	for (i = 0; i <=2; i++)
	{
		region_in.crypto_dsc.info.free_space[i]=0x00000000;//16 Byte MSB 0
	}
	region_in.crypto_dsc.info.free_space_ = 0; // 1bit 0
	region_in.crypto_dsc.info.direction = req_xfer->ctx.ctx_op.dir; //1 bit direction
	region_in.crypto_dsc.info.length = req_xfer->ctx.ctx_op.cryptlen; //11 bit data len (maximum of data> 1500 byte)
	region_in.crypto_dsc.info.aadsize = req_xfer->ctx.ctx_op.assoclen - 8;// substract iv len
	switch(req_xfer->ctx.ctx_op.keylen)
	{
		case 16: 
			region_in.crypto_dsc.info.keysize = 0;//key type for 128 bit key
			break;
		case 24: 
			region_in.crypto_dsc.info.keysize = 1;//key type for 192 bit key
			break;
		case 32: 
			region_in.crypto_dsc.info.keysize = 2;//key type for 256 bit key
			break;
	}

	// ICV-AUTHENTAG
	memcpy(region_in.crypto_dsc.icv , buff + req_xfer->ctx.ctx_op.cryptlen + req_xfer->ctx.ctx_op.assoclen , ICV_SIZE);
	
	// KEY
	switch (req_xfer->ctx.ctx_op.keylen)
	{
		case 16:
			for (i = 0; i < (KEYMEM - 16)/4; i++)
			{
				key_tmp[i]=0x00000000;
			}
			for (i = (KEYMEM - 16)/4; i < KEYMEM/4;i++)
			{
				key_tmp[i]=req_xfer->ctx.ctx_op.key[KEYMEM/4-1-i];

			}
			break;
		case 24:
			for (i = 0; i < (KEYMEM - 24)/4; i++)
			{
				key_tmp[i]=0x00000000;
			}
			for (i = (KEYMEM - 24)/4; i < KEYMEM/4;i++)
			{
				key_tmp[i]=req_xfer->ctx.ctx_op.key[KEYMEM/4-1-i];
			}
			break;
		case 32:
			for (i = 0; i < KEYMEM/4;i++)
			{
				key_tmp[i]=req_xfer->ctx.ctx_op.key[KEYMEM/4-1-i];
			}
			break;
	}
	key = key_tmp;

	memcpy(region_in.crypto_dsc.key,key,KEY_SIZE);
	region_in.crypto_dsc.iv.nonce = req_xfer->ctx.ctx_op.nonce;
	region_in.crypto_dsc.iv.iv[1] = cpu_to_be32(*(u32 *)(buff + region_in.crypto_dsc.info.aadsize));// Little edian with ESP4
	region_in.crypto_dsc.iv.iv[0] = cpu_to_be32(*(u32 *)(buff + region_in.crypto_dsc.info.aadsize + 4));// Little edian with ESP4
	region_in.crypto_dsc.iv.tail = 0x00000001;
	//AAD
	for (i = 0; i < region_in.crypto_dsc.info.aadsize /4 ; i++)
	{
		region_in.crypto_dsc.aad[(region_in.crypto_dsc.info.aadsize /4 - 1) - i] = cpu_to_be32(*(u32*)(buff + i*4 ));
		// 8/12 LSB bytes is AAD
	}
	for (i = region_in.crypto_dsc.info.aadsize / 4 ; i < 4 ; i ++)
	{
		region_in.crypto_dsc.aad[i] = 0x00000000;// 4 or 8 Bytes MSB is 0x
	}

	// Setting transfer description
    req_xfer->crypto_dsc.info = region_in.crypto_dsc.info;
    memcpy(req_xfer->crypto_dsc.icv, region_in.crypto_dsc.icv, ICV_SIZE); 
    memcpy(req_xfer->crypto_dsc.key, region_in.crypto_dsc.key, KEY_SIZE); 
    req_xfer->crypto_dsc.iv = region_in.crypto_dsc.iv;
    memcpy(req_xfer->crypto_dsc.aad, region_in.crypto_dsc.aad, AAD_SIZE); 
	

	if (region_in.crypto_dsc.info.length % 16 == 0)
		set_tag(req_xfer, 16, 0x20 + 0x10 * (region_in.crypto_dsc.info.length/16 ));
	else 
		set_tag(req_xfer, 16, 0x20 + 0x10 * (region_in.crypto_dsc.info.length/16 + 1));
#ifdef BKCRYPTO_DEBUG	
	pr_err("bkcrypto.c:dequeue: sg_nents:%d -sg_length:%d- page_link:\
		%x- offset:%lx-dma_address:%llx\n",sg_nents(req_xfer->sg_in),
		req_xfer->sg_in->length,req_xfer->sg_in->page_link,
		req_xfer->sg_in->offset,req_xfer->sg_in->dma_address);
#endif
    /* Step 3: Submit to device-specific layer of driver  */
	res = xdma_xfer_submit_queue(req_xfer);
	if (case_nents == 2)
		kfree(buff);
	if (res != -EINPROGRESS)
        pr_err("Unusual result\n");
	return res;
}

static void bkcrypto_dequeue_work(struct work_struct *work)
{
	struct bkcrypto_work_data *data =
			container_of(work, struct bkcrypto_work_data, work);
	bkcrypto_dequeue_req(data->mydevice);
}

/* Handle xfer callback request*/
static int handle_crypto_xfer_callback(struct xfer_req *data, int res)
{    
	char *buf = NULL;
	char *buf_aad = NULL;
	char *buf_iv = NULL;
	u32 *buf_tagout = NULL;
	char tmp;// tmp for exchange
    int i = 0;
	size_t len;
	// Step 4: Get data in callback function
	struct scatterlist *sg = data->sg_out;
	struct crypto_async_request *base = (struct crypto_async_request *) data->base;
	struct aead_request *aead_req ;
	int ret=0;
	int src_nents;
	int case_nents=0;

	aead_req = aead_request_cast(base);
	src_nents = sg_nents(aead_req->src);
	len = (size_t)(aead_req->cryptlen + aead_req->assoclen + 16);

#ifdef BKCRYPTO_DEBUG
	pr_err("Complete with res = %d ! This is callback function! \n", res);
	pr_err("bkcrypto.c (callback): Address of req:%p - assoclen+Cryptlen =  %d %d \n",aead_req,  
            aead_req->assoclen, aead_req->cryptlen);
	pr_err("bkcrypto.c:callback: sg_nents:%d -sg_length:%d- \
		page_link:%x- offset:%lx-dma_address:%llx\n",
		sg_nents(aead_req->dst),aead_req->dst->length,
		aead_req->dst->page_link,aead_req->dst->offset,
		aead_req->dst->dma_address);
	if(data->ctx.ctx_op.dir == 0)
	{
		pr_info("----------------------------this is decrypt\n");
	}
	else
	{
		pr_info("----------------------------this is encrypt\n");
	}
#endif
	
	if (!base){ 
		pr_err("Module bkcrypto: CAN NOT HANDLE A null POINTER\n");
		return res;
	}
	
    if (res == -1)
	{
		ret = res;
		goto err_busy;
	}

	if(src_nents == 1)
	{
		buf = sg_virt (sg);
		case_nents = 1;
	}
	else if(src_nents > 1)
	{
		buf =kzalloc(len, GFP_KERNEL);
		len = sg_pcopy_to_buffer(aead_req->src,src_nents,buf,len,0);
		case_nents = 2;
	}
	else if(src_nents < 1)
		pr_err("error: number of entry in sg list");

	// Set buffer for AAD to insert to the first 8/12 Bytes in sg_out
	buf_aad = (u8 *)(&data->crypto_dsc.aad);
	//Invert the Byte order of AAD buffer
	for (i = 0; i < (data->crypto_dsc.info.aadsize/2); i++)
	{
		tmp = buf_aad[i];
		buf_aad[i] = buf_aad[data->crypto_dsc.info.aadsize -1 - i];
		buf_aad[data->crypto_dsc.info.aadsize -1 - i] = tmp;
	}
	// Set buffer for iv to insert to the following 8 Bytes in sg_out
	buf_iv = (u8 *)(&data->crypto_dsc.iv.iv);
	//Inver tbyte order of iv Buffer
	for(i = 0; i < 4; i++)
	{
		tmp = buf_iv[i];
		buf_iv[i] = buf_iv[7-i];
		buf_iv[7-i] = tmp;
	}
	// Copy two buffer into buffer of sg list
	memcpy(buf,buf_aad,data->crypto_dsc.info.aadsize);
	memcpy(buf + data->crypto_dsc.info.aadsize,buf_iv,8);

	len = (size_t)(aead_req->cryptlen + aead_req->assoclen + 16);

	// Copy authentication tag from buffer (sg_out)
	ret = get_tag(data , buf_tagout);
	if(ret)
		goto err_busy;
	// Compare two authentication tag
	if(data->ctx.ctx_op.dir == 0)
	{
		ret = bkcrypto_compare_icv(buf_tagout,data->crypto_dsc.icv);//1st: tag_out; 2nd: tag_in 
		// ret = 0 (successfull); ret = -EBADMSG (authenction failed)
	}
err_busy:
		bkcrypto_handle_result(base ,ret);
	switch (ret)
	{
		case 0:
			pr_err("%d:%s:Return value in callback: Done Successfully",__LINE__ , __func__ );
			break;
		case -1:
			pr_err("%d:%s:Return value in callback function: DMA Busy",__LINE__ , __func__ );
			break;
		case -74:
			pr_err("%d:%s:Return value in callback function:Authenc failed",__LINE__ , __func__ );
			break;
	}

	if(data)
		kfree(data);
	if(case_nents == 2)
		kfree(buf);
	return res;
}


/* Adding or Registering algorithm instace of AEAD /SK cipher crypto*/
static int bkcrypto_add_algs(struct bkcrypto_dev *mydevice)
{
	int i,j,ret = 0;
	for (i = 0; i < ARRAY_SIZE(bkcrypto_algs); i++){
		bkcrypto_algs[i]->mydevice = mydevice_glo; 
		// assign struct pointer in bkcrypto_algs to global varibles ( mydevice_glo)
		if (bkcrypto_algs[i]->type == BKCRYPTO_ALG_TYPE_SKCIPHER)
			ret = crypto_register_skcipher(&bkcrypto_algs[i]->alg.skcipher);
		else if (bkcrypto_algs[i]->type == BKCRYPTO_ALG_TYPE_AEAD)
			ret = crypto_register_aead(&bkcrypto_algs[i]->alg.aead);
		else
			ret = crypto_register_ahash(&bkcrypto_algs[i]->alg.ahash);
 	}
	if(ret)
		goto fail;
	return 0;
fail:
 	for (j = 0; j < i; j++) {
		if (bkcrypto_algs[j]->type == BKCRYPTO_ALG_TYPE_SKCIPHER)
			crypto_unregister_skcipher(&bkcrypto_algs[j]->alg.skcipher);
		else if (bkcrypto_algs[j]->type == BKCRYPTO_ALG_TYPE_AEAD)
			crypto_unregister_aead(&bkcrypto_algs[j]->alg.aead);
		else
			crypto_unregister_ahash(&bkcrypto_algs[j]->alg.ahash);
	}
	return ret;
}

static void bkcrypto_remove_algs(struct bkcrypto_dev *mydevice)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(bkcrypto_algs); i++) {
		if (bkcrypto_algs[i]->type == BKCRYPTO_ALG_TYPE_SKCIPHER)
			crypto_unregister_skcipher(&bkcrypto_algs[i]->alg.skcipher);
		else if (bkcrypto_algs[i]->type == BKCRYPTO_ALG_TYPE_AEAD)
			crypto_unregister_aead(&bkcrypto_algs[i]->alg.aead);
		else
			crypto_unregister_ahash(&bkcrypto_algs[i]->alg.ahash);
	}
	printk(KERN_INFO "unregister 3 types of algorithms \n");
}

static int bkcrypto_probe(void){
	struct bkcrypto_dev *mydevice;
	int ret;
	// If globle variable for this device is allocated, we just return it immediately
	if (mydevice_glo) {
		printk(KERN_INFO "ONLY 1 DEVICE AUTHORIZED");
		return -EEXIST;
	}
	// Kernel allocate dymanic memory for new struct crypto device
	mydevice = kzalloc(sizeof(struct bkcrypto_dev), GFP_KERNEL);
	// Checking after allocate
	if(!mydevice)
	{
		printk(KERN_INFO "Module bkcrypto: failed to allocate data structure for device driver\n");
		return -ENOMEM;
	}
	// Configure each engines
	/*
	- initiate the queue of result request (kcalloc to alloc dynamic memory for array)
	- initiate dev_id used for interrupt setting function 
	(@dev_id: A cookie passed back to the handler function)
	(including pointer of device and number of engines).
	- set up interrupt process and handler.
	- initiate workqueue for dequeue (refer to code below.)
	- initiate tasklet (dont know what next - may be fallback in function).
	- initiate the queue of pending request (refer to code below)
	- assume the number of request = 0;
	- no request => busy (value indicates for current request proccesed) =0
	- init lock for spin lock
	*/

	// initiate queue for this device
	crypto_init_queue(&mydevice->queue, BKCRYPTO_DEFAULT_MAX_QLEN);
	// initiate lock
	spin_lock_init(&mydevice->queue_lock);
	spin_lock_init(&submit_lock);
	INIT_LIST_HEAD(&mydevice->complete_queue);

	mydevice_glo = mydevice;
	mydevice->work_data.mydevice = mydevice;
	INIT_WORK(&mydevice->work_data.work, bkcrypto_dequeue_work);
	mydevice->workqueue = create_singlethread_workqueue("my_single_thread_workqueue");
	if (!mydevice->workqueue) {
			ret = -ENOMEM;
			goto err_reg_clk;
	}
	// register the neccesary algorithms for handle requests.
	ret = bkcrypto_add_algs(mydevice);

	if (ret){
	printk(KERN_INFO "Failed to register algorithms\n");
	}
	printk("device successfully registered \n");

	return 0;
err_reg_clk:
	printk(KERN_INFO "ERROR REG_CLK AND WORKQUEUE");
	return 0;
}
//entry point when driver was loaded
static int __init FPGAcrypt_init(void) 
{
	
 	// Register probe
	printk(KERN_INFO "Hello, World! - Module bkcrypto \n");
 	//probe with simulation
	bkcrypto_probe();

 return 0;
}

//entry point when driver was remove
static void __exit FPGAcrypt_exit(void) 
{
	bkcrypto_remove_algs(mydevice_glo);
    destroy_workqueue(mydevice_glo->workqueue);
	kfree(mydevice_glo);
	printk(KERN_INFO "Delete workqueue and unregister algorithms\n");
	printk(KERN_INFO "Goodbye, World!\n");
}

module_init(FPGAcrypt_init);
module_exit(FPGAcrypt_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Duy H.Ho");
MODULE_DESCRIPTION("A prototype Linux module for crypto in FPGA-PCIE card");
MODULE_VERSION("0.01");
