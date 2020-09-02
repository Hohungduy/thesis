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


//#define TESTCRYPTO_DEBUG
//#define ASYNC 
//#define MULTIPLE_TESTCASES


#define TIMEOUT 5000 //miliseconds

#ifdef TESTCRYPTO_DEBUG
static unsigned int req_num = 20;
module_param(req_num, uint, 0000);
MODULE_PARM_DESC(req_num, "request and number");
#endif

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


//---------------------------------------------------------------------------------------------
/* Test GCM-AES in ESP mode (RFC 4106) - Type: AEAD cipher */
//---------------------------------------------------------------------------------------------
/* Callback function for aead type */

static unsigned int test_rfc4106_encdec(struct aead_def *ad, int enc)
{
    int rc;
#ifdef TESTCRYPTO_DEBUG
    pr_err(KERN_INFO "Module testcrypto: STARTING test_rfc4106_encdec\n");
    pr_err("Module testcrypto.c: Address of req->iv:%p - data =  %8.0x %8.0x \n",ad->req->iv,  
            *((u32 *)(&ad->req->iv[4])), *((u32 *)(&ad->req->iv[0])));
#endif
    if (enc)
        rc = crypto_aead_encrypt(ad->req);
    else
        rc = crypto_aead_decrypt(ad->req);
#ifdef TESTCRYPTO_DEBUG
    pr_err(KERN_INFO "AFTER crypto_aead_encrypt\n");
#endif
    ad->done = false;

    switch (rc) 
    {
    case 0:
#ifdef TESTCRYPTO_DEBUG
        pr_err("Module testcrypto: Case 0 \n");
#endif
        break;
    case -EINPROGRESS:
    case -EBUSY:
#ifdef TESTCRYPTO_DEBUG
        pr_err("Module testcrypto: Case einprogress + ebusy \n");
#endif
        rc = wait_for_completion_interruptible(&ad->result.completion);

        if ((!rc) && (!ad->result.err)) {
            reinit_completion(&ad->result.completion);
                    }
        break;
    default:
        break;
    }
    init_completion(&ad->result.completion);
        return rc;
}



static void test_rfc4106_cb(struct crypto_async_request *req, int error)
{
    struct aead_def *ad = req->data;
    struct tcrypt_result *result = &ad->result;
    
    #ifdef ASYNC

    struct scatterlist *sg = ad->sg;
    struct crypto_aead *aead =ad->tfm;
    struct aead_request *aead_req = ad->req;
    char *scratchpad = ad->scratchpad;
    char *AAD =ad->AAD;
    char *ivdata =ad->ivdata;
    char *authentag =ad->authentag;
    char *ciphertext =ad->ciphertext;
    char *packet =ad->packet; 
    u8 *done=req->data;
    count ++;

    #endif
#ifdef TESTCRYPTO_DEBUG
    pr_err(KERN_INFO "Module testcrypto: STARTING test_rfc4106_cb\n");
#endif
    if(error == -EINPROGRESS)
    {
        return;

    }
    result->err = error;
    
    complete(&result->completion);
    
    #ifdef ASYNC

    if (aead)
        crypto_free_aead(aead);
    if (aead_req)
        aead_request_free(aead_req);
    if (ivdata)
        kfree(ivdata);
    if (AAD)
        kfree(AAD);
    if (scratchpad)
        kfree(scratchpad);
    if(sg)
        kfree(sg);
    if(packet)
        kfree(packet);
    if(authentag)
        kfree(authentag);
    if(ciphertext)
        kfree(ciphertext);
    if(ad)
        kfree(ad);
    pr_err(KERN_INFO "Module testcrypto: Encryption finishes successfully\n");

    #endif

}


