
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/aer.h>
/* include early, to verify it depends only on the headers above */
#include "libxdma.h"
#include "version.h"
#include "xdma_crypto.h"
#include "linux/scatterlist.h"
#include <linux/mm.h>
#include <asm/cacheflush.h>
#include "xdma_region.h"
#include "xdma_crypto.h"
#include "crypto_testcases.h"
#include "common.h"

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_LICENSE("Dual BSD/GPL");

static unsigned int req_num = 1;
module_param(req_num, uint, 0000);
MODULE_PARM_DESC(req_num, "request and number");

static unsigned int xfer = 1;
module_param(xfer, uint, 0000);
MODULE_PARM_DESC(xfer, "there is or not xfer");

// This is a kernel module as an example to use crypto function


// Modify if it is neccessary
#define MAX_REQ (15)
#define DATA_LENGTH (1888/4)
// #define TESTCASE_1_DATA_LENGTH (20)

// Just for debug, dont care

void *get_base(void);
struct xdma_crdev *get_crdev(void);
struct scatterlist sg[MAX_REQ*2];

// Skip if scatterlist is okay

int get_sg_from_buf(void *buff, struct scatterlist *sg, int size)
{
    struct page *pg;

    unsigned int offset = offset_in_page(buff);
    unsigned int nbytes = 
        min_t(unsigned int, PAGE_SIZE - offset, size);
    // pr_err("aae");
    pg = virt_to_page(buff);
    if (!pg){
        pr_err("Cannot convert buffer to page");
        return 0;
    }
    // pr_err("aaj");
    flush_dcache_page(pg);
    sg_set_page(sg, pg, nbytes, offset);

    return true;
}

// This is callback function

int crypto_complete(void *data, int res)
{
    char *buf;
    int i = 0;

    // Step 4: Get data in callback
    struct scatterlist *sg = ((struct xfer_req *)data)->sg_out;

    // Step 5: Do your things - Here we print the data out
    pr_err("complete ehehe %d\n", res);
    buf = sg_virt (sg);
    for (i = 0; i < 1500; i += 4)
    {
        pr_err("address = %i , data = %x",i,  *((u32 *)(&buf[i])));
    }

    // Step 6: Free xfer_req
    kfree(buf); // Skip this if it is not necessary
    free_xfer_req(data); // data is xfer_req

    return 0;
}

// Begin of example module

/*

    xfer: example mode
        0: print queue only
            print_xmit_list: list of submitted reqs which are not sent to card
            print_processing_list: list of submitted reqs which are sent to card
            print_deliver_list: list of reqs which are ready for callback
        1: send req and print queue
    req_num: number of transfer (packet)
        default: 1

*/

u32 i, j;
int res;
struct xfer_req *req[MAX_REQ];
u32 *buff[MAX_REQ];
// struct xdma_crdev *crdev;
// struct list_head *processing;
// unsigned long flags;
struct mycrypto_context ctx;

#define TESTCASE_1_LENGTH (72)

static int __init test_init(void)
{
    u32 i, j;
    int res;
    struct xfer_req *req[MAX_REQ];
    u32 *buff[MAX_REQ];
    // struct xdma_crdev *crdev;
    // struct list_head *processing;
    // unsigned long flags;
    struct mycrypto_context ctx;



    if (xfer)
    {
        // Prepare sg list - skip if your sg list is ready
        for (i = 0; i < req_num; i++){
            // Step 1: Allocate xfer_req and check
            req[i] = alloc_xfer_req();
            if (req[i] == 0)
                return 0;

            // Allocate buffer for sg - skip if you had sg
            buff[i] = (u32 *)kzalloc(TESTCASE_1_LENGTH*4, GFP_KERNEL | GFP_ATOMIC);
            sg_init_one(&sg[i], buff[i], TESTCASE_1_LENGTH*4 );
            sg_mark_end(&sg[i]);
            // Step 2: Set value for req
            set_sg(req[i], &sg[i]);
            set_callback(req[i], crypto_complete);
                // Set value for ctx (testcase_1)
                    // INFO
#define KEYSIZE_128 (0)
#define KEYSIZE_192 (1)
#define KEYSIZE_256 (2)
            req[i]->crypto_dsc.info = testcase_1_in.crypto_dsc.info;
#define AADSIZE_8 (8)
#define AADSIZE_12 (12)
#define DIRECTION_ENCRYPT (1)
#define DIRECTION_DECRYPT (0)
                    // ICV
#define ICV_SIZE (4*4)
            memcpy(req[i]->crypto_dsc.icv, testcase_1_in.crypto_dsc.icv, ICV_SIZE); 
                    // KEY
#define KEY_SIZE (8*4)
            memcpy(req[i]->crypto_dsc.key, testcase_1_in.crypto_dsc.key, KEY_SIZE); 
                    // IV
            req[i]->crypto_dsc.iv = testcase_1_in.crypto_dsc.iv;
                    // AAD
#define AAD_SIZE (4*4)
            memcpy(req[i]->crypto_dsc.aad, testcase_1_in.crypto_dsc.aad, AAD_SIZE); 

            set_ctx(req[i], ctx);
        }
        // Set value for buffer - skip if you had buffer
        for (i = 0; i <  req_num; i ++){
            for (j =0; j < TESTCASE_1_LENGTH/4; j++){
                buff[i][j] = testcase_1_in.data[j];
            }
        }

        // Step 3: Submit to card
        for (i = 0; i <  req_num; i ++){
            res = xdma_xfer_submit_queue(req[i]);
            if (res != -EINPROGRESS)
                pr_err("Unusual result\n");
            pr_err("submitted req %d \n", i);
        }
    }


    // pr_err("Test module print\n");
    // print_xmit_list();
    // print_deliver_list();
    // print_processing_list();
    // pr_err("Test module print end\n");
    // while (1)
    // {
    //     /* code */
    // }
    

    return 0;
}

static void __exit test_exit(void)
{
	pr_err("Bye! End of example");
}

module_init(test_init);
module_exit(test_exit);