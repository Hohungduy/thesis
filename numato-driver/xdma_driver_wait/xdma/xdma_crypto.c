#include "xdma_crypto.h"

struct xdma_pci_dev *g_xpdev;

struct xdma_crdev *get_crdev(void)
{
    return g_xpdev->crdev;
}
EXPORT_SYMBOL_GPL(get_crdev);
int choose_channel(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;

    spin_lock_bh(&crdev->channel_lock);
    if (crdev->channel_load[0] < crdev->channel_load[1])
    {
        crdev->channel_load[0]++;
        spin_unlock_bh(&crdev->channel_lock);
        return 0;
    }
    else
    {
        crdev->channel_load[1]++;
        spin_unlock_bh(&crdev->channel_lock);
        return 1;
    }
}

int update_load(int channel)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    spin_lock_bh(&crdev->channel_lock);
    switch (channel)
    {   
    case 0:
        crdev->channel_load[0]--;
        break;
    case 1:
        crdev->channel_load[1]--;
        break;
    default:
        pr_info("Wrong channel!");
        break;
    }
    spin_unlock_bh(&crdev->channel_lock);
    return 0;
}
void send_event(struct event *ev)
{
    // unsigned long flags;
    // struct xdma_crdev *crdev = g_xpdev->crdev;
    // struct crypto_agent *agent = &crdev->agent[0];
    // spin_lock_irqsave(&agent->deliver_events_list_lock, flags);
    // list_add(&ev->lh, &agent->deliver_events_list);
    // spin_unlock_irqrestore(&agent->deliver_events_list_lock, flags);
    // wake_up(&agent->wq_deliver_event);
}

struct event* get_next_event(void)
{
    struct event *e = NULL;
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

// int xfer_deliver_thread(void *data)
// {
//     struct event *e;
    
    // // int cpu;
    // u32 xfer_id;
    // struct xdma_crdev *crdev = (struct xdma_crdev *)data;
    // unsigned long flags;
    // struct crypto_agent *agent = &crdev->agent[0];
    // int engine_idx = 0;
    // // struct list_head * processing_queue = 
    //     // &crdev->processing_queue;
    // // struct list_head * backlog_queue = 
    // //     &crdev->agent[0].backlog_queue;
    // struct xfer_req *next_req;
    // struct reqion *region;
    // u64 ep_addr;
    // int res;
    // struct event *ev;
    // int i;

    // printk("xfer_deliver_thread on\n");

    // while (true) {
    //     wait_event(agent->wq_deliver_event, (e = get_next_event()) );
    //     if (e && e->deliver_thread){
    //         printk("xfer_deliver_thread active\n");
    //         spin_lock_irqsave(&agent->deliver_events_list_lock, flags);
    //         list_del(&e->lh);
    //         spin_unlock_irqrestore(&agent->deliver_events_list_lock, flags);
    //         kfree(e);
    //         while(!is_engine_empty_out(engine_idx)){
    //             pr_info("engine out is not empty\n");

    //             /* Event processing */

    //             // Get xfer_id of next req
    //             // check in the req_processing
    //             spin_lock_irqsave(&crdev->processing_queue_lock, flags);
                
    //             if (list_empty(&crdev->processing_queue)){
    //                 pr_info("interrupt but list empty!\n\n");
    //                 spin_unlock_irqrestore(&crdev->processing_queue_lock, flags);
    //                 break;
    //             }
    //             next_req = list_first_entry(&crdev->processing_queue,
    //                 struct xfer_req, list);
    //             xfer_id = next_req->id;
    //             pr_info("xfer_id = %d\n", xfer_id);
    //             // remove req from processing queue 
    //             list_del(&next_req->list);
    //             spin_unlock_irqrestore(&crdev->processing_queue_lock, flags);
    //             // check in the engine
    //             if (is_engine_empty_out(engine_idx))
    //             {
    //                 pr_info("interrupt but engine empty\n");
    //                 break;
    //             }
    //             region = 
    //                 (struct reqion *)get_region_ep_addr_out(engine_idx);    
    //             // build scatterlist
    //             ep_addr = get_data_ep_addr_out(engine_idx);
    //             // xfer data to host 
    //             res = xdma_xfer_submit(crdev->xdev,
    //                 choose_channel(), FALSE, ep_addr, 
    //                 &next_req->sg_table, FALSE, 3);
    //             increase_tail_idx_out(engine_idx);
    //             if (res < 0)
    //             {
    //                 pr_info("can not read!!!\n");
    //                 continue;
    //             }
    //             // add req to list of agents
    //             spin_lock_irqsave(&crdev->agent[0].rcv.data[0].callback_queue_lock, flags);
    //             list_add_tail(&next_req->list, &crdev->agent[0].rcv.data[0].callback_queue);
    //             spin_unlock_irqrestore(&crdev->agent[0].rcv.data[0].callback_queue_lock, flags);
                
                
    //             pr_info("send event complete\n");
    //             ev = kmalloc(sizeof(struct event), GFP_ATOMIC | GFP_KERNEL);
    //             ev->print = 0;
    //             ev->stop = 0;
    //             ev->deliver_thread = 0;
    //             for (i = 0; i < CORE_NUM; i++){
    //                 ev->rcv_thread[i] = 0;
    //             }
    //             ev->rcv_thread[0] = 1;
    //             pr_info("sent event complete\n");
    //             send_event(ev);

