#ifndef _COMMON_
#define _COMMON_
#include "mycrypto.h"
#include "cipher.h"
/**
 * struct mycrypto_dev
 * @pdev:       infor pci device
 * @lock:        lock key uses for spin lock
 * @flags:
 * @req:		current crypto request
 * @queue:		fifo of the pending crypto requests
 * @complete_queue:	fifo of the processed requests by the engine
 *
 * Structure storing CESA engine information.
 */
struct mycrypto_dev{
    	struct pci_dev *pdev; // do it late
    	struct mycrypto_config config;
    	u32			flags;
	char *buffer; // using for testing
	/* Store for current requests when bailing out of the dequeueing
	 * function when no enough resources are available.
	 */
	struct crypto_async_request *req;// filled in dequeue
	struct crypto_async_request *backlog;// filled in dequeue
	/*need   work queue or we can use dequeue function in enqueue function*/
	struct workqueue_struct *workqueue;
	struct mycrypto_work_data work_data;
	/* use for callback function*/
    	//struct tasklet_struct tasklet;
	/* store request in crypto queue*/
	struct crypto_queue	queue;
	spinlock_t queue_lock;
	// head node for containing processed requests.
	struct list_head complete_queue; 
    	struct list_head	alg_list;
	struct timer_list mycrypto_ktimer;
	atomic_t engine_used; 
	/* number of engine used (set in probe, modify and 
	increase in queue request function(after encrypting/decrypting) */
	//struct mycrypto_engine *engine;
	
};
/**
 * struct mycrypto_req_operations - request operations
 * @handle_request:	launch the crypto operation on the next chunk ((should return 0 if the
 *		operation, -EINPROGRESS if it needs more steps or an error
 *		code))
 * @handle_result:complete the request, i.e copy result or context from device when needed 
 * then cleanup the. check error code and theen clean up the crypto request, then retrieve call-back
 * function
 */
struct mycrypto_req_operation {
	
	int (*handle_request)(struct crypto_async_request *req);
	int (*handle_result)(struct crypto_async_request *req,bool *should_complete);
	
};
/* transformation object context
* it is stored in tfm ->__crt_ctx
* and tfm = req->base.tfm 
*
*/
struct mycrypto_cipher_op{
	struct mycrypto_req_operation base;
	struct mycrypto_dev *mydevice;
	void *src;// src data -before operation
	void *dst;// dest data -after operation
	u32 dir; // direction
	u32 flags;
	u32 mode;//algoritm used
	int len;// blocksize
	u8 key[AES_KEYSIZE_128];// key
	u8 *iv; //iv pointer
	u32 keylen;//keylen
	/* all the belows is using for AEAD specific*/
	u32 hash_alg;
	u32 state_sz;
	__be32 ipad[SHA512_DIGEST_SIZE / sizeof(u32)];
	__be32 opad[SHA512_DIGEST_SIZE / sizeof(u32)];

	/* use for gcm */
	struct crypto_cipher *hkaes;// transformation object
};

/*
 *  struct mycrypto_cipher_req -- cipher request ctx 
 *  which is stored in req ->_ctx
 * 	@dir: The direction of processing ( encrypt or decrypt)
 * 	@src_nents:	number of entries in the src sg list
 * 	@dst_nents:	number of entries in the dest sg list
*/

struct mycrypto_cipher_req{
	u32 dir; // direction ( encrypt or decrypt)
	int src_nents;
	int dst_nents;
};

/* struct mycrypto_context
*  -  @mycrypto_cipher_op: contains the transformation objects 
*   (it is stored in tfm ->__crt_ctx and tfm = req->base.tfm )
*  -  @mycrypto_cipher_req: contains the context of request 
*   which is stored in req ->_ctx
*  - error_code: contains bits which represents for error code.

*/
struct mycrypto_context{
    struct mycrypto_cipher_op *ctx; 
    struct mycrypto_cipher_req *req_ctx;
    u32 error_code:7;
};


//debug
#endif
