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
#include <linux/timer.h>
/* get system time */
#include <linux/jiffies.h> 
#include <linux/timekeeping.h>
#include <crypto/aes.h>
#include <crypto/gcm.h>
#include <crypto/des.h>
#include <crypto/aead.h>
#include <crypto/internal/aead.h>
#include <crypto/sha.h>
#include "mycrypto.h"
#include "xdma_region.h"
#include "xdma_crypto.h"
#include "crypto_testcases.h"
#include "common.h"
#include "xdma_crypto.h"

//#include "cipher.h"


#define AAAA
//#define not_free

#ifdef AAAA
#define pr_aaa(...)
#else 
#define pr_aaa pr_err
#endif

struct region_in region_in;
struct region_out region_out;
spinlock_t submit_lock;
int mycrypto_check_errors(struct mycrypto_dev *mydevice, struct mycrypto_context *ctx);
void set_xfer_mycryptocontext(struct crypto_async_request *base, struct xfer_req *req_xfer);
static int handle_crypto_xfer_callback(struct xfer_req *data, int res);
/* Limit of the crypto queue before reaching the backlog */
#define MYCRYPTO_DEFAULT_MAX_QLEN 128
// global variable for device
struct mycrypto_dev *mydevice_glo;
//struct timer_list mycrypto_ktimer;
// static int my_crypto_add_algs(struct mycrypto_dev *mydevice)


