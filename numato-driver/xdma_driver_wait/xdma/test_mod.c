
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
// #include <linux

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_LICENSE("Dual BSD/GPL");

static unsigned int req_num = 1;
module_param(req_num, uint, 0000);
MODULE_PARM_DESC(req_num, "request and number");

static unsigned int xfer = 0;
module_param(xfer, uint, 0000);
MODULE_PARM_DESC(xfer, "there is or not xfer");

static unsigned int write_mem = 0;
module_param(write_mem, uint, 0000);
MODULE_PARM_DESC(write_mem, "there is or not xfer");

#define MAX_REQ (15)

void *get_base(void);

int get_sg_from_buf(void *buff, struct scatterlist *sg, int size)
{
    struct page *pg;

    unsigned int offset = offset_in_page(buff);
    unsigned int nbytes = 
        min_t(unsigned int, PAGE_SIZE - offset, size);
    // pr_info("aae");
    pg = virt_to_page(buff);
    if (!pg){
        pr_info("Cannot convert buffer to page");
        return FALSE;
    }
    // pr_info("aaj");
    flush_dcache_page(pg);
    sg_set_page(sg, pg, nbytes, offset);

    return true;
}

static int __init test_init(void)
{
    __uint128_t i, j;
    struct xfer_req *req[MAX_REQ];
    char *buff[MAX_REQ];
    struct scatterlist *sg;

    if (xfer)
    {
        sg = (struct scatterlist *)kmalloc(req_num*sizeof(*sg), GFP_ATOMIC | GFP_KERNEL);
        for (i = 0; i < req_num; i++){
            req[i] = alloc_xfer_req();
            req[i]->id = current->pid;
            buff[i] = (char *)kmalloc(i*8 + 1400, GFP_KERNEL | GFP_ATOMIC);
            get_sg_from_buf(buff[i], &sg[i], i*8 + 1400 );
            req[i]->sg = &sg[i];
        }

        for (i = 0; i <  req_num; i ++){
            for (j =0; j < i*8 + 1400; j++){
                buff[i][j] = (i & 0x0F ) | ((j & 0x0F) << 4);
            }
        }

        for (i = 0; i <  req_num; i ++){
            xdma_xfer_submit_queue(req[i]);
            pr_info("submit %d \n", i);
        }
    }
    if (write_mem)
    {
        struct base *base = 
            (struct base *)get_base();
        struct outbound * out_base = &base->engine[0]->out;
        for (i = 2; i < REGION_NUM; i++)
        {
            __uint128_t write_bytes = 0;
            __uint128_t region_descriptor = 0xABCDABCD;
            __uint128_t xfer_id = i;
            __uint128_t xfer_dsc = 0;
            __uint128_t crypto_result = 1;
            __uint128_t data_len = i*8 + 1400;
            write_bytes = region_descriptor | (xfer_id << 32)
                | (xfer_dsc << 64) | (crypto_result << 72) | (data_len << 80); 
            memcpy_toio(&out_base->region[i].region_dsc, &write_bytes, 16);
            // iowrite32(0xABCDABCD, &out_base->region[i].region_dsc);
            // iowrite32(0 | (1 << 8) | (i*8 + 1400) << 16, &out_base->region[i].xfer_dsc);
            
            for (j =0; j < ((i*8 + 1400)/16); j++){
                write_bytes = (i << 96) | j;
                // write_bytes = 0;
                memcpy_toio((__uint128_t *)&out_base->region[i].data + j, &write_bytes, 16);
            }
        }
        iowrite32(2,&base->engine[0]->comm.tail_outb);
        iowrite32(8,&base->engine[0]->comm.head_outb);
    }

    print_req_queue();
    print_req_processing();

    return 0;
}

static void __exit test_exit(void)
{
	
}

module_init(test_init);
module_exit(test_exit);