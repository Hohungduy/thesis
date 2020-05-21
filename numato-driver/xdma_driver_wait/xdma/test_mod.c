
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/aer.h>
/* include early, to verify it depends only on the headers above */
#include "libxdma.h"
#include "version.h"
#include "xdma_crypto.h"
#include "linux/scatterlist.h"

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_LICENSE("Dual BSD/GPL");

static unsigned int req_num = 1;
module_param(req_num, uint, 1);
MODULE_PARM_DESC(req_num, "request and number");

#define MAX_REQ (15)


static int __init test_init(void)
{
    int i, j;
    struct xfer_req *req[MAX_REQ];
    char *buff[MAX_REQ];
    struct scatterlist sg[MAX_REQ];

    for (i = 0; i <  req_num; i ++){
        for (j =0; j < i*8 + 1400; j++){
            buff[i][j] = (i & 0x0F ) | ((j & 0x0F) << 4);
        }
    }

    for (i = 0; i < req_num; i++){
        req[i] = alloc_xfer_req();
        buff[i] = (char *)kmalloc(i*8 + 1400, GFP_KERNEL);
        sg_init_one(&sg[i], buff[i], i*8 + 1400);

        req[i]->sg = &sg[i];
    }



    for (i = 0; i <  req_num; i ++){
        xdma_xfer_submit_queue(req[i]);
        pr_info("submit %d \n", i);
    }

    return 0;
}

static void __exit test_exit(void)
{
	
}

module_init(test_init);
module_exit(test_exit);