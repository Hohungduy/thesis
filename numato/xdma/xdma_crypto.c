#include "xdma_crypto.h"

struct xdma_pci_dev *g_xpdev;

#define BAR_0_ADDR (g_xpdev->xdev->bar[0])

void print_req(struct xfer_req *req)
{
    pr_err("print_req\n");
    pr_err("out_region %p\n", req->out_region);
    pr_err("in_region %p\n", req->in_region);
    pr_err("id = %d\n", req->id);
    pr_err("region_idx = %d\n", req->region_idx);
    pr_err("data_ep_addr = %lld\n", req->data_ep_addr);
    pr_err("orig_nents = %d\n", req->sg_table.orig_nents);
    pr_err("nents = %d\n", req->sg_table.nents);

    pr_err("key = %x %x %x %x %x %x %x %x\n", 
                 req->crypto_dsc.key[0], req->crypto_dsc.key[1],
                 req->crypto_dsc.key[2], req->crypto_dsc.key[3],
                 req->crypto_dsc.key[4], req->crypto_dsc.key[5],
                 req->crypto_dsc.key[6], req->crypto_dsc.key[7]);
    pr_err("nonce = %x\n", req->crypto_dsc.iv.nonce);
    pr_err("iv = %x %x\n", req->crypto_dsc.iv.iv[0], req->crypto_dsc.iv.iv[1]);
    pr_err("tail = %x\n", req->crypto_dsc.iv.tail);

    pr_err("length = %d\n", req->crypto_dsc.info.length);
    pr_err("direction = %d\n", req->crypto_dsc.info.direction);
    pr_err("keysize = %d\n", req->crypto_dsc.info.keysize);
    pr_err("addsize = %d\n", req->crypto_dsc.info.aadsize);

    pr_err("icv = %x %x %x %x\n", req->crypto_dsc.icv[0], req->crypto_dsc.icv[1],
                             req->crypto_dsc.icv[2], req->crypto_dsc.icv[3]);
    pr_err("add = %x %x %x %x\n\n\n", req->crypto_dsc.aad[0], req->crypto_dsc.aad[1],
                             req->crypto_dsc.aad[2], req->crypto_dsc.aad[3]);
}

struct xdma_crdev *get_crdev(void)
{
    return g_xpdev->crdev;
}
EXPORT_SYMBOL_GPL(get_crdev);

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
    pr_err("err_handler");
    return IRQ_HANDLED;
}

int min_channel_load(int *channel)
{
    int i, min = 0;
    for (i = 0; i < CHANNEL_NUM; i++)
    {
        if (channel[i] < channel[min])
            min = i;
    }
    channel[min]++;
    return min;
}

int write_dsc(struct xfer_req *req)
{
    return 0;
}

int xmit_deliver_task(void *data)
{
    struct crypto_agent *agent = (struct crypto_agent *)data; 
    // struct list_head *processing = &agent->processing_queue;
    struct xmit_handler *xmit = &agent->xmit;
    struct list_head *p, *n;
    struct xfer_req *req;
    static int channel_load[CHANNEL_NUM];
    int min_channel;
    unsigned long flags;
    int engine_idx = 0;
    pr_err("deliver_task wakeup\n");

    while (true) {
        wait_event_interruptible(xmit->wq_xmit_event, 
            (       (!list_empty(&xmit->deliver_list)) 
                ||  (agent->xmit.status == XMIT_STATUS_STOP)
            ) 
        );
        if (unlikely(agent->xmit.status == XMIT_STATUS_STOP))
            break;
        // print_xmit_deliver_list();
        
        list_for_each_safe(p, n, &xmit->deliver_list){

#ifdef BUFFER
            if (is_engine_full(engine_idx) || 
                (get_tail_inb_idx(engine_idx) == (xmit->booking + 1) % REGION_NUM))
            {
                if (is_engine_full(engine_idx))
                    // pr_err("%s: enginge full\n", __func__);
                if (get_tail_inb_idx(engine_idx) == (xmit->booking + 1) % REGION_NUM)
                    // pr_err("%s: booked all\n", __func__);

                // TODO: 

                continue;
            }
            else
            {
#endif
                min_channel = min_channel_load(channel_load);

                req = list_entry(p, struct xfer_req, list); 
                spin_lock_irqsave(&xmit->deliver_list_lock, flags);
                list_del(&req->list);
                spin_unlock_irqrestore(&xmit->deliver_list_lock, flags);

#ifdef BUFFER
                req->in_region = get_region_ep_addr(engine_idx, xmit->booking);
                req->region_idx = xmit->booking;
                req->data_ep_addr = get_region_data_ep_addr(engine_idx, xmit->booking);

                xmit->booking = (xmit->booking + 1) % REGION_NUM;
#else  
                req->in_region = (struct region_in *)get_region_addr(engine_idx);
                req->data_ep_addr = 0x70;//get_region_data_ep_addr(engine_idx, xmit->booking);
                pr_err("deliver task print before add to xmit_queue\n");
#endif
#ifdef BUFFER
                pr_err("deliver task data_ep_addr %llx\n", req->data_ep_addr);
#else  

#endif
                spin_lock_irqsave(&xmit->xmit_queue_lock[min_channel], flags);
                list_add(&req->list, &xmit->xmit_queue[min_channel]);
                spin_unlock_irqrestore(&xmit->xmit_queue_lock[min_channel], flags);
                wake_up(&xmit->wq_xmit_event);
#ifdef BUFFER
            }
#endif
        }
    }
    do_exit(0);
}

