
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
#define MAX_REQ (16)
#define DATA_LENGTH (1888/4)
#define TESTCASE_1_LENGTH (28)

// Just for debug, dont care

void *get_base(void);
struct xdma_crdev *get_crdev(void);
struct scatterlist sg[MAX_REQ*2];

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

// This is callback function

int crypto_complete(struct xfer_req *data, int res)
{
    char *buf;
    int i = 0;

    // Step 4: Get data in callback
    struct scatterlist *sg = data->sg_out;

    pr_err("Complete with res = %d ! This is callback function! \n", res);
    // Step 5: Do your things - Here we print the data out
    
    buf = sg_virt (sg);
    for (i = 0; i < TESTCASE_1_LENGTH; i += 16)
    {
        pr_err("address = %3.3x , data = %8.0x %8.0x %8.0x %8.0x \n", i ,
            *((u32 *)(&buf[i + 12])), *((u32 *)(&buf[i + 8])), 
            *((u32 *)(&buf[i + 4])), *((u32 *)(&buf[i])));
    }
    pr_err("tag = %8.0x %8.0x %8.0x %8.0x \n", 
            *(data->tag + 3), *(data->tag + 2),
            *(data->tag + 1), *(data->tag));
    // Step 6: Free xfer_req
    kfree(buf); // Skip this if it is not necessary
    kfree(data->tag);
    free_xfer_req(data); // data is xfer_req

    return 0;
}

// Begin of example module

/*

    xfer: example mode
        0: print queue only
            print_xmit_list: list of submitted reqs which are not sent to card
            print_processing_list: list of submitted reqs which are sent to card
            print_xmit_deliver_list: list of reqs which are ready for callback
        1: send req and print queue
    req_num: number of transfer (packet)
        default: 1

*/

u32 i, j;
int res;
struct xfer_req *req[MAX_REQ];
u32 *buff[MAX_REQ];
struct bkcrypto_context ctx;


static int __init test_init(void)
{
    u32 i, j;
    int res;
    struct xfer_req *req[MAX_REQ];
    u32 *buff[MAX_REQ];
    struct bkcrypto_context ctx;

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
            set_sg_in(req[i], &sg[i]);
            set_sg_out(req[i], &sg[i]);
            set_callback(req[i], &crypto_complete);
                // Set value for ctx (testcase_1)
                    // INFO
            req[i]->crypto_dsc.info = testcase_6_in.crypto_dsc.info;
                    // ICV
            memcpy(req[i]->crypto_dsc.icv, testcase_6_in.crypto_dsc.icv, ICV_SIZE); 
                    // KEY
            memcpy(req[i]->crypto_dsc.key, testcase_6_in.crypto_dsc.key, KEY_SIZE); 
                    // IV
            req[i]->crypto_dsc.iv = testcase_6_in.crypto_dsc.iv;
                    // AAD
            memcpy(req[i]->crypto_dsc.aad, testcase_6_in.crypto_dsc.aad, AAD_SIZE); 

            set_ctx(req[i], ctx);

            // Set outbound info -- testcase 1
            set_tag(req[i], 16, 0x40, (u32 *)kmalloc(16, GFP_ATOMIC | GFP_KERNEL));
        }
        // Set value for buffer - skip if you had buffer
        for (i = 0; i <  req_num; i ++){
            for (j =0; j < TESTCASE_1_LENGTH/4; j++){
                buff[i][j] = testcase_6_in.data[j];
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
    return 0;
}

static void __exit test_exit(void)
{
	pr_err("Bye! End of example");
}

module_init(test_init);
module_exit(test_exit);