
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
struct xdma_crdev *get_crdev(void);


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
        return 0;
    }
    // pr_info("aaj");
    flush_dcache_page(pg);
    sg_set_page(sg, pg, nbytes, offset);

    return true;
}

int crypto_complete(void *data, int res)
{
    
    char *buf;
    int i = 0;
    struct scatterlist *sg = ((struct xfer_req *)data)->sg;

    pr_info("complete ehehe %d\n", res);
    buf = sg_virt (sg);
    for (i = 0; i < 1500; i += 4)
    {
        pr_info("address = %i , data = %x",i,  *((u32 *)(&buf[i])));
    }
    return 0;
}

static int __init test_init(void)
{
    __uint128_t i, j;
    struct xfer_req *req[MAX_REQ];
    char *buff[MAX_REQ];
    struct scatterlist *sg;
    struct xdma_crdev *crdev;
    struct list_head *processing;
    unsigned long flags;

    if (xfer)
    {
        sg = (struct scatterlist *)kmalloc(req_num*sizeof(*sg), GFP_ATOMIC | GFP_KERNEL);
        for (i = 0; i < req_num; i++){
            req[i] = alloc_xfer_req();
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
            pr_info("submit %lud \n", (unsigned long int)i);
        }
    }
    if (write_mem)
    {
        struct base *base = 
            (struct base *)get_base();
        struct outbound * out_base = &base->engine[0]->out;
#define begin_test (1)
#define end_test (6)
        
        for (i = begin_test; i < end_test; i++)
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
        iowrite32(1,&base->engine[0]->comm.tail_outb);
        iowrite32(6,&base->engine[0]->comm.head_outb);

        crdev = get_crdev();
        processing = &crdev->agent[0].processing_queue;
        
        
        sg = (struct scatterlist *)kmalloc(5*sizeof(*sg), GFP_ATOMIC | GFP_KERNEL);
        for (i = begin_test; i < end_test; i++){
            req[i] = alloc_xfer_req();
            req[i]->id = i;
            buff[i] = (char *)kzalloc(i*8 + 1400, GFP_KERNEL | GFP_ATOMIC);
            get_sg_from_buf(buff[i], &sg[i], i*8 + 1400 );
            req[i]->sg = &sg[i];
            req[i]->sg_table.sgl = &sg[i];
            req[i]->sg_table.nents = 1;
            req[i]->sg_table.orig_nents = 1;

            spin_lock_irqsave(&crdev->agent[0].agent_lock, flags);
            list_add(&req[i]->list, &crdev->agent[0].processing_queue);
            spin_unlock_irqrestore(&crdev->agent[0].agent_lock, flags);
            req[i]->crypto_complete = crypto_complete;
            req[i]->res = i;
        }
    }
    /*

    */
    pr_info("Test module print\n");

    print_xmit_list();
    print_deliver_list();
    print_processing_list();
    pr_info("Test module print end\n");

    return 0;
}

static void __exit test_exit(void)
{
	
}

module_init(test_init);
module_exit(test_exit);