int xmit_task(void *data)
{
    struct task_data *task_data = (struct task_data *)data; 
    struct xmit_handler *xmit = task_data->xmit;
    int channel_idx = task_data->idx;
    struct crypto_agent *agent = 
        container_of(xmit, struct crypto_agent, xmit);
    unsigned long flags;

    struct list_head *xmit_queue = &xmit->xmit_queue[channel_idx];
    struct list_head *processing = &agent->processing_queue;
    spinlock_t *lock = &xmit->xmit_queue_lock[channel_idx];

    // bool write = TRUE;
    bool dma_mapped = 0;
    int engine_idx = 0;
    int timeout_ms = 3;
    int res;

    struct xfer_req *req;
    u64 ep_addr;
    pr_err("xmit_task wakeup %d\n", channel_idx);
    while (true) {
        pr_err("xmit_task %d wait_event \n", channel_idx);
        wait_event_interruptible(xmit->wq_xmit_event, 
            (       (!list_empty(xmit_queue)) 
                ||  (agent->xmit.status == XMIT_STATUS_STOP)
            ) 
        );
        pr_err("xmit_task running %d\n", channel_idx);

        if (unlikely(agent->xmit.status == XMIT_STATUS_STOP))
            break;
            

        while (!list_empty(xmit_queue))
        {
            pr_err("xmit_thread processing %d\n", channel_idx);
            // TODO: Condition to xmit ???? lock ???


            // remove first req from backlog
            spin_lock_irqsave(lock, flags);
            req = list_first_entry(xmit_queue, 
                struct xfer_req, list);
            list_del(&req->list);
            spin_unlock_irqrestore(lock, flags);

            ep_addr = req->data_ep_addr;

            // TODO: Write crypto info (No need to lock)
            memcpy_toio(&req->in_region->crypto_dsc, 
                    &req->crypto_dsc, sizeof(struct crypto_dsc_in));

            // submit req from req_queue to engine 
            pr_err("submit xmit to channel %d region %d", channel_idx, req->region_idx);
            res = xdma_xfer_submit(g_xpdev->crdev->xdev, channel_idx, 1, 
                ep_addr, &req->sg_table, dma_mapped, timeout_ms);

            if (res < 0)
            {
                pr_err("Send failed req_id = %d\n, res = %d", req->id, res);
                // TODO: 
                continue;
            }
            // Write region dsc + see booking, modify head, tail
            spin_lock_irqsave(&agent->agent_lock, flags);
            // Write xfer_id
            memset32((void *)req->in_region, 0xABCDACBD, 1);
            memset32((void *)req->in_region + 4, req->id, 1);
            memset32((void *)req->in_region + 12, 0, 1);
            memset32((void *)req->in_region + 8, req->crypto_dsc.info.length, 1);


#ifdef BUFFER
            active_inb_region(engine_idx, req->region_idx);
            // tail_idx = get_tail_inb_idx(engine_idx);
            increase_head_inb_idx(engine_idx , xmit->booking);
#else  

#endif
            // add to tail of processing queue
            list_add_tail(&req->list, processing);
            spin_unlock_irqrestore(&agent->agent_lock, flags);

            trigger_engine(engine_idx);
        }
    }
    do_exit(0);
}