    //             if (unlikely(e->print)){
    //                 printk("deliver agent on\n");
    //                 pr_info("trigger_work receive event\n");
    //                 pr_info("print = %d", e->print);
    //                 pr_info("stop = %d", e->stop);
    //             }
    //         }
    //         // // xdma_user_isr_enable(crdev->xdev, 0x01);
    //         if (e->stop)
    //             break;
            
    //     }
    // }
    // kfree(e);
//     do_exit(0);
// }

int min_channel_load(int *channel)
{
    int i, min = 0;
    for (i = 0; i < CHANNEL_NUM; i++)
    {
        if (channel[i] < channel[min])
            min = i;
    }
    return min;
}

int deliver_task(void *data)
{
    struct crypto_agent *agent = (struct crypto_agent *)data; 
    struct list_head *processing = &agent->processing_queue;
    struct xmit_handler *xmit = &agent->xmit;
    struct list_head *p, *n;
    struct xfer_req *req;
    static int channel_load[CHANNEL_NUM];
    int min_channel;
    unsigned long flags;
    pr_info("deliver_task wakeup\n");

    while (true) {
        printk("deliver_task wait_event\n");
        wait_event(xmit->wq_xmit_event, 
            (       (!list_empty(&xmit->deliver_list)) 
                ||  (agent->xmit.status == XMIT_STATUS_STOP)
            ) 
        );
        if (unlikely(agent->xmit.status == XMIT_STATUS_STOP))
            break;
        printk("deliver_task running\n");
        print_deliver_list();
        
        list_for_each_safe(p, n, &xmit->deliver_list){
            min_channel = min_channel_load(channel_load);

            req = list_entry(p, struct xfer_req, list); 
            spin_lock_irqsave(&xmit->deliver_list_lock, flags);
            list_del(&req->list);
            spin_unlock_irqrestore(&xmit->deliver_list_lock, flags);

            // xmit->booking = (xmit->booking + 1) % REGION_NUM;

            spin_lock_irqsave(&xmit->xmit_queue_lock[min_channel], flags);
            list_add(&req->list, &xmit->xmit_queue[min_channel]);
            spin_unlock_irqrestore(&xmit->xmit_queue_lock[min_channel], flags);
        }
        
        delete_deliver_list(g_xpdev->crdev);
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
    pr_info("xmit_task wakeup %d\n", channel_idx);
    int engine_idx = 0;
    u32 xfer_id;
    unsigned long flags;

    struct list_head *xmit_queue = &xmit->xmit_queue[channel_idx];
    struct list_head *processing = &agent->processing_queue;
    spinlock_t *lock = &xmit->xmit_queue_lock[channel_idx];

    bool write = TRUE;
    bool dma_mapped = 0;
    int timeout_ms = 3;
    int res;
    int status;
    u32 tail_idx;
    int booking_idx;

    struct xfer_req *req;
    struct region *region_base;
    u64 ep_addr;

    printk("xmit_thread on\n");
    while (true) {
        printk("xmit_task wait_event\n");
        wait_event(xmit->wq_xmit_event, 
            (       (!list_empty(&xmit->xmit_queue[channel_idx])) 
                ||  (agent->xmit.status == XMIT_STATUS_STOP)
            ) 
        );
        printk("xmit_task running\n");

        if (unlikely(agent->xmit.status == XMIT_STATUS_STOP))
            break;
            

        while (!list_empty(xmit_queue))
        {
            printk("xmit_threadd processing\n");

            // remove first req from backlog
            spin_lock_irqsave(lock, flags);
            req = list_first_entry(xmit_queue, 
                struct xfer_req, list);
            list_del(&req->list);
            spin_unlock_irqrestore(lock, flags);

            region_base = req->in_region;
            ep_addr = req->data_ep_addr;

            // Write crypto info // No need to lock
            

            // submit req from req_queue to engine 
            res = xdma_xfer_submit(g_xpdev->crdev->xdev, channel_idx, write, 
                ep_addr, &req->sg_table, dma_mapped, timeout_ms);
            if (res < 0)
            {
                pr_info("Send failed req_id = %d\n", req->id);
                continue;
            }

            // Write region dsc + see booking, modify head, tail
            spin_lock_irqsave(&agent->agent_lock, flags);
            // Write xfer_id
            write_inb_xfer_id(engine_idx, req->region_idx, req->id);
            active_inb_region(engine_idx, req->region_idx);
            // tail_idx = get_tail_inb_idx(engine_idx);
            increase_head_inb_idx(engine_idx , xmit->booking);
            // add to tail of processing queue
            list_add_tail(&req->list, processing);
            spin_unlock_irqrestore(&agent->agent_lock, flags);
        }
    }
    do_exit(0);
}

// int xfer_rcv_thread(void *data)
// {

    // // struct list_head *cbq = &thread_data->callback_queue;
    // // spinlock_t *lock = &thread_data->callback_queue_lock;
    // struct xdma_crdev *crdev = g_xpdev->crdev;
    // struct xfer_req *req;
    // struct list_head *p,*n;
    // struct event *e;
    // struct crypto_agent *agent= &crdev->agent[0];
    // unsigned long flags;

    // printk("xfer_rcv_thread on\n");

    // while (true) {
    //     wait_event(agent->wq_deliver_event, (e = get_next_event()) );
    //     if (e && e->rcv_thread[thread_data->cpu]){
    //         printk("xfer_rcv_thread active cpu = %d\n", thread_data->cpu);
    //         spin_lock_irqsave(&agent->deliver_events_list_lock, flags);
    //         list_del(&e->lh);
    //         spin_unlock_irqrestore(&agent->deliver_events_list_lock, flags);
    //         /* Event processing */

    //         // // Get xfer_id of next req
    //         if (list_empty(&thread_data->callback_queue))
    //             continue;
    //         list_for_each_safe(p, n, &thread_data->callback_queue)
    //         {
    //             req = list_entry(p, struct xfer_req, list);
    //             list_del(p);
    //             local_bh_disable();
    //             req->crypto_complete(req, req->res);
    //             local_bh_enable();
    //             kfree(req);
    //         }
    //         if (unlikely(e->print)){
    //             printk("deliver agent on\n");
    //             pr_info("trigger_work receive event\n");
    //             pr_info("print = %d", e->print);
    //             pr_info("stop = %d", e->stop);
    //         }
    //         if (e->stop)
    //             break;
    //         kfree(e);
    //     }
    // }
    // kfree(e);
    // do_exit(0);

//     return 0;
// }

void trigger_work(void)
{
    struct event *ev;
    int i;
    pr_info("trigger_work\n");
    ev = kmalloc(sizeof(struct event), GFP_ATOMIC | GFP_KERNEL);
    ev->print = 0;
    ev->stop = 0;
    ev->deliver_thread = 1;
    for (i = 0; i < CORE_NUM; i++){
        ev->rcv_thread[i] = 0;
    }
    pr_info("trigger_work send_event\n");
    send_event(ev);
}

irqreturn_t xfer_rcv(int irq_no, void *dev)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    pr_info("xfer_rcv interrupt\n");
    xdma_user_isr_disable(crdev->xdev, 0x01);
    trigger_work();
    return IRQ_HANDLED;
}


int crdev_create(struct xdma_pci_dev *xpdev)
{
    int agent_idx, channel_idx;
    struct xdma_crdev* crdev;
    struct crypto_agent *agent;
    struct xmit_handler *xmit;
    struct rcv_handler *rcv;

    crdev = (struct xdma_crdev *)kzalloc(sizeof(*crdev), GFP_KERNEL);
    if (!crdev)
    {
        pr_info("Nomem \n");
        goto free;
    }
    g_xpdev = xpdev;
    xpdev->crdev = crdev;
    crdev->xpdev = xpdev;
    crdev->xdev = xpdev->xdev;

    // Timer
    crdev->blinky.interval = 1;
    timer_setup(&crdev->blinky.timer, blinky_timeout, 0);
    crdev->blinky.led = RED_BLUE;
    // Config region base

    set_engine_base(xpdev->xdev->bar[0] + ENGINE_OFFSET(0), 0);
    set_led_base(xpdev->xdev->bar[0] + LED_OFFSET);
    
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
        xmit->deliver_task =  kthread_create_on_node
            (deliver_task, (void *)&crdev->agent,  
            cpu_to_node(agent_idx % CORE_NUM),"crdev_xmit");
        INIT_LIST_HEAD(&xmit->deliver_list);
        spin_lock_init(&xmit->deliver_list_lock);
        
        agent->xmit.status = XMIT_STATUS_ACTIVE;
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
        wake_up_process(xmit->deliver_task);

        // xfer_rcv_task
        // for (i = 0; i < CORE_NUM; i++)
        // {
        //     agent->rcv.data[i].xfer_rcv_task = kthread_create_on_node
        //         (xfer_rcv_thread, (void *)&agent->rcv.data[i], 
        //             cpu_to_node(i), "crdev_%d_agent", i);
        //     INIT_LIST_HEAD(&agent->rcv.data[i].callback_queue);
        //     spin_lock_init(&agent->rcv.data[i].callback_queue_lock);
        //     wake_up_process(agent->rcv.data[i].xfer_rcv_task);
        //     agent->rcv.data[i].cpu = i;
        // }
        // agent->rcv.xfer_deliver_task =  kthread_create_on_node
        //     (xfer_deliver_thread, (void *)&crdev, cpu_to_node(1) ,"crdev_deliver");
        // wake_up_process(agent->rcv.xfer_deliver_task);
        INIT_LIST_HEAD(&agent->processing_queue);
        agent->agent_idx = agent_idx;
        agent->xfer_idex = 0;
        spin_lock_init(&agent->agent_lock);
    }
    //init user_irq
    xdma_user_isr_enable(xpdev->xdev, 0x03);
    // timer
    mod_timer(&crdev->blinky.timer, 
        jiffies + (unsigned int)(crdev->blinky.interval*HZ));
    return 0;
free:


