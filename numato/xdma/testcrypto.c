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

static unsigned int req_num = 1;
module_param(req_num, uint, 0000);
MODULE_PARM_DESC(req_num, "request and number");

static unsigned int test_choice = 2;
module_param(test_choice, uint, 0000);
MODULE_PARM_DESC(test_choice, "Choose test case");

static unsigned int cipher_choice = 3;// 0 for cbc(skcipher), 1 for gcm(aead), 2 for gcm-esp(aead)
module_param(cipher_choice, uint, 0000);
MODULE_PARM_DESC(cipher_choice, "Choose cipher type: 0 for cbc, 1 for gcm, 2 for rfc4106");

static unsigned int endec = 1; // 1 for encrypt; 2 for decrypt
module_param(endec, uint, 0000);
MODULE_PARM_DESC(endec, "Choose mode: 1 for encrypt and 2 for decrypt");

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
void print_sg_content(struct scatterlist *sg ,size_t len)
{
	char *sg_buffer =NULL; //for printing
	int i;
	sg_buffer =kzalloc(len, GFP_KERNEL);
	len =sg_pcopy_to_buffer(sg,1,sg_buffer,len,0);
	//pr_info("%d: print sg in dequeue function",__LINE__);
    for(i = 0; i < (len); i +=16)
    {
		pr_err("Print sg: address = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i ,
            *((u32 *)(&sg_buffer[i + 12])), *((u32 *)(&sg_buffer[i + 8])), 
            *((u32 *)(&sg_buffer[i + 4])), *((u32 *)(&sg_buffer[i])));
    }
	kfree(sg_buffer);
}
void print_sg_virt_content(struct scatterlist *sg)
{
    unsigned int length;
    unsigned int i;
    char *buf;

    length = sg->length;
    buf = sg_virt (sg);

    for (i = 0; i < length; i += 16)
    {
        pr_err("Print virual sg: address = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i ,
            *((u32 *)(&buf[i + 12])), *((u32 *)(&buf[i + 8])), 
            *((u32 *)(&buf[i + 4])), *((u32 *)(&buf[i])));
    }
}
//--------------------------------------------------------------------------------
/* Test CBC mode -Type skcipher */
//-------------------------------------------------------------------------------
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

    printk(KERN_INFO "Module testcrypto: skcipher encrypt returned with result %d\n", rc);
    return rc;
}