int rcv_deliver_task(void *data)
{
    struct crypto_agent *agent = (struct crypto_agent *)data;
    int engine_idx = 0;
    struct xfer_req *req = NULL;
    struct list_head *p,*n;
    unsigned long flags;
    int channel_idx, min_load_ch_idx;
    u32 xfer_id;
    pr_err("xfer_rcv_thread on\n");
    while (true) {
        wait_event_interruptible(agent->rcv.wq_rcv_event, (agent->need_deliver == TRUE) );
        pr_err("rcv_deliver thread on\n");
        // TODO: Sanity checks
        if (list_empty(&agent->processing_queue))
        {
            // pr_err("Interrupt but processing queue empty\n");
            continue;
        }

        // TODO: Read booking, head, tail
        spin_lock_irqsave(&agent->agent_lock, flags);
#ifdef BUFFER

        for (i = agent->rcv.booking; 
            i != (get_head_outb_idx(engine_idx) - 1) % REGION_NUM; 
            i = (i + 1) % REGION_NUM)
        {
#else  

#endif
        // TODO: Find in processing list the xfer_req struct based on xfer_id
#ifdef BUFFER 
            
#else
            xfer_id = get_xfer_id_outb_idx(engine_idx, 0);
            spin_unlock_irqrestore(&agent->agent_lock, flags);
#endif 
            pr_err("Searching for xfer\n");
            list_for_each_safe(p, n, &agent->processing_queue)
            {
                // TODO: Remove xfer_req from xfer_processing
                req = list_entry(p, struct xfer_req, list);
                if (req->id == xfer_id)
                {
                    pr_err("Found matching id\n");
                    list_del(&req->list);
                    break;
                }
            }
            pr_err("Found! \n");
            // TODO: Choose channel and add to rcv_list of that channel
            spin_lock_bh(&g_xpdev->crdev->channel_lock);
            min_load_ch_idx = 0;
            for (channel_idx = 0; channel_idx < CHANNEL_NUM; channel_idx++)
            {
                if (g_xpdev->crdev->channel_load[min_load_ch_idx]
                    > g_xpdev->crdev->channel_load[channel_idx])
                    min_load_ch_idx = channel_idx;
            }

            g_xpdev->crdev->channel_load[min_load_ch_idx]++;
            spin_unlock_bh(&g_xpdev->crdev->channel_lock);

            spin_lock_bh(&agent->rcv.rcv_queue_lock[min_load_ch_idx]);

            list_add_tail(&req->list, &agent->rcv.rcv_queue[min_load_ch_idx]);

            spin_unlock_bh(&agent->rcv.rcv_queue_lock[min_load_ch_idx]);

#ifdef BUFFER
            agent->rcv.booking++;
#else  

#endif
            wake_up_interruptible(&agent->rcv.wq_rcv_event);
#ifdef BUFFER
        }
#else  

#endif
        spin_lock_irqsave(&agent->agent_lock, flags);
        agent->need_deliver = FALSE;
        spin_unlock_irqrestore(&agent->agent_lock, flags);

    }
    do_exit(0);
}

