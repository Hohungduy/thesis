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

/*choose mode for test
 @mode 1: test timer and callback.
 @mode 2: test with lower layer 
*/
// static unsigned int mode = 1;
// module_param(mode, uint, 0000);
// MODULE_PARM_DESC(mode, "choose mode");

struct region_in testcase_in;
struct region_out testcase_out;

int mycrypto_check_errors(struct mycrypto_dev *mydevice, struct mycrypto_context *ctx);
void set_xfer_mycryptocontext(struct crypto_async_request *req, struct xfer_req *req_xfer);
static int handle_crypto_xfer_callback(struct xfer_req *data, int res);
/* Limit of the crypto queue before reaching the backlog */
#define MYCRYPTO_DEFAULT_MAX_QLEN 128
// global variable for device
struct mycrypto_dev *mydevice_glo;
//struct timer_list mycrypto_ktimer;

// static int my_crypto_add_algs(struct mycrypto_dev *mydevice)
void print_sg_content(struct scatterlist *sg ,size_t len)
{
	char *sg_buffer =NULL; //for printing
	int i;
	sg_buffer =kzalloc(len, GFP_KERNEL);
	len =sg_pcopy_to_buffer(sg,1,sg_buffer,len,0);
	//pr_info("%d: print sg in dequeue function",__LINE__);
    for(i = 0; i < (len); i +=16)
    {
		pr_err("address = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i ,
            *((u32 *)(&sg_buffer[i + 12])), *((u32 *)(&sg_buffer[i + 8])), 
            *((u32 *)(&sg_buffer[i + 4])), *((u32 *)(&sg_buffer[i])));
    }
	pr_err("                     %s, %d, %p", __func__, __LINE__,sg_buffer );

	kfree(sg_buffer);
}
void print_sg_virt_content(struct scatterlist *sg)
{
    unsigned int length;
    unsigned int i;
    char *buf;

	if (!sg){
		pr_err("Print sg failed, sg NULL \n");
		return;
	}

    length = sg_nents(sg);
    buf = sg_virt (sg);
	pr_err("print sg buffer with length %d\n", length);

    for (i = 0; i < length; i += 16)
    {
        pr_err("Print virual sg: address = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i ,
            *((u32 *)(&buf[i + 12])), *((u32 *)(&buf[i + 8])), 
            *((u32 *)(&buf[i + 4])), *((u32 *)(&buf[i])));
    }
}
// void print_processing_list_mycrypto(struct list_head *backlog)
// {
//     //struct xdma_crdev *crdev = g_xpdev->crdev;
//     struct list_head *p;
//     // int i = 0;
// 	pr_err("%s: id = %d \n",__func__ ,__LINE__);
//     list_for_each(p, backlog){
//         struct crypto_async_request *req = list_entry(p, struct crypto_async_request, list);
//         pr_err("%s: id = %p \n",__func__ ,req);
//     }
// }
static struct mycrypto_alg_template *mycrypto_algs[] = {
	&mycrypto_alg_authenc_hmac_sha256_cbc_aes,
	&mycrypto_alg_authenc_hmac_sha256_ctr_aes,
	&mycrypto_alg_gcm_aes,
	&mycrypto_alg_rfc4106_gcm_aes,
	&mycrypto_alg_cbc_aes
};

void set_xfer_mycryptocontext(struct crypto_async_request *base, struct xfer_req *req_xfer)
{
	struct aead_request *req = aead_request_cast(base);
	struct mycrypto_cipher_op ctx = * (struct mycrypto_cipher_op *)crypto_tfm_ctx(req->base.tfm);
	struct mycrypto_cipher_req req_ctx = * (struct mycrypto_cipher_req *)aead_request_ctx(req);
	req_xfer->ctx.ctx_op = ctx;
	req_xfer->ctx.ctx_req = req_ctx;
	req_xfer->base = base;
	pr_info("%d : set_xfer_mycryptocontext",__LINE__);
	//req_xfer->sg = req->src;
	//req_xfer->crypto_complete = handle_crypto_xfer_callback;

}
static inline void mycrypto_handle_result(struct crypto_async_request *req)
{
	//struct crypto_async_request *req;
	struct mycrypto_req_operation *opr_ctx;
	int ret;
	bool should_complete;
	//req = mydevice->req;//prototype ( retrieve from complete queue)
	printk(KERN_INFO "Module mycrypto: handle result\n");
	opr_ctx = crypto_tfm_ctx(req->tfm);
	ret = opr_ctx->handle_result(req, &should_complete);
	if (should_complete) 
	{
			local_bh_disable();
			req->complete(req, ret);
			local_bh_enable();
	}
	printk(KERN_INFO "Module mycrypto: callback successfully \n");

}