int mycrypto_compare_icv(u32 *tag_out, u32 *tag_in)
{
	int ret;
	int success = 0;// 4 : success ; <4: Fail
	int i;
	for (i = 0; i < 4; i++)
	{
		if((tag_out[i]) == (tag_in[i]))
			success ++;
		pr_err("Chunk:%d tag_in: %8.0x tag_out:%8.0x\n",i, tag_in[i],tag_out[i]);
		// pr_err("tag in:%x - Chunk:%d",tag_in[i],i);
		// pr_err("success value in loop:%d\n", success);
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

static struct mycrypto_alg_template *mycrypto_algs[] = {
	&mycrypto_alg_authenc_hmac_sha256_cbc_aes,
	&mycrypto_alg_authenc_hmac_sha256_ctr_aes,
	&mycrypto_alg_gcm_aes,
	&mycrypto_alg_rfc4106_gcm_aes,
	&mycrypto_alg_cbc_aes
};

void set_xfer_mycryptocontext(struct crypto_async_request *base, struct xfer_req *req_xfer)
{
	struct aead_request *req; 
	struct mycrypto_cipher_op ctx ;
	req= aead_request_cast(base);
	ctx = *(struct mycrypto_cipher_op *)crypto_tfm_ctx(req->base.tfm);
	req_xfer->ctx.ctx_op = ctx;
	req_xfer->base = base;
	pr_info("%d : set_xfer_mycryptocontext",__LINE__);
	
}
static inline void mycrypto_handle_result(struct crypto_async_request *base, int ret)
{
	struct mycrypto_req_operation *opr_ctx;
	bool should_complete=true;
	//req = mydevice->req;//prototype ( retrieve from complete queue)
	printk(KERN_INFO "Module mycrypto: handle result\n");
	opr_ctx = crypto_tfm_ctx(base->tfm);
	pr_aaa("%d:%s: size of mycrypto_req_operation:%d",__LINE__,__func__,sizeof(struct mycrypto_req_operation));
	// ret = opr_ctx->handle_result(base, &should_complete);

	if (should_complete) 
	{
			local_bh_disable();
			base->complete(base, ret);
			local_bh_enable();
	}
		
	printk(KERN_INFO "Module mycrypto: callback successfully \n");

}

struct crypto_async_request *mycrypto_dequeue_req_locked(struct mycrypto_dev *mydevice,
			   struct crypto_async_request **backlog)
{
	struct crypto_async_request *base;//asyn_req
	*backlog = crypto_get_backlog(&mydevice->queue);
	pr_err("Backlog pointer: %p",*backlog);
	base = crypto_dequeue_request(&mydevice->queue);
	if (!base)
		return NULL;
	return base;
}

void mycrypto_dequeue_req(struct mycrypto_dev *mydevice)
{
	struct crypto_async_request *base = NULL, *backlog = NULL;
	// struct mycrypto_req_operation *opr_ctx;
	struct xfer_req *req_xfer = NULL;
	struct aead_request *aead_req ;
	u8 *buff;
	int res;
	u32 i;
	__le32 *key;
	__le32 key_tmp[8];
	u32 *tag_outbound;
	size_t len;
	u32 i_icv = 0;
	// int i_submit;
	printk(KERN_INFO "Module mycrypto: dequeue request (after a period time by using workqueue)\n");
	
	// Test resubmit when one packet has been dropped because of dma dump error.
	/* If a request wasn't properly dequeued because of a lack of resources,
	 * proceeded it first,
	 */
	// base = mydevice->req;// get req from current request stored in struct device
	// // backlog=mydevice->backlog;
	// if (base)
	// {
	// 	goto handle_req;
	// }

	/*Get request from crypto queue*/
	spin_lock_bh(&mydevice->queue_lock);
	if (!mydevice->req) 
		{
			base = mycrypto_dequeue_req_locked(mydevice, &backlog);
			mydevice->req = base;
			// mydevice->backlog=backlog;
		}
	spin_unlock_bh(&mydevice->queue_lock);

	if (!base){
		return ;
	}
	if (backlog)
		backlog->complete(backlog, -EINPROGRESS);

// handle_req:
	// Step 1: Allocate request for xfer_req (pcie layer)
	
	req_xfer = alloc_xfer_req();
    if (!req_xfer)
	{
		return ;
	}

	aead_req = aead_request_cast(base);
	//Step 2: Set value for req_xfer
    
	//Set sg_in, sg_out in struct req_xfer
	
	//Set callback function to process request from pcie layer
	set_callback(req_xfer, &handle_crypto_xfer_callback);

	//Set neccesary context data for transfer request(req_xfer)
	set_xfer_mycryptocontext(base, req_xfer);

	//Print content of scattergather-list
	pr_err("%d :Module mycrypto:print sg content in dequeue function\n",__LINE__);
	len = (size_t)(aead_req->cryptlen + aead_req->assoclen + 16);
	print_sg_content(aead_req->src);

	// Get buf from list
	buff = sg_virt (aead_req->src);
	pr_info("%d : set buf",__LINE__);
	sg_init_one(&req_xfer->sg, buff , aead_req->src->length);
	sg_init_one(&req_xfer->sg_rcv, buff , aead_req->src->length);
	set_sg(req_xfer, &req_xfer->sg);
	/*Set value for struct region in*/
	//Struct region is temporary variable

	// INFO
	pr_info("%d :Module mycrypto: set info\n",__LINE__);
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

	//ICV-AUTHENTAG
	memcpy(region_in.crypto_dsc.icv , buff + req_xfer->ctx.ctx_op.cryptlen + req_xfer->ctx.ctx_op.assoclen , ICV_SIZE);
	pr_info("%d :Module mycrypto: icv set\n",__LINE__);
	//print icv (authetication tag in struct region_in) after being set
	pr_err("region_in.crypto_dsc.icv = %3.3x , data =  %8.0x %8.0x %8.0x %8.0x \n", i_icv ,
			(region_in.crypto_dsc.icv[i_icv + 3]),(region_in.crypto_dsc.icv[i_icv + 2]), 
            (region_in.crypto_dsc.icv[i_icv + 1]),(region_in.crypto_dsc.icv[i_icv]));
	
	//KEY
	//print context for key
	for (i = 0; i < KEYMEM/4 ; i+=4)
    {
        pr_aaa("req_xfer->ctx.ctx_op.key = %3.3x , data =  %8.0x %8.0x %8.0x %8.0x \n", i ,
			(req_xfer->ctx.ctx_op.key[i + 3]),(req_xfer->ctx.ctx_op.key[i + 2]), 
            (req_xfer->ctx.ctx_op.key[i + 1]),(req_xfer->ctx.ctx_op.key[i]));
    }
	//copy key to key_tmp
	/*
		128 bit key:
					16 Byte MSB: 0x00000000 00000000 00000000 00000000
					16 Byte LSB: Key(16 Bytes) stored in context data of req_xfer
		192 bit Key:
					16 Byte MSB: 0x00000000 00000000 Key(8 Bytes MSB)
					16 Byte LSB: Key(16 Bytes LSB) stored in context data of req_xfer
		256 bit Key:
					16 Byte MSB: Key (MSB)
					16 Byte LSB: Key (LSB)
	*/
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
	pr_info("%d :Module mycrypto: key set\n",__LINE__);
	//print key stored in region_in
	for (i = 0; i < KEYMEM/4 ; i+=4)
    {
        pr_aaa("region_in.crypto_dsc.key = %3.3x , data =  %8.0x %8.0x %8.0x %8.0x \n", i ,
			(region_in.crypto_dsc.key[i + 3]), (region_in.crypto_dsc.key[i + 2]), 
            (region_in.crypto_dsc.key[i + 1]), (region_in.crypto_dsc.key[i]));
    }

	//IV
	region_in.crypto_dsc.iv.nonce = req_xfer->ctx.ctx_op.nonce;
	// region_in.crypto_dsc.iv.iv[1] = cpu_to_be32(*(u32 *)(&req_xfer->ctx.ctx_op.iv[0]));// Little edian with ESP4
	// region_in.crypto_dsc.iv.iv[0] = cpu_to_be32(*(u32 *)(&req_xfer->ctx.ctx_op.iv[4]));// Little edian with ESP4
	region_in.crypto_dsc.iv.iv[1] = cpu_to_be32(*(u32 *)(buff + region_in.crypto_dsc.info.aadsize));// Little edian with ESP4
	region_in.crypto_dsc.iv.iv[0] = cpu_to_be32(*(u32 *)(buff + region_in.crypto_dsc.info.aadsize + 4));// Little edian with ESP4
	// memcpy(region_in.crypto_dsc.iv.iv , buff + region_in.crypto_dsc.info.aadsize , 8);
	region_in.crypto_dsc.iv.tail = 0x00000001;
	pr_err("Module mycrypto: Address of req_xfer->ctx.ctx_op.iv:%p - data =  %8.0x %8.0x \n",req_xfer->ctx.ctx_op.iv,  
            *((u32 *)(&req_xfer->ctx.ctx_op.iv[4])), *((u32 *)(&req_xfer->ctx.ctx_op.iv[0])));
    pr_err("Module mycrypto:region_in.crypto_dsc.iv.iv - data =  %8.0x %8.0x \n",  
            (region_in.crypto_dsc.iv.iv[1]), (region_in.crypto_dsc.iv.iv[0]));
    
	//AAD
	for (i = 0; i < region_in.crypto_dsc.info.aadsize /4 ; i++)
	{
		region_in.crypto_dsc.aad[(region_in.crypto_dsc.info.aadsize /4 - 1) - i] = cpu_to_be32(*(u32*)(buff + i*4 ));
		// 8/12 LSB bytes is AAD
	}
	for (i = region_in.crypto_dsc.info.aadsize / 4 ; i < 4;i ++)
	{
		region_in.crypto_dsc.aad[i] = 0x00000000;// 4 or 8 Bytes MSB is 0x
	}
	pr_err("region_in.crypto_dsc.aad  , data =  %8.0x %8.0x %8.0x %8.0x \n",
			(region_in.crypto_dsc.aad[3]), (region_in.crypto_dsc.aad[2]), 
            (region_in.crypto_dsc.aad[1]), (region_in.crypto_dsc.aad[0]));
	
    // Set value for crypto descryption in request memory layout
    // INFO
    req_xfer->crypto_dsc.info = region_in.crypto_dsc.info;
    // ICV
    memcpy(req_xfer->crypto_dsc.icv, region_in.crypto_dsc.icv, ICV_SIZE); 
    // KEY
    memcpy(req_xfer->crypto_dsc.key, region_in.crypto_dsc.key, KEY_SIZE); 
    //IV
    req_xfer->crypto_dsc.iv = region_in.crypto_dsc.iv;
    // AAD
    memcpy(req_xfer->crypto_dsc.aad, region_in.crypto_dsc.aad, AAD_SIZE); 

    // Set outbound data - TAG
	tag_outbound = kzalloc(16, GFP_ATOMIC | GFP_KERNEL);

	if (region_in.crypto_dsc.info.length % 16 == 0)
	    set_tag(req_xfer, 16, 0x20 + 0x10 * (region_in.crypto_dsc.info.length/16 ), tag_outbound);
	else 
    	set_tag(req_xfer, 16, 0x20 + 0x10 * (region_in.crypto_dsc.info.length/16 + 1), tag_outbound); 
	

    // Step 3: Submit to card
	pr_err("--------- sg info --------\n");
    
    pr_err("nents = %d, addr = %p, lenth = %d\n", sg_nents( req_xfer->sg_in), sg_virt( req_xfer->sg_in),req_xfer->sg_in->length);
    
    pr_err("--------- sg info --------\n");
	
	// for (i_submit = 0; i_submit < 5; i_submit++)
	// {
	pr_err("sizeof sg from network:%d \n",sizeof(*(aead_req->src)));
	pr_err("sizeof sg from network:%d \n",sizeof(*(req_xfer->sg_in)));	
	pr_err("check sg from network: %x %x %x %x %x %x %x %x\n", 
		*((u32 *)(aead_req->src)),*((u32 *)(aead_req->src) + 1), *((u32 *)(aead_req->src) + 2), *((u32 *)(aead_req->src) + 3),
		*((u32 *)(aead_req->src) + 4), *((u32 *)(aead_req->src) + 5), *((u32 *)(aead_req->src) + 6), *((u32 *)(aead_req->src) + 7) );
	
	pr_err("check sg from pcie   : %x %x %x %x %x %x %x %x\n",
		*((u32 *)(req_xfer->sg_in)),*((u32 *)(req_xfer->sg_in) + 1), *((u32 *)(req_xfer->sg_in) + 2), *((u32 *)(req_xfer->sg_in) + 3),
		*((u32 *)(req_xfer->sg_in) + 4), *((u32 *)(req_xfer->sg_in) + 5), *((u32 *)(req_xfer->sg_in) + 6), *((u32 *)(req_xfer->sg_in) + 7) );
		spin_lock_bh(&submit_lock);
		res = xdma_xfer_submit_queue(req_xfer);
		spin_unlock_bh(&submit_lock);
	
	// }

	if (res != -EINPROGRESS)
        pr_err("Unusual result\n");
	// Set req and backlog here........
    pr_aaa("%d: Module mycrypto : submitted req %d \n",__LINE__,i);
		
}

static void mycrypto_dequeue_work(struct work_struct *work)
{
	struct mycrypto_work_data *data =
			container_of(work, struct mycrypto_work_data, work);
	pr_aaa("%d:%s - Data pointer:%p; mydevice pointer:%p",__LINE__,__func__,data,data->mydevice);
	mycrypto_dequeue_req(data->mydevice);
}

//----------------------------------------------------------------
/* Handle xfer callback request
*/
static int handle_crypto_xfer_callback(struct xfer_req *data, int res)
{    
	char *buf;
	char *buf_aad = NULL;
	char *buf_iv = NULL;
	char tmp;// tmp for exchange
    int i = 0;
	size_t len;
	// Step 4: Get data in callback
	struct scatterlist *sg = data->sg_out;
	struct crypto_async_request *base = (struct crypto_async_request *) data->base;
	struct mycrypto_dev *mydevice = mydevice_glo;
	struct aead_request *aead_req ;
	int ret=0;
	aead_req = aead_request_cast(base);
	if (!base){ 
		pr_aaa("Module mycrypto: CAN NOT HANDLE A null POINTER\n");
		return res;
	}
	pr_err("Complete with res = %d ! This is callback function! \n", res);
	pr_info("Module mycrypto: handle callback function from pcie layer \n");
	pr_err("Module mycrypto: Address of req_xfer->ctx.ctx_op.iv:%p - data =  %8.0x %8.0x \n",data->ctx.ctx_op.iv,  
            *((u32 *)(&data->ctx.ctx_op.iv[4])), *((u32 *)(&data->ctx.ctx_op.iv[0])));
	
	// Step 5: Do your things - Here we print the data out
    if (res == -1)
	{
		ret = res;
		goto err_busy;
	}
    buf = sg_virt (sg);
	
	
	// Set buffer for AAD to insert to the first 8/12 Bytes in sg_out
	buf_aad = (u8 *)(&data->crypto_dsc.aad);
	pr_info("buf_aad: %x %x %x %x %x %x %x %x %x %x %x %x\n", buf_aad[0],buf_aad[1],buf_aad[2],buf_aad[3],buf_aad[4],buf_aad[5],buf_aad[6],buf_aad[7],buf_aad[8],buf_aad[9],buf_aad[10],buf_aad[11]);
	//Invert the Byte order of AAD buffer
	for (i = 0; i < (data->crypto_dsc.info.aadsize/2); i++)
	{
		tmp = buf_aad[i];
		buf_aad[i] = buf_aad[data->crypto_dsc.info.aadsize -1 - i];
		buf_aad[data->crypto_dsc.info.aadsize -1 - i] = tmp;
	}
	pr_info("buf_aad: %x %x %x %x %x %x %x %x %x %x %x %x\n", buf_aad[0],buf_aad[1],buf_aad[2],buf_aad[3],buf_aad[4],buf_aad[5],buf_aad[6],buf_aad[7],buf_aad[8],buf_aad[9],buf_aad[10],buf_aad[11]);
	
	
	// Set buffer for iv to insert to the following 8 Bytes in sg_out
	buf_iv = (u8 *)(&data->crypto_dsc.iv.iv);
	pr_info("buf_iv: %x %x %x %x %x %x %x %x \n", buf_iv[0],buf_iv[1],buf_iv[2],buf_iv[3],buf_iv[4],buf_iv[5],buf_iv[6],buf_iv[7]);
	//Inver tbyte order of iv Buffer
	for(i = 0; i < 4; i++)
	{
		tmp = buf_iv[i];
		buf_iv[i] = buf_iv[7-i];
		buf_iv[7-i] = tmp;
	}
	pr_info("buf_iv: %x %x %x %x %x %x %x %x \n", buf_iv[0],buf_iv[1],buf_iv[2],buf_iv[3],buf_iv[4],buf_iv[5],buf_iv[6],buf_iv[7]);
	

	// Copy two buffer into buffer of sg list
	memcpy(buf,buf_aad,data->crypto_dsc.info.aadsize);
	memcpy(buf + data->crypto_dsc.info.aadsize,buf_iv,8);

	//Print sg content
	len = (size_t)(aead_req->cryptlen + aead_req->assoclen + 16);
	// print_sg_content(sg);

	// Copy authentication tag from buffer (sg_out)
	memcpy(buf + data->ctx.ctx_op.cryptlen + data->ctx.ctx_op.assoclen, data->tag,ICV_SIZE);
    pr_err("tag = %8.0x %8.0x %8.0x %8.0x \n", 
            *(data->tag + 3), *(data->tag + 2),
            *(data->tag + 1), *(data->tag));
	
	// Compare two authentication tag
	if(data->ctx.ctx_op.dir == 0)
	{
		ret = mycrypto_compare_icv(data->tag,data->crypto_dsc.icv);//1st: tag_out; 2nd: tag_in 
		// ret = 0 (successfull); ret = -EBADMSG (authenction failed)
	}
err_busy:
	// if(ret != -1)
	// {
		mydevice->req = NULL; // No dma busy
		mycrypto_handle_result(base ,ret);

	// }

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
	queue_work(mydevice->workqueue,&mydevice->work_data.work);
		   	
	if(data->tag)
		kfree(data->tag);
	// free_xfer_req(data); // data is xfer_req
	kfree(data);
	return res;
}


//--------------timer handler---------------------------------------
static void handle_timer(struct timer_list *t)
{
	struct mycrypto_dev *mydevice =from_timer(mydevice,t,mycrypto_ktimer);
	struct crypto_async_request *req;
	int ret = 0;
	req = mydevice->req;
	printk(KERN_INFO "Module mycrypto: HELLO timer\n");
	
	if (!mydevice){
		printk(KERN_ERR "Module mycrypto: CAN NOT HANDLE A null POINTER\n");
		return;
	}
	
	mycrypto_handle_result(req,ret);
	queue_work(mydevice->workqueue,
		   &mydevice->work_data.work);
}

// static void mycrypto_configure(struct mycrypto_dev *mydevice)
// {
// 	mydevice->config.engines = 2;
// }

/* Adding or Registering algorithm instace of AEAD /SK cipher crypto*/
static int mycrypto_add_algs(struct mycrypto_dev *mydevice)
{
	int i,j,ret = 0;
	for (i = 0; i < ARRAY_SIZE(mycrypto_algs); i++){
		mycrypto_algs[i]->mydevice = mydevice_glo; 
		// assign struct pointer in mycrypto_algs to global varibles ( mydevice_glo)
		if (mycrypto_algs[i]->type == MYCRYPTO_ALG_TYPE_SKCIPHER)
			ret = crypto_register_skcipher(&mycrypto_algs[i]->alg.skcipher);
		else if (mycrypto_algs[i]->type == MYCRYPTO_ALG_TYPE_AEAD)
			ret = crypto_register_aead(&mycrypto_algs[i]->alg.aead);
		else
			ret = crypto_register_ahash(&mycrypto_algs[i]->alg.ahash);
 	}
	if(ret)
		goto fail;
	return 0;
fail:
 	for (j = 0; j < i; j++) {
		if (mycrypto_algs[j]->type == MYCRYPTO_ALG_TYPE_SKCIPHER)
			crypto_unregister_skcipher(&mycrypto_algs[j]->alg.skcipher);
		else if (mycrypto_algs[j]->type == MYCRYPTO_ALG_TYPE_AEAD)
			crypto_unregister_aead(&mycrypto_algs[j]->alg.aead);
		else
			crypto_unregister_ahash(&mycrypto_algs[j]->alg.ahash);
	}
	return ret;
}

static void mycrypto_remove_algs(struct mycrypto_dev *mydevice)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(mycrypto_algs); i++) {
		if (mycrypto_algs[i]->type == MYCRYPTO_ALG_TYPE_SKCIPHER)
			crypto_unregister_skcipher(&mycrypto_algs[i]->alg.skcipher);
		else if (mycrypto_algs[i]->type == MYCRYPTO_ALG_TYPE_AEAD)
			crypto_unregister_aead(&mycrypto_algs[i]->alg.aead);
		else
			crypto_unregister_ahash(&mycrypto_algs[i]->alg.ahash);
	}
	printk(KERN_INFO "unregister 3 types of algorithms \n");
}

