# ifndef _CIPHER_H_
# define _CIPHER_H_

#include <crypto/aes.h>
#include <crypto/gcm.h>
#include <crypto/des.h>
#include <crypto/aead.h>
#include <crypto/internal/aead.h>
#include <crypto/sha.h>
#include <crypto/authenc.h>

/* driver logic flags */
#define AES_MODE_CBC 0
#define AES_MODE_GCM 1
#define AES_MODE_AUTHENC_HMAC_CBC 2
#define AES_MODE_AUTHENC_HMAC_CTR 3

#define MYCRYPTO_DIR_DECRYPT 0
#define MYCRYPTO_DIR_ENCRYPT 1
static int mycrypto_skcipher_handle_request(struct crypto_async_request *base);
static int mycrypto_skcipher_handle_result(struct crypto_async_request *base,bool *should_complete);

/* transformation object context
* it is stored in tfm ->__crt_ctx
* and tfm = req->base.tfm 
*
*/

//base: filled in cra_init
//mydevice: filled in cra_init
//src/dst:
//dir: filled mycrypto_queue_req
//flags:
//mode: filled in mycrypto_queue_req
//len : filled in mycrypto_queue_req
//key: filled in setkey
//keylen: filled in setkey
//iv filled in mycrypto_queue_req

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
// dir: filled in mycrypto_queue_req
// src_nents + dst_nents: filled in mycrypto_skcipher_req_init

struct mycrypto_cipher_req{
	u32 dir; // direction ( encrypt or decrypt)
	int src_nents;
	int dst_nents;
};
#endif