int rcv_task(void *data)
{
    struct task_data *task_data = (struct task_data *)data;
    struct rcv_handler *rcv = task_data->rcv;
    int channel_idx = task_data->idx;
    int res;
    unsigned long flags;
    u64 outbound_data_addr;
    struct xfer_req *req;

    pr_err("rcv_task chanel = %d on\n", channel_idx);

    while (true) {
        wait_event_interruptible(rcv->wq_rcv_event, 
        (  (!list_empty(&rcv->rcv_queue[channel_idx])) ||
           (rcv->status == RCV_STATUS_STOP)
        )
        );
        if (likely(rcv->status == RCV_STATUS_STOP))
            break;
        pr_err("rcv_task active %d\n", channel_idx);
#ifdef BUFFER

#else  
        outbound_data_addr = 0x10020;
#endif
        
        // Get the req to rcv
        spin_lock_irqsave(&rcv->rcv_queue_lock[channel_idx], flags);
        req = list_first_entry(&rcv->rcv_queue[channel_idx], struct xfer_req, list);
        list_del(&req->list);
        spin_unlock_irqrestore(&rcv->rcv_queue_lock[channel_idx], flags);
#ifdef BUFFER

#else
        memcpy_fromio(req->tag, BAR_0_ADDR + 0x10000 + req->tag_offset, req->tag_length);
#endif
        res = xdma_xfer_submit(g_xpdev->crdev->xdev,
            channel_idx, FALSE, outbound_data_addr, 
            &req->sg_table, FALSE, 3);
        if (res < 0)
        {
            pr_err("can not read!!!\n");
            continue;
        }

#ifdef BUFFER
        active_outb_region(engine_idx, req->region_idx);
#else 
        // TODO: Free region + lock ???
#endif

#ifdef BUFFER
        // TODO: Read Crypto Dsc

        // TODO: Prepare for callback 
        
        // TODO: Check booking idex and tail idex
        spin_lock_irqsave(&g_xpdev->crdev->agent[agent_idx].agent_lock, flags);
        increase_tail_outb_idx(engine_idx, g_xpdev->crdev->agent[0].rcv.booking);
        spin_unlock_irqrestore(&g_xpdev->crdev->agent[agent_idx].agent_lock, flags);
#else  

#endif
        // Call callback
        spin_lock_irqsave(&rcv->rcv_callback_queue_lock[channel_idx], flags);
        list_add_tail(&req->list, &rcv->rcv_callback_queue[channel_idx]);
        spin_unlock_irqrestore(&rcv->rcv_callback_queue_lock[channel_idx], flags);
        wake_up_interruptible(&rcv->wq_rcv_event);
    }
    do_exit(0);
}

int rcv_callback_task(void *data)
{
    struct task_data *task_data = (struct task_data *)data;
    struct rcv_handler *rcv = task_data->rcv;
    int channel_idx = task_data->idx;
    unsigned long flags;
    struct xfer_req *req;
    int res;
    pr_err("rcv_callback_task %d on\n", channel_idx);
    while (true) {
        wait_event_interruptible(rcv->wq_rcv_event, 
            (   (!list_empty(&rcv->rcv_callback_queue[channel_idx])) ||
                (rcv->status == RCV_STATUS_STOP)    
            ));
        pr_err("rcv_callback_task %d running\n", channel_idx);

        if (rcv->status == RCV_STATUS_STOP)
            break;
        while(!list_empty(&rcv->rcv_callback_queue[channel_idx]))
        {
            spin_lock_irqsave(&rcv->rcv_callback_queue_lock[channel_idx], flags);
            req = list_first_entry(&rcv->rcv_callback_queue[channel_idx], 
                struct xfer_req, list);
            list_del(&req->list);
            spin_unlock_irqrestore(&rcv->rcv_callback_queue_lock[channel_idx], flags);
            local_bh_disable();
            if (req->crypto_complete)
                    res = req->crypto_complete(req, req->id);

            local_bh_enable();
        }
    }
    do_exit(0);
}

void trigger_rcv_deliver_task(void)
{
    struct crypto_agent *agent = &g_xpdev->crdev->agent[0];
    unsigned long flags;
    spin_lock_irqsave(&agent->agent_lock, flags);
    agent->need_deliver = TRUE;
    spin_unlock_irqrestore(&agent->agent_lock, flags);
    wake_up_interruptible(&agent->rcv.wq_rcv_event);
}

