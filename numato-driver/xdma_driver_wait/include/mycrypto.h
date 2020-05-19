# ifndef _MYCRYPTO_H_
#define _MYCRYPTO_H_

#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/skcipher.h>
#include <crypto/internal/aead.h>
#include <linux/scatterlist.h>
#include <linux/highmem.h>
#include <linux/crypto.h>
#include "common.h"
#define BUFFER_SIZE 128

// Type of algorithms registered
enum mycrypto_alg_type {
	MYCRYPTO_ALG_TYPE_SKCIPHER,
	MYCRYPTO_ALG_TYPE_AEAD,
	MYCRYPTO_ALG_TYPE_AHASH,
};

// Template for specific transformation.
struct mycrypto_alg_template {
	struct mycrypto_dev *mydevice;
	enum mycrypto_alg_type type;
	union {
		struct skcipher_alg skcipher;
		struct aead_alg aead;
		struct ahash_alg ahash;
	} alg;
};

/* Prameters for Result data desciptor */
/* Discuss with Long later */
// 
struct mycrypto_result_data{
	u32 packet_length:17;
	u32 error_code:15;
	u8 hash_bytes:1;
	u8 hash_length:6;
	u8 checksum:1;
	u8 des_overflow;
	u8 buffer_overflow;

};	

//Testing: Parameter for configuring the engine.
struct mycrypto_config {
	u32 engines;
	u32 cd_size;
	u32 cd_offset;
	u32 rd_size;
	u32 rd_offset;
};

// Work data for workqueue
struct mycrypto_work_data {
	struct work_struct work;
	struct mycrypto_dev *mydevice;
};

// Testing: used for pcie layer
struct mycrypto_desc_ring {
	void *base;
	void *base_end;
	dma_addr_t base_dma;
	/* write and read pointers */
	void *write;
	void *read;
	/* descriptor element offset */
	unsigned offset;
};

struct mycrypto_engine{
	spinlock_t lock;
	
	/* command/result rings */
	struct mycrypto_desc_ring cdr;//command
	struct mycrypto_desc_ring rdr;//result
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

	/* Number of requests in the engine. */
	int requests;
	/* The ring is currently handling at least one request */
	bool busy;

	struct list_head complete_queue; 
};
/**
 * struct mycrypto_dev
 * @pdev:       infor pci device
 * @regs:		engine registers
 * @sram:		SRAM memory region
 * @sram_dma:   DMA address of the SRAM memory region
 * @lock:        lock key uses for spin lock
 * @clk: 
 * @regs_clk:
 * @flags:
 * @req:		current crypto request
 * @queue:		fifo of the pending crypto requests
 * @complete_queue:	fifo of the processed requests by the engine
 *
 * Structure storing CESA engine information.
 */
struct mycrypto_dev{
    struct pci_dev *pdev; // do it later
	void __iomem *regs; // not used
    void __iomem *bar[3]; // not used
    void __iomem *sram; // not used
	struct device *dev;// not used

    struct mycrypto_config config;
	struct clk *clk; // not used
	struct clk *reg_clk; // not used
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
	struct mycrypto_engine *engine;
	
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
void mycrypto_dequeue_req(struct mycrypto_dev *mydevice);
int mycrypto_check_errors(struct mycrypto_dev *mydevice, struct mycrypto_context *ctx);

/* available algorithms */
// extern struct aead_alg my_crypto_gcm_aes_alg;
extern struct mycrypto_alg_template mycrypto_alg_authenc_hmac_sha256_cbc_aes;
extern struct mycrypto_alg_template mycrypto_alg_gcm_aes;
extern struct mycrypto_alg_template mycrypto_alg_authenc_hmac_sha256_ctr_aes;
extern struct mycrypto_alg_template mycrypto_alg_cbc_aes;


#endif