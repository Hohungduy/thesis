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
#include <linux/timer.h>
/* get system time */
#include <linux/jiffies.h> 


#define AAAA
//#define not_free

#ifdef AAAA
#define pr_aaa(...)
#else 
#define pr_aaa pr_err
#endif

#ifdef not_free
#define kfree_aaa(...)
#else 
#define  kfree_aaa kfree
#endif


#define TIMEOUT 5000 //miliseconds

// static unsigned int req_num = 20;
// module_param(req_num, uint, 0000);
// MODULE_PARM_DESC(req_num, "request and number");

// static unsigned int loop_num = 20;
// module_param(loop_num, uint, 0000);
// MODULE_PARM_DESC(loop_num, "request and loop number");

static unsigned int test_choice = 2;
module_param(test_choice, uint, 0000);
MODULE_PARM_DESC(test_choice, "Choose test case");

static unsigned int cipher_choice = 3;// 0 for cbc(skcipher), 1 for gcm(aead), 2 for gcm-esp(aead)
module_param(cipher_choice, uint, 0000);
MODULE_PARM_DESC(cipher_choice, "Choose cipher type: 0 for cbc, 1 for gcm, 2 for rfc4106");

static unsigned int endec = 1; // 1 for encrypt; 2 for decrypt
module_param(endec, uint, 0000);
MODULE_PARM_DESC(endec, "Choose mode: 1 for encrypt and 2 for decrypt");
// set timer
static struct timer_list testcrypto_ktimer;
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
struct aead_def
 {
    struct scatterlist *sg;
    struct crypto_aead *tfm;
    struct aead_request *req;
    struct tcrypt_result result;
    char *scratchpad ;
    char *AAD ;
    char *ivdata ;
    char *authentag ;
    char *ciphertext;
    char *packet ; 
    u8 done;
};