struct crypto_async_request *mycrypto_dequeue_req_locked(struct mycrypto_dev *mydevice,
			   struct crypto_async_request **backlog)
{
	struct crypto_async_request *req;
	*backlog = crypto_get_backlog(&mydevice->queue);
	req = crypto_dequeue_request(&mydevice->queue);
	if (!req)
		return NULL;
	return req;
}

void mycrypto_dequeue_req(struct mycrypto_dev *mydevice)
{
	struct crypto_async_request *req = NULL, *backlog = NULL;
	//struct mycrypto_req_operation *opr_ctx;
	struct xfer_req *req_xfer = NULL;
	struct aead_request *aead_req ;
	u8 *buff;
	int res;
	u32 i;
	__le32 *key;
	__le32 key_tmp[8];
	u32 *tag_outbound;
	
	printk(KERN_INFO "Module mycrypto: dequeue request (after a period time by using workqueue)\n");
	spin_lock_bh(&mydevice->queue_lock);
	pr_err("%d:%s\n",__LINE__,__func__);
	if (!mydevice->req) {
		pr_err("%d:%s\n",__LINE__,__func__);
		req = mycrypto_dequeue_req_locked(mydevice, &backlog);
			pr_err("%d:%s\n",__LINE__,__func__);
		mydevice->req = req;
	}

	spin_unlock_bh(&mydevice->queue_lock);
	if (!mydevice->req) {
		pr_err("%d:%s\n",__LINE__,__func__);
	}
	pr_info("%d:%s\n",__LINE__,__func__);
	if (!req){
		return ;

	}
	pr_info("%d:%s\n",__LINE__,__func__);
	if (backlog)
		backlog->complete(backlog, -EINPROGRESS);
	pr_info("%d:%s\n",__LINE__,__func__);
	// Step 1: Allocate request for xfer_req (pcie layer)
	
	req_xfer = alloc_xfer_req ();
	aead_req = aead_request_cast(req);
	pr_info("%d:%s",__LINE__,__func__);
    // Step 2: Set value for req_xfer
    
	set_sg(req_xfer, aead_req->src);
	pr_info("%d:%s",__LINE__,__func__);
	//print_content
	print_sg_virt_content(aead_req->src);
    set_callback(req_xfer, &handle_crypto_xfer_callback);
	set_xfer_mycryptocontext(req, req_xfer);
	//Set value for struct testcase
	pr_info("%d :Module mycrypto:print sg content in dequeue function\n",__LINE__);
	print_sg_virt_content(req_xfer->sg_in);

	buff = sg_virt (req_xfer->sg_in);
	//pr_info("%d : set buf",__LINE__);
	// INFO
	for (i = 0; i <=2; i++)
	{
		testcase_in.crypto_dsc.info.free_space[i]=0x00000000;
	}
	testcase_in.crypto_dsc.info.free_space_ = 0;
	testcase_in.crypto_dsc.info.direction = req_xfer->ctx.ctx_op.dir;
	testcase_in.crypto_dsc.info.length = req_xfer->ctx.ctx_op.cryptlen;
	testcase_in.crypto_dsc.info.aadsize = req_xfer->ctx.ctx_op.assoclen - 8;// substract iv len
	switch(req_xfer->ctx.ctx_op.keylen)
	{
		case 16: testcase_in.crypto_dsc.info.keysize = 0;
					break;
		case 24: testcase_in.crypto_dsc.info.keysize = 1;
					break;
		case 32: testcase_in.crypto_dsc.info.keysize = 2;
					break;
	}
	pr_info("%d :Module mycrypto: set info\n",__LINE__);
	//ICV-AUTHENTAG
	memcpy(testcase_in.crypto_dsc.icv,buff + req_xfer->ctx.ctx_op.cryptlen + req_xfer->ctx.ctx_op.assoclen,ICV_SIZE);
	pr_info("%d :Module mycrypto: icv set\n",__LINE__);
	// for (i = 0; i < 4; i++)
	// {
	// 	testcase_in.crypto_dsc.icv[i] = *((u32*)(buff + req_xfer->ctx.ctx_op.cryptlen + req_xfer->ctx.ctx_op.assoclen + 16 - i*4) );
	// }
	//KEY
	for (i = 0; i < KEYMEM/4 ; i+=4)
    {
        pr_err("req_xfer->ctx.ctx_op.key = %3.3x , data =  %8.0x %8.0x %8.0x %8.0x \n", i ,
			(req_xfer->ctx.ctx_op.key[i + 3]),(req_xfer->ctx.ctx_op.key[i + 2]), 
            (req_xfer->ctx.ctx_op.key[i + 1]),(req_xfer->ctx.ctx_op.key[i]));
    }
	switch (req_xfer->ctx.ctx_op.keylen)
	{
		case 16:
			for (i = 0; i < (KEYMEM - 16)/4; i++)
			{
				key_tmp[i]=0x00000000;
			}
			for (i = (KEYMEM - 16)/4; i < KEYMEM/4;i++)
			{
				key_tmp[i]=req_xfer->ctx.ctx_op.key[i-((KEYMEM - 16)/4)];
			}
			break;
		case 24:
			for (i = 0; i < (KEYMEM - 24)/4; i++)
			{
				key_tmp[i]=0x00000000;
			}
			for (i = (KEYMEM - 24)/4; i < KEYMEM/4;i++)
			{
				key_tmp[i]=req_xfer->ctx.ctx_op.key[i-(KEYMEM - 24)/4];
			}
			break;
		case 32:
			for (i = 0; i < KEYMEM/4;i++)
			{
				key_tmp[i]=req_xfer->ctx.ctx_op.key[i];
			}
			break;
	}
	key = &(key_tmp[0]);

	// for (i = 0; i < KEYMEM/4 ; i++)
    // {
    //     pr_err("key_tmp = %3.3x , data =  %8.0x %8.0x %8.0x %8.0x \n", i ,
	// 		*((u32 *)(&key_tmp[i + 3])), *((u32 *)(&key_tmp[i + 2])), 
    //         *((u32 *)(&key_tmp[i + 1])), *((u32 *)(&key_tmp[i])));
    // }
	pr_info("%d :Module mycrypto: key set\n",__LINE__);
	for (i = 0; i < KEYMEM/4 ; i+=4)
    {
        pr_err("key_tmp = %3.3x , data =  %8.0x %8.0x %8.0x %8.0x \n", i ,
			(key_tmp[i + 3]), (key_tmp[i + 2]), 
            (key_tmp[i + 1]), (key_tmp[i]));
    }
	
	// for (i = 0; i < (req_xfer->ctx.ctx_op.keylen/4); i++)
	// {
	// 	testcase_in.crypto_dsc.key[i] = *(u32 *)(key + req_xfer->ctx.ctx_op.keylen - i*4);
	// }
	memcpy(testcase_in.crypto_dsc.key,key,KEY_SIZE);
	for (i = 0; i < KEYMEM/4 ; i+=4)
    {
        pr_err("testcase_in.crypto_dsc.key = %3.3x , data =  %8.0x %8.0x %8.0x %8.0x \n", i ,
			(testcase_in.crypto_dsc.key[i + 3]), (testcase_in.crypto_dsc.key[i + 2]), 
            (testcase_in.crypto_dsc.key[i + 1]), (testcase_in.crypto_dsc.key[i]));
    }
    //pr_err("testcase_in.crypto_dsc.key = %32.0x",(testcase_in.crypto_dsc.key));
	//IV
	testcase_in.crypto_dsc.iv.nonce = req_xfer->ctx.ctx_op.nonce;
	testcase_in.crypto_dsc.iv.iv[0] = *(u32 *)(req_xfer->ctx.ctx_op.iv);
	testcase_in.crypto_dsc.iv.iv[1] = *(u32 *)(req_xfer->ctx.ctx_op.iv + 4);
	testcase_in.crypto_dsc.iv.tail = 0x00000001;

	pr_err("Module mycrypto: Address of req_xfer->ctx.ctx_op.iv:%p - data =  %8.0x %8.0x \n",req_xfer->ctx.ctx_op.iv,  
            *((u32 *)(&req_xfer->ctx.ctx_op.iv[4])), *((u32 *)(&req_xfer->ctx.ctx_op.iv[0])));
    pr_err("Module mycrypto:testcase_in.crypto_dsc.iv.iv - data =  %8.0x %8.0x \n",  
            (testcase_in.crypto_dsc.iv.iv[1]), (testcase_in.crypto_dsc.iv.iv[0]));
    
	//AAD
		pr_err("%d:%s\n",__LINE__,__func__);

	for (i = 0; i < testcase_in.crypto_dsc.info.aadsize /4; i++)
	{
		testcase_in.crypto_dsc.aad[i] = *(u32*)(buff + req_xfer->ctx.ctx_op.assoclen - 8 - i*4 );
	}
			pr_err("%d:%s\n",__LINE__,__func__);

	memcpy(testcase_in.crypto_dsc.aad,buff,testcase_in.crypto_dsc.info.aadsize);
    // Set value for ctx (context) testcase)
    // INFO
			pr_err("%d:%s\n",__LINE__,__func__);

    req_xfer->crypto_dsc.info = testcase_in.crypto_dsc.info;
    // ICV
			pr_err("%d:%s\n",__LINE__,__func__);

    memcpy(req_xfer->crypto_dsc.icv, testcase_in.crypto_dsc.icv, ICV_SIZE); 
    // KEY
			pr_err("%d:%s\n",__LINE__,__func__);

    memcpy(req_xfer->crypto_dsc.key, testcase_in.crypto_dsc.key, KEY_SIZE); 
    //IV
			pr_err("%d:%s\n",__LINE__,__func__);

    req_xfer->crypto_dsc.iv = testcase_in.crypto_dsc.iv;
    // AAD
			pr_err("%d:%s\n",__LINE__,__func__);

    memcpy(req_xfer->crypto_dsc.aad, testcase_in.crypto_dsc.aad, AAD_SIZE); 
		pr_err("%d:%s\n",__LINE__,__func__);

    //set_ctx(req_xfer, ctx);
    // Set outbound info -- testcase 1
	tag_outbound = kmalloc(16, GFP_ATOMIC | GFP_KERNEL);
	pr_err("%d, %s: %p %p %p\n",__LINE__,__func__, req_xfer, 
		&testcase_in ,tag_outbound );
	pr_err("%d, %s: %p %d %p\n",__LINE__,__func__, req_xfer, 
		0x20 + (testcase_in.crypto_dsc.info.length/16 + 1) * 16,tag_outbound );
    set_tag(req_xfer, 16, 0x20 + 0x10 * (testcase_in.crypto_dsc.info.length/16 + 1), tag_outbound);
	pr_err("%d:%s\n",__LINE__,__func__);
    // Step 3: Submit to card	
	res = xdma_xfer_submit_queue(req_xfer);
    if (res != -EINPROGRESS)
        pr_err("Unusual result\n");
    pr_err("%d: Module mycrypto : submitted req %d \n",__LINE__,i);
		
	// Testing handle request function
	//opr_ctx = crypto_tfm_ctx(req->tfm);
	//opr_ctx->handle_request(req);
	//mod_timer(&mydevice->mycrypto_ktimer,jiffies + 1*HZ);	
	
}

