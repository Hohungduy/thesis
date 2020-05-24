#include "xdma_crypto.h"

struct xdma_crdev crdev;
struct transport_engine transdev;

void send_event(struct event *ev)
{
    unsigned long flags;
    spin_lock_irqsave(&crdev.rcv_data.events_lock, flags);
    list_add(&ev->lh, &crdev.rcv_data.events_list);
    spin_unlock_irqrestore(&crdev.rcv_data.events_lock, flags);
    wake_up(&crdev.rcv_data.wq_event);
}

struct event* get_next_event(void)
{
    struct event *e = NULL;
    unsigned long flags;
    spin_lock_irqsave(&crdev.rcv_data.events_lock, flags);
    if (!list_empty(&crdev.rcv_data.events_list))
    {
        e = list_first_entry(&crdev.rcv_data.events_list, struct event, lh);
        if (e)
            list_del(&e->lh);
    }
    spin_unlock_irqrestore(&crdev.rcv_data.events_lock, flags);
    return e;
}

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

int xfer_deliver_thread(void *data)
{
    struct event *e;
    printk("xfer_deliver_thread on\n");
    while (true) {
        wait_event(crdev.rcv_data.wq_event, (e = get_next_event()) );
        if (e){

            /* Event processing */



            
            if (e->print){
                printk("deliver agent on\n");
                pr_info("trigger_work receive event\n");
                pr_info("print = %d", e->print);
                pr_info("stop = %d", e->stop);
            }

            xdma_user_isr_enable(crdev.xdev, 0x01);
            if (e->stop)
                break;
        }
    }
    kfree(e);
    do_exit(0);
}

int xfer_rcv_thread(void *data)
{
    return 0;
}

void trigger_work(void)
{
    struct event *ev;
    pr_info("trigger_work\n");
    ev = kmalloc(sizeof(struct event), GFP_ATOMIC | GFP_KERNEL);
    INIT_LIST_HEAD(&ev->lh);
    ev->print = 0;
    ev->stop = 0;
    INIT_LIST_HEAD(&ev->lh);
    pr_info("trigger_work send_event\n");
    send_event(ev);
}

irqreturn_t xfer_rcv(int irq_no, void *dev)
{
    pr_info("xfer_rcv interrupt\n");
    xdma_user_isr_disable(crdev.xdev, 0x01);
    trigger_work();
    return IRQ_HANDLED;
}

int crdev_create(struct xdma_pci_dev *xpdev)
{
    struct xdma_dev *xdev;
    int i;

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

    xdma_user_isr_register(xdev, 0x01, xfer_rcv, &crdev);
    xdma_user_isr_register(xdev, 0x02, err_handler, &crdev);

    // event 
    init_waitqueue_head(&crdev.rcv_data.wq_event);
    INIT_LIST_HEAD(&crdev.rcv_data.events_list);
    spin_lock_init(&crdev.rcv_data.events_lock);

    // xfer_rcv_task
    for (i = 0; i < CORE_NUM; i++)
    {
        crdev.rcv_data.xfer_rcv_task[i] = kthread_create_on_node(xfer_rcv_thread,
             (void *)&crdev.rcv_data, cpu_to_node(i), "crdev_%d_agent", i);
        INIT_LIST_HEAD(&crdev.rcv_data.callback_queue[i]);
        spin_lock_init(&crdev.rcv_data.callback_queue_lock[i]);
        wake_up_process(crdev.rcv_data.xfer_rcv_task[i]);
    }
    crdev.rcv_data.xfer_deliver_task =  kthread_create
        (xfer_deliver_thread, (void *)&crdev.rcv_data, "crdev_deliver");
    wake_up_process(crdev.rcv_data.xfer_deliver_task);


    //init user_irq

    xdma_user_isr_enable(xdev, 0x03);


    // timer
    mod_timer(&crdev.blinky.timer, 
        jiffies + (unsigned int)(crdev.blinky.interval*HZ));

    return 0;
}

int update_load(int channel)
{
    spin_lock_bh(&transdev.lock);
    switch (channel)
    {   
    case 0:
        transdev.channel[0]--;
        break;
    case 1:
        transdev.channel[1]--;
        break;
    default:
        pr_info("Wrong channel!");
        break;
    }
    spin_unlock_bh(&transdev.lock);
    return 0;
}

int choose_channel(void)
{
    spin_lock_bh(&transdev.lock);
    if (transdev.channel[0] < transdev.channel[1])
    {
        transdev.channel[0]++;
        spin_unlock_bh(&transdev.lock);
        return 0;
    }
    else
    {
        transdev.channel[1]++;
        spin_unlock_bh(&transdev.lock);
        return 1;
    }
}

void crdev_cleanup(void)
{
    struct list_head *p, *n;
    int i;
    struct event *ev;
    
    pr_info("stop crdev deliver\n");
    ev = kmalloc(sizeof(struct event), GFP_ATOMIC | GFP_KERNEL);
    INIT_LIST_HEAD(&ev->lh);
    ev->print = 1;
    ev->stop = 1;
    INIT_LIST_HEAD(&ev->lh);
    send_event(ev);

    del_timer_sync(&crdev.blinky.timer);
    pr_info("destroy req_queue\n");
    list_for_each_safe(p, n, &crdev.req_queue){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    pr_info("destroy req_processing\n");
    list_for_each_safe(p, n, &crdev.req_processing){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    // pr_info("destroy xfer_rcv_task\n");
    // for (i = 0; i < CORE_NUM; i++)
    // {
    //     kthread_stop(crdev.rcv_data.xfer_rcv_task[i]);
    // }
    pr_info("destroy events list\n");
    list_for_each_safe(p, n, &crdev.rcv_data.events_list){
        struct event* e = list_entry(p, struct event, lh);
        list_del(p);
        kfree(e);
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
            update_load(channel);
            spin_lock_bh(&crdev.lock);
            // Write region dsc

            // update engine buff idx & region dsc & datalen
            iowrite32((u32)(current->pid), &region_base->xfer_id);
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