irqreturn_t xfer_rcv(int irq_no, void *dev)
{
    pr_err("xfer_rcv interrupt\n");
    clear_usr_irq(0);
    trigger_rcv_deliver_task();
    return IRQ_HANDLED;
}
void create_rcv_handler(struct rcv_handler *rcv)
{
    struct xdma_crdev* crdev = g_xpdev->crdev;
    int agent_idx = 0;
    int channel_idx;

    init_waitqueue_head(&rcv->wq_rcv_event);
    INIT_LIST_HEAD(&rcv->rcv_events_list);
    spin_lock_init(&rcv->rcv_events_list_lock);

    rcv->rcv_deliver_task = kthread_create_on_node
        (rcv_deliver_task, (void *)&crdev->agent[agent_idx],
        cpu_to_node(agent_idx % CORE_NUM), "crdev_rcv_deliver");
    for (channel_idx = 0; channel_idx < CORE_NUM; channel_idx++)
    {
        rcv->rcv_task[channel_idx] = kthread_create_on_node
            (rcv_task, (void *)&rcv->task_data[channel_idx], 
                cpu_to_node(channel_idx % CORE_NUM), "crdev_rcv_%d", channel_idx);
        spin_lock_init(&rcv->rcv_queue_lock[channel_idx]);
        INIT_LIST_HEAD(&rcv->rcv_queue[channel_idx]);
        rcv->task_data[channel_idx].idx = channel_idx;
        rcv->task_data[channel_idx].rcv = rcv;

        rcv->rcv_callback_task[channel_idx] = kthread_create_on_node
            (rcv_callback_task, (void *)&rcv->task_callback_data[channel_idx],
            cpu_to_node(channel_idx % CORE_NUM), "crdev_callback_%d", channel_idx);
            INIT_LIST_HEAD(&rcv->rcv_callback_queue[channel_idx]);
            rcv->task_callback_data[channel_idx].idx = channel_idx;
            rcv->task_callback_data[channel_idx].rcv = rcv;      
        wake_up_process(rcv->rcv_task[channel_idx]);
        wake_up_process(rcv->rcv_callback_task[channel_idx]);
    }
    wake_up_process(rcv->rcv_deliver_task);
    rcv->status = RCV_STATUS_ACTIVE;
    spin_lock_init(&rcv->region_lock);


}
void create_xmit_handler(struct xmit_handler *xmit)
{
    struct xdma_crdev* crdev = g_xpdev->crdev;
    int agent_idx = 0;
    int channel_idx;

    xmit->xmit_deliver_task =  kthread_create_on_node
        (xmit_deliver_task, (void *)&crdev->agent,  
        cpu_to_node(agent_idx % CORE_NUM),"crdev_xmit");
    INIT_LIST_HEAD(&xmit->deliver_list);
    spin_lock_init(&xmit->deliver_list_lock);
    
    xmit->status = XMIT_STATUS_ACTIVE;
    spin_lock_init(&xmit->region_lock);

    init_waitqueue_head(&xmit->wq_xmit_event);
    INIT_LIST_HEAD(&xmit->xmit_events_list);
    spin_lock_init(&xmit->xmit_events_list_lock);

    for (channel_idx = 0; channel_idx < CHANNEL_NUM; channel_idx ++)
    {
        xmit->xmit_task[channel_idx] = 
            kthread_create_on_node(xmit_task, &xmit->task_data[channel_idx], 
            cpu_to_node(channel_idx % CORE_NUM), "xmit_task_%d", 
            channel_idx % CORE_NUM);
        spin_lock_init(&xmit->xmit_queue_lock[channel_idx]);
        INIT_LIST_HEAD(&xmit->xmit_queue[channel_idx]);
        xmit->task_data[channel_idx].idx  = channel_idx;
        xmit->task_data[channel_idx].xmit = xmit;
        wake_up_process(xmit->xmit_task[channel_idx]);
    }
    wake_up_process(xmit->xmit_deliver_task);
}


