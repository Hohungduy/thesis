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

#include "testcrypto.h"

struct tcrypt_result {
    struct completion completion;
    int err;
};
/* tie all data structures together */
struct skcipher_def {
    struct scatterlist sg;
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct tcrypt_result result;
    //struct crypto_wait wait;
};
struct aead_def {
    struct scatterlist *sg;
    struct crypto_aead *tfm;
    struct aead_request *req;
    struct tcrypt_result result;
};

/* Callback function for skcipher type*/
static void test_skcipher_cb(struct crypto_async_request *req, int error)
{
    struct tcrypt_result *result = req->data;
    printk(KERN_INFO "Module testcrypto: STARTING test_skcipher_cb\n");
    if (error == -EINPROGRESS)
        return;
    result->err = error;
    complete(&result->completion);
    printk(KERN_INFO "Module testcrypto:Encryption finished successfully\n");
}

/* Perform skcipher cipher operation */
static unsigned int test_skcipher_encdec(struct skcipher_def *sk, int enc)
{
    int rc;
    printk(KERN_INFO "Module testcrypto: STARTING test_skcipher_encdec\n");
    // if (enc)
    //     rc = crypto_wait_req(crypto_skcipher_encrypt(sk->req), &sk->wait);
    // else
    //     rc = crypto_wait_req(crypto_skcipher_decrypt(sk->req), &sk->wait);
    if (enc)
        rc = crypto_skcipher_encrypt(sk->req);
    else
        rc = crypto_skcipher_decrypt(sk->req);

    switch (rc) 
    {
    case 0:
        break;
    case -EINPROGRESS:
    case -EBUSY:
        rc = wait_for_completion_interruptible(
            &sk->result.completion);
        if (!rc && !sk->result.err) {
            reinit_completion(&sk->result.completion);
            break;
        }
    default:
        pr_info("Module testcrypto: skcipher encrypt returned with %d result %d\n", rc, sk->result.err);
        break;
    }
    init_completion(&sk->result.completion);
    // if (rc)
    //         printk(KERN_INFO "Module testcrypto: skcipher encrypt returned with result %d\n", rc);

    printk(KERN_INFO "Module testcrypto: skcipher encrypt returned with result %d\n", rc);
    return rc;
}