static void mycrypto_dequeue_work(struct work_struct *work)
{
	struct mycrypto_work_data *data =
			container_of(work, struct mycrypto_work_data, work);
	pr_err("%d:%s - Data pointer:%p; mydevice pointer:%p",__LINE__,__func__,data,data->mydevice);
	mycrypto_dequeue_req(data->mydevice);
}

//----------------------------------------------------------------
/* Handle xfer callback request
*/
static int handle_crypto_xfer_callback(struct xfer_req *data, int res)
{    
	char *buf;
    int i = 0;
	// Step 4: Get data in callback
	struct scatterlist *sg = data->sg_out;
	struct crypto_async_request *req = (struct crypto_async_request *) data->base;
	struct mycrypto_dev *mydevice = mydevice_glo;
	pr_err("Complete with res = %d ! This is callback function! \n", res);
	pr_info("Module mycrypto: handle callback function from pcie layer \n");
	// Step 5: Do your things - Here we print the data out
    
    buf = sg_virt (sg);
    for (i = 0; i < 72 ; i += 16)
    {
        pr_err("address = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i ,
            *((u32 *)(&buf[i + 12])), *((u32 *)(&buf[i + 8])), 
            *((u32 *)(&buf[i + 4])), *((u32 *)(&buf[i])));
    }
    pr_err("tag = %8.0x %8.0x %8.0x %8.0x \n", 
            *(data->tag + 3), *(data->tag + 2),
            *(data->tag + 1), *(data->tag));
	
	if (!req){ 
		pr_err("Module mycrypto: CAN NOT HANDLE A null POINTER\n");
		return res;
	}
	mydevice->req=NULL;
	mycrypto_handle_result(req);
	queue_work(mydevice->workqueue,
		   &mydevice->work_data.work);
	pr_err("                     %s, %d, %p", __func__, __LINE__,data->tag );
	pr_err("                     %s, %d, %p", __func__, __LINE__,data);

	kfree(data->tag);
	free_xfer_req(data); // data is xfer_req
	return res;
}
//--------------------------------------------------------------------
//--------------timer handler---------------------------------------
static void handle_timer(struct timer_list *t)
{
	struct mycrypto_dev *mydevice =from_timer(mydevice,t,mycrypto_ktimer);
	struct crypto_async_request *req;
	req = mydevice->req;

	printk(KERN_INFO "Module mycrypto: HELLO timer\n");
	
	if (!mydevice){
		printk(KERN_ERR "Module mycrypto: CAN NOT HANDLE A null POINTER\n");
		return;
	}
	
	mycrypto_handle_result(req);
	queue_work(mydevice->workqueue,
		   &mydevice->work_data.work);
	// handle result copy from buffer and callback
	// dequeue again
	//mycrypto_skcipher_handle_result()
	//mod_timer(&mycrypto_ktimer, jiffies + 2*HZ);
}