int crdev_create(struct xdma_pci_dev *xpdev)
{
    int agent_idx;
    struct xdma_crdev* crdev;
    struct crypto_agent *agent;
    struct xmit_handler *xmit;
    struct rcv_handler *rcv;
    struct crypto_engine engine;


    crdev = (struct xdma_crdev *)kzalloc(sizeof(*crdev), GFP_KERNEL);
    if (!crdev)
    {
        pr_err("Nomem \n");
        goto free;
    }
    g_xpdev = xpdev;
    xpdev->crdev = crdev;
    crdev->xpdev = xpdev;
    crdev->xdev = xpdev->xdev;

    // Timer
    crdev->blinky.interval = 1;
    timer_setup(&crdev->blinky.timer, blinky_timeout, 0);
    crdev->blinky.led = RED;
    // Config region base
#ifdef BUFFER

#else
    engine.comm = BAR_0_ADDR + 0x11000;
    engine.in = BAR_0_ADDR;
    engine.out = BAR_0_ADDR + 0x10000;
    engine.status = BAR_0_ADDR + 0x20000;
    engine.irq = BAR_0_ADDR + 0x40000;
#endif
    set_engine_base(engine, 0);
    set_led_base(BAR_0_ADDR);
    
    // lock
    spin_lock_init(&crdev->channel_lock);
    // transport engine == xdma engine
    xdma_user_isr_register(xpdev->xdev, 0x01, xfer_rcv, crdev);
    xdma_user_isr_register(xpdev->xdev, 0x02, err_handler, crdev);

    /*
        agent
    */
    for (agent_idx = 0; agent_idx < AGENT_NUM; agent_idx ++)
    {   
        agent = &crdev->agent[agent_idx];
        xmit = &agent->xmit;
        rcv = &agent->rcv;

        // xmit kthread
        create_xmit_handler(xmit);

        // xfer_rcv_task
        create_rcv_handler(rcv);

        // Agent common
        INIT_LIST_HEAD(&agent->processing_queue);
        agent->agent_idx = agent_idx;
        agent->xfer_idex = 0;
        agent->need_deliver = FALSE;
        spin_lock_init(&agent->agent_lock);
    }
    //init user_irq
    xdma_user_isr_enable(xpdev->xdev, 0x03);
    // timer
    mod_timer(&crdev->blinky.timer, 
        jiffies + (unsigned int)(crdev->blinky.interval*HZ));
    return 0;
free:

    // TODO:
    return -1;
}


void delete_rcv_handler(struct rcv_handler *rcv, int agent_idx)
{
    struct list_head *p, *n;
    int channel_idx;
    rcv->status = RCV_STATUS_STOP;

    list_for_each_safe(p, n, &rcv->rcv_events_list)
    {
        struct event *event = list_entry(p, struct event, lh);
        list_del(p);
        kfree(event);
    }

    for (channel_idx = 0; channel_idx < CHANNEL_NUM; channel_idx++)
    {
        list_for_each_safe(p, n, &rcv->rcv_queue[channel_idx])
        {
            struct xfer_req *req = list_entry(p, struct xfer_req, list);
            list_del(p);
            kfree(req);
        }
        list_for_each_safe(p, n, &rcv->rcv_callback_queue[channel_idx])
        {
            struct xfer_req *req = list_entry(p, struct xfer_req, list);
            list_del(p);
            kfree(req);
        }
    }

}