/* Initialize and trigger cipher operation */
static int test_skcipher(void)
{
    struct skcipher_def sk;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    struct crypto_async_request * async_req = NULL;
    char *scratchpad = NULL;
    char *ivdata = NULL;
    unsigned char key[16];
    char *scratchpad_copy,*ivdata_copy;
    int i_data,i_iv;
    int assoclen;
    int ivlen;
    int ret = -EFAULT;
    size_t len;
    /*
    * Allocate a tfm (a transformation object) and set the key.
    *
    * In real-world use, a tfm and key are typically used for many
    * encryption/decryption operations.  But in this example, we'll just do a
    * single encryption operation with it (which is not very efficient).
     */
    printk(KERN_INFO "Module testcrypto: starting test_skcipher1\n");
    skcipher = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(skcipher)) {
        pr_info("could not allocate skcipher handle\n");
        return PTR_ERR(skcipher);
    }
    /* Allocate a request object */
    req = skcipher_request_alloc(skcipher, GFP_KERNEL);
    if (!req) {
        pr_info("could not allocate skcipher request\n");
        ret = -ENOMEM;
        goto out;
    }

    skcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                      test_skcipher_cb,
                      &sk.result);

    /* AES 128 with random key - 16 octetcs */
    get_random_bytes(&key, 16);
    
    if (crypto_skcipher_setkey(skcipher, key, 16)) {
        pr_info("key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }
    printk(KERN_INFO "Key value - after setkey:\n");
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[0],key[1],key[2],key[3]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[4],key[5],key[6],key[7]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[8],key[9],key[10],key[11]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[12],key[13],key[14],key[15]);
    /* IV will be random */
    ivlen =16;
    ivdata = kmalloc(ivlen, GFP_KERNEL);
    if (!ivdata) {
        pr_info("could not allocate ivdata\n");
        goto out;
    }
    get_random_bytes(ivdata, ivlen);
    ivdata_copy = ivdata; // copy pointer
    printk(KERN_INFO "IV: \n");
    //printk(KERN_INFO "VALUE OF POINTER _ORIGINAL1: %px\n",ivdata_copy);
    for(i_iv = 1; i_iv <= ivlen; i_iv++)
    {
        printk(KERN_INFO "%x \t", (*ivdata_copy));
        ivdata_copy ++;
        if ((i_iv % 4) == 0)
            printk(KERN_INFO "\n");
    }
    /* Input data will be random */
    scratchpad = kmalloc(16, GFP_KERNEL);

    if (!scratchpad) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }

    get_random_bytes(scratchpad, 16);
    scratchpad_copy = scratchpad; 
    printk(KERN_INFO "Data payload :\n");
    for(i_data = 1; i_data <= 16; i_data ++)
    {
        printk(KERN_INFO "%x \t", (*scratchpad_copy));
        scratchpad_copy ++;
        if ((i_data % 4) == 0)
            printk(KERN_INFO "\n");
    }
    sk.tfm = skcipher;
    sk.req = req;
    /*
     * Encrypt the data in-place.
     *
     * For simplicity, in this example we wait for the request to complete
     * before proceeding, even if the underlying implementation is asynchronous.
     *
     * To decrypt instead of encrypt, just change crypto_skcipher_encrypt() to
     * crypto_skcipher_decrypt().
     */

    /* We encrypt one block */
    sg_init_one(&sk.sg, scratchpad, 16);
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, 16, ivdata);
    // crypto_init_wait(&sk.wait);
    init_completion(&sk.result.completion);

    /* encrypt data ( 1 for encryption)*/
    ret = test_skcipher_encdec(&sk, 1);
    //printk(KERN_INFO "starting test_skcipher2(ret): %d\n", ret);
    if (ret){
        pr_err("Error encrypting data: %d\n", ret);
        goto out;
    
    }
    //async_req = &sk.req->base;
    
    //scratchpad_copy = (char*)async_req->data;
    printk(KERN_INFO "Data payload-after function test encrypt:\n");
    len = (size_t)sk.req->cryptlen;
    len =sg_pcopy_to_buffer(sk.req->src,1,scratchpad_copy,len,0);
    //len =sg_pcopy_from_buffer(&sk.sg,1,scratchpad_copy,len,0);

    for(i_data = 1; i_data <= 16; i_data ++)
    {
        printk(KERN_INFO "%x \t", (*scratchpad_copy));
        scratchpad_copy ++;
        if ((i_data % 4) == 0)
            printk(KERN_INFO "\n");
    }

    // pr_info("Encryption triggered successfully\n");
    printk(KERN_INFO "Module Testcrypto: Encryption triggered successfully\n");

out:
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);
    if (ivdata)
        kfree(ivdata);
    if (scratchpad)
        kfree(scratchpad);
    return ret;
}

//---------------------------------------------------------------------------------------------


/* Callback function for aead type */
static void test_aead_cb(struct crypto_async_request *req, int error)
{
    struct tcrypt_result *result = req->data;
    printk(KERN_INFO "Module testcrypto: STARTING test_AEAD_cb\n");
    if(error == -EINPROGRESS)
        return;
    result->err = error;
    complete(&result->completion);
    printk(KERN_INFO "Module testcrypto: Encryption finishes successfully\n");
}