/* Initialize and trigger cipher operation */
static int test_skcipher(void)
{
    struct skcipher_def sk;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    //struct crypto_async_request * async_req = NULL;
    char *scratchpad = NULL;
    char *ivdata = NULL;
    unsigned char key[16];
    char *scratchpad_copy,*ivdata_copy;
    int i_data,i_iv;
    //int assoclen;
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
     * crypto_skcipher_decrypt().ccc
     */

    /* We encrypt one block */
    sg_init_one(&sk.sg, scratchpad, 16);
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, 16, ivdata);
    // crypto_init_wait(&sk.wait);
    init_completion(&sk.result.completion);

    /* encrypt data ( 1 for encryption)*/
    ret = test_skcipher_encdec(&sk, 1);
    if (ret){
        pr_err("Error encrypting data: %d\n", ret);
        goto out;
    
    }
    printk(KERN_INFO "Data payload-after function test encrypt:\n");
    len = (size_t)sk.req->cryptlen;
    len =sg_pcopy_to_buffer(sk.req->src,1,scratchpad_copy,len,0);
    for(i_data = 1; i_data <= 16; i_data ++)
    {
        printk(KERN_INFO "%x \t", (*scratchpad_copy));
        scratchpad_copy ++;
        if ((i_data % 4) == 0)
            printk(KERN_INFO "\n");
    }
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
/* Test GCM-AES in normal mode - Type: AEAD cipher */
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
        if ((!rc) && (!ad->result.err)) {
            reinit_completion(&ad->result.completion);
        }
        break;
    default:
        pr_info("Module testcrypto: aead encrypt returned with %d result %d\n",
            rc, ad->result.err);
        break;
    }
    init_completion(&ad->result.completion);
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
    //char scratchpad[60];
    char *AAD = NULL;
    //char AAD[8]; 
    char *ivdata = NULL;
    //char ivdata[12];
    unsigned char key[16];
    int ret = -EFAULT;
    int assoclen;
    int authlen, ivlen;
    char *ivdata_copy = NULL; // for printing
    char *AAD_copy = NULL; //for printing
    char *scratchpad_copy = NULL;//for printing
    char *sg_buffer =NULL; //for printing
    char *sg_buffer_copy = NULL; //for printing
    int i_iv,i_AAD,i_data,i_sg,i_key;
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
    //get_random_bytes(&key, 16);
    for (i_key =0 ; i_key < 16;i_key++)
    {
        key[i_key]=key_const[i_key];
    }
    printk(KERN_INFO "Key value - before setkey: \n");
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[0],key[1],key[2],key[3]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[4],key[5],key[6],key[7]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[8],key[9],key[10],key[11]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[12],key[13],key[14],key[15]);
    if (crypto_aead_setkey(aead, key, 16)) {
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
    ivdata = kzalloc(ivlen, GFP_KERNEL);
    if (!ivdata) {
        pr_info("could not allocate ivdata\n");
        goto out;
    }
    //get_random_bytes(ivdata, 12);
    for (i_iv =0 ; i_iv < 12;i_iv++)
    {
        ivdata[i_iv]=ivdata_const[i_iv];
    }
    ivdata_copy = ivdata; // copy pointer
    printk(KERN_INFO "IV: \n");
    for(i_iv = 1; i_iv <= ivlen; i_iv++)
    {
        printk(KERN_INFO "%x \t", (*ivdata_copy));
        ivdata_copy ++;
        if ((i_iv % 4) == 0)
            printk(KERN_INFO "\n");
    }
    /* AAD value (comprise of SPI and Sequence number) */
    AAD = kzalloc(8, GFP_KERNEL);
    if (!AAD) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }
    //get_random_bytes(AAD,8);
    // 8 bytes
    //AAD[8] = {0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef};
    for (i_AAD = 0; i_AAD <8; i_AAD ++)
    {
        AAD[i_AAD] = AAD_const[i_AAD];
    }
    assoclen = 8;
    AAD_copy =AAD;
    printk(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    
    for(i_AAD = 1; i_AAD <= 8; i_AAD ++)
    {
        printk(KERN_INFO "%x \t", (*AAD_copy));
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
    //get_random_bytes(scratchpad, 16);
    // 60 bytes
    for (i_data =0 ; i_data < 60;i_data++)
    {
        scratchpad[i_data]=scratchpad_const[i_data];
    }

    scratchpad_copy = scratchpad; 
    printk(KERN_INFO "Data payload :\n");
    
    for(i_data = 1; i_data <= 60; i_data ++)
    {
        printk(KERN_INFO "%x \t", (*scratchpad_copy));
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
    // inituate one sgtable list

    ad.sg = kmalloc_array(2,sizeof(struct scatterlist),GFP_KERNEL);// 2 entries
    if(!ad.sg){
        ret = -ENOMEM;
        goto out;
    }
    sg_buffer =kzalloc(84, GFP_KERNEL);// 84 = 8 byte (AAD -1st entry) + 60 bytes data(2nd entry)+16 bytes (2nd entry)
    sg_init_table(ad.sg, 2 ); // 2 entries
    sg_set_buf(&ad.sg[0], AAD, 8);
    sg_set_buf(&ad.sg[1],scratchpad,320);
    aead_request_set_ad(req,assoclen);// 12 means that there are 12 octects ( 96 bits) in AAD fields.
    aead_request_set_crypt(req, ad.sg, ad.sg, 60, ivdata);// 60 bytes (data for encryption or decryption) 
    init_completion(&ad.result.completion);
    /* for printing */
    printk(KERN_INFO "ASSOCLEN:%d\n", req->assoclen);
    printk(KERN_INFO "CRYPTLEN:%d\n", req->cryptlen);
    printk(KERN_INFO "LENGTH OF SCATTERLIST : %d \n", ad.sg->length);
    printk(KERN_INFO "LENGTH OF SCATTERLIST 0: %d \n", ad.sg[0].length);
    printk(KERN_INFO "LENGTH OF SCATTERLIST 1: %d \n", ad.sg[1].length);
    // printk(KERN_INFO "length of scatterlist 2: %d \n", ad.sg[2].length);
    printk(KERN_INFO "lenght of scatterlist(src 0): %d\n",req->src[0].length);
    printk(KERN_INFO "length of scatterlist(src 1): %d \n",req->src[1].length);
    //printk(KERN_INFO "length of scatterlist(src 2): %d \n",req->src[2].length);
    printk(KERN_INFO "length of scatterlist(src):%d \n",req->src->length);
    printk(KERN_INFO "length of scatterlist(dst 0): %d \n",req->dst[0].length);
    printk(KERN_INFO "length of scatterlist(dst 1): %d \n",req->dst[1].length);
    // printk(KERN_INFO "length of scatterlist(dst 2): %d \n",req->dst[2].length);
    printk(KERN_INFO "length of scatterlist (dst):%d \n",req->dst->length);
    len = (size_t)ad.req->cryptlen + (size_t)ad.req->assoclen + (size_t)authlen;
    printk(KERN_INFO "LENGHTH FOR ASSOC+ CRYPT: %d \n", len);
    printk(KERN_INFO "Data payload-before function test encrypt:\n");
    len =sg_pcopy_to_buffer(ad.req->src,2,sg_buffer,len,0);
    sg_buffer_copy =sg_buffer;
    for(i_sg = 1; i_sg <= (req->assoclen +req->cryptlen+authlen); i_sg ++)
    {
        printk(KERN_INFO "%x \t", (*sg_buffer_copy));
        sg_buffer_copy ++;
        if ((i_sg % 4) == 0)
            printk(KERN_INFO "\n");
    }

    /* encrypt data ( 1 for encryption)*/
    ret = test_aead_encdec(&ad, 1);
    if (ret){
        pr_err("Error encrypting data: %d\n", ret);
        goto out;
    }
    len = (size_t)ad.req->cryptlen + (size_t)ad.req->assoclen+(size_t)authlen;
    len =sg_pcopy_to_buffer(ad.req->src,2,sg_buffer,len,0);
    printk(KERN_INFO "Data payload-after function test encrypt:\n");
    sg_buffer_copy =sg_buffer;
    for(i_sg = 1; i_sg <= (req->assoclen +req->cryptlen+authlen); i_sg ++)
    {
        printk(KERN_INFO "%x \t", (*sg_buffer_copy));
        sg_buffer_copy ++;
        if ((i_sg % 4) == 0)
            printk(KERN_INFO "\n");
    }
    printk(KERN_INFO "Module Testcrypto: Encryption triggered successfully\n");
out:
    if (aead)
        crypto_free_aead(aead);
    printk(KERN_INFO "aead after kfree:\n" );
    if (req)
        aead_request_free(req);
    printk(KERN_INFO "req after kfree:\n" );
    if (ivdata)
        kfree(ivdata);
    printk(KERN_INFO "ivdata after kfree:\n" );
    if (AAD)
        kfree(AAD);
    printk(KERN_INFO "AAD after kfree:\n" );
    if (scratchpad)
        kfree(scratchpad);
    printk(KERN_INFO "scratchpad after kfree:\n" );
    if(sg_buffer)
        kfree(sg_buffer);
    printk(KERN_INFO "sg_buffer after kfree:\n" );
    return ret;
}


//---------------------------------------------------------------------------------------------
/* Test GCM-AES in ESP mode (RFC 4106) - Type: AEAD cipher */
//---------------------------------------------------------------------------------------------
/* Callback function for aead type */

static void test_rfc4106_cb(struct crypto_async_request *req, int error)
{
    struct tcrypt_result *result = req->data;
    //struct aead_request *req = container_of(base, struct aead_request, base); // for test
    printk(KERN_INFO "Module testcrypto: STARTING test_rfc4106_cb\n");
    if(error == -EINPROGRESS)
        return;
    result->err = error;
    complete(&result->completion);
    printk(KERN_INFO "Module testcrypto: Encryption finishes successfully\n");
}

/* Perform aead cipher operation */
static unsigned int test_rfc4106_encdec(struct aead_def *ad, int enc)
{
    int rc;
    printk(KERN_INFO "Module testcrypto: STARTING test_rfc4106_encdec\n");

    if (enc)
        rc = crypto_aead_encrypt(ad->req);
    else
        rc = crypto_aead_decrypt(ad->req);
    printk(KERN_INFO "AFTER crypto_aead_encrypt\n");
    switch (rc) 
    {
    case 0:
        pr_info("Module testcrypto: Case 0 \n");
        break;
    case -EINPROGRESS:
    case -EBUSY:
        pr_info("Module testcrypto: Case einprogress + ebusy \n");
        rc = wait_for_completion_interruptible(
            &ad->result.completion);
        if ((!rc) && (!ad->result.err)) {
            reinit_completion(&ad->result.completion);
        }
        break;
    default:
        pr_info("Module testcrypto: aead encrypt returned with %d result %d\n",
            rc, ad->result.err);
        break;
    }
    init_completion(&ad->result.completion);
    pr_info("Module testcrypto:%p\n",&ad->result.completion);
    printk(KERN_INFO "Module testcrypto: aead encrypt returned with result %d\n", rc);
    return rc;
}

/* Initialize and trigger aead cipher operation */
static int test_rfc4106(int test_choice, int endec)
{
    struct aead_def ad;
    struct crypto_aead *aead = NULL;
    struct aead_request *req = NULL;
    char *scratchpad = NULL;
    //char scratchpad[60];
    char *AAD = NULL;
    //char AAD[8]; 
    char *ivdata = NULL;
    //char ivdata[12];
    unsigned char key[20];
    int ret = -EFAULT;
    unsigned int assoclen = 0;// AAD_length
    unsigned int authlen;// Tag_length
    unsigned int ivlen;//iv length
    unsigned int keylen = 0;//key length
    unsigned int data_len = 0;// scratchpad length
    char *ivdata_copy = NULL; // for printing
    char *AAD_copy = NULL; //for printing
    char *scratchpad_copy = NULL;//for printing
    char *sg_buffer =NULL; //for printing
    char *sg_buffer_copy = NULL; //for printing
    int i_iv,i_AAD,i_data,i_sg,i_key;// index for assign value into iv,AAD,data,key in loop
    unsigned int sg_buffer_len;
    size_t len;
    
    printk(KERN_INFO "Module testcrypto: starting test_rfc4106\n");
    switch(test_choice)
    {
        case 1:
            pr_info("This is test case 1\n");
            assoclen = test1_len.AAD_len;
            keylen = test1_len.key_len;
            data_len = test1_len.scratchpad_len;
            break;
            // do not need ivlen
        case 2:
            pr_info("This is test case 2\n");
            assoclen = test2_len.AAD_len;
            keylen = test2_len.key_len;
            data_len = test2_len.scratchpad_len;
            // do not need ivlen
            break;
        case 3:
            pr_info("This is test case 3\n");
            assoclen = test3_len.AAD_len;
            keylen = test3_len.key_len;
            data_len = test3_len.scratchpad_len;
            // do not need ivlen
            break;
        case 4:
            pr_info("This is test case 4\n");
            assoclen = test4_len.AAD_len;
            keylen = test4_len.key_len;
            data_len = test4_len.scratchpad_len;
            // do not need ivlen
            break;
        case 5:
            pr_info("This is test case 5\n");
            assoclen = test5_len.AAD_len;
            keylen = test5_len.key_len;
            data_len = test5_len.scratchpad_len;
            // do not need ivlen
            break;   
    }

    /*
    * Allocate a tfm (a transformation object) and set the key.
    *
    * In real-world use, a tfm and key are typically used for many
    * encryption/decryption operations.  But in this example, we'll just do a
    * single encryption operation with it (which is not very efficient).
     */
    
    aead = crypto_alloc_aead("rfc4106(gcm(aes))", 0, 0);
    if (IS_ERR(aead)) {
        pr_info("could not allocate aead handle (rfc 4106)\n");
        return PTR_ERR(aead);
    }
    /* Allocate a request object */
    req = aead_request_alloc(aead, GFP_KERNEL);
    if (!req) {
        pr_info("could not allocate aead request (rfc 4106)\n");
        ret = -ENOMEM;
        goto out;
    }

    /*Check the size of iv and authentication data (Tag for authentication)  */
    authlen = crypto_aead_authsize(aead);
    ivlen = crypto_aead_ivsize(aead);
    printk(KERN_INFO "authentication size: %d\n", authlen);
    printk(KERN_INFO "iv size:%d \n", ivlen);

    aead_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                            test_rfc4106_cb,
                            &ad.result); // for instruction set and hardware
    
    /* AES 128 with random key */
    //get_random_bytes(&key, 16);

    for (i_key =0 ; i_key < keylen;i_key++)
    {

        key[i_key]=key_const2[i_key];
    }
    
    if (crypto_aead_setkey(aead, key, 20)) {
        pr_info("key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }
    printk(KERN_INFO "Key value - after setkey:\n");
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[0],key[1],key[2],key[3]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[4],key[5],key[6],key[7]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[8],key[9],key[10],key[11]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[12],key[13],key[14],key[15]);
    printk(KERN_INFO "Salt in Nonce: %x\t%x\t%x\t%x \n",key[16],key[17],key[18],key[19]);
    /* IV will be random */
    ivdata = kzalloc(ivlen, GFP_KERNEL);
    if (!ivdata) {
        pr_info("could not allocate ivdata\n");
        goto out;
    }
    //get_random_bytes(ivdata, 12);
    for (i_iv =0 ; i_iv < ivlen;i_iv++)
    {
        ivdata[i_iv]=ivdata_const2[i_iv];
    }
    ivdata_copy = ivdata; // copy pointer
    printk(KERN_INFO "IV: \n");
    for(i_iv = 1; i_iv <= ivlen + 10; i_iv++)
    {
        printk(KERN_INFO "%x \t", (*ivdata_copy));
        ivdata_copy ++;
        if ((i_iv % 4) == 0)
            printk(KERN_INFO "\n");
    }

    /* AAD value (comprise of SPI and Sequence number)-64 bit 
       ( not with extended sequence number-96 bit) */
    AAD = kzalloc(assoclen, GFP_KERNEL);
    if (!AAD) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }
    //get_random_bytes(AAD,8);
    // 8 bytes
    AAD_copy =AAD_const2;
    printk(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    
    for(i_AAD = 1; i_AAD <= assoclen + 10; i_AAD ++)
    {
        printk(KERN_INFO "%x \t", (*AAD_copy));
        AAD_copy ++;
        if ((i_AAD % 4) == 0)
            printk(KERN_INFO "\n");
    }
    for (i_AAD = 0; i_AAD < assoclen; i_AAD ++)
    {
        AAD[i_AAD] = AAD_const2[i_AAD];
    }
    AAD_copy =AAD;
    printk(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    
    for(i_AAD = 1; i_AAD <= assoclen +10; i_AAD ++)
    {
        printk(KERN_INFO "%x \t", (*AAD_copy));
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
    //get_random_bytes(scratchpad, 16);
    // 60 bytes

    for (i_data = 0 ; i_data < data_len;i_data++)
    {
        scratchpad[i_data]=scratchpad_const2[i_data];
    }

    scratchpad_copy = scratchpad; 
    printk(KERN_INFO "Data payload :\n");
    
    for(i_data = 1; i_data <= data_len ; i_data ++)
    {
        printk(KERN_INFO "%x \t", (*scratchpad_copy));
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

    ad.sg = kmalloc_array(2,sizeof(struct scatterlist),GFP_KERNEL);// 2 entries
    if(!ad.sg){
        ret = -ENOMEM;
        goto out;
    }
    sg_buffer_len = assoclen + data_len + authlen;
    sg_buffer =kzalloc(sg_buffer_len, GFP_KERNEL);// 92 = 8 byte (AAD -1st entry) + +8 byte iv + 60 bytes data(2nd entry)+16 bytes (2nd entry)
    sg_init_table(ad.sg, 2 ); // 2 entries
    sg_set_buf(&ad.sg[0], AAD, assoclen);
    sg_set_buf(&ad.sg[1],scratchpad,320);// for fall-back cases
    aead_request_set_ad(req,assoclen + ivlen);// 12 means that there are 12 octects ( 96 bits) in AAD fields.
    aead_request_set_crypt(req, ad.sg, ad.sg, data_len -ivlen, ivdata);// 60 bytes (data for encryption or decryption)  + 8 byte iv
    init_completion(&ad.result.completion);
    /* for printint */
    printk(KERN_INFO "assoclen + ivlen:%d\n", req->assoclen);
    printk(KERN_INFO "cryptlen:%d\n", req->cryptlen);
    printk(KERN_INFO "lenth of scatterlist : %d \n", ad.sg->length);
    printk(KERN_INFO "lenth of scatterlist 0: %d \n", ad.sg[0].length);
    printk(KERN_INFO "lenth of scatterlist 1: %d \n", ad.sg[1].length);
    // printk(KERN_INFO "length of scatterlist 2: %d \n", ad.sg[2].length);
    printk(KERN_INFO "lenght of scatterlist(src 0): %d\n",req->src[0].length);
    printk(KERN_INFO "length of scatterlist(src 1): %d \n",req->src[1].length);
    // printk(KERN_INFO "length of scatterlist(src 2): %d \n",req->src[2].length);
    printk(KERN_INFO "length of scatterlist(src):%d \n",req->src->length);
    printk(KERN_INFO "length of scatterlist(dst 0): %d \n",req->dst[0].length);
    printk(KERN_INFO "length of scatterlist(dst 1): %d \n",req->dst[1].length);
    // printk(KERN_INFO "length of scatterlist(dst 2): %d \n",req->dst[2].length);
    printk(KERN_INFO "length of scatterlist (dst):%d \n",req->dst->length);
    len = (size_t)ad.req->cryptlen + (size_t)ad.req->assoclen + (size_t)authlen ;
    
    printk(KERN_INFO "length packets: %d \n", len);
    printk(KERN_INFO "packet - BEFORE function test encrypt:\n");
    len =sg_pcopy_to_buffer(ad.req->src,2,sg_buffer,len,0);
    sg_buffer_copy =sg_buffer;
    for(i_sg = 1; i_sg <= (req->assoclen + req->cryptlen + authlen); i_sg ++)
    {
        printk(KERN_INFO "%x \t", (*sg_buffer_copy));
        sg_buffer_copy ++;
        if ((i_sg % 4) == 0)
            printk(KERN_INFO "\n");
    }

    /* encrypt data ( 1 for encryption)*/
    ret = test_rfc4106_encdec(&ad, endec);
    if (ret){
        pr_err("Error encrypting data: %d\n", ret);
        goto out;
    }      
    len = (size_t)ad.req->cryptlen + (size_t)ad.req->assoclen+(size_t)authlen ;
    len =sg_pcopy_to_buffer(ad.req->src,2,sg_buffer,len,0);
    printk(KERN_INFO "Packet - AFTER function test encrypt:\n");
    sg_buffer_copy = sg_buffer;
    for(i_sg = 1; i_sg <= (req->assoclen + req->cryptlen + authlen); i_sg ++)
    {
        printk(KERN_INFO "%x \t", (*sg_buffer_copy));
        sg_buffer_copy ++;
        if ((i_sg % 4) == 0)
            printk(KERN_INFO "\n");
    }
    printk(KERN_INFO "Module Testcrypto: Encryption triggered successfully\n");
out:
    if (aead)
        crypto_free_aead(aead);
    printk(KERN_INFO "aead -rfc 4106 after kfree:\n" );
    if (req)
        aead_request_free(req);
    printk(KERN_INFO "req -rfc 4106 after kfree:\n" );
    if (ivdata)
        kfree(ivdata);
    printk(KERN_INFO "ivdata -rfc 4106 after kfree:\n" );
    if (AAD)
        kfree(AAD);
    printk(KERN_INFO "AAD -rfc 4106 after kfree:\n" );
    if (scratchpad)
        kfree(scratchpad);
    printk(KERN_INFO "scratchpad -rfc 4106 after kfree:\n" );
    if(sg_buffer)
        kfree(sg_buffer);
    printk(KERN_INFO "sg_buffer -rfc 4106 after kfree:\n" );
    return ret;
}

//------Test_esp_rfc4106-------
/* Initialize and trigger aead cipher operation */
static int test_esp_rfc4106(int test_choice, int endec)
{
    struct aead_def ad;
    struct crypto_aead *aead = NULL;
    struct aead_request *req = NULL;
    char *scratchpad = NULL;
    //char scratchpad[60];
    char *AAD = NULL;
    //char AAD[8]; 
    char *ivdata = NULL;
    //char ivdata[12];
    unsigned char key[20];
    int ret = -EFAULT;
    unsigned int assoclen = 0;// AAD_length
    unsigned int authlen;// Tag_length
    unsigned int ivlen;//iv length
    unsigned int keylen = 0;//key length
    unsigned int data_len = 0;// scratchpad length
    char *ivdata_copy = NULL; // for printing
    char *AAD_copy = NULL; //for printing
    char *scratchpad_copy = NULL;//for printing
    //char *sg_buffer =NULL; //for printing
    //char *sg_buffer_copy = NULL; //for printing
    int i_iv,i_AAD,i_data,i_key,i_authen;// index for assign value into iv,AAD,data,key in loop
    unsigned int sg_buffer_len;
    char *authentag =NULL;
    char *ciphertext=NULL;
    char *packet = NULL; 
    char *packet_data = NULL;//plaintext
    char *packet_cipher = NULL;//ciphertext 
    char *packet_authen = NULL;
    size_t len;
    
    printk(KERN_INFO "Module testcrypto: starting test_esp_rfc4106\n");
    switch(test_choice)
    {
        case 1:
            pr_info("This is test case 1\n");
            assoclen = test1_len.AAD_len;
            keylen = test1_len.key_len;
            data_len = test1_len.scratchpad_len;
            break;
            // do not need ivlen
        case 2:
            pr_info("This is test case 2\n");
            assoclen = test2_len.AAD_len;
            keylen = test2_len.key_len;
            data_len = test2_len.scratchpad_len;
            // do not need ivlen
            break;
        case 3:
            pr_info("This is test case 3\n");
            assoclen = test3_len.AAD_len;
            keylen = test3_len.key_len;
            data_len = test3_len.scratchpad_len;
            // do not need ivlen
            break;
        case 4:
            pr_info("This is test case 4\n");
            assoclen = test4_len.AAD_len;
            keylen = test4_len.key_len;
            data_len = test4_len.scratchpad_len;
            // do not need ivlen
            break;
        case 5:
            pr_info("This is test case 5\n");
            assoclen = test5_len.AAD_len;
            keylen = test5_len.key_len;
            data_len = test5_len.scratchpad_len;
            // do not need ivlen
            break;
        case 6:
            pr_info("This is test case 5\n");
            assoclen = test5_len.AAD_len;
            keylen = test5_len.key_len;
            data_len = test5_len.scratchpad_len;
            // do not need ivlen
            break;     
    }

    /*
    * Allocate a tfm (a transformation object) and set the key.
    *
    * In real-world use, a tfm and key are typically used for many
    * encryption/decryption operations.  But in this example, we'll just do a
    * single encryption operation with it (which is not very efficient).
     */
    
    aead = crypto_alloc_aead("rfc4106(gcm(aes))", 0, 0);
    if (IS_ERR(aead)) {
        pr_info("could not allocate aead handle (rfc 4106)\n");
        return PTR_ERR(aead);
    }
    /* Allocate a request object */
    req = aead_request_alloc(aead, GFP_KERNEL);
    if (!req) {
        pr_info("could not allocate aead request (rfc 4106)\n");
        ret = -ENOMEM;
        goto out;
    }

    /*Check the size of iv and authentication data (Tag for authentication)  */
    authlen = crypto_aead_authsize(aead);
    ivlen = crypto_aead_ivsize(aead);
    printk(KERN_INFO "authentication size: %d\n", authlen);
    printk(KERN_INFO "iv size:%d \n", ivlen);

    aead_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                            test_rfc4106_cb,
                            &ad.result); // for instruction set and hardware
    
    /* AES 128 with random key */
    //get_random_bytes(&key, 16);

    for (i_key =0 ; i_key < keylen;i_key++)
    {

        key[i_key]=key_const2[i_key];
    }
    
    if (crypto_aead_setkey(aead, key, 20)) {
        pr_info("key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }
    printk(KERN_INFO "Key value - after setkey:\n");
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[0],key[1],key[2],key[3]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[4],key[5],key[6],key[7]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[8],key[9],key[10],key[11]);
    printk(KERN_INFO "%x\t%x\t%x\t%x \n",key[12],key[13],key[14],key[15]);
    printk(KERN_INFO "Salt in Nonce: %x\t%x\t%x\t%x \n",key[16],key[17],key[18],key[19]);
    /* IV will be random */
    ivdata = kzalloc(ivlen, GFP_KERNEL);
    if (!ivdata) {
        pr_info("could not allocate ivdata\n");
        goto out;
    }
    //get_random_bytes(ivdata, 12);
    for (i_iv =0 ; i_iv < ivlen;i_iv++)
    {
        ivdata[i_iv]=ivdata_const2[i_iv];
    }
    ivdata_copy = ivdata; // copy pointer
    printk(KERN_INFO "IV: \n");
    for(i_iv = 0; i_iv < ivlen + 10; i_iv+=16)
    {
       pr_err("address of IV = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i_iv ,
            *((u32 *)(&ivdata_copy[i_iv + 12])),*((u32 *)(&ivdata_copy[i_iv + 8])), 
            *((u32 *)(&ivdata_copy[i_iv + 4])), *((u32 *)(&ivdata_copy[i_iv])));
    }

    /* AAD value (comprise of SPI and Sequence number)-64 bit 
       ( not with extended sequence number-96 bit) */
    AAD = kzalloc(assoclen, GFP_KERNEL);
    if (!AAD) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }
    //get_random_bytes(AAD,8);
    // 8 bytes
    AAD_copy =AAD_const2;
    printk(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    
    for (i_AAD = 0; i_AAD < assoclen; i_AAD ++)
    {
        AAD[i_AAD] = AAD_const2[i_AAD];
    }
    AAD_copy =AAD;
    printk(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    
    for(i_AAD = 0; i_AAD < assoclen +10; i_AAD +=16)
    {
        pr_err("address of AAD = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i_AAD ,
            *((u32 *)(&AAD_copy[i_AAD + 12])),*((u32 *)(&AAD_copy[i_AAD + 8])), 
            *((u32 *)(&AAD_copy[i_AAD + 4])), *((u32 *)(&AAD_copy[i_AAD])));
    }
    
    /* Input data will be random */
    scratchpad = kzalloc(320, GFP_KERNEL);
    if (!scratchpad) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }
    //get_random_bytes(scratchpad, 16);
    // 60 bytes

    for (i_data = 0 ; i_data < data_len;i_data++)
    {
        scratchpad[i_data]=scratchpad_const2[i_data];
    }

    scratchpad_copy = scratchpad; 
    printk(KERN_INFO "Data payload :\n");
    
    for(i_data = 0; i_data < data_len ; i_data +=16)
    {
    pr_err("address of data_scratchpad = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i_data ,
            *((u32 *)(&scratchpad_copy[i_data + 12])),*((u32 *)(&scratchpad_copy[i_data + 8])), 
            *((u32 *)(&scratchpad_copy[i_data + 4])), *((u32 *)(&scratchpad_copy[i_data])));
    }

    /*Input authentication tag*/
    authentag = kzalloc(16,GFP_KERNEL);
    if (!authentag) {
        pr_info("could not allocate packet\n");
        goto out;
    }
 
    for (i_authen = 0 ; i_authen < 16;i_authen++)
    {
        authentag[i_authen]=authentag_const2[i_authen];
    }

    printk(KERN_INFO "Authentag :\n");
    for(i_authen = 0; i_authen < 16 ; i_authen +=16)
    {
    pr_err("address of authentag = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i_authen ,
            *((u32 *)(&authentag[i_data + 12])),*((u32 *)(&authentag[i_data + 8])), 
            *((u32 *)(&authentag[i_data + 4])), *((u32 *)(&authentag[i_data])));
    }
    /*Input cipher text content*/

    ciphertext = kzalloc(320, GFP_KERNEL);
    if (!ciphertext) {
        pr_info("could not allocate ciphertext\n");
        goto out;
    }

    for (i_data = 0 ; i_data < data_len;i_data++)
    {
        ciphertext[i_data]=ciphertext_const2[i_data];
    }
 
    printk(KERN_INFO "Data payload :\n");
    for(i_data = 0; i_data < data_len ; i_data +=16)
    {
    pr_err("address of ciphertext = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i_data ,
            *((u32 *)(&ciphertext[i_data + 12])),*((u32 *)(&ciphertext[i_data + 8])), 
            *((u32 *)(&ciphertext[i_data + 4])), *((u32 *)(&ciphertext[i_data])));
    }

    /*Input packet content*/
    packet = kzalloc(1000,GFP_KERNEL);
    if (!packet) {
        pr_info("could not allocate packet\n");
        goto out;
    }
    for (i_AAD = 0; i_AAD < assoclen; i_AAD ++)
    {
        packet[i_AAD] = AAD[i_AAD];
    }

    switch(endec)
    {
        case 0:
        // mode decrypt: input needs tag
            packet_cipher = packet + assoclen;
            for (i_data = 0; i_data < data_len ;i_data++)
            {
            packet_cipher[i_data] = ciphertext[i_data];
            }
            packet_authen = packet_cipher + data_len;
            for (i_authen = 0 ; i_authen < 16;i_authen++)
            {
            packet_authen[i_authen]=authentag[i_authen];
            }
            break;
        case 1:
        // mode encrypt: input dont need tag
            packet_data = packet + assoclen;
            for (i_data = 0; i_data < data_len ;i_data++)
            {
                packet_data[i_data] = scratchpad[i_data];
            }
            break;
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

    //ad.sg = kmalloc_array(2,sizeof(struct scatterlist),GFP_KERNEL);// 2 entries
    ad.sg = kzalloc(sizeof(struct scatterlist),GFP_KERNEL);
    if(!ad.sg){
        ret = -ENOMEM;
        goto out;
    }
    sg_buffer_len = assoclen + data_len + authlen;
    //sg_buffer =kzalloc(sg_buffer_len, GFP_KERNEL);// 92 = 8 byte (AAD -1st entry) + +8 byte iv + 60 bytes data(2nd entry)+16 bytes (2nd entry)
    sg_init_table(ad.sg, 1 ); // 1 entries
    sg_set_buf(ad.sg, packet, assoclen + data_len + authlen);
    //sg_set_buf(&ad.sg[1],scratchpad,320);// for fall-back cases
    aead_request_set_ad(req,assoclen+ivlen);// 12 means that there are 12 octects ( 96 bits) in AAD fields.
    aead_request_set_crypt(req, ad.sg, ad.sg, data_len-ivlen, ivdata);// 60 bytes (data for encryption or decryption)  + 8 byte iv
    scatterwalk_map_and_copy(ivdata, req->dst, req->assoclen-ivlen, ivlen, 1);
    init_completion(&ad.result.completion);
    /* for printint */

    len = (size_t)ad.req->cryptlen + (size_t)ad.req->assoclen + (size_t)authlen ;
    
    printk(KERN_INFO "length packets: %d \n", len);
    printk(KERN_INFO "%d packet - BEFORE function test encrypt:\n", __LINE__);
    print_sg_content(ad.req->src,len);
    print_sg_virt_content(ad.req->src);

    /* encrypt data ( 1 for encryption)*/
    ret = test_rfc4106_encdec(&ad, endec);
    if (ret){
        pr_err("Error encrypting data: %d\n", ret);
        goto out;
    } 
    ivdata_copy = req->iv; // copy pointer
    pr_info("IV after encrypt (in esp->iv): \n");
    for(i_iv = 0; i_iv <= 8 + 24; i_iv+=16)
    {
      pr_err("address of IV = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i_iv ,
            *((u32 *)(&ivdata_copy[i_iv + 12])),*((u32 *)(&ivdata_copy[i_iv + 8])), 
            *((u32 *)(&ivdata_copy[i_iv + 4])), *((u32 *)(&ivdata_copy[i_iv])));
    }     
    len = (size_t)ad.req->cryptlen + (size_t)ad.req->assoclen+(size_t)authlen ;

    print_sg_content(ad.req->src,len);
    print_sg_virt_content(ad.req->src);
    printk(KERN_INFO "Module Testcrypto: Encryption triggered successfully\n");
out:
    if (aead)
        crypto_free_aead(aead);
    printk(KERN_INFO "Module Testcrypto: aead -rfc 4106 after kfree:\n" );
    if (req)
        aead_request_free(req);
    printk(KERN_INFO "Module Testcrypto: req -rfc 4106 after kfree:\n" );
    if (ivdata)
        kfree(ivdata);
    printk(KERN_INFO "Module Testcrypto: ivdata -rfc 4106 after kfree:\n" );
    if (AAD)
        kfree(AAD);
    printk(KERN_INFO "Module Testcrypto: AAD -rfc 4106 after kfree:\n" );
    if (scratchpad)
        kfree(scratchpad);
    printk(KERN_INFO "Module Testcrypto: scratchpad -rfc 4106 after kfree:\n" );
    // if(sg_buffer)
    //     kfree(sg_buffer);
    // printk(KERN_INFO "Module Testcrypto: sg_buffer -rfc 4106 after kfree:\n" );
    // if(packet)
    //     kfree(packet);
    // printk(KERN_INFO "Module Testcrypto: authentag -rfc4106 after kfree:\n");
    // if(authentag)
    //     kfree(authentag);
    // printk(KERN_INFO "Module Testcrypto: packet -rfc4106 after kfree:\n");
    // if(ciphertext)
    //     kfree(ciphertext);
    // printk(KERN_INFO "Module Testcrypto: ciphertext -rfc4106 after kfree:\n");
    return ret;
}


static int __init test_init(void)
{
    int i;
    //int cipher_choice = 2;// 0 for cbc(skcipher), 1 for gcm(aead), 2 for gcm-esp(aead)
    //int test_choice = 2;
    //int engine_type = 0; // 0 for software, 1 for instruction set,2 for hardware engine
    //int endec = MODE_DIR_ENCRYPT;
    printk(KERN_INFO "Module testcrypto: info: init test \n Pid: %d", current->pid);
    /* for gcm(aes) - test aead and rfc4106(gcm(aes)) - test gcm*/
    switch (test_choice)
    {
        case 1: 
        
            for (i =0 ; i < test1_len.key_len;i++)
            {
                key_const2[i]=key_test1[i];
            }
            for (i=0; i < test1_len.AAD_len; i++)
            {
                AAD_const2[i]=AAD_test1[i];
            }
            for (i=0; i < test1_len.scratchpad_len; i++)
            {
                scratchpad_const2[i]=scratchpad_test1[i];
            }
            for (i=0; i< test1_len.ivdata_len ; i++)
            {
                ivdata_const2[i]= ivdata_test1[i];
            }
            for (i=0; i< 16; i++)
            {
                authentag_const2[i]= authentag_test1[i];
            }
            break;
        case 2:
            for (i =0 ; i < test2_len.key_len;i++)
            {
                key_const2[i]=key_test2[i];
            }
            for (i=0; i < test2_len.AAD_len; i++)
            {
                AAD_const2[i]=AAD_test2[i];
            }
            for (i=0; i < test2_len.scratchpad_len; i++)
            {
                scratchpad_const2[i]=scratchpad_test2[i];
            }
            for (i=0; i < test2_len.scratchpad_len; i++)
            {
                ciphertext_const2[i]=ciphertext_test2[i];
            }
            for (i=0; i< test2_len.ivdata_len; i++)
            {
                ivdata_const2[i]= ivdata_test2[i];
            }
            for (i=0; i< 16; i++)
            {
                authentag_const2[i]= authentag_test2[i];
            }
            break;
        case 3:
            for (i =0 ; i < test3_len.key_len;i++)
            {
                key_const2[i]=key_test3[i];
            }
            for (i=0; i < test3_len.AAD_len; i++)
            {
                AAD_const2[i]=AAD_test3[i];
            }
            for (i=0; i < test3_len.scratchpad_len; i++)
            {
                scratchpad_const2[i]=scratchpad_test3[i];
            }
            for (i=0; i< test3_len.ivdata_len; i++)
            {
                ivdata_const2[i]= ivdata_test3[i];
            }
            for (i=0; i< 16; i++)
            {
                authentag_const2[i]= authentag_test3[i];
            }
            break;
        case 4:
            for (i =0 ; i < test4_len.key_len;i++)
            {
                key_const2[i]=key_test4[i];
            }
            for (i=0; i < test4_len.AAD_len; i++)
            {
                AAD_const2[i]=AAD_test4[i];
            }
            for (i=0; i < test4_len.scratchpad_len; i++)
            {
                scratchpad_const2[i]=scratchpad_test4[i];
            }
            for (i=0; i< test4_len.ivdata_len; i++)
            {
                ivdata_const2[i]= ivdata_test4[i];
            }
            for (i=0; i< 16; i++)
            {
                authentag_const2[i]= authentag_test4[i];
            }
            break;
        case 5:
            for (i =0 ; i < test5_len.key_len;i++)
            {
                key_const2[i]=key_test5[i];
            }
            for (i=0; i < test5_len.AAD_len; i++)
            {
                AAD_const2[i]=AAD_test5[i];
            }
            for (i=0; i < test5_len.scratchpad_len; i++)
            {
                scratchpad_const2[i]=scratchpad_test5[i];
            }
            for (i=0; i< test5_len.ivdata_len; i++)
            {
                ivdata_const2[i]= ivdata_test5[i];
            }
            for (i=0; i< 16; i++)
            {
                authentag_const2[i]= authentag_test5[i];
            }
            break;
        case 6:
            for (i =0 ; i < test6_len.key_len;i++)
            {
                key_const2[i]=key_test6[i];
            }
            for (i=0; i < test6_len.AAD_len; i++)
            {
                AAD_const2[i]=AAD_test6[i];
            }
            for (i=0; i < test6_len.scratchpad_len; i++)
            {
                scratchpad_const2[i]=scratchpad_test6[i];
            }
            for (i=0; i< test6_len.ivdata_len; i++)
            {
                ivdata_const2[i]= ivdata_test6[i];
            }
            for (i=0; i< 16; i++)
            {
                authentag_const2[i]= authentag_test6[i];
            }
            break;
    }
    for (i = 0; i < req_num; i ++){
        if (cipher_choice == 0)
            test_skcipher();
        else if (cipher_choice == 1)
            test_aead();
        else if (cipher_choice == 2)
            test_rfc4106(test_choice,endec); 
        else if (cipher_choice == 3)
            test_esp_rfc4106(test_choice,endec);
    }
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