    return -1;
}

void delete_deliver_list(struct xdma_crdev *crdev)
{
    struct list_head *p, *n;
    int agent_idx = 0;
    int channel_idx;
    unsigned long flags;
    pr_info("remove deliver_list agent_idx = %d\n", agent_idx);
    spin_lock_irqsave(&crdev->agent[agent_idx].xmit.deliver_list_lock, flags);
    list_for_each_safe(p, n, &crdev->agent[agent_idx].xmit.deliver_list){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    spin_unlock_irqrestore(&crdev->agent[agent_idx].xmit.deliver_list_lock, flags);
}

void crdev_cleanup(void)
{
    struct list_head *p, *n;
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct event *ev;
    int agent_idx;
    int channel_idx;
    unsigned long flags;
    // Stop timer
    del_timer_sync(&crdev->blinky.timer);
    
    pr_info("stop xmit side\n");
    for (agent_idx = 0; agent_idx < AGENT_NUM; agent_idx++)
    {
        pr_info("remove processing queue agent_idx = %d\n", agent_idx);
        spin_lock_irqsave(&crdev->agent[agent_idx].agent_lock, flags);
        list_for_each_safe(p, n, &crdev->agent[agent_idx].processing_queue){
            struct xfer_req* req = list_entry(p, struct xfer_req, list);
            list_del(p);
            kfree(req);
        }
        spin_unlock_irqrestore(&crdev->agent[agent_idx].agent_lock, flags);

        //xmit

        crdev->agent[agent_idx].xmit.status = XMIT_STATUS_STOP;
        
        delete_deliver_list(crdev);
        
        pr_info("remove event_list agent_idx = %d\n", agent_idx);
        spin_lock_irqsave(&crdev->agent[agent_idx].xmit.xmit_events_list_lock, flags);
        list_for_each_safe(p, n, &crdev->agent[agent_idx].xmit.xmit_events_list){
            struct event* ev = list_entry(p, struct event, lh);
            list_del(p);
            kfree(ev);
        }
        spin_unlock_irqrestore(&crdev->agent[agent_idx].xmit.xmit_events_list_lock, flags);

        for (channel_idx = 0; channel_idx < CHANNEL_NUM; channel_idx++)
        {
            pr_info("remove xmit agent_idx = %d channel_idx = %d\n", agent_idx, channel_idx);
            spin_lock_irqsave(&crdev->agent[agent_idx].xmit.xmit_queue_lock[channel_idx], flags);
            list_for_each_safe(p, n, &crdev->agent[agent_idx].xmit.xmit_queue[channel_idx]){
                struct xfer_req* req = list_entry(p, struct xfer_req, list);
                list_del(p);
                kfree(req);
            }
            spin_unlock_irqrestore(&crdev->agent[agent_idx].xmit.xmit_queue_lock[channel_idx], flags);
        }

        // rcv
    
    }
    
    // pr_info("destroy req_queue\n");
    // list_for_each_safe(p, n, &crdev->agent[0].xmit.backlog_queue){
    //     struct xfer_req* req = list_entry(p, struct xfer_req, list);
    //     list_del(p);
    //     kfree(req);
    // }
    // pr_info("destroy req_processing\n");
    // list_for_each_safe(p, n, &crdev->processing_queue){
    //     struct xfer_req* req = list_entry(p, struct xfer_req, list);
    //     list_del(p);
    //     kfree(req);
    // }
    // // pr_info("destroy xfer_rcv_task\n");
    // // for (i = 0; i < CORE_NUM; i++)
    // // {
    // //     kthread_stop(agent->rcv.xfer_rcv_task[i]);
    // // }
    // pr_info("destroy events list\n");
    // list_for_each_safe(p, n, &crdev->agent[0].deliver_events_list){
    //     struct event* e = list_entry(p, struct event, lh);
    //     list_del(p);
    //     kfree(e);
    // }
}

int xdma_xfer_submit_queue(struct xfer_req * xfer_req)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    spinlock_t *deliver_lock = 
        &crdev->agent[0].xmit.deliver_list_lock;
    
    xfer_req->sg_table.sgl = xfer_req->sg;
    xfer_req->sg_table.nents = 1;//sg_nents(xfer_req->sg);
    xfer_req->sg_table.orig_nents = 1;//sg_nents(xfer_req->sg);
    
    spin_lock_bh(deliver_lock);
    list_add_tail(&xfer_req->list, &crdev->agent[0].xmit.deliver_list);
    spin_unlock_bh(deliver_lock);
    wake_up(&crdev->agent[0].xmit.wq_xmit_event);
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
    crdev->agent[0].xfer_idex++;
    xfer->id = crdev->agent[0].xfer_idex;
    spin_unlock_irqrestore(&crdev->agent[0].agent_lock, flags);

    return xfer;
}
EXPORT_SYMBOL_GPL(alloc_xfer_req);