//------Test_esp_rfc4106------
/* Initialize and trigger aead cipher operation */
static int test_esp_rfc4106(int test_choice, int endec)
{
    struct crypto_aead *aead = NULL;
    struct aead_request *aead_req = NULL;
    /* Buffer for storing data*/
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

#ifdef TESTCRYPTO_DEBUG
    size_t len;
#endif
    struct aead_def *ad;

    ad = kzalloc(sizeof(struct aead_def) , GFP_KERNEL | GFP_DMA);
    
    if(!ad){
        pr_err("could not allocate ad struct\n");
        ret = -ENOMEM;
        goto out;
    }
#ifdef TESTCRYPTO_DEBUG
    pr_err(KERN_INFO "Module testcrypto: starting test_esp_rfc4106\n");
#endif
    switch(test_choice)
    {
        case 1:
#ifdef TESTCRYPTO_DEBUG
            pr_err("This is test case 1\n");
#endif
            assoclen = test1_len.AAD_len;
            keylen = test1_len.key_len;
            data_len = test1_len.scratchpad_len;
            break;
            // do not need ivlen
        case 2:
#ifdef TESTCRYPTO_DEBUG
            pr_err("This is test case 2\n");
#endif
            assoclen = test2_len.AAD_len;
            keylen = test2_len.key_len;
            data_len = test2_len.scratchpad_len;
            // do not need ivlen
            break;
        case 3:
#ifdef TESTCRYPTO_DEBUG
            pr_err("This is test case 3\n");
#endif
            assoclen = test3_len.AAD_len;
            keylen = test3_len.key_len;
            data_len = test3_len.scratchpad_len;
            // do not need ivlen
            break;
        case 4:
#ifdef TESTCRYPTO_DEBUG
            pr_err("This is test case 4\n");
#endif
            assoclen = test4_len.AAD_len;
            keylen = test4_len.key_len;
            data_len = test4_len.scratchpad_len;
            // do not need ivlen
            break;
        case 5:
#ifdef TESTCRYPTO_DEBUG
            pr_err("This is test case 5\n");
#endif
            assoclen = test5_len.AAD_len;
            keylen = test5_len.key_len;
            data_len = test5_len.scratchpad_len;
            // do not need ivlen
            break;
        case 6:
#ifdef TESTCRYPTO_DEBUG
            pr_err("This is test case 5\n");
#endif
            assoclen = test6_len.AAD_len;
            keylen = test6_len.key_len;
            data_len = test6_len.scratchpad_len;
            // do not need ivlen
            break;     
        case 7:
#ifdef TESTCRYPTO_DEBUG
            pr_err("This is test case 5\n");
#endif
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
        pr_err("could not allocate aead handle (rfc 4106)\n");
        return PTR_ERR(aead);
    }
    /* Allocate a request object */
    aead_req = aead_request_alloc(aead, GFP_KERNEL | GFP_DMA);
    if (!aead_req) {
        pr_err("could not allocate aead request (rfc 4106)\n");
        ret = -ENOMEM;
        goto out;
    }

    
    /*Check the size of iv and authentication data (Tag for authentication)  */
    authlen = crypto_aead_authsize(aead);
    ivlen = crypto_aead_ivsize(aead);

#ifdef TESTCRYPTO_DEBUG
    pr_err("Module testcrypto: Address of aead_req:%p \n",aead_req);
    pr_err("%d: Module testcrypto - aead_req pointer:%p - asys_req pointer offset:%x \
    - assoc pointer offset:%x - cryptlen pointer offset:%x - iv pointer offset:%x \
    - src pointer offset:%x - dst pointer offset:%x - ctx pointer offset(SIZEOF:%x):%x\n",
    __LINE__, aead_req , offsetof(struct aead_request, base),offsetof(struct aead_request,assoclen),
    offsetof(struct aead_request,cryptlen),offsetof(struct aead_request,iv),
    offsetof(struct aead_request,src),offsetof(struct aead_request,dst),offsetof(struct aead_request,__ctx),sizeof(void *));

    pr_err(KERN_INFO "authentication size: %d\n", authlen);
    pr_err(KERN_INFO "iv size:%d \n", ivlen);
    
#endif
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
    if (crypto_aead_setkey(aead, key, 20)) 
    {
            pr_err("key could not be set\n");
            ret = -EAGAIN;
            goto out;
    }
#ifdef TESTCRYPTO_DEBUG
    pr_err(KERN_INFO "Key value :\n");
    pr_err(KERN_INFO "%x\t%x\t%x\t%x \n",key[0],key[1],key[2],key[3]);
    pr_err(KERN_INFO "%x\t%x\t%x\t%x \n",key[4],key[5],key[6],key[7]);
    pr_err(KERN_INFO "%x\t%x\t%x\t%x \n",key[8],key[9],key[10],key[11]);
    pr_err(KERN_INFO "%x\t%x\t%x\t%x \n",key[12],key[13],key[14],key[15]);
    pr_err(KERN_INFO "Salt in Nonce: %x\t%x\t%x\t%x \n",key[16],key[17],key[18],key[19]);
#endif
    /* IV will be random */
    ivdata = kzalloc(ivlen, GFP_KERNEL | GFP_DMA);

    if (!ivdata) {
        pr_err("could not allocate ivdata\n");
        goto out;
    }
    //get_random_bytes(ivdata, 12);
    for (i_iv =0 ; i_iv < ivlen;i_iv++)
    {
        ivdata[i_iv]=ivdata_const2[i_iv];
    }
    
#ifdef TESTCRYPTO_DEBUG

    pr_err(KERN_INFO "IV: \n");
    for(i_iv = 0; i_iv < ivlen ; i_iv+=16)
    {
    pr_err("data = %8.0x %8.0x \n", 
            *((u32 *)(&ivdata[4])), *((u32 *)(&ivdata[0])));
    }
#endif
    /* AAD value (comprise of SPI and Sequence number)-64 bit 
       ( not with extended sequence number-96 bit) */

    AAD = kzalloc(assoclen, GFP_KERNEL | GFP_DMA);
    if (!AAD) {
        pr_err("could not allocate scratchpad\n");
        goto out;
    }

    for (i_AAD = 0; i_AAD < assoclen; i_AAD ++)
    {
        AAD[i_AAD] = AAD_const2[i_AAD];
    }

#ifdef TESTCRYPTO_DEBUG
    
    pr_err("assoclen:%d\n",assoclen);
    pr_err(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    pr_err(KERN_INFO "Ascociated Authenctication Data (SPI+Sequence number): \n");
    switch(assoclen)
    {
        case 8:
            pr_err("data = %8.0x %8.0x  \n", 
                *((u32 *)(&AAD[4])), *((u32 *)(&AAD[0])));
        break;
        case 12:
            pr_err("data = %8.0x %8.0x %8.0x \n",
                *((u32 *)(&AAD[8])), *((u32 *)(&AAD[4])), *((u32 *)(&AAD[0])));
        break;
    }
#endif

    /* Input data will be random */
    scratchpad = kzalloc(DATA_LENGTH, GFP_KERNEL | GFP_DMA );
    if (!scratchpad) {
        pr_err("could not allocate scratchpad\n");
        goto out;
    }

    //get_random_bytes(scratchpad, 16);
    // 60 bytes

    for (i_data = 0 ; i_data < data_len;i_data++)
    {
        scratchpad[i_data] = scratchpad_const2[i_data];
    }
    
#ifdef TESTCRYPTO_DEBUG
    
    pr_err("datalen:%d",data_len);
    pr_err(KERN_INFO "Data payload :\n");
    for(i_data = 0; i_data < data_len ; i_data +=16)
    {
    pr_err("data = %8.0x %8.0x %8.0x %8.0x \n",
            *((u32 *)(&scratchpad[i_data+12])),*((u32 *)(&scratchpad[i_data+8])), 
            *((u32 *)(&scratchpad[i_data+4])), *((u32 *)(&scratchpad[i_data])));
    }
#endif

    /*Input authentication tag*/
    authentag = kzalloc(16,GFP_KERNEL | GFP_DMA );
    if (!authentag) {
        pr_err("could not allocate packet\n");
        goto out;
    }
 
    for (i_authen = 0 ; i_authen < 16;i_authen++)
    {
        authentag[i_authen]=authentag_const2[i_authen];
    }

#ifdef TESTCRYPTO_DEBUG
    pr_err(KERN_INFO "Authentag :\n");
    for(i_authen = 0; i_authen < 16 ; i_authen +=16)
    {
        pr_err("data = %8.0x %8.0x %8.0x %8.0x \n",
            *((u32 *)(&authentag[12])),*((u32 *)(&authentag[8])), 
            *((u32 *)(&authentag[4])), *((u32 *)(&authentag[0])));
    }
#endif
    
    /*Input cipher text content*/

    ciphertext = kzalloc(DATA_LENGTH, GFP_KERNEL | GFP_DMA );
    if (!ciphertext) {
        pr_err("could not allocate ciphertext\n");
        goto out;
    }

    for (i_data = 0 ; i_data < data_len;i_data++)
    {
        ciphertext[i_data]=ciphertext_const2[i_data];
    }
 
#ifdef TESTCRYPTO_DEBUG

    pr_err(KERN_INFO "Data cipher payload :\n");
    for(i_data = 0; i_data < data_len ; i_data +=16)
    {
    pr_err("address of ciphertext = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i_data ,
            *((u32 *)(&ciphertext[i_data + 12])),*((u32 *)(&ciphertext[i_data + 8])), 
            *((u32 *)(&ciphertext[i_data + 4])), *((u32 *)(&ciphertext[i_data])));
    }

#endif
    /*Input packet content*/
    packet = kzalloc(PACKET_LENGTH,GFP_KERNEL | GFP_DMA);
    if (!packet) {
        pr_err("could not allocate packet\n");
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

    aead_request_set_ad(aead_req , assoclen + ivlen);// 12 means that there are 12 octects ( 96 bits) in AAD fields.
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

    scatterwalk_map_and_copy(ivdata, aead_req->dst, aead_req->assoclen-ivlen, ivlen, 1);
    init_completion(&ad->result.completion);
    /* for printing */

    
#ifdef TESTCRYPTO_DEBUG
    pr_err(KERN_INFO "BEFORE allocate data and AAD into sg list of request\n");
    print_sg_content(ad->req->src);

    len = (size_t)ad->req->cryptlen + (size_t)ad->req->assoclen + (size_t)authlen + ivlen;
    pr_err("IV before encrypt (in esp->iv): \n");
    pr_err("Address of aead_req->iv:%p - data = %8.0x %8.0x - Offset:%x\n",&(aead_req->iv), 
            *((u32 *)(&aead_req->iv[4])), *((u32 *)(&aead_req->iv[0])),offsetof(struct aead_request,iv));
    pr_err("Module testcrypto: Address of aead_req:%p - assoclen+Cryptlen =  %d %d \n",aead_req,  
            aead_req->assoclen, aead_req->cryptlen);
        	    
    pr_err(KERN_INFO "length packets: %d \n", len);
    pr_err(KERN_INFO "%d packet - BEFORE function test encrypt:\n", __LINE__);
    pr_err("%d: %s - PID:%d - pointer of req.data:%p\n",__LINE__ , __func__ ,  current->pid , aead_req->base.data);
    pr_err("%d: %s - PID:%d - pointer of req.data:%p\n",__LINE__ , __func__ ,  current->pid , ad->req->base.data);
#endif
    
    /* encrypt data ( 1 for encryption)*/

    
    // while (!done_flag)
    // {
    ret = test_rfc4106_encdec(ad, endec);
    // count ++;
    // }

    if (ret){
        pr_err("Error encrypting data: %d\n", ret);
        goto out;
    } 
#ifdef TESTCRYPTO_DEBUG
    print_sg_content(ad->req->src);
    pr_err("IV after encrypt (in esp->iv): \n");
    pr_err("Address of aead_req->iv:%p - data = %8.0x %8.0x - Offset:%x\n",&(aead_req->iv), 
            *((u32 *)(&aead_req->iv[4])), *((u32 *)(&aead_req->iv[0])),offsetof(struct aead_request,iv));
    pr_err("Module testcrypto:Data after test_rfc4106_encdec \n");
    pr_err(KERN_INFO "Module Testcrypto: Encryption triggered successfully\n");
#endif
        

out:

#ifdef ASYNC

#else
    if (aead)
        crypto_free_aead(aead);

    if (aead_req)
        aead_request_free(aead_req);

    if (ivdata)
        kfree(ivdata);

    if (AAD)
        kfree(AAD);
  
    if (scratchpad)
        kfree(scratchpad);

    if(ad->sg)
        kfree(ad->sg);

    if(packet)
        kfree(packet);

    if(authentag)
        kfree(authentag);

    if(ciphertext)
        kfree(ciphertext);

    if(ad)
        kfree(ad);
    
#endif
    return ret;
}

    int i;

static int __init test_init(void)
{
#ifdef MULTIPLE_TESTCASES
            /* Test case 1 */
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
            if (cipher_choice == 3)
            {
                test_esp_rfc4106(1,endec);
            }
             /* Test case 2 */
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
            
            if (cipher_choice == 3)
            {
                test_esp_rfc4106(2,endec);
            } 
             /* Test case 3 */          
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
            if (cipher_choice == 3)
            {
                test_esp_rfc4106(3,endec);
            }
             /* Test case 4 */
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

            if (cipher_choice == 4)
            {
                test_esp_rfc4106(1,endec);
            }
             /* Test case 5 */
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
            if (cipher_choice == 5)
            {
                test_esp_rfc4106(1,endec);
            }

#else
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

#ifdef ASYNC

#else
        count ++;
#endif
        }
    }

#endif
    return 0;
}

static void __exit test_exit(void)
{
    pr_err(KERN_INFO "Module testcrypto: info: exit test\n");
    del_timer_sync(&testcrypto_ktimer);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Duy H.Ho");
MODULE_DESCRIPTION("A prototype Linux module for TESTING crypto in FPGA-PCIE card");
MODULE_VERSION("0.01");