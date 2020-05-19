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
    INIT_LIST_HEAD(&crdev.req_backlog);

    // 

    return 0;
}

void crdev_cleanup(void)
{
    struct list_head *p, *n;

    del_timer_sync(&crdev.blinky.timer);
    
    list_for_each_safe(p, n, &crdev.req_backlog){
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
    struct scatterlist *sg = xfer_req->sg;
    struct sg_table sg_table;
    bool dma_mapped = FALSE;
    int timeout_ms = 3;
    int res;
    int status;
    u64 ep_addr;

    /** sg_table*/
    sg_table.sgl = sg;
    sg_table.nents = sg_nents(sg);
    sg_table.orig_nents = sg_nents(sg);

    /** read the status*/
    status = is_engine_full(channel);
    if (status < 0)
        return -1;
    if (status == 1){
        
        // Add req to buffer
        
        
        goto submit_to_card;
    } else {
        // add req to backlog and return
    }

submit_to_card:

    /** Read to fill endpoint addr */


    /** fill header and crypto descryption */


    res = xdma_xfer_submit(dev_hndl, channel, write, 
        ep_addr, &sg_table, dma_mapped, timeout_ms);
    return res;
}