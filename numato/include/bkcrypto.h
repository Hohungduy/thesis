#ifndef _BKCRYPTO_H_
#define _BKCRYPTO_H_

#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/skcipher.h>
#include <crypto/internal/aead.h>
#include <linux/scatterlist.h>
#include <linux/highmem.h>
#include <linux/crypto.h>
//#include "common.h"
#define BUFFER_SIZE 128

#define KEYSIZE_128 (0)
#define KEYSIZE_192 (1)
#define KEYSIZE_256 (2)
#define AADSIZE_8 (8)
#define AADSIZE_12 (12)
#define DIRECTION_ENCRYPT (1)
#define DIRECTION_DECRYPT (0)
#define ICV_SIZE (4*4)
#define KEY_SIZE (8*4)
#define AAD_SIZE (4*4)
//length of key field in region memory
#define KEYMEM (32)
// Type of algorithms registered
enum bkcrypto_alg_type {
	BKCRYPTO_ALG_TYPE_SKCIPHER,
	BKCRYPTO_ALG_TYPE_AEAD,
	BKCRYPTO_ALG_TYPE_AHASH,
};

// Template for specific transformation.
struct bkcrypto_alg_template {
	struct bkcrypto_dev *mydevice;
	enum bkcrypto_alg_type type;
	union {
		struct skcipher_alg skcipher;
		struct aead_alg aead;
		struct ahash_alg ahash;
	} alg;
};

/* Prameters for Result data desciptor */
/* Discuss with Long later */
// 
struct bkcrypto_result_data{
	u32 packet_length:17;
	u32 error_code:15;
	u8 hash_bytes:1;
	u8 hash_length:6;
	u8 checksum:1;
	u8 des_overflow;
	u8 buffer_overflow;

};	

//Testing: Parameter for configuring the engine.
struct bkcrypto_config {
	u32 engines;
	u32 cd_size;
	u32 cd_offset;
	u32 rd_size;
	u32 rd_offset;
};

// Work data for workqueue
struct bkcrypto_work_data {
	struct work_struct work;
	struct bkcrypto_dev *mydevice;
};

// Testing: used for pcie layer
struct bkcrypto_desc_ring {
	void *base;
	void *base_end;
	dma_addr_t base_dma;
	/* write and read pointers */
	void *write;
	void *read;
	/* descriptor element offset */
	unsigned offset;
};

struct bkcrypto_engine{
	spinlock_t lock;
	
	/* command/result rings */
	struct bkcrypto_desc_ring cdr;//command
	struct bkcrypto_desc_ring rdr;//result
	/* Store for current requests when bailing out of the dequeueing
	 * function when no enough resources are available.
	 */
	struct crypto_async_request *req;// filled in dequeue
	struct crypto_async_request *backlog;// filled in dequeue
	/*need   work queue or we can use dequeue function in enqueue function*/
	struct workqueue_struct *workqueue;
	struct bkcrypto_work_data work_data;
	/* use for callback function*/
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
 * struct bkcrypto_dev
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
struct bkcrypto_dev{
    struct pci_dev *pdev; // do it later
	void __iomem *regs; // not used
    void __iomem *bar[3]; // not used
    void __iomem *sram; // not used
	struct device *dev;// not used

    struct bkcrypto_config config;
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
	struct bkcrypto_work_data work_data;
	/* use for callback function*/
	/* store request in crypto queue*/
	struct crypto_queue	queue;
	spinlock_t queue_lock;
	// head node for containing processed requests.
	struct list_head complete_queue; 
    struct list_head	alg_list;
	atomic_t engine_used; 
	/* number of engine used (set in probe, modify and 
	increase in queue request function(after encrypting/decrypting) */
	struct bkcrypto_engine *engine;
	
};
/**
 * struct bkcrypto_req_operations - request operations
 * @handle_request:	launch the crypto operation on the next chunk ((should return 0 if the
 *		operation, -EINPROGRESS if it needs more steps or an error
 *		code))
 * @handle_result:complete the request, i.e copy result or context from device when needed 
 * then cleanup the. check error code and theen clean up the crypto request, then retrieve call-back
 * function
 */
struct bkcrypto_req_operation {
	
	int (*handle_request)(struct crypto_async_request *req);
	int (*handle_result)(struct crypto_async_request *req,bool *should_complete);
	
};
// void bkcrypto_dequeue_req(struct bkcrypto_dev *mydevice);
int bkcrypto_dequeue_req(struct bkcrypto_dev *mydevice);

/* available algorithms */
// extern struct aead_alg my_crypto_gcm_aes_alg;

extern struct bkcrypto_alg_template bkcrypto_alg_gcm_aes;
// extern struct bkcrypto_alg_template bkcrypto_alg_cbc_aes;// for futher development
extern struct bkcrypto_alg_template bkcrypto_alg_rfc4106_gcm_aes;

#endif