void delete_xmit_deliver_list(struct xdma_crdev *crdev)
{
    struct list_head *p, *n;
    int agent_idx = 0;
    unsigned long flags;
    pr_err("remove deliver_list agent_idx = %d\n", agent_idx);
    spin_lock_irqsave(&crdev->agent[agent_idx].xmit.deliver_list_lock, flags);
    list_for_each_safe(p, n, &crdev->agent[agent_idx].xmit.deliver_list){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    spin_unlock_irqrestore(&crdev->agent[agent_idx].xmit.deliver_list_lock, flags);
}

void delete_xmit_handler(struct xmit_handler *xmit, int agent_idx)
{
    struct list_head *p, *n;
    int channel_idx;
    unsigned long flags;

    xmit->status = XMIT_STATUS_STOP;

    spin_lock_irqsave(&xmit->deliver_list_lock, flags);
    list_for_each_safe(p, n, &xmit->deliver_list){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    spin_unlock_irqrestore(&xmit->deliver_list_lock, flags);

    spin_lock_irqsave(&xmit->xmit_events_list_lock, flags);
    list_for_each_safe(p, n, &xmit->xmit_events_list){
        struct event* ev = list_entry(p, struct event, lh);
        list_del(p);
        kfree(ev);
    }
    spin_unlock_irqrestore(&xmit->xmit_events_list_lock, flags);
    for (channel_idx = 0; channel_idx < CHANNEL_NUM; channel_idx++)
    {
        pr_err("remove xmit agent_idx = %d channel_idx = %d\n", agent_idx, channel_idx);
        spin_lock_irqsave(&xmit->xmit_queue_lock[channel_idx], flags);
        list_for_each_safe(p, n, &xmit->xmit_queue[channel_idx]){
            struct xfer_req* req = list_entry(p, struct xfer_req, list);
            list_del(p);
            kfree(req);
        }
        spin_unlock_irqrestore(&xmit->xmit_queue_lock[channel_idx], flags);
    }
}

void crdev_cleanup(void)
{
    struct list_head *p, *n;
    struct xdma_crdev *crdev = g_xpdev->crdev;
    int agent_idx;
    unsigned long flags;
    // Stop timer
    del_timer_sync(&crdev->blinky.timer);
    
    pr_err("stop xmit side\n");
    for (agent_idx = 0; agent_idx < AGENT_NUM; agent_idx++)
    {
        pr_err("remove processing queue agent_idx = %d\n", agent_idx);
        spin_lock_irqsave(&crdev->agent[agent_idx].agent_lock, flags);
        list_for_each_safe(p, n, &crdev->agent[agent_idx].processing_queue){
            struct xfer_req* req = list_entry(p, struct xfer_req, list);
            list_del(p);
            kfree(req);
        }
        spin_unlock_irqrestore(&crdev->agent[agent_idx].agent_lock, flags);

        // xmit
        delete_xmit_handler(&crdev->agent[agent_idx].xmit, agent_idx);
        // rcv
        delete_rcv_handler(&crdev->agent[agent_idx].rcv, agent_idx);
    
        list_for_each_safe(p, n, &crdev->agent[agent_idx].processing_queue){
            struct xfer_req* req = list_entry(p, struct xfer_req, list);
            list_del(p);
            kfree(req);
        }

    }
}

int xdma_xfer_submit_queue(struct xfer_req * xfer_req)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    spinlock_t *deliver_lock = 
        &crdev->agent[0].xmit.deliver_list_lock;
    
    xfer_req->sg_table.sgl = xfer_req->sg_in;
    xfer_req->sg_table.nents = 1;//sg_nents(xfer_req->sg);
    xfer_req->sg_table.orig_nents = 1;//sg_nents(xfer_req->sg);
    pr_err("%s %d", __func__, __LINE__);

    spin_lock_bh(deliver_lock);
    list_add_tail(&xfer_req->list, &crdev->agent[0].xmit.deliver_list);
    spin_unlock_bh(deliver_lock);
    wake_up_interruptible(&crdev->agent[0].xmit.wq_xmit_event);
    return -EINPROGRESS;
}

EXPORT_SYMBOL_GPL(xdma_xfer_submit_queue);

struct xfer_req *alloc_xfer_req(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct xfer_req *xfer;
    unsigned long flags;
    xfer = (struct xfer_req *)kzalloc(sizeof(*xfer), GFP_KERNEL);
    if (!xfer)
        return 0;
    
    spin_lock_irqsave(&crdev->agent[0].agent_lock, flags);
    xfer->id = crdev->agent[0].xfer_idex;
    crdev->agent[0].xfer_idex++;
    spin_unlock_irqrestore(&crdev->agent[0].agent_lock, flags);

    return xfer;
}
EXPORT_SYMBOL_GPL(alloc_xfer_req);

int set_tag(struct xfer_req *req, int length, int offset, u32* buff)
{
    if (!req)
        return -1;
    req->tag_length = length;
    req->tag_offset = offset;
    req->tag = buff;
    return 0;
}
EXPORT_SYMBOL_GPL(set_tag);

int set_sg(struct xfer_req *req, struct scatterlist *sg)
{
    if (req == NULL)
        return -1;
    req->sg_in = sg;
    req->sg_out= sg;
    req->sg_table.sgl = sg;
    req->sg_table.orig_nents = 1;//sg_nents(sg);
    req->sg_table.nents = 1;
    pr_err("sg_nents = %d\n", sg_nents(sg));
    return 0;
}
EXPORT_SYMBOL_GPL(set_sg);

int set_callback(struct xfer_req *req, int (*cb)(struct xfer_req *req, int res))
{
    if (req == NULL)
        return -1;
    req->crypto_complete = cb;
    return 0;
}
EXPORT_SYMBOL_GPL(set_callback);


int set_ctx(struct xfer_req *req, struct mycrypto_context ctx)
{
    if (req == NULL)
        return -1;

    req->ctx = ctx;
    return 0;

}
EXPORT_SYMBOL_GPL(set_ctx);

int get_result(struct xfer_req *req, int *res)
{
    if (req == NULL)
    return -1;
    *res = req->res;
    return 0;
}
EXPORT_SYMBOL_GPL(get_result);


void free_xfer_req(struct xfer_req *req)
{
    kfree(req);
}
EXPORT_SYMBOL_GPL(free_xfer_req);

void print_xmit_list(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct list_head *p;
    int j;
    for (j = 0 ; j < CHANNEL_NUM; j++)
    {
        list_for_each(p, &crdev->agent[0].xmit.xmit_queue[j]){
            struct xfer_req *req = list_entry(p, struct xfer_req, list);
            pr_err("%s: agent = %d id = %d channel = %d\n",__func__,0, req->id, j);
        }
    }
    
}
EXPORT_SYMBOL_GPL(print_xmit_list);

void print_rcv_list(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct list_head *p;
    int j;
    for (j = 0 ; j < CHANNEL_NUM; j++)
    {
        list_for_each(p, &crdev->agent[0].rcv.rcv_queue[j]){
            struct xfer_req *req = list_entry(p, struct xfer_req, list);
            pr_err("%s: agent = %d id = %d channel = %d\n",__func__,0, req->id, j);
        }
    }
    
}
EXPORT_SYMBOL_GPL(print_rcv_list);

void print_callback_list(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct list_head *p;
    int j;
    for (j = 0 ; j < CHANNEL_NUM; j++)
    {
        list_for_each(p, &crdev->agent[0].rcv.rcv_callback_queue[j]){
            struct xfer_req *req = list_entry(p, struct xfer_req, list);
            pr_err("%s: agent = %d id = %d channel = %d\n",__func__,0, req->id, j);
        }
    }
    
}
EXPORT_SYMBOL_GPL(print_callback_list);

void print_xmit_deliver_list(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct list_head *p;
    // int i = 0;
    list_for_each(p, &crdev->agent[0].xmit.deliver_list){
        struct xfer_req *req = list_entry(p, struct xfer_req, list);
        pr_err("%s: deliver_list id = %d \n",__func__, req->id);
    }
}
EXPORT_SYMBOL_GPL(print_xmit_deliver_list);


void print_processing_list(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct list_head *p;
    // int i = 0;
    list_for_each(p, &crdev->agent[0].processing_queue){
        struct xfer_req *req = list_entry(p, struct xfer_req, list);
        pr_err("%s: id = %d \n",__func__ ,req->id);
    }
}
EXPORT_SYMBOL_GPL(print_processing_list);

// void send_event(struct event *ev)
// {
    // unsigned long flags;
    // struct xdma_crdev *crdev = g_xpdev->crdev;
    // struct crypto_agent *agent = &crdev->agent[0];
    // spin_lock_irqsave(&agent->deliver_events_list_lock, flags);
    // list_add(&ev->lh, &agent->deliver_events_list);
    // spin_unlock_irqrestore(&agent->deliver_events_list_lock, flags);
    // wake_up(&agent->wq_deliver_event);
// }

// struct event* get_next_event(void)
// {
    // struct event *e = NULL;
    // unsigned long flags;
    // struct xdma_crdev *crdev = g_xpdev->crdev;
    // struct crypto_agent *agent = &crdev->agent[0];
    // spin_lock_irqsave(&agent->deliver_events_list_lock, flags);
    // if (!list_empty(&agent->deliver_events_list))
    // {
    //     e = list_first_entry(&agent->deliver_events_list, struct event, lh);
    //     // if (e)
    //     //     list_del(&e->lh);
    // }
    // spin_unlock_irqrestore(&agent->deliver_events_list_lock, flags);
//     return e;
// }