static void mycrypto_configure(struct mycrypto_dev *mydevice)
{
	mydevice->config.engines = 2;
}
//------------------------------------------------------------
// Adding or Registering algorithm instace of AEAD /SK cipher crypto
static int mycrypto_add_algs(struct mycrypto_dev *mydevice)
{
 int i,j,ret = 0;
 for (i = 0; i < ARRAY_SIZE(mycrypto_algs); i++) {
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
	//mydevice->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
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
	printk(KERN_INFO "Hello, World!\n");
 //probe with simulation
	mycrypto_probe();
 return 0;
}

//entry point when driver was remove
static void __exit FPGAcrypt_exit(void) 
{
	mycrypto_remove_algs(mydevice_glo);
	// flush_workqueue(mydevice_glo->workqueue);
    destroy_workqueue(mydevice_glo->workqueue);
	//-----------delete timer------------------
	del_timer_sync(&mydevice_glo->mycrypto_ktimer);
	pr_err("                     %s, %d, %p", __func__, __LINE__, mydevice_glo);
	kfree(mydevice_glo);
	printk(KERN_INFO "Delete workqueue and unregister algorithms\n");
	printk(KERN_INFO "Goodbye, World!\n");
}

module_init(FPGAcrypt_init);
module_exit(FPGAcrypt_exit);

/*
static struct pci_driver my_pcie_crypto_driver = {
	.name = "my_pcie_crypto_driver",
	.id_table = pcie_crypto_tpls,
	.probe = my_pcie_crypto_probe,
	.remove = my_pcie_crypto_remove,
};
*/


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Duy H.Ho");
MODULE_DESCRIPTION("A prototype Linux module for crypto in FPGA-PCIE card");
MODULE_VERSION("0.01");