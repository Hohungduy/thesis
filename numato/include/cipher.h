# ifndef _CIPHER_H_
# define _CIPHER_H_

#include <crypto/aes.h>
#include <crypto/gcm.h>
#include <crypto/ctr.h>
#include <crypto/des.h>
#include <crypto/aead.h>
#include <crypto/internal/aead.h>
#include <crypto/sha.h>
#include <crypto/authenc.h>
#include <crypto/ghash.h>
#include "bkcrypto.h"
/* driver logic flags */
#define AES_MODE_CBC 0
#define AES_MODE_GCM 1
#define AES_MODE_AUTHENC_HMAC_CBC 2
#define AES_MODE_AUTHENC_HMAC_CTR 3
#define AUTHENC_MODE_GHASH 4

#define BKCRYPTO_DIR_DECRYPT 0
#define BKCRYPTO_DIR_ENCRYPT 1
int bkcrypto_skcipher_handle_request(struct crypto_async_request *base);
int bkcrypto_skcipher_handle_result(struct crypto_async_request *base,bool *should_complete);

int bkcrypto_aead_handle_request(struct crypto_async_request *base);
int bkcrypto_aead_handle_result(struct crypto_async_request *base,bool *should_complete);
/* transformation object context
* it is stored in tfm ->__crt_ctx
* and tfm = req->base.tfm 
*
*/

//base: filled in cra_init
//mydevice: filled in cra_init
//src/dst:
//dir: filled bkcrypto_queue_req
//flags:
//mode: filled in bkcrypto_queue_req
//len : filled in bkcrypto_queue_req
//key: filled in setkey
//keylen: filled in setkey
//iv filled in bkcrypto_queue_req

struct bkcrypto_cipher_op{
	struct bkcrypto_req_operation base;
	struct bkcrypto_dev *mydevice;
	void *src;// src data -before operation
	void *dst;// dest data -after operation
	u32 dir; // direction
	u32 flags;
	u32 mode;//algoritm used
	int len;// blocksize
	u32 key[AES_MAX_KEYLENGTH_U32];// key
	//u8 key[AES_KEYSIZE_128];// key
	u32 nonce; // store nonce (4 Bytes salt)
	u8 *iv; //iv pointer (8 Bytes iv)
	u32 keylen;//keylen

	unsigned int assoclen;
	unsigned int cryptlen;
	unsigned int authsize; // another name: digestsize
	
	/* all the belows is using for AEAD specific*/
	//u32 hash_alg;
	//u32 state_sz;
	//__be32 ipad[SHA512_DIGEST_SIZE / sizeof(u32)];
	//__be32 opad[SHA512_DIGEST_SIZE / sizeof(u32)];

	/* use for gcm */
	// struct crypto_cipher *hkaes;// transformation object
};

/*
 *  struct bkcrypto_cipher_req -- cipher request ctx 
 *  which is stored in req ->_ctx
 * 	@dir: The direction of processing ( encrypt or decrypt)
 * 	@src_nents:	number of entries in the src sg list
 * 	@dst_nents:	number of entries in the dest sg list
*/
// dir: filled in bkcrypto_queue_req
// src_nents + dst_nents: filled in bkcrypto_skcipher_req_init

struct bkcrypto_cipher_req{
	u32 dir; // direction ( encrypt or decrypt)
	int src_nents;
	int dst_nents;
};


#endif