static int mycrypto_probe(void){
	struct mycrypto_dev *mydevice;
	int ret;
	// If globle variable for this device is allocated, we just return it immediately
	if (mydevice_glo) {
		printk(KERN_INFO "ONLY 1 DEVICE AUTHORIZED");
		return -EEXIST;
	}
	// Kernel allocate dymanic memory for new struct crypto device
	mydevice = kzalloc(sizeof(struct mycrypto_dev), GFP_KERNEL);
	// Checking after allocate
	if(!mydevice)
	{
		printk(KERN_INFO "Module mycrypto: failed to allocate data structure for device driver\n");
		return -ENOMEM;
	}
	// Allocate dynamic memory for linear buffer ( for testing).
	// mydevice->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
	// Configure parameters for this crypto device.
	// mycrypto_configure(mydevice);
	// mydevice->engine = kcalloc(mydevice->config.engines,sizeof(*mydevice->engine),GFP_KERNEL);
	// if (!mydevice->engine) {
	// 	printk(KERN_INFO "Module mycrypto: failed to allocate data structure for engine\n");
	// 	ret = -ENOMEM;
	// 	return ret;
	// }

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
	crypto_init_queue(&mydevice->queue, MYCRYPTO_DEFAULT_MAX_QLEN);
	// initiate lock
	spin_lock_init(&mydevice->queue_lock);
	spin_lock_init(&submit_lock);
	INIT_LIST_HEAD(&mydevice->complete_queue);
	// create workqueue and tasklet
	//tasklet_init(&mydevice->tasklet, mycrypto_tasklet_callback, (unsigned long)mydevice);
	mydevice_glo = mydevice;
	mydevice->work_data.mydevice = mydevice;
	INIT_WORK(&mydevice->work_data.work, mycrypto_dequeue_work);
	mydevice->workqueue = create_singlethread_workqueue("my_single_thread_workqueue");
	if (!mydevice->workqueue) {
			ret = -ENOMEM;
			goto err_reg_clk;
	}
	// register the neccesary algorithms for handle requests.
	ret = mycrypto_add_algs(mydevice);

	if (ret){
	printk(KERN_INFO "Failed to register algorithms\n");
	}
	printk("device successfully registered \n");
	// Set up timer and callback handler (using for testing).
	timer_setup(&mydevice->mycrypto_ktimer,handle_timer,0);

	//mod_timer(&mydevice->mycrypto_ktimer, jiffies + 2*HZ);
	return 0;
err_reg_clk:
	printk(KERN_INFO "ERROR REG_CLK AND WORKQUEUE");
	return 0;
}
//entry point when driver was loaded
static int __init FPGAcrypt_init(void) 
{
	
 	// Register probe
	printk(KERN_INFO "Hello, World! - Module Mycrypto \n");
 	//probe with simulation
	mycrypto_probe();

 return 0;
}

//entry point when driver was remove
static void __exit FPGAcrypt_exit(void) 
{
	mycrypto_remove_algs(mydevice_glo);
    destroy_workqueue(mydevice_glo->workqueue);
	//-----------delete timer------------------
	del_timer_sync(&mydevice_glo->mycrypto_ktimer);
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