void free_xfer_req(struct xfer_req *req)
{
    kfree(req);
}
EXPORT_SYMBOL_GPL(free_xfer_req);

void print_xmit_list(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct list_head *p;
    int i = 0, j;
    for (j = 0 ; j < CHANNEL_NUM; j++)
    {
        list_for_each(p, &crdev->agent[0].xmit.xmit_queue[j]){
            struct xfer_req *req = list_entry(p, struct xfer_req, list);
            pr_info("xmit_list agent = %d id = %d channel = %d\n",0, req->id, j);
        }
    }
    
}
EXPORT_SYMBOL_GPL(print_xmit_list);

void print_deliver_list(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct list_head *p;
    int i = 0;
    list_for_each(p, &crdev->agent[0].xmit.deliver_list){
        struct xfer_req *req = list_entry(p, struct xfer_req, list);
        pr_info("deliver_list id = %d \n", req->id);
    }
}
EXPORT_SYMBOL_GPL(print_deliver_list);

void print_processing_list(void)
{
    struct xdma_crdev *crdev = g_xpdev->crdev;
    struct list_head *p;
    int i = 0;
    list_for_each(p, &crdev->agent[0].processing_queue){
        struct xfer_req *req = list_entry(p, struct xfer_req, list);
        pr_info("processing_list id = %d \n", req->id);
    }
}
EXPORT_SYMBOL_GPL(print_processing_list);