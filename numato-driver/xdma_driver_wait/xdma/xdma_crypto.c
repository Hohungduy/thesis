#include "xdma_crypto.h"

struct xdma_crdev crdev;

void blinky_timeout(struct timer_list *timer)
{
    struct blinky *blinky;
    struct xdma_crdev *crdev;

    blinky = container_of(timer, struct blinky, timer);
    crdev = container_of(blinky, struct xdma_crdev, blinky);

    switch (blinky->led)
    {
    case RED:
        toggle_red_led();
        break;
    case BLUE:
        toggle_blue_led();
        break;
    case RED_BLUE:
        toggle_red_led();
        toggle_blue_led();
        break;
    default:
        break;
    }
    dbg_desc("Timer blinky function\n");
    mod_timer(&blinky->timer, jiffies + (unsigned int)(blinky->interval*HZ));
}

int crdev_create(struct xdma_pci_dev *xpdev)
{
    struct xdma_dev *xdev;

    xpdev->crdev = &crdev;

    crdev.xpdev = xpdev;
    crdev.xdev = xpdev->xdev;
    xdev = crdev.xdev;

    // Timer
    crdev.blinky.interval = 1;
    timer_setup(&crdev.blinky.timer, blinky_timeout, 0);
    crdev.blinky.led = RED_BLUE;
    // Config region base
    set_engine_base(xdev->bar[0] + ENGINE_OFFSET(0), 0);
    set_led_base(xdev->bar[0] + LED_OFFSET);
    // lock
    spin_lock_init(&crdev.lock);

    // backlog
    INIT_LIST_HEAD(&crdev.req_queue);
    INIT_LIST_HEAD(&crdev.req_processing);


    // 

    return 0;
}

void crdev_cleanup(void)
{
    struct list_head *p, *n;

    del_timer_sync(&crdev.blinky.timer);
    
    list_for_each_safe(p, n, &crdev.req_queue){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    list_for_each_safe(p, n, &crdev.req_processing){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
}

// ssize_t xdma_xfer_submit(void *dev_hndl, int channel, bool write, u64 ep_addr,
			// struct sg_table *sgt, bool dma_mapped, int timeout_ms);

ssize_t xdma_xfer_submit_queue(struct xfer_req * xfer_req)
{
    void *dev_hndl = crdev.xdev;
    int channel = 0;
    bool write = TRUE;
    struct sg_table sg_table;
    bool dma_mapped = FALSE;
    int timeout_ms = 3;
    int res;
    int status;
    void * ep_addr;
    int engine_idx = 0;
    struct xfer_req *next_req;
    struct region *region_base;

    /** read the status*/
    status = is_engine_full(engine_idx);

    // add req to backlog
    list_add_tail(&crdev.req_queue, &xfer_req->list);
    if (status == 1) // full
    {
        // done, return 
        return -1;
    }
    else // can submit
    {
        while ((!is_engine_full(engine_idx)) && list_empty(&crdev.req_queue))
        {
            // get ep_addr
            region_base = get_next_region_ep_addr(engine_idx);
            ep_addr = get_next_data_ep_addr(engine_idx);
            // Write crypto info

            /** sg_table*/
            next_req = list_first_entry(&crdev.req_queue, struct xfer_req, list);
            sg_table.sgl = next_req->sg;
            sg_table.nents = sg_nents(next_req->sg);
            sg_table.orig_nents = sg_nents(next_req->sg);

            // submit req from req_queue to engine 
            res = xdma_xfer_submit(dev_hndl, channel, write, 
                (u64)ep_addr, &sg_table, dma_mapped, timeout_ms);
            // Write region dsc

            // update engine buff idx & region dsc & datalen
            iowrite32(sg_dma_len(next_req->sg), &region_base->data_len);
            active_next_region(engine_idx);
            increase_head_idx(engine_idx);
            // move to req_processing
            list_del(&next_req->list);
            list_add_tail(&next_req->list, &crdev.req_processing);

        }

        return 0;
    }
}

EXPORT_SYMBOL_GPL(xdma_xfer_submit_queue);

struct xfer_req *alloc_xfer_req(void)
{
    struct xfer_req *xfer;
    xfer = (struct xfer_req*)kmalloc(sizeof(*xfer), GFP_KERNEL);
    if (!xfer)
        return 0;
    spin_lock_init(&xfer->lock);
    return xfer;
}
EXPORT_SYMBOL_GPL(alloc_xfer_req);
void free_xfer_req(struct xfer_req *req)
{
    kfree(req);
}
EXPORT_SYMBOL_GPL(free_xfer_req);