u32 count;
int done_flag = 0; //stop 
//--------------timer handler---------------------------------------
static void handle_timer(struct timer_list *t)
{
    done_flag = 1;
    pr_err("Number of req after 60s:%d\n\n", count);
    
    // del_timer_sync(&testcrypto_ktimer);
    // mod_timer(&testcrypto_ktimer, jiffies + msecs_to_jiffies(TIMEOUT));
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


// //---------------------------------------------------------------------------------------------
// /* Test GCM-AES in ESP mode (RFC 4106) - Type: AEAD cipher */
// //---------------------------------------------------------------------------------------------
// /* Callback function for aead type */

/* Perform aead cipher operation */
//u8 done;
static unsigned int test_rfc4106_encdec(struct aead_def *ad, int enc)
{
    int rc;
    pr_aaa(KERN_INFO "Module testcrypto: STARTING test_rfc4106_encdec\n");
        pr_aaa("Module mycrypto-cipher.c: Address of req->iv:%p - data =  %8.0x %8.0x \n",ad->req->iv,  
            *((u32 *)(&ad->req->iv[4])), *((u32 *)(&ad->req->iv[0])));
    if (enc)
        rc = crypto_aead_encrypt(ad->req);
    else
        rc = crypto_aead_decrypt(ad->req);
    pr_aaa(KERN_INFO "AFTER crypto_aead_encrypt\n");
    // pr_err("%d:%s: ad pointer:%p",__LINE__,__func__,ad);
    ad->done = false;

    switch (rc) 
    {
    case 0:
        pr_aaa("Module testcrypto: Case 0 \n");
        
        break;
    case -EINPROGRESS:
    case -EBUSY:
        pr_aaa("Module testcrypto: Case einprogress + ebusy \n");
        rc = wait_for_completion_interruptible(&ad->result.completion);
        // while(!ad->done)
        // {
        //     pr_err("%d: %s - PID:%d -ad->done:%d \n",__LINE__ , __func__ ,  current->pid , ad->done);
        // };
        // rc=0;
        if ((!rc) && (!ad->result.err)) {
            reinit_completion(&ad->result.completion);
                    }
        break;
    default:
        break;
    }
    init_completion(&ad->result.completion);
    //pr_aaa("Module testcrypto:%p\n",&ad->result.completion);
        return rc;
}



static void test_rfc4106_cb(struct crypto_async_request *req, int error)
{
    // struct tcrypt_result *result = req->data;
    struct aead_def *ad = req->data;
    struct tcrypt_result *result = &ad->result;
    
    // struct scatterlist *sg = ad->sg;
    // struct crypto_aead *aead =ad->tfm;
    // struct aead_request *aead_req = ad->req;
    // char *scratchpad = ad->scratchpad;
    // char *AAD =ad->AAD;
    // char *ivdata =ad->ivdata;
    // char *authentag =ad->authentag;
    // char *ciphertext =ad->ciphertext;
    // char *packet =ad->packet; 
    // u8 *done=req->data;
    // count ++;
    
    //struct aead_request *req = container_of(base, struct aead_request, base); // for test
    pr_aaa(KERN_INFO "Module testcrypto: STARTING test_rfc4106_cb\n");
    if(error == -EINPROGRESS)
    {
        return;

    }
    result->err = error;
    
    complete(&result->completion);
    
    // pr_aaa("%d: %s - PID:%d - done:%d\n", __LINE__ , __func__ , current->pid , *done);
    // *done=true;
    // pr_aaa("%d: %s - PID:%d - pointer of data : %p\n",__LINE__ , __func__ ,  current->pid , req->data);
    // pr_aaa("%d: %s - PID:%d - done:%d\n", __LINE__ , __func__ , current->pid , *done);
    // pr_aaa("%d: %s - PID:%d - *(req->data):%d\n", __LINE__ , __func__ , current->pid , *(u8*)(req->data));

     
    // pr_err(KERN_INFO "Module testcrypto: Encryption finishes successfully\n");


    // if (aead)
    //     crypto_free_aead(aead);
    // if (aead_req)
    //     aead_request_free(aead_req);
    // if (ivdata)
    //     kfree_aaa(ivdata);
    // if (AAD)
    //     kfree_aaa(AAD);
    // if (scratchpad)
    //     kfree_aaa(scratchpad);
    // if(sg)
    //     kfree_aaa(sg);
    // if(packet)
    //     kfree_aaa(packet);
    // if(authentag)
    //     kfree_aaa(authentag);
    // if(ciphertext)
    //     kfree_aaa(ciphertext);
    // if(ad)
    //     kfree_aaa(ad);

}





//------Test_esp_rfc4106------
/* Initialize and trigger aead cipher operation */
static int test_esp_rfc4106(int test_choice, int endec)
{
    struct crypto_aead *aead = NULL;
    struct aead_request *aead_req = NULL;
    
    char *scratchpad = NULL;
    char *AAD = NULL;
    char *ivdata = NULL;
    char *authentag =NULL;
    char *ciphertext=NULL;
    char *packet = NULL; 
    char *packet_data = NULL;//plaintext
    char *packet_cipher = NULL;//ciphertext 
    char *packet_authen = NULL;
    unsigned char key[20];

    int ret = -EFAULT;
    unsigned int assoclen = 0;// AAD_length
    unsigned int authlen;// Tag_length
    unsigned int ivlen;//iv length
    unsigned int keylen = 0;//key length
    unsigned int data_len = 0;// scratchpad length
    int i_iv,i_AAD,i_data,i_key,i_authen;// index for assign value into iv,AAD,data,key in loop
    unsigned int sg_buffer_len;

    size_t len;
    struct aead_def *ad;

    ad = kzalloc(sizeof(struct aead_def) , GFP_KERNEL | GFP_DMA);

    pr_aaa(KERN_INFO "Module testcrypto: starting test_esp_rfc4106\n");
    switch(test_choice)
    {
        case 1:
            pr_aaa("This is test case 1\n");
            assoclen = test1_len.AAD_len;
            keylen = test1_len.key_len;
            data_len = test1_len.scratchpad_len;
            break;
            // do not need ivlen
        case 2:
            pr_aaa("This is test case 2\n");
            assoclen = test2_len.AAD_len;
            keylen = test2_len.key_len;
            data_len = test2_len.scratchpad_len;
            // do not need ivlen
            break;
        case 3:
            pr_aaa("This is test case 3\n");
            assoclen = test3_len.AAD_len;
            keylen = test3_len.key_len;
            data_len = test3_len.scratchpad_len;
            // do not need ivlen
            break;
        case 4:
            pr_aaa("This is test case 4\n");
            assoclen = test4_len.AAD_len;
            keylen = test4_len.key_len;
            data_len = test4_len.scratchpad_len;
            // do not need ivlen
            break;
        case 5:
            pr_aaa("This is test case 5\n");
            assoclen = test5_len.AAD_len;
            keylen = test5_len.key_len;
            data_len = test5_len.scratchpad_len;
            // do not need ivlen
            break;
        case 6:
            pr_aaa("This is test case 5\n");
            assoclen = test6_len.AAD_len;
            keylen = test6_len.key_len;
            data_len = test6_len.scratchpad_len;
            // do not need ivlen
            break;     
        case 7:
            pr_aaa("This is test case 5\n");
            assoclen = test7_len.AAD_len;
            keylen = test7_len.key_len;
            data_len = test7_len.scratchpad_len;
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
        pr_aaa("could not allocate aead handle (rfc 4106)\n");
        return PTR_ERR(aead);
    }
    /* Allocate a request object */
    aead_req = aead_request_alloc(aead, GFP_KERNEL | GFP_DMA);
        if (!aead_req) {
        pr_aaa("could not allocate aead request (rfc 4106)\n");
        ret = -ENOMEM;
        goto out;
    }
    pr_aaa("Module testcrypto: Address of aead_req:%p \n",aead_req);
    pr_aaa("%d: Module testcrypto - aead_req pointer:%p - asys_req pointer offset:%x \
    - assoc pointer offset:%x - cryptlen pointer offset:%x - iv pointer offset:%x \
    - src pointer offset:%x - dst pointer offset:%x - ctx pointer offset(SIZEOF:%x):%x\n",
    __LINE__, aead_req , offsetof(struct aead_request, base),offsetof(struct aead_request,assoclen),
    offsetof(struct aead_request,cryptlen),offsetof(struct aead_request,iv),
    offsetof(struct aead_request,src),offsetof(struct aead_request,dst),offsetof(struct aead_request,__ctx),sizeof(void *));
    
        /*Check the size of iv and authentication data (Tag for authentication)  */
    authlen = crypto_aead_authsize(aead);
        ivlen = crypto_aead_ivsize(aead);
        pr_aaa(KERN_INFO "authentication size: %d\n", authlen);
        pr_aaa(KERN_INFO "iv size:%d \n", ivlen);
    

    aead_request_set_callback(aead_req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                            test_rfc4106_cb,
                            ad); // for instruction set and hardware
                            //&ad->result
        /* AES 128 with random key */
    //get_random_bytes(&key, 16);

    for (i_key =0 ; i_key < keylen;i_key++)
    {
        key[i_key]=key_const2[i_key];
    }
        if (crypto_aead_setkey(aead, key, 20)) {
        pr_aaa("key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }
        pr_aaa(KERN_INFO "Key value :\n");
    pr_aaa(KERN_INFO "%x\t%x\t%x\t%x \n",key[0],key[1],key[2],key[3]);
    pr_aaa(KERN_INFO "%x\t%x\t%x\t%x \n",key[4],key[5],key[6],key[7]);
    pr_aaa(KERN_INFO "%x\t%x\t%x\t%x \n",key[8],key[9],key[10],key[11]);
    pr_aaa(KERN_INFO "%x\t%x\t%x\t%x \n",key[12],key[13],key[14],key[15]);
    pr_aaa(KERN_INFO "Salt in Nonce: %x\t%x\t%x\t%x \n",key[16],key[17],key[18],key[19]);
    
    /* IV will be random */
    ivdata = kzalloc(ivlen, GFP_KERNEL | GFP_DMA);

    if (!ivdata) {
        pr_aaa("could not allocate ivdata\n");
        goto out;
    }
    //get_random_bytes(ivdata, 12);
    for (i_iv =0 ; i_iv < ivlen;i_iv++)
    {
        ivdata[i_iv]=ivdata_const2[i_iv];
    }
    //ivdata_copy = ivdata; // copy pointer
    pr_aaa(KERN_INFO "IV: \n");
    for(i_iv = 0; i_iv < ivlen ; i_iv+=16)
    {
    pr_aaa("data = %8.0x %8.0x \n", 
            *((u32 *)(&ivdata[4])), *((u32 *)(&ivdata[0])));
    }

    /* AAD value (comprise of SPI and Sequence number)-64 bit 
       ( not with extended sequence number-96 bit) */
        pr_aaa("assoclen:%d\n",assoclen);
    pr_aaa("assoclen:%p\n",AAD);

    AAD = kzalloc(assoclen, GFP_KERNEL | GFP_DMA);
    if (!AAD) {
        pr_aaa("could not allocate scratchpad\n");
        goto out;
    }
    //get_random_bytes(AAD,8);
    // 8 bytes
    //AAD_copy =AAD_const2;
        pr_aaa(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    
    for (i_AAD = 0; i_AAD < assoclen; i_AAD ++)
    {
        AAD[i_AAD] = AAD_const2[i_AAD];
    }

    //AAD_copy =AAD;
        pr_aaa(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    switch(assoclen)
    {
        case 8:
            // for(i_AAD = 0; i_AAD < assoclen ; i_AAD +=16)
            // {
    
                        pr_aaa("data = %8.0x %8.0x  \n", 
                *((u32 *)(&AAD[4])), *((u32 *)(&AAD[0])));
            // }
        break;
        case 12:
            // for(i_AAD = 0; i_AAD < assoclen ; i_AAD +=16)
            // {
                                pr_aaa("data = %8.0x %8.0x %8.0x \n",
                *((u32 *)(&AAD[8])), *((u32 *)(&AAD[4])), *((u32 *)(&AAD[0])));
            // }
        break;
    }

    /* Input data will be random */
    scratchpad = kzalloc(DATA_LENGTH, GFP_KERNEL | GFP_DMA );
    if (!scratchpad) {
        pr_aaa("could not allocate scratchpad\n");
        goto out;
    }
    //get_random_bytes(scratchpad, 16);
    // 60 bytes

    for (i_data = 0 ; i_data < data_len;i_data++)
    {
        scratchpad[i_data] = scratchpad_const2[i_data];
    }

    //scratchpad_copy = scratchpad; 
    pr_aaa(KERN_INFO "Data payload :\n");
        	    pr_aaa("datale:%d",data_len);
    
    // for(i_data = 0; i_data < data_len ; i_data +=16)
    // {
    // pr_aaa("data = %8.0x %8.0x %8.0x %8.0x \n",
    //         *((u32 *)(&scratchpad[i_data+12])),*((u32 *)(&scratchpad[i_data+8])), 
    //         *((u32 *)(&scratchpad[i_data+4])), *((u32 *)(&scratchpad[i_data])));
    // }

    /*Input authentication tag*/
    authentag = kzalloc(16,GFP_KERNEL | GFP_DMA );
    if (!authentag) {
        pr_aaa("could not allocate packet\n");
        goto out;
    }
 
    for (i_authen = 0 ; i_authen < 16;i_authen++)
    {
        authentag[i_authen]=authentag_const2[i_authen];
    }

    pr_aaa(KERN_INFO "Authentag :\n");
    // for(i_authen = 0; i_authen < 16 ; i_authen +=16)
    // {
            	    pr_aaa("data = %8.0x %8.0x %8.0x %8.0x \n",
            *((u32 *)(&authentag[12])),*((u32 *)(&authentag[8])), 
            *((u32 *)(&authentag[4])), *((u32 *)(&authentag[0])));
    // }
    /*Input cipher text content*/

    ciphertext = kzalloc(DATA_LENGTH, GFP_KERNEL | GFP_DMA );
    if (!ciphertext) {
        pr_aaa("could not allocate ciphertext\n");
        goto out;
    }

    for (i_data = 0 ; i_data < data_len;i_data++)
    {
        ciphertext[i_data]=ciphertext_const2[i_data];
    }
 
    pr_aaa(KERN_INFO "Data cipher payload :\n");
        	    for(i_data = 0; i_data < data_len ; i_data +=16)
    {
    pr_aaa("address of ciphertext = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i_data ,
            *((u32 *)(&ciphertext[i_data + 12])),*((u32 *)(&ciphertext[i_data + 8])), 
            *((u32 *)(&ciphertext[i_data + 4])), *((u32 *)(&ciphertext[i_data])));
    }

    /*Input packet content*/
    packet = kzalloc(PACKET_LENGTH,GFP_KERNEL | GFP_DMA);
    if (!packet) {
        pr_aaa("could not allocate packet\n");
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
   
    ad->tfm = aead;
    ad->req = aead_req;
    ad->ciphertext = ciphertext;
    ad->packet=packet;
    ad->authentag=authentag;
    ad->AAD = AAD;
    ad->scratchpad = scratchpad;
    ad->ivdata = ivdata;

    pr_aaa(KERN_INFO "BEFORE allocate data and AAD into sg list of request\n");
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

    //ad->sg = kmalloc_array(2,sizeof(struct scatterlist),GFP_KERNEL | GFP_DMA);// 2 entries
    ad->sg = kzalloc(sizeof(struct scatterlist),GFP_KERNEL | GFP_DMA);
        	
    if(!ad->sg){
        ret = -ENOMEM;
        goto out;
    }
    sg_buffer_len = assoclen + data_len + authlen;
        	

    sg_init_one(ad->sg,packet,assoclen + data_len + authlen);
        	
    // sg_mark_end(ad->sg);
        	
    // sg_set_buf(&ad->sg[1],scratchpad,DATA_LENGTH);// for fall-back cases
    aead_request_set_ad(aead_req , assoclen+ivlen);// 12 means that there are 12 octects ( 96 bits) in AAD fields.
        	    switch(endec)
    {
        case 0:
        //decrypt
            aead_request_set_crypt(aead_req , ad->sg, ad->sg, data_len - ivlen + authlen, ivdata);// 60 bytes (data for encryption or decryption)  + 8 byte iv
            break;
        case 1:
        //encrypt
            aead_request_set_crypt(aead_req , ad->sg, ad->sg, data_len - ivlen, ivdata);// 60 bytes (data for encryption or decryption)  + 8 byte iv
            break;
    }
            pr_aaa("IV before encrypt (in esp->iv): \n");
    pr_aaa("Address of aead_req->iv:%p - data = %8.0x %8.0x - Offset:%x \n",&(aead_req->iv), 
            *((u32 *)(&aead_req->iv[4])), *((u32 *)(&aead_req->iv[0])),offsetof(struct aead_request,iv));
    scatterwalk_map_and_copy(ivdata, aead_req->dst, aead_req->assoclen-ivlen, ivlen, 1);

        pr_aaa("IV before encrypt (in esp->iv): \n");
    pr_aaa("Address of aead_req->iv:%p - data = %8.0x %8.0x - Offset:%x\n",&(aead_req->iv), 
            *((u32 *)(&aead_req->iv[4])), *((u32 *)(&aead_req->iv[0])),offsetof(struct aead_request,iv));
    pr_aaa("Module testcrypto: Address of aead_req:%p - assoclen+Cryptlen =  %d %d \n",aead_req,  
            aead_req->assoclen, aead_req->cryptlen);
    
    init_completion(&ad->result.completion);
    /* for printing */

    len = (size_t)ad->req->cryptlen + (size_t)ad->req->assoclen + (size_t)authlen + ivlen;
        	    
    pr_aaa(KERN_INFO "length packets: %d \n", len);
    pr_aaa(KERN_INFO "%d packet - BEFORE function test encrypt:\n", __LINE__);
        // print_sg_content(ad->req->src);
        pr_aaa("%d: %s - PID:%d - pointer of req.data:%p\n",__LINE__ , __func__ ,  current->pid , aead_req->base.data);
    pr_aaa("%d: %s - PID:%d - pointer of req.data:%p\n",__LINE__ , __func__ ,  current->pid , ad->req->base.data);
    /* encrypt data ( 1 for encryption)*/
    // for (i_loop=0; i_loop < loop_num; i_loop++)
    // {
    // while (!done_flag)
    // {
    ret = test_rfc4106_encdec(ad, endec);
    // count ++;
    
    // }
    // print_sg_content(ad->req->src);
    // mdelay(10000);
        
    if (ret){
        pr_aaa("Error encrypting data: %d\n", ret);
                goto out;
    } 
    //ivdata_copy = req->iv; // copy pointer
    pr_aaa("IV after encrypt (in esp->iv): \n");

    pr_aaa("Address of aead_req->iv:%p - data = %8.0x %8.0x - Offset:%x\n",&(aead_req->iv), 
            *((u32 *)(&aead_req->iv[4])), *((u32 *)(&aead_req->iv[0])),offsetof(struct aead_request,iv));
     
    len = (size_t)ad->req->cryptlen + (size_t)ad->req->assoclen+(size_t)authlen ;
    pr_aaa("Module testcrypto:Data after test_rfc4106_encdec \n");

    
    pr_aaa(KERN_INFO "Module Testcrypto: Encryption triggered successfully\n");

out:

    if (aead)
        crypto_free_aead(aead);
    pr_aaa(KERN_INFO "Module Testcrypto: aead -rfc 4106 after kfree_aaa:\n" );

    if (aead_req)
        aead_request_free(aead_req);
    pr_aaa(KERN_INFO "Module Testcrypto: aead_req -rfc 4106 after kfree_aaa:\n" );

    if (ivdata)
        kfree_aaa(ivdata);
    pr_aaa(KERN_INFO "Module Testcrypto: ivdata -rfc 4106 after kfree_aaa:\n" );

    if (AAD)
        kfree_aaa(AAD);
    pr_aaa(KERN_INFO "Module Testcrypto: AAD -rfc 4106 after kfree_aaa:\n" );
  
    if (scratchpad)
        kfree_aaa(scratchpad);
    pr_aaa(KERN_INFO "Module Testcrypto: scratchpad -rfc 4106 after kfree_aaa:\n" );

    if(ad->sg)
        kfree_aaa(ad->sg);
    pr_aaa(KERN_INFO "Module Testcrypto: ad->sg -rfc 4106 after kfree_aaa:\n" );

    if(packet)
        kfree_aaa(packet);
    pr_aaa(KERN_INFO "Module Testcrypto: authentag -rfc4106 after kfree_aaa:\n");


    if(authentag)
        kfree_aaa(authentag);
    pr_aaa(KERN_INFO "Module Testcrypto: packet -rfc4106 after kfree_aaa:\n");

 
    if(ciphertext)
        kfree_aaa(ciphertext);
    pr_aaa(KERN_INFO "Module Testcrypto: ciphertext -rfc4106 after kfree_aaa:\n");

    if(ad)
        kfree_aaa(ad);
    pr_aaa(KERN_INFO "Module Testcrypto: ad -rfc4106 after kfree_aaa:\n");
 
    return ret;
}

    int i;

static int __init test_init(void)
{
    //int cipher_choice = 2;// 0 for cbc(skcipher), 1 for gcm(aead), 2 for gcm-esp(aead)
    //int test_choice = 2;
    //int engine_type = 0; // 0 for software, 1 for instruction set,2 for hardware engine
    //int endec = MODE_DIR_ENCRYPT;
    // pr_err(KERN_INFO "Module testcrypto: info: init test \n Pid: %d", current->pid);
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
        case 7:
            for (i =0 ; i < test7_len.key_len;i++)
            {
                key_const2[i]=key_test7[i];
            }
            for (i=0; i < test7_len.AAD_len; i++)
            {
                AAD_const2[i]=AAD_test7[i];
            }
            for (i=0; i < test7_len.scratchpad_len; i++)
            {
                scratchpad_const2[i]=scratchpad_test7[i];
            }
            for (i=0; i < test7_len.scratchpad_len; i++)
            {
                ciphertext_const2[i]=ciphertext_test7[i];
            }
            for (i=0; i< test7_len.ivdata_len; i++)
            {
                ivdata_const2[i]= ivdata_test7[i];
            }
            for (i=0; i< 16; i++)
            {
                authentag_const2[i]= authentag_test7[i];
            }
            break;
    }
    // Set up timer and callback handler (using for testing).
	timer_setup(&testcrypto_ktimer,handle_timer,0);
    // setup timer interval to based on TIMEOUT Macro
    mod_timer(&testcrypto_ktimer, jiffies + 60*HZ);

    while(!done_flag)
    {
    if (cipher_choice == 3)
    {
        test_esp_rfc4106(test_choice,endec);
        // mdelay(300);
        // pr_aaa("--------------------------%d-------------------: %s - PID:%d\n",__LINE__ , __func__ ,  current->pid);
        // pr_err("------------------------Number of req-------------------: %d\n",count);
        count ++;
    }
    }
    // mdelay(1000);
    return 0;
}

static void __exit test_exit(void)
{
    pr_aaa(KERN_INFO "Module testcrypto: info: exit test\n");
    del_timer_sync(&testcrypto_ktimer);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Duy H.Ho");
MODULE_DESCRIPTION("A prototype Linux module for TESTING crypto in FPGA-PCIE card");
MODULE_VERSION("0.01");