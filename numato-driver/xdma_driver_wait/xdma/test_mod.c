
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
// #include <linux

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_LICENSE("Dual BSD/GPL");

static unsigned int req_num = 1;
module_param(req_num, uint, 0000);
MODULE_PARM_DESC(req_num, "request and number");

static unsigned int xfer = 0;
module_param(xfer, uint, 0000);
MODULE_PARM_DESC(xfer, "there is or not xfer");

#define MAX_REQ (15)

int get_sg_from_buf(void *buff, struct scatterlist *sg, int size)
{
    struct page *pg;

    unsigned int offset = offset_in_page(buff);
    unsigned int nbytes = 
        min_t(unsigned int, PAGE_SIZE - offset, size);
    pr_info("aae");
    pg = virt_to_page(buff);
    if (!pg){
        pr_info("Cannot convert buffer to page");
        return FALSE;
    }
    pr_info("aaj");
    flush_dcache_page(pg);
    sg_set_page(sg, pg, nbytes, offset);

    return true;
}

static int __init test_init(void)
{
    int i, j;
    struct xfer_req *req[MAX_REQ];
    char *buff[MAX_REQ];
    struct scatterlist *sg;
    pr_info("hjuiutyhrh");
    if (xfer == 1)
    {
        sg = (struct scatterlist *)kmalloc(req_num*sizeof(*sg), GFP_ATOMIC | GFP_KERNEL);
        // pr_info("aaa");
        for (i = 0; i < req_num; i++){
            // pr_info("aab");
            req[i] = alloc_xfer_req();
            req[i]->id = current->pid;
            buff[i] = (char *)kmalloc(i*8 + 1400, GFP_KERNEL | GFP_ATOMIC);
            // sg_init_one(&sg[i], buff[i], i*8 + 1400);
            // pr_info("aac");
            get_sg_from_buf(buff[i], &sg[i], i*8 + 1400 );

            req[i]->sg = &sg[i];
        }

        for (i = 0; i <  req_num; i ++){
            // pr_info("saa");
            for (j =0; j < i*8 + 1400; j++){
                buff[i][j] = (i & 0x0F ) | ((j & 0x0F) << 4);
                // pr_info("buff[%d][%d] = %d",i, j, buff[i][j]);
            }
        }

        for (i = 0; i <  req_num; i ++){
            xdma_xfer_submit_queue(req[i]);
            pr_info("submit %d \n", i);
        }
    }
    pr_info("hjuiuh");
    print_req_queue();
    pr_info("hjuiuhaaa");
    print_req_processing();

    return 0;
}

static void __exit test_exit(void)
{
	
}

module_init(test_init);
module_exit(test_exit);