/* Perform aead cipher operation */
static unsigned int test_aead_encdec(struct aead_def *ad, int enc)
{
    int rc;
    printk(KERN_INFO "Module testcrypto: STARTING test_aead_encdec\n");
    // if (enc)
    //     rc = crypto_wait_req(crypto_skcipher_encrypt(sk->req), &sk->wait);
    // else
    //     rc = crypto_wait_req(crypto_skcipher_decrypt(sk->req), &sk->wait);
    if (enc)
        rc = crypto_aead_encrypt(ad->req);
    else
        rc = crypto_aead_decrypt(ad->req);
    printk(KERN_INFO "AFTER crypto_aead_encrypt\n");
    switch (rc) 
    {
    case 0:
        break;
    case -EINPROGRESS:
    case -EBUSY:
        rc = wait_for_completion_interruptible(
            &ad->result.completion);
        if (!rc && !ad->result.err) {
            reinit_completion(&ad->result.completion);
            break;
        }
    default:
        pr_info("Module testcrypto: aead encrypt returned with %d result %d\n",
            rc, ad->result.err);
        break;
    }
    init_completion(&ad->result.completion);
    // if (rc)
    //         printk(KERN_INFO "Module testcrypto: skcipher encrypt returned with result %d\n", rc);

    
    printk(KERN_INFO "Module testcrypto: aead encrypt returned with result %d\n", rc);
    return rc;
}

