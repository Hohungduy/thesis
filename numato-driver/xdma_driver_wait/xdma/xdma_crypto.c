#include "xdma_crypto.h"

struct xdma_crdev crdev;
struct transport_engine transdev;

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

irqreturn_t err_handler(int irq_no, void *dev)
{
    pr_info("err_handler");
    return IRQ_HANDLED;
}

// int xfer_rcv_threads_create(unsigned int num_threads)
// {
// 	struct xdma_kthread *thp;
// 	int i;
// 	int rv;

// 	if (thread_cnt) {
// 		pr_warn("threads already created!");
// 		return 0;
// 	}

// 	pr_info("xdma_threads_create\n");

// 	thread_cnt = num_threads;

// 	cs_threads = kzalloc(thread_cnt * sizeof(struct xdma_kthread),
// 					GFP_KERNEL);
// 	if (!cs_threads)
// 		return -ENOMEM;

// 	/* N dma writeback monitoring threads */
// 	thp = cs_threads;
// 	for (i = 0; i < thread_cnt; i++, thp++) {
// 		thp->cpu = i;
// 		thp->timeout = 0;
// 		thp->fproc = xdma_thread_cmpl_status_proc;
// 		thp->fpending = xdma_thread_cmpl_status_pend;
// 		rv = xdma_kthread_start(thp, "cmpl_status_th", i);
// 		if (rv < 0)
// 			goto cleanup_threads;
// 	}

// 	return 0;

// cleanup_threads:
// 	kfree(cs_threads);
// 	cs_threads = NULL;
// 	thread_cnt = 0;

// 	return rv;
// }

irqreturn_t xfer_rcv(int irq_no, void *dev)
{
    pr_info("xfer_rcv");



    return IRQ_HANDLED;
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

    // transport engine == xdma engine
    spin_lock_init(&transdev.lock);
    crdev.transport = &transdev;


    //init user_irq
    xdma_user_isr_register(xdev, 0x01, xfer_rcv, &crdev);
    xdma_user_isr_register(xdev, 0x02, err_handler, &crdev);
    xdma_user_isr_enable(xdev, 0x03);


    // 
    mod_timer(&crdev.blinky.timer, jiffies + (unsigned int)(crdev.blinky.interval*HZ));

    return 0;
}

int choose_channel(void)
{
    spin_lock_bh(&transdev.lock);
    if (transdev.channel_0 < transdev.channel_1)
    {
        transdev.channel_0++;
        spin_unlock_bh(&transdev.lock);
        return 0;
    }
    else
    {
        transdev.channel_1++;
        spin_unlock_bh(&transdev.lock);
        return 1;
    }
    
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
    int channel = choose_channel();
    bool write = 1;
    struct sg_table sg_table;
    bool dma_mapped = 0;
    int timeout_ms = 3;
    int res;
    int status;
    u64 ep_addr;
    int engine_idx = 0;
    struct xfer_req *next_req;
    struct region *region_base;

    /** read the status*/
    spin_lock_bh(&crdev.lock);
    status = is_engine_full(engine_idx);
    // add req to backlog
    list_add_tail(&xfer_req->list, &crdev.req_queue);
    if (status == 1) // full
    {
        // done, return 
        spin_unlock_bh(&crdev.lock);
        pr_info("engine buff is full\n");
        return -1;
    }
    else // can submit
    {
        while ((!is_engine_full(engine_idx)) && !list_empty(&crdev.req_queue))
        {
            // get ep_addr
            region_base = get_next_region_ep_addr(engine_idx);
            ep_addr = get_next_data_ep_addr(engine_idx);
            spin_unlock_bh(&crdev.lock);
            pr_info("send to ep_addr = %lld", ep_addr);
            // Write crypto info

            /** sg_table*/
            next_req = list_first_entry(&crdev.req_queue, struct xfer_req, list);
            sg_table.sgl = next_req->sg;
            sg_table.nents = 1;//sg_nents(next_req->sg);
            sg_table.orig_nents = 1;//sg_nents(next_req->sg);
            
            // submit req from req_queue to engine 
            res = xdma_xfer_submit(dev_hndl, channel, write, 
                (u64)ep_addr, &sg_table, dma_mapped, timeout_ms);
            spin_lock_bh(&crdev.lock);
            // Write region dsc

            // update engine buff idx & region dsc & datalen
            iowrite32(sg_dma_len(next_req->sg), &region_base->data_len);
            active_next_region(engine_idx);
            increase_head_idx(engine_idx);
            // move to req_processing
            list_del(&next_req->list);
            list_add_tail(&next_req->list, &crdev.req_processing);
            spin_lock_bh(&crdev.lock);
        }
        return 0;
    }
}

EXPORT_SYMBOL_GPL(xdma_xfer_submit_queue);

struct xfer_req *alloc_xfer_req(void)
{
    struct xfer_req *xfer;
    xfer = (struct xfer_req *)kzalloc(sizeof(*xfer), GFP_KERNEL);
    if (!xfer)
        return 0;
    return xfer;
}
EXPORT_SYMBOL_GPL(alloc_xfer_req);
void free_xfer_req(struct xfer_req *req)
{
    kfree(req);
}
EXPORT_SYMBOL_GPL(free_xfer_req);

void print_req_queue(void)
{
    struct list_head *p;
    int i = 0;
    list_for_each(p, &crdev.req_queue){
        struct xfer_req *req = list_entry(p, struct xfer_req, list);
        pr_info("req_queue i = %d pid = %d \n", i, req->id);
    }
}
EXPORT_SYMBOL_GPL(print_req_queue);

void print_req_processing(void)
{
    struct list_head *p;
    int i = 0;
    list_for_each(p, &crdev.req_processing){
        struct xfer_req *req = list_entry(p, struct xfer_req, list);
        pr_info("req_processing i = %d pid = %d \n", i, req->id);
    }
}
EXPORT_SYMBOL_GPL(print_req_processing);