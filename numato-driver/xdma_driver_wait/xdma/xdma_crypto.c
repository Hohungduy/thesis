#include "xdma_crypto.h"

struct xdma_crdev crdev;
struct transport_engine transdev;

struct xdma_crdev *get_crdev(void)
{
    return &crdev;
}
EXPORT_SYMBOL_GPL(get_crdev);
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
void send_event(struct event *ev)
{
    unsigned long flags;
    struct crypto_agent *agent = &crdev.agent;
    spin_lock_irqsave(&agent->events_list_lock, flags);
    list_add(&ev->lh, &agent->events_list);
    spin_unlock_irqrestore(&agent->events_list_lock, flags);
    wake_up(&agent->wq_event);
}

struct event* get_next_event(void)
{
    struct event *e = NULL;
    unsigned long flags;
    struct crypto_agent *agent = &crdev.agent;
    spin_lock_irqsave(&agent->events_list_lock, flags);
    if (!list_empty(&agent->events_list))
    {
        e = list_first_entry(&agent->events_list, struct event, lh);
        // if (e)
        //     list_del(&e->lh);
    }
    spin_unlock_irqrestore(&agent->events_list_lock, flags);
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
    // int cpu;
    u32 xfer_id;
    unsigned long flags;
    struct crypto_agent *agent = &crdev.agent;
    struct xdma_crdev *crdev = (struct xdma_crdev *)data;
    int engine_idx = 0;
    // struct list_head * processing_queue = 
        // &crdev->processing_queue;
    // struct list_head * backlog_queue = 
    //     &crdev->agent.backlog_queue;
    struct xfer_req *next_req;
    struct reqion *region;
    u64 ep_addr;
    int res;
    printk("xfer_deliver_thread on\n");

    while (true) {
        wait_event(agent->wq_event, (e = get_next_event()) );
        if (e && e->deliver_thread){
            printk("xfer_deliver_thread active\n");

            // while(!is_engine_empty_out(engine_idx)){
                spin_lock_irqsave(&agent->events_list_lock, flags);
                // list_del(&e->lh);
                spin_unlock_irqrestore(&agent->events_list_lock, flags);
                // kfree(e);
            //     /* Event processing */

            //     // Get xfer_id of next req
            //     // check in the req_processing
            //     spin_lock_irqsave(&crdev->processing_queue_lock, flags);
                
            //     if (list_empty(&crdev->processing_queue)){
            //         pr_info("interrupt but list empty!\n\n");
            //         spin_unlock_irqrestore(&crdev->processing_queue_lock, flags);
            //         break;
            //     }
            //     next_req = list_first_entry(&crdev->processing_queue,
            //         struct xfer_req, list);
            //     xfer_id = next_req->id;
            //     pr_info("xfer_id = %d\n", xfer_id);
            //     // remove req from processing queue 
            //     list_del(&next_req->list);
            //     spin_unlock_irqrestore(&crdev->processing_queue_lock, flags);
            //     // check in the engine
            //     if (is_engine_empty_out(engine_idx))
            //     {
            //         pr_info("interrupt but engine empty\n");
            //         break;
            //     }
            //     region = 
            //         (struct reqion *)get_region_ep_addr_out(engine_idx);    
            //     // build scatterlist
            //     ep_addr = get_data_ep_addr_out(engine_idx);
            //     // xfer data to host 
            //     res = xdma_xfer_submit(&crdev->xdev,
            //         choose_channel(), FALSE, ep_addr, 
            //         &next_req->sg_table, FALSE, 3);
            //     if (res < 0)
            //     {
            //         pr_info("can not read!!!\n");
            //         continue;
            //     }
            //     // add req to list of agents
            //     spin_lock_irqsave(&crdev->agent.rcv.data[0].callback_queue_lock, flags);
            //     list_add_tail(&next_req->list, &crdev->agent.rcv.data[0].callback_queue);
            //     spin_unlock_irqrestore(&crdev->agent.rcv.data[0].callback_queue_lock, flags);
                
            //     if (unlikely(e->print)){
            //         printk("deliver agent on\n");
            //         pr_info("trigger_work receive event\n");
            //         pr_info("print = %d", e->print);
            //         pr_info("stop = %d", e->stop);
            //     }
            // }
            // // xdma_user_isr_enable(crdev->xdev, 0x01);
            if (e->stop)
                break;
            
        }
    }
    kfree(e);
    do_exit(0);
}