/* Initialize and trigger aead cipher operation */
static int test_aead(void)
{
    struct aead_def ad;
    struct crypto_aead *aead = NULL;
    struct aead_request *req = NULL;
    char *scratchpad = NULL;
    char *AAD = NULL;
    char *ivdata = NULL;
    unsigned char key[16];
    int ret = -EFAULT;
    int assoclen;
    int authlen, ivlen;
    char *ivdata_copy = NULL; // for printing
    char *AAD_copy = NULL; //for printing
    char *scratchpad_copy = NULL;//for printing
    char *sg_buffer =NULL; //for printing
    int i_iv,i_AAD,i_data,i_sg;
    size_t len;
    /*
    * Allocate a tfm (a transformation object) and set the key.
    *
    * In real-world use, a tfm and key are typically used for many
    * encryption/decryption operations.  But in this example, we'll just do a
    * single encryption operation with it (which is not very efficient).
     */
    printk(KERN_INFO "Module testcrypto: starting test_aead\n");
    aead = crypto_alloc_aead("gcm(aes)", 0, 0);
    if (IS_ERR(aead)) {
        pr_info("could not allocate aead handle\n");
        return PTR_ERR(aead);
    }
    /* Allocate a request object */
    req = aead_request_alloc(aead, GFP_KERNEL);
    if (!req) {
        pr_info("could not allocate aead request\n");
        ret = -ENOMEM;
        goto out;
    }

    /*Check the size of iv and authentication data (Tag for authentication)  */
    authlen = crypto_aead_authsize(aead);
    ivlen = crypto_aead_ivsize(aead);
    printk(KERN_INFO "authentication size: %d\n", authlen);
    printk(KERN_INFO "iv size:%d \n", ivlen);
    aead_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                      test_aead_cb,
                      &ad.result);

    /* AES 128 with random key */
    get_random_bytes(&key, 16);
    printk(KERN_INFO "Key value - before setkey: \n");
    printk(KERN_INFO "%d\t%d\t%d\t%d \n",key[0],key[1],key[2],key[3]);
    printk(KERN_INFO "%d\t%d\t%d\t%d \n",key[4],key[5],key[6],key[7]);
    printk(KERN_INFO "%d\t%d\t%d\t%d \n",key[8],key[9],key[10],key[11]);
    printk(KERN_INFO "%d\t%d\t%d\t%d \n",key[12],key[13],key[14],key[15]);
    //printk(KERN_INFO "%d \n", key);
    //print_hex_dump_bytes("", DUMP_PREFIX_NONE,key,ARRAY_SIZE(key));
    if (crypto_aead_setkey(aead, key, 16)) {
        pr_info("key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }
    printk(KERN_INFO "Key value - after setkey:\n");
    printk(KERN_INFO "%d\t%d\t%d\t%d \n",key[0],key[1],key[2],key[3]);
    printk(KERN_INFO "%d\t%d\t%d\t%d \n",key[4],key[5],key[6],key[7]);
    printk(KERN_INFO "%d\t%d\t%d\t%d \n",key[8],key[9],key[10],key[11]);
    printk(KERN_INFO "%d\t%d\t%d\t%d \n",key[12],key[13],key[14],key[15]);
    //print_hex_dump_bytes("", DUMP_PREFIX_NONE,key,ARRAY_SIZE(key));
    //printk(KERN_INFO "%d \n", key);
    /* IV will be random */
    ivdata = kzalloc(ivlen, GFP_KERNEL);
    if (!ivdata) {
        pr_info("could not allocate ivdata\n");
        goto out;
    }
    get_random_bytes(ivdata, 12);
    
    ivdata_copy = ivdata; // copy pointer
    printk(KERN_INFO "IV: \n");
    //printk(KERN_INFO "VALUE OF POINTER _ORIGINAL1: %px\n",ivdata_copy);
    for(i_iv = 1; i_iv <= ivlen; i_iv++)
    {
        printk(KERN_INFO "%d \t", (*ivdata_copy));
        ivdata_copy ++;
        if ((i_iv % 4) == 0)
            printk(KERN_INFO "\n");
    }
    //printk(KERN_INFO "VALUE OF POINTER _after: %px\n",ivdata_copy);
    //printk(KERN_INFO "VALUE OF POINTER _ORIGINAL2: %px\n",ivdata);

    /* AAD value (comprise of SPI and Sequence number) */
    AAD = kzalloc(8, GFP_KERNEL);
    if (!AAD) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }
    get_random_bytes(AAD,8);
    assoclen = 8;
    AAD_copy =AAD;
    printk(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    
    for(i_AAD = 1; i_AAD <= 8; i_AAD ++)
    {
        printk(KERN_INFO "%d \t", (*AAD_copy));
        AAD_copy ++;
        if ((i_AAD % 4) == 0)
            printk(KERN_INFO "\n");
    }
    
    /* Input data will be random */
    scratchpad = kzalloc(320, GFP_KERNEL);
    if (!scratchpad) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }
    get_random_bytes(scratchpad, 16);
    scratchpad_copy = scratchpad; 
    printk(KERN_INFO "Data payload :\n");
    for(i_data = 1; i_data <= 16; i_data ++)
    {
        printk(KERN_INFO "%d \t", (*scratchpad_copy));
        scratchpad_copy ++;
        if ((i_data % 4) == 0)
            printk(KERN_INFO "\n");
    }

    ad.tfm = aead;
    ad.req = req;
    printk(KERN_INFO "BEFORE allocate data and AAD into sg list of request\n");
    /*
     * Encrypt the data in-place.
     *
     * For simplicity, in this example we wait for the request to complete
     * before proceeding, even if the underlying implementation is asynchronous.
     *
     * To decrypt instead of encrypt, just change crypto_aead_encrypt() to
     * crypto_aead_decrypt() by using parameter enc.
     *  if enc = 1 , we do encrypt
     *  while enc = 0, we do decrypt.
     */

    /* We encrypt one block */
    // initiate one sg entry list
    //sg_init_one(&ad.sg, scratchpad, 16);
    // inituate one sgtable list

    ad.sg = kmalloc_array(2,sizeof(struct scatterlist),GFP_KERNEL);// 2 entries
    if(!ad.sg){
        ret = -ENOMEM;
        goto out;
    }
    sg_buffer =kzalloc(40, GFP_KERNEL);
    sg_init_table(ad.sg, 2 ); // 2 entries
    sg_set_buf(&ad.sg[0], AAD, 8);
    //ad.sg = sg_next(ad.sg);
    //printk(KERN_INFO "sg_next \n");
    sg_set_buf(&ad.sg[1],scratchpad,320);
    //sg_mark_end(ad.sg);
    aead_request_set_ad(req,assoclen);// 12 means that there are 12 octects ( 96 bits) in AAD fields.
    aead_request_set_crypt(req, ad.sg, ad.sg, 16, ivdata);// 28 bytes = 12 bytes (IV) + 16 bytes (data) 
    // crypto_init_wait(&sk.wait);
    init_completion(&ad.result.completion);

    printk(KERN_INFO "ASSOCLEN:%d\n", req->assoclen);
    printk(KERN_INFO "CRYPTLEN:%d\n", req->cryptlen);
    printk(KERN_INFO "LENGTH OF SCATTERLIST : %d \n", ad.sg->length);
    printk(KERN_INFO "LENGTH OF SCATTERLIST 0: %d \n", ad.sg[0].length);
    printk(KERN_INFO "LENGTH OF SCATTERLIST 1: %d \n", ad.sg[1].length);
    printk(KERN_INFO "length of scatterlist 2: %d \n", ad.sg[2].length);
    printk(KERN_INFO "lenght of scatterlist(src 0): %d\n",req->src[0].length);
    printk(KERN_INFO "length of scatterlist(src 1): %d \n",req->src[1].length);
    printk(KERN_INFO "length of scatterlist(src 2): %d \n",req->src[2].length);
    printk(KERN_INFO "length of scatterlist(src):%d \n",req->src->length);
    printk(KERN_INFO "length of scatterlist(dst 0): %d \n",req->dst[0].length);
    printk(KERN_INFO "length of scatterlist(dst 1): %d \n",req->dst[1].length);
    printk(KERN_INFO "length of scatterlist(dst 2): %d \n",req->dst[2].length);
    printk(KERN_INFO "length of scatterlist (dst):%d \n",req->dst->length);
    len = (size_t)ad.req->cryptlen + (size_t)ad.req->assoclen + (size_t)authlen;
    //len =  (size_t)ad.req->assoclen;
    printk(KERN_INFO "LENGHTH FOR ASSOC+ CRYPT: %d \n", len);
    printk(KERN_INFO "Data payload-before function test encrypt:\n");
    len =sg_pcopy_to_buffer(ad.req->src,2,sg_buffer,len,0);
    //len =sg_pcopy_from_buffer(&sk.sg,1,scratchpad_copy,len,0);

    for(i_sg = 1; i_sg <= (req->assoclen +req->cryptlen+authlen); i_sg ++)
    {
        printk(KERN_INFO "%d \t", (*sg_buffer));
        sg_buffer ++;
        if ((i_sg % 4) == 0)
            printk(KERN_INFO "\n");
    }

    /* encrypt data ( 1 for encryption)*/

    ret = test_aead_encdec(&ad, 1);
    //printk(KERN_INFO "starting test_skcipher2(ret): %d\n", ret);
    if (ret){
        pr_err("Error encrypting data: %d\n", ret);
        goto out;
    }
    // pr_info("Encryption triggered successfully\n");
            
    len = (size_t)ad.req->cryptlen + (size_t)ad.req->assoclen+(size_t)authlen;
    len =sg_pcopy_to_buffer(ad.req->src,2,sg_buffer,len,0);
    //len =sg_pcopy_from_buffer(&sk.sg,1,scratchpad_copy,len,0);
    printk(KERN_INFO "Data payload-after function test encrypt:\n");
    for(i_sg = 1; i_sg <= (req->assoclen +req->cryptlen+authlen); i_sg ++)
    {
        printk(KERN_INFO "%d \t", (*sg_buffer));
        sg_buffer ++;
        if ((i_sg % 4) == 0)
            printk(KERN_INFO "\n");
    }
    printk(KERN_INFO "Module Testcrypto: Encryption triggered successfully\n");
out:
    if (aead)
        crypto_free_aead(aead);
    if (req)
        aead_request_free(req);
    if (ivdata)
        kfree(ivdata);
    if (AAD)
        kfree(AAD);
    if (scratchpad)
        kfree(scratchpad);
    return ret;
}

static int __init test_init(void)
{
    printk(KERN_INFO "Module testcrypto: info: init test\n");
    test_aead();
    return 0;
}

static void __exit test_exit(void)
{
    printk(KERN_INFO "Module testcrypto: info: exit test\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Duy H.Ho");
MODULE_DESCRIPTION("A prototype Linux module for TESTING crypto in FPGA-PCIE card");
MODULE_VERSION("0.01");