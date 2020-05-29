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
#include "include/mycrypto.h"
#include "include/common.h"

/* Limit of the crypto queue before reaching the backlog */
#define MYCRYPTO_DEFAULT_MAX_QLEN 128
// global variable for device
struct mycrypto_dev *mydevice_glo;
//struct timer_list mycrypto_ktimer;

// static int my_crypto_add_algs(struct mycrypto_dev *mydevice)

static struct mycrypto_alg_template *mycrypto_algs[] ={
	&mycrypto_alg_authenc_hmac_sha256_cbc_aes,
	&mycrypto_alg_authenc_hmac_sha256_ctr_aes,
	&mycrypto_alg_gcm_aes,
	&mycrypto_alg_cbc_aes
};

// static void mycrypto_tasklet_callback(unsigned long data)
// {

// }

static inline void mycrypto_handle_result(struct mycrypto_dev *mydevice)
{
	struct crypto_async_request *req;
	struct mycrypto_req_operation *opr_ctx;
	int ret;
	bool should_complete;
	req = mydevice->req;//prototype ( retrieve from complete queue)
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
	struct mycrypto_req_operation *opr_ctx;
	printk(KERN_INFO "module mycrypto: dequeue request (after a period time by using workqueue)\n");
	
	spin_lock_bh(&mydevice->queue_lock);
	if (!mydevice->req) {
		req = mycrypto_dequeue_req_locked(mydevice, &backlog);
		mydevice->req = req;
	}
	spin_unlock_bh(&mydevice->queue_lock);

	if (!req)
		return;
	if (backlog)
		backlog->complete(backlog, -EINPROGRESS);
	
	// Testing handle request function
	opr_ctx = crypto_tfm_ctx(req->tfm);
	opr_ctx->handle_request(req);

	mod_timer(&mydevice->mycrypto_ktimer,jiffies + 1*HZ);
}
static void mycrypto_dequeue_work(struct work_struct *work)
{
	struct mycrypto_work_data *data =
			container_of(work, struct mycrypto_work_data, work);
	mycrypto_dequeue_req(data->mydevice);
}
//--------------------------------------------------------------------
//--------------timer handler---------------------------------------
static void handle_timer(struct timer_list *t)
{
	struct mycrypto_dev *mydevice =from_timer(mydevice,t,mycrypto_ktimer);
	printk(KERN_INFO "Module mycrypto: HELLO timer\n");
	
	if (!mydevice){
		printk(KERN_ERR "CAN NOT HANDLE A null POINTER\n");
		return;
	}
	
	mycrypto_handle_result(mydevice);
	queue_work(mydevice->workqueue,
		   &mydevice->work_data.work);
	// handle result copy from buffer and callback
	// dequeue again
	//mycrypto_skcipher_handle_result()
	//mod_timer(&mycrypto_ktimer, jiffies + 2*HZ);
}
// static void configure_timer(struct timer_list *mycrypto_ktimer)
// {
// 	struct mycrypto_dev * mydevice_glo;
// 	mycrypto_ktimer->expires = jiffies + 2*HZ ;
// 	mycrypto_ktimer->function = handle_timer;
// 	mycrypto_ktimer->data = (unsigned long)(mydevice_glo);

// }

static void mycrypto_configure(struct mycrypto_dev *mydevice)
{
	mydevice->config.engines = 2;
	// priv->config.cd_size = (sizeof(struct safexcel_command_desc) / sizeof(u32));
	// priv->config.cd_offset = (priv->config.cd_size + mask) & ~mask;
	// priv->config.rd_size = (sizeof(struct safexcel_result_desc) / sizeof(u32));
	// priv->config.rd_offset = (priv->config.rd_size + mask) & ~mask;
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
	mydevice = kzalloc(sizeof(*mydevice), GFP_KERNEL);
	// Checking after allocate
	if(!mydevice)
	{
		printk(KERN_INFO "Module mycrypto: failed to allocate data structure for device driver\n");
		return -ENOMEM;
	}
	// Allocate dynamic memory for linear buffer ( for testing).
	mydevice->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
	// Configure parameters for this crypto device.
	mycrypto_configure(mydevice);
	mydevice->engine = kcalloc(mydevice->config.engines,sizeof(*mydevice->engine),GFP_KERNEL);
	if (!mydevice->engine) {
		printk(KERN_INFO "Module mycrypto: failed to allocate data structure for engine\n");
		ret = -ENOMEM;
		return ret;
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
	// Set up timer (using for testing).
	timer_setup(&mydevice->mycrypto_ktimer,handle_timer,0);
	// printk(KERN_INFO "mydevice pointer: %px \n",mydevice);
	// printk(KERN_INFO "mydevice_glo pointer: %px \n",mydevice_glo);
	// printk(KERN_INFO "VALUE OF flags: %d \n", mydevice->flags);
	// printk(KERN_INFO "buffer stores request (in mydevice):%px \n", &mydevice->buffer);
	// printk(KERN_INFO "req of mydevice:%px \n",&mydevice->req);
	// printk(KERN_INFO "backlof req of mydevice:%px \n",&mydevice->backlog);

	//mod_timer(&mydevice->mycrypto_ktimer, jiffies + 2*HZ);
	return 0;
err_reg_clk:
	printk(KERN_INFO "ERROR REG_CLK AND WORKQUEUE");
	return 0;
}
//entry point when driver was loaded
static int __init FPGAcrypt_init(void) 
{
 //struct mycrypto_dev *mydevice;
 // Register probe
 
	printk(KERN_INFO "Hello, World!\n");
 //probe with simulation
	mycrypto_probe();
	//mod_timer(&mydevice_glo->mycrypto_ktimer, jiffies + 4*HZ);
//  //-------init kernel timer--------------//
// 	init_timers(&mycrypto_ktimer);
// 	configure_timer(&mycrypto_ktimer);

// 	// -- TIMER START
// 	add_timer(&mycrypto_ktimer);

 return 0;
}

//entry point when driver was remove
static void __exit FPGAcrypt_exit(void) 
{
	mycrypto_remove_algs(mydevice_glo);
	kfree(mydevice_glo);
	flush_workqueue(mydevice_glo->workqueue);
    destroy_workqueue(mydevice_glo->workqueue);
	//-----------delete timer------------------
	del_timer_sync(&mydevice_glo->mycrypto_ktimer);
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
//module_pci_driver(geode_aes_driver)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Duy H.Ho");
MODULE_DESCRIPTION("A prototype Linux module for crypto in FPGA-PCIE card");
MODULE_VERSION("0.01");