int xmit_thread(void *data)
{
    // struct event *e;
    int engine_idx = 0, channel;
    // u32 xfer_id;
    unsigned long flags;
    struct xfer_req *next_req;
    struct crypto_agent * agent = &crdev.agent;
    struct xdma_crdev *crdev = (struct xdma_crdev *)data;
    struct list_head * processing_queue = 
        &crdev->processing_queue;
    struct list_head * backlog_queue = 
        &agent->xmit.backlog_queue;

    void *dev_hndl = crdev->xdev;
    // int channel = choose_channel();
    bool write = 1;
    // struct sg_table sg_table;
    bool dma_mapped = 0;
    int timeout_ms = 3;
    int res;
    // int status;
    u64 ep_addr;
    // int engine_idx = 0;
    // struct xfer_req *next_req;
    struct region *region_base;

    printk("xmit_thread on\n");
    while (true) {
        // printk("xmit_thread wait_event\n");
        wait_event(agent->wq_event, ((!list_empty(backlog_queue)) 
            || (agent->xmit.status == XMIT_STATUS_STOP)));
        printk("xmit_thread running\n");
        if (unlikely(agent->xmit.status == XMIT_STATUS_STOP))
            break;
        
        if (likely(is_engine_full(engine_idx)))
        {
            msleep(20);
            continue;
        }
            

        while ((!is_engine_full(engine_idx)) && 
                        (!list_empty(backlog_queue)) )
        {
            printk("xmit_threadd processing\n");
            // remove first req from backlog
            // spin_lock_bh(&agent->xmit.backlog_queue_lock);
            // next_req = list_first_entry(backlog_queue, 
            //     struct xfer_req, list);
            // list_del(&next_req->list);
            // spin_unlock_bh(&agent->xmit.backlog_queue_lock);

            // // add to tail of processing queue
            // spin_lock_irqsave(&crdev->processing_queue_lock, flags);
            // list_add_tail(&next_req->list, processing_queue);
            // spin_unlock_irqrestore(&crdev->processing_queue_lock, flags);

            // /* send data to card  */ // lock ???????????

            // channel = choose_channel();
            // spin_lock_irqsave(&crdev->region_lock, flags);
            // region_base = get_next_region_ep_addr(engine_idx);
            // ep_addr = get_next_data_ep_addr(engine_idx);
            // spin_unlock_irqrestore(&crdev->region_lock, flags);

            // pr_info("send to ep_addr = %lld", ep_addr);
            // // Write crypto info
            
            // // submit req from req_queue to engine 
            // res = xdma_xfer_submit(dev_hndl, channel, write, 
            //     (u64)ep_addr, &next_req->sg_table, dma_mapped, timeout_ms);

            // // res < 0 ???????????????????????

            // spin_lock_irqsave(&crdev->region_lock, flags);
            // update_load(channel);
            // // Write region dsc

            // // update engine buff idx & region dsc & datalen
            // iowrite32((u32)(current->pid), &region_base->xfer_id);
            // next_req->id = current->pid;
            // iowrite32(sg_dma_len(next_req->sg), &region_base->data_len);
            // active_next_region(engine_idx);
            // increase_head_idx(engine_idx);

            // spin_unlock_irqrestore(&crdev->region_lock, flags);
        }
    }
    do_exit(0);
}

int xfer_rcv_thread(void *data)
{
    struct rcv_thread *thread_data = (struct rcv_thread *)data;
    // struct list_head *cbq = &thread_data->callback_queue;
    // spinlock_t *lock = &thread_data->callback_queue_lock;

    struct xfer_req *req;
    struct list_head *p,*n;
    struct event *e;
    struct crypto_agent *agent= &crdev.agent;
    unsigned long flags;

    printk("xfer_rcv_thread on\n");

    while (true) {
        wait_event(agent->wq_event, (e = get_next_event()) );
        if (e && e->rcv_thread[thread_data->cpu]){
            printk("xfer_rcv_thread active cpu = %d\n", thread_data->cpu);
            // spin_lock_irqsave(&agent->events_list_lock, flags);
            // list_del(&e->lh);
            // spin_unlock_irqrestore(&agent->events_list_lock, flags);
            // /* Event processing */

            // // Get xfer_id of next req
            // if (list_empty(&thread_data->callback_queue))
            //     continue;
            // list_for_each_safe(p, n, &thread_data->callback_queue)
            // {
            //     req = list_entry(p, struct xfer_req, list);
            //     list_del(p);
            //     local_bh_disable();
            //     req->crypto_complete(req, req->res);
            //     local_bh_enable();
            //     kfree(req);
            // }
            // if (unlikely(e->print)){
            //     printk("deliver agent on\n");
            //     pr_info("trigger_work receive event\n");
            //     pr_info("print = %d", e->print);
            //     pr_info("stop = %d", e->stop);
            // }
            // if (e->stop)
            //     break;
            // kfree(e);
        }
    }
    kfree(e);
    do_exit(0);

    return 0;
}

void trigger_work(void)
{
    struct event *ev;
    int i;
    pr_info("trigger_work\n");
    ev = kmalloc(sizeof(struct event), GFP_ATOMIC | GFP_KERNEL);
    INIT_LIST_HEAD(&ev->lh);
    ev->print = 0;
    ev->stop = 0;
    ev->deliver_thread = 1;
    for (i = 0; i < CORE_NUM; i++){
        ev->rcv_thread[i] = 0;
    }
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
    struct crypto_agent *agent; 
    xpdev->crdev = &crdev;

    crdev.xpdev = xpdev;
    crdev.xdev = xpdev->xdev;
    xdev = crdev.xdev;
    agent = &xpdev->crdev->agent;

    // Timer
    crdev.blinky.interval = 1;
    pr_info("line : %d\n", __LINE__);
    timer_setup(&crdev.blinky.timer, blinky_timeout, 0);
    crdev.blinky.led = RED_BLUE;
    // Config region base
    set_engine_base(xdev->bar[0] + ENGINE_OFFSET(0), 0);
    set_led_base(xdev->bar[0] + LED_OFFSET);
    // lock
    spin_lock_init(&crdev.processing_queue_lock);
    pr_info("line : %d\n", __LINE__);
    // processing queue
    INIT_LIST_HEAD(&crdev.processing_queue);
    pr_info("line : %d\n", __LINE__);
    // transport engine == xdma engine
    spin_lock_init(&transdev.lock);
    crdev.transport = &transdev;
    pr_info("line : %d\n", __LINE__);
    xdma_user_isr_register(xdev, 0x01, xfer_rcv, &crdev);
    xdma_user_isr_register(xdev, 0x02, err_handler, &crdev);

    // region
    pr_info("line : %d\n", __LINE__);
    spin_lock_init(&crdev.region_lock);

    /*
        agent
    */
    
    // xmit kthread
    pr_info("line : %d\n", __LINE__);
    agent->xmit.xmit_task =  kthread_create_on_node
        (xmit_thread, (void *)&crdev,  cpu_to_node(0),"crdev_xmit");
    pr_info("line : %d\n", __LINE__);
    INIT_LIST_HEAD(&agent->xmit.backlog_queue);
    pr_info("line : %d\n", __LINE__);
    spin_lock_init(&agent->xmit.backlog_queue_lock);
    agent->xmit.status = XMIT_STATUS_ACTIVE;
    pr_info("line : %d\n", __LINE__);
    wake_up_process(agent->xmit.xmit_task);

    // event
    pr_info("line : %d\n", __LINE__);
    init_waitqueue_head(&agent->wq_event);
    pr_info("line : %d\n", __LINE__);
    INIT_LIST_HEAD(&agent->events_list);
    pr_info("line : %d\n", __LINE__);
    spin_lock_init(&agent->events_list_lock);

    // xfer_rcv_task
    pr_info("line : %d\n", __LINE__);
    for (i = 0; i < CORE_NUM; i++)
    {
        agent->rcv.data[i].xfer_rcv_task = kthread_create_on_node
            (xfer_rcv_thread, (void *)&agent->rcv.data[i], 
                cpu_to_node(i), "crdev_%d_agent", i);
        INIT_LIST_HEAD(&agent->rcv.data[i].callback_queue);
        spin_lock_init(&agent->rcv.data[i].callback_queue_lock);
        wake_up_process(agent->rcv.data[i].xfer_rcv_task);
        agent->rcv.data[i].cpu = i;
    }
    pr_info("line : %d\n", __LINE__);
    agent->rcv.xfer_deliver_task =  kthread_create_on_node
        (xfer_deliver_thread, (void *)&crdev, cpu_to_node(1) ,"crdev_deliver");
    pr_info("line : %d\n", __LINE__);
    wake_up_process(agent->rcv.xfer_deliver_task);

    // xfer_idx
    atomic_set(&crdev.xfer_idex, 0);
    //init user_irq

    xdma_user_isr_enable(xdev, 0x03);
    pr_info("line : %d\n", __LINE__);

    // timer
    mod_timer(&crdev.blinky.timer, 
        jiffies + (unsigned int)(crdev.blinky.interval*HZ));

    return 0;
}
void crdev_cleanup(void)
{
    struct list_head *p, *n;
    // int i;
    struct event *ev;
    
    pr_info("stop crdev deliver\n");
    ev = kmalloc(sizeof(struct event), GFP_ATOMIC | GFP_KERNEL);
    INIT_LIST_HEAD(&ev->lh);
    ev->print = 1;
    ev->stop = 1;
    INIT_LIST_HEAD(&ev->lh);
    send_event(ev);

    pr_info("stop xmit deliver\n");
    crdev.agent.xmit.status = XMIT_STATUS_STOP;


    del_timer_sync(&crdev.blinky.timer);
    pr_info("destroy req_queue\n");
    list_for_each_safe(p, n, &crdev.agent.xmit.backlog_queue){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    pr_info("destroy req_processing\n");
    list_for_each_safe(p, n, &crdev.processing_queue){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    // pr_info("destroy xfer_rcv_task\n");
    // for (i = 0; i < CORE_NUM; i++)
    // {
    //     kthread_stop(agent->rcv.xfer_rcv_task[i]);
    // }
    pr_info("destroy events list\n");
    list_for_each_safe(p, n, &crdev.agent.events_list){
        struct event* e = list_entry(p, struct event, lh);
        list_del(p);
        kfree(e);
    }
}

int xdma_xfer_submit_queue(struct xfer_req * xfer_req)
{
    spinlock_t *backlog_queue_lock = 
        &crdev.agent.xmit.backlog_queue_lock;
    
    xfer_req->sg_table.sgl = xfer_req->sg;
    xfer_req->sg_table.nents = 1;//sg_nents(xfer_req->sg);
    xfer_req->sg_table.orig_nents = 1;//sg_nents(xfer_req->sg);
    
    spin_lock_bh(backlog_queue_lock);
    list_add_tail(&xfer_req->list, &crdev.agent.xmit.backlog_queue);
    spin_unlock_bh(backlog_queue_lock);
    wake_up(&crdev.agent.wq_event);
    return -EINPROGRESS;
}

EXPORT_SYMBOL_GPL(xdma_xfer_submit_queue);

struct xfer_req *alloc_xfer_req(void)
{
    struct xfer_req *xfer;
    xfer = (struct xfer_req *)kzalloc(sizeof(*xfer), GFP_KERNEL);
    xfer->id = atomic_inc_return(&crdev.xfer_idex);
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
    list_for_each(p, &crdev.agent.xmit.backlog_queue){
        struct xfer_req *req = list_entry(p, struct xfer_req, list);
        pr_info("backlog_queue i = %d pid = %d \n", i, req->id);
    }
}
EXPORT_SYMBOL_GPL(print_req_queue);

void print_req_processing(void)
{
    struct list_head *p;
    int i = 0;
    list_for_each(p, &crdev.processing_queue){
        struct xfer_req *req = list_entry(p, struct xfer_req, list);
        pr_info("processing_queue i = %d pid = %d \n", i, req->id);
    }
}
EXPORT_SYMBOL_GPL(print_req_processing);