#include "xdma_crypto.h"

struct xdma_pci_dev *g_xpdev;

#define BAR_0_ADDR (g_xpdev->xdev->bar[0])



void debug_mem_in(void);
void debug_mem_out(void);

// void print_req(struct xfer_req *req)
// {
//     pr_err("print_req\n");
//     pr_err("out_region %p\n", req->out_region);
//     pr_err("in_region %p\n", req->in_region);
//     pr_err("id = %d\n", req->id);
//     pr_err("region_idx = %d\n", req->region_idx);
//     pr_err("data_ep_addr = %lld\n", req->data_ep_addr);
//     pr_err("orig_nents = %d\n", req->sg_table.orig_nents);
//     pr_err("nents = %d\n", req->sg_table.nents);
//     pr_err("key = %x %x %x %x %x %x %x %x\n", 
//                  req->crypto_dsc.key[0], req->crypto_dsc.key[1],
//                  req->crypto_dsc.key[2], req->crypto_dsc.key[3],
//                  req->crypto_dsc.key[4], req->crypto_dsc.key[5],
//                  req->crypto_dsc.key[6], req->crypto_dsc.key[7]);
//     pr_err("nonce = %x\n", req->crypto_dsc.iv.nonce);
//     pr_err("iv = %x %x\n", req->crypto_dsc.iv.iv[0], req->crypto_dsc.iv.iv[1]);
//     pr_err("tail = %x\n", req->crypto_dsc.iv.tail);
//     pr_err("length = %d\n", req->crypto_dsc.info.length);
//     pr_err("direction = %d\n", req->crypto_dsc.info.direction);
//     pr_err("keysize = %d\n", req->crypto_dsc.info.keysize);
//     pr_err("addsize = %d\n", req->crypto_dsc.info.aadsize);
//     pr_err("icv = %x %x %x %x\n", req->crypto_dsc.icv[0], req->crypto_dsc.icv[1],
//                              req->crypto_dsc.icv[2], req->crypto_dsc.icv[3]);
//     pr_err("add = %x %x %x %x\n\n\n", req->crypto_dsc.aad[0], req->crypto_dsc.aad[1],
//                              req->crypto_dsc.aad[2], req->crypto_dsc.aad[3]);
// }

#define get_crdev() (g_xpdev->crdev)

// struct xdma_crdev *get_crdev(void)
// {
//     return g_xpdev->crdev;
// }
// EXPORT_SYMBOL_GPL(get_crdev);

void blinky_timeout(struct timer_list *timer)
{
    struct blinky *blinky;
    blinky = container_of(timer, struct blinky, timer);
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
    // dbg_desc("Timer blinky function\n");
    mod_timer(&blinky->timer, jiffies + (unsigned int)(blinky->interval*HZ));
}

// irqreturn_t err_handler(int irq_no, void *dev)
// {
//     // pr_err("%s:%d: err_handler\n", __func__, __LINE__);
//     return IRQ_HANDLED;
// }

// int min_channel_load(int *channel)
// {
//     // int i, min = 0;
//     // for (i = 0; i < CHANNEL_NUM; i++)
//     // {
//     //     if (channel[i] < channel[min])
//     //         min = i;
//     // }
//     // channel[min]++;
//     // return min;
//     return 0;
// }

// int write_dsc(struct xfer_req *req)
// {
//     return 0;
// }

// int xmit_deliver_task(void *data)
// {
//     struct crypto_agent *agent = (struct crypto_agent *)data; 
//     struct xmit_handler *xmit = &agent->xmit;
//     struct list_head *p, *n;
//     struct xfer_req *req;
//     static int channel_load[CHANNEL_NUM];
//     int min_channel;
//     unsigned long flags;
//     int engine_idx = 0;

//     // pr_err("%s:%d: deliver_task wakeup\n", __func__, __LINE__);
//     while (true) {
//             wait_event_interruptible(xmit->wq_xmit_event, 
//                 (       (!list_empty(&xmit->deliver_list)) 
//                     ||  (agent->xmit.status == XMIT_STATUS_STOP)
//                 ) 
//             );
//             if (unlikely(agent->xmit.status == XMIT_STATUS_STOP))
//                 break;
//             list_for_each_safe(p, n, &xmit->deliver_list){
//             min_channel = min_channel_load(channel_load);
//             req = list_entry(p, struct xfer_req, list); 
//             spin_lock_irqsave(&xmit->deliver_list_lock, flags);
//             list_del(&req->list);
//             spin_unlock_irqrestore(&xmit->deliver_list_lock, flags);
//             req->in_region = (struct region_in *)get_region_addr(engine_idx);                
//             switch (req->crypto_dsc.info.aadsize)
//             {
//             case 8:
//                 req->data_ep_addr=0x60;
//                 break;
//             case 12:
//                 req->data_ep_addr=0x5C;
//                 break;               
//             default:
//                 // pr_err("Wrong aadsize:%d\n",req->crypto_dsc.info.aadsize);
//                 break;
//             }
//             spin_lock_irqsave(&xmit->xmit_queue_lock[min_channel], flags);
//             list_add(&req->list, &xmit->xmit_queue[min_channel]);
//             spin_unlock_irqrestore(&xmit->xmit_queue_lock[min_channel], flags);
//             wake_up(&xmit->wq_xmit_event);
//         }
//     }
//     do_exit(0);
// }

// int xmit_task(void *data)
// {
//     struct task_data *task_data = (struct task_data *)data; 
//     struct xmit_handler *xmit = task_data->xmit;
//     int channel_idx = task_data->idx;
//     struct crypto_agent *agent = 
//         container_of(xmit, struct crypto_agent, xmit);
//     unsigned long flags;
//     struct xdma_crdev * crdev = get_crdev();

//     struct list_head *xmit_queue = &xmit->xmit_queue[channel_idx];
//     struct list_head *processing = &agent->processing_queue;
//     spinlock_t *lock = &xmit->xmit_queue_lock[channel_idx];
//     // bool dma_mapped = 0;
//     int engine_idx = 0;
//     int timeout_ms = 1;
//     int res;
//     // enum agent_status status;

//     struct xfer_req *req;
//     u64 ep_addr;
//     // pr_err("xmit_task wakeup %d\n", channel_idx);
//     while (true) {
//         // pr_err("xmit_task %d wait_event \n", channel_idx);
//         wait_event_interruptible(xmit->wq_xmit_event, 
//             (       (!list_empty(xmit_queue)) 
//                 ||  (agent->xmit.status == XMIT_STATUS_STOP)
//             ) 
//         );
//         // pr_err("xmit_task running %d\n", channel_idx);
//         if (unlikely(agent->xmit.status == XMIT_STATUS_STOP))
//             break;
//         if (crdev->xfer_status != XFER_STATUS_FREE)
//             continue;
//         crdev->xfer_status = XFER_STATUS_XMIT;

//         // pr_err("xmit_thread processing %d\n", channel_idx);
//         // TODO: Condition to xmit ???? lock ???

//         // remove first req from backlog
//         spin_lock_irqsave(lock, flags);
//         req = list_first_entry(xmit_queue,struct xfer_req, list);
//         list_del(&req->list);
//         spin_unlock_irqrestore(lock, flags);
    
//         ep_addr = req->data_ep_addr;

//         // submit req from req_queue to engine 
//         // pr_err("submit xmit to channel %d region %d", 
//             // channel_idx, req->region_idx);
            
//         // res = xdma_xfer_submit(g_xpdev->crdev->xdev, channel_idx, 1, 
//             // ep_addr, &req->sg_table, dma_mapped, timeout_ms);
//         req->sg_table.orig_nents = 1;
//         req->sg_table.nents = 1;
//         mdelay(10);
//         res = xdma_xfer_submit(g_xpdev->crdev->xdev, 1, 1, 
//             ep_addr, &req->sg_table, FALSE, timeout_ms);
//         // pr_err("Sent req_id = %d, res = %d \n", req->id, res);
//         if (res < 0)
//         {
//             pr_err("------------------  SEND FAILED HAHAHA ---------------\n");
//             req->res = -1;
//             spin_lock_irqsave(&agent->agent_lock, flags);
//             crdev->xfer_status = XFER_STATUS_FREE;
//             list_add_tail(&req->list, &agent->rcv.rcv_callback_queue[channel_idx]);
//             spin_unlock_irqrestore(&agent->agent_lock, flags);
//             g_xpdev->crdev->agent[0].status = AGENT_STATUS_FREE;
//             wake_up_interruptible(&agent->rcv.wq_rcv_event);
//             // pr_err("pag_link:%lx ; ofset:%x ; length:%x ; dma_address: %x,",req->sg_in->page_link,req->sg_in->offset,req->sg_in->length,req->sg_in->dma_address);

//             debug_mem_in();

//             pr_err("------------------  SEND FAILED HAHAHA ---------------\n");
//             continue;
//         }
//         // TODO: Write crypto info (No need to lock)
//         memcpy_toio(&req->in_region->crypto_dsc, 
//             &req->crypto_dsc, sizeof(struct crypto_dsc_in) );
//         // Write region dsc + see booking, modify head, tail
//         // Write xfer_id
//         memset32((void *)req->in_region, 0xABCDACBD, 1);
//         memset32((void *)req->in_region + 4, req->id, 1);
//         memset32((void *)req->in_region + 12, 0, 1);
//         memset32((void *)req->in_region + 8, req->crypto_dsc.info.length, 1);
//         // add to tail of processing queue

//         spin_lock_irqsave(&agent->agent_lock, flags);
//         list_add_tail(&req->list, processing);
//         trigger_engine(engine_idx);
//         spin_unlock_irqrestore(&agent->agent_lock, flags);
        
//     }
//     do_exit(0);
// }

// int rcv_deliver_task(void *data)
// {
//     struct crypto_agent *agent = (struct crypto_agent *)data;
//     int engine_idx = 0;
//     struct xfer_req *req = NULL;
//     struct list_head *p,*n;
//     unsigned long flags;
//     int channel_idx, min_load_ch_idx;
//     u32 xfer_id;
//     struct xdma_crdev *crdev = get_crdev();
//     // pr_err("xfer_rcv_thread on\n");
//     while (true) {
//         wait_event_interruptible(agent->rcv.wq_rcv_event, (agent->need_deliver == TRUE) );
//         // pr_err("rcv_deliver thread on\n");
//         // TODO: Sanity checks
//         if (list_empty(&agent->processing_queue))
//         {
//             // pr_err("Interrupt but processing queue empty\n");
//             continue;
//         }
//         // TODO: Read booking, head, tail
//         spin_lock_irqsave(&agent->agent_lock, flags);
//         // TODO: Find in processing list the xfer_req struct based on xfer_id            
//         xfer_id = get_xfer_id_outb_idx(engine_idx, 0);
//         spin_unlock_irqrestore(&agent->agent_lock, flags);
//         // pr_err("Searching for xfer\n");
//         list_for_each_safe(p, n, &agent->processing_queue)
//         {
//             // TODO: Remove xfer_req from xfer_processing
                    
//             req = list_entry(p, struct xfer_req, list);
//             if (req->id == xfer_id)
//             {
//                 // pr_err("Found matching id\n");
//                 list_del(&req->list);
//                 break;
//             }
//         }
//         // pr_err("Found! \n");
//         // TODO: Choose channel and add to rcv_list of that channel
//         spin_lock_bh(&g_xpdev->crdev->channel_lock);
//         min_load_ch_idx = 0;
//         g_xpdev->crdev->channel_load[min_load_ch_idx]++;
//         spin_unlock_bh(&g_xpdev->crdev->channel_lock);
//         spin_lock_bh(&agent->rcv.rcv_queue_lock[min_load_ch_idx]);
//         list_add_tail(&req->list, &agent->rcv.rcv_queue[min_load_ch_idx]);
//         spin_unlock_bh(&agent->rcv.rcv_queue_lock[min_load_ch_idx]);
//         wake_up_interruptible(&agent->rcv.wq_rcv_event);
//         spin_lock_irqsave(&agent->agent_lock, flags);
//         agent->need_deliver = FALSE;
//         crdev->xfer_status = XFER_STATUS_RCV;
//         spin_unlock_irqrestore(&agent->agent_lock, flags);
//     }
//     do_exit(0);
// }

// int rcv_task(void *data)
// {
//     struct task_data *task_data = (struct task_data *)data;
//     struct rcv_handler *rcv = task_data->rcv;
//     int channel_idx = task_data->idx;
//     int res;
//     unsigned long flags;
//     u64 outbound_data_addr = 0;
//     struct xfer_req *req;
//     struct xdma_crdev *crdev = get_crdev();
//     // pr_err("rcv_task chanel = %d on\n", channel_idx);

//     while (true) {
//         wait_event_interruptible(rcv->wq_rcv_event, 
//         (  (!list_empty(&rcv->rcv_queue[channel_idx])) ||
//            (rcv->status == RCV_STATUS_STOP)
//         )
//         );
//         if (likely(rcv->status == RCV_STATUS_STOP))
//             break;
//         if (crdev->xfer_status != XFER_STATUS_RCV)
//             continue;
//         // pr_err("rcv_task active %d\n", channel_idx);

//         // Get the req to rcv
//         spin_lock_irqsave(&rcv->rcv_queue_lock[channel_idx], flags);
//         req = list_first_entry(&rcv->rcv_queue[channel_idx], struct xfer_req, list);
//         list_del(&req->list);
//         spin_unlock_irqrestore(&rcv->rcv_queue_lock[channel_idx], flags);

//         if(req->crypto_dsc.info.aadsize == 8)
//             	outbound_data_addr = 0x10020 - 16;
//         else if(req->crypto_dsc.info.aadsize == 12)
//                 outbound_data_addr = 0x10020 - 20;
//         else 
//         {
//             outbound_data_addr = 0x10020 - 16;
//             // pr_err("%s:%d: Wrong aadsize %d\n", __func__, __LINE__, req->crypto_dsc.info.aadsize);
//         }

//         memcpy_fromio(req->tag, BAR_0_ADDR + 0x10000 + req->tag_offset, req->tag_length);
//         req->sg_table.sgl = &req->sg_rcv;
//         req->sg_table.orig_nents = 1;
//         req->sg_table.nents = 1;
//         debug_mem_in();
//     	res = xdma_xfer_submit(g_xpdev->crdev->xdev,
//             1, FALSE, outbound_data_addr, &req->sg_table, FALSE, 1);
//         if (res < 0)
//         {
//             // pr_err("can not read!!!\n");
//             continue;
//         }
//         req->res = 0;
//         // TODO: Free region + lock ???
//         spin_lock_irqsave(&g_xpdev->crdev->agent[0].agent_lock, flags);
//         g_xpdev->crdev->agent[0].status = AGENT_STATUS_FREE;
//         crdev->xfer_status = XFER_STATUS_FREE;
//         spin_unlock_irqrestore(&g_xpdev->crdev->agent[0].agent_lock, flags);
            	
//         // Call callback
//         spin_lock_irqsave(&rcv->rcv_callback_queue_lock[channel_idx], flags);
//         list_add_tail(&req->list, &rcv->rcv_callback_queue[channel_idx]);
//         spin_unlock_irqrestore(&rcv->rcv_callback_queue_lock[channel_idx], flags);
//         wake_up_interruptible(&rcv->wq_rcv_event);
//     }
//     do_exit(0);
// }

int callback_task(void *data)
{
    struct xdma_crdev* crdev = (struct xdma_crdev *)data;
    struct xfer_req *req;
    int res;
    while (true) {
        wait_event_interruptible(crdev->cb_wq, 
          ( req = list_first_entry(&crdev->cb_queue, struct xfer_req, list) ));

        spin_lock_bh(&crdev->agent_lock);
        req = list_first_entry(&crdev->cb_queue, 
            struct xfer_req, list);
        list_del(&req->list);
        crdev->req_num--;
        spin_unlock_bh(&crdev->agent_lock);

        local_bh_disable();
        if (req->crypto_complete)
                res = req->crypto_complete(req, req->res);
        local_bh_enable();
    }
    do_exit(0);
}


int crypto_task(void *data)
{
    struct xdma_crdev* crdev = (struct xdma_crdev *)data;
    struct xfer_req *req;
    struct sg_table sgt;
    int timeout_ms;
    u64 ep_addr;
    void *in_region ;
    void *out_region;


    timeout_ms = 3;
    in_region = get_region_in();
    out_region = get_region_out();

    while (true) {
        wait_event_interruptible(crdev->crypto_wq,
            ( req = list_first_entry(&crdev->req_queue, struct xfer_req, list)));

        // remove first req from backlog
        spin_lock_bh(&crdev->agent_lock);
        list_del(&req->list);
        spin_unlock_bh(&crdev->agent_lock);
    
        switch (req->crypto_dsc.info.aadsize)
        {
            case 8:
                ep_addr = 0x60;
                break;
            case 12:
                ep_addr = 0x5C;
                break;               
            default:
                pr_err("Wrong aadsize:%d\n", req->crypto_dsc.info.aadsize);
                goto err_aadsize;
                break;
        }

        // submit req from req_queue to engine 
        sgt.sgl = req->sg_in;
        sgt.orig_nents = 1;
        sgt.nents = 1;

        reinit_completion(&crdev->encrypt_done);

        req->res = xdma_xfer_submit(crdev->xdev, DEFAULT_CHANNEL_IDX, 
            XFER_WRITE, ep_addr, &sgt, FALSE, timeout_ms);
        if (req->res < 0)
        {

            goto xmit_failed;
        }

        // Write crypto info
        // memcpy_toio(in_region + offsetof(struct region_in, crypto_dsc),
        //     &req->crypto_dsc, sizeof(struct crypto_dsc_in) );
        // // Write xfer_id
        // memset32(in_region, 0xABCDACBD, 1);
        // memset32(in_region + 4, req->id, 1);
        // memset32(in_region + 12, 0, 1);
        // memset32(in_region + 8, req->crypto_dsc.info.length, 1);
        
        memcpy_toio(in_region + offsetof(struct region_in, crypto_dsc),
            &req->crypto_dsc, sizeof(struct crypto_dsc_in) );
        // Write xfer_id
        memset32(in_region + offsetof(struct region_in, region_dsc), 
            0xABCDACBD, 1);
        memset32(in_region + offsetof(struct region_in, xfer_id), 
            req->id, 1);
        memset32(in_region + offsetof(struct region_in, xfer_dsc), 
            0, 1);
        memset32(in_region + offsetof(struct region_in, data_len),
            req->crypto_dsc.info.length, 1);
        
        // add to tail of processing queue
        spin_lock_bh(&crdev->agent_lock);
        list_add_tail(&req->list, &crdev->cb_queue);
        trigger_engine(DEFAULT_ENGINE);
        spin_unlock_bh(&crdev->agent_lock);

        wait_for_completion(&crdev->encrypt_done);

        // Encrypt Done!

        sgt.sgl = req->sg_out;
        sgt.orig_nents = 1;
        sgt.nents = 1;

        switch (req->crypto_dsc.info.aadsize)
        {
            case 8:
                ep_addr = 0x10020 - 16;
                break;
            case 12:
                ep_addr = 0x10020 - 20;
                break;               
            default:
                goto err_aadsize;
                break;
        }
        req->res = xdma_xfer_submit(crdev->xdev, DEFAULT_CHANNEL_IDX, 
            XFER_READ, ep_addr, &sgt, FALSE, timeout_ms);
        if (req->res < 0)
        {
            goto rcv_failed;
        }
        // TAG
        memcpy_fromio(req->tag, 
            BAR_0_ADDR + 0x10000 + req->tag_offset, req->tag_length);

        spin_lock_bh(&crdev->agent_lock);
        list_add_tail(&req->list, &crdev->cb_queue);
        spin_unlock_bh(&crdev->agent_lock);

        continue;
err_aadsize:
        pr_err("------------------  AAD_SIZE  ---------------\n");
        req->res = -1;
        spin_lock_bh(&crdev->agent_lock);
        list_add_tail(&req->list, &crdev->cb_queue);
        spin_unlock_bh(&crdev->agent_lock);
        debug_mem_in();
        debug_mem_out();
        wake_up_interruptible(&crdev->cb_wq);
        pr_err("------------------  AAD_SIZE ---------------\n");
        continue;

xmit_failed:
        pr_err("------------------  XMIT FAILED ---------------\n");
        req->res = -1;
        spin_lock_bh(&crdev->agent_lock);
        list_add_tail(&req->list, &crdev->cb_queue);
        spin_unlock_bh(&crdev->agent_lock);
        debug_mem_in();
        wake_up_interruptible(&crdev->cb_wq);
        pr_err("------------------  XMIT FAILED ---------------\n");
        continue;

rcv_failed:
        pr_err("------------------  RCV FAILED ---------------\n");
        req->res = -1;
        spin_lock_bh(&crdev->agent_lock);
        list_add_tail(&req->list, &crdev->cb_queue);
        spin_unlock_bh(&crdev->agent_lock);
        debug_mem_in();
        debug_mem_out();
        wake_up_interruptible(&crdev->cb_wq);
        pr_err("------------------  RCV FAILED ---------------\n");
        continue;
    }
    do_exit(0);
}

irqreturn_t xfer_rcv(int irq_no, void *dev)
{
    struct xdma_crdev *crdev = get_crdev();
    clear_usr_irq(CRYTO_DONE_IRQ);
    complete(&crdev->encrypt_done);
    return IRQ_HANDLED;
}


int crdev_create(struct xdma_pci_dev *xpdev)
{
    struct xdma_crdev* crdev;
    struct crypto_engine engine;

    // mutex_init(&xmit_mutex);
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

    // Config region base
    engine.comm   = BAR_0_ADDR + COMM_REGION_OFFSET;
    engine.in     = BAR_0_ADDR + IN_REGION_OFFSET;
    engine.out    = BAR_0_ADDR + OUT_REGION_OFFSET;
    engine.status = BAR_0_ADDR + STATUS_REGION_OFFSET;
    engine.irq    = BAR_0_ADDR + IRQ_REIGON_OFFSET;
    set_engine_base(engine, 0);
    set_led_base(BAR_0_ADDR);
    // Timer
    crdev->blinky.interval = 1;
    timer_setup(&crdev->blinky.timer, blinky_timeout, 0);
    crdev->blinky.led = RED;
    
    xdma_user_isr_register(xpdev->xdev, 0x01, xfer_rcv, crdev);

    crdev->channel_idx = DEFAULT_CHANNEL_IDX;
    crdev->xfer_idex = 0;
    spin_lock_init(&crdev->agent_lock);
    INIT_LIST_HEAD(&crdev->req_queue);
    INIT_LIST_HEAD(&crdev->cb_queue);
    // init_waitqueue_head(&crdev->crypto_wq);
    // init_waitqueue_head(&crdev->cb_wq);

    // init_completion(&crdev->encrypt_done);

    // crdev->crypto_task = kthread_create_on_node(crypto_task, crdev, 
    //         cpu_to_node(DEFAULT_CORE), "crypto_task");
    // crdev->callback_task = kthread_create_on_node(callback_task, crdev, 
    //         cpu_to_node(DEFAULT_CORE), "cb_task");
    // // Wake up routine
    // wake_up_process(crdev->crypto_task);
    // wake_up_process(crdev->callback_task);

    // //init user_irq
    // xdma_user_isr_enable(xpdev->xdev, 0x01);
    // timer
    mod_timer(&crdev->blinky.timer, 
        jiffies + (unsigned int)(crdev->blinky.interval*HZ));
    return 0;
free:
    // TODO:
    return -1;
}

void crdev_cleanup(void)
{
    struct list_head *p, *n;
    struct xdma_crdev *crdev = get_crdev();
    // Stop timer
    del_timer_sync(&crdev->blinky.timer);
    
    pr_err("crdev cleanup\n");
    spin_lock_irq(&crdev->agent_lock);

    list_for_each_safe(p, n, &crdev->req_queue){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }
    list_for_each_safe(p, n, &crdev->cb_queue){
        struct xfer_req* req = list_entry(p, struct xfer_req, list);
        list_del(p);
        kfree(req);
    }

    kthread_stop(crdev->crypto_task);
    kthread_stop(crdev->callback_task);

    spin_unlock_irq(&crdev->agent_lock);

}

int xdma_xfer_submit_queue(struct xfer_req * xfer_req)
{
    struct xdma_crdev *crdev = get_crdev();
    struct list_head *req_queue = &crdev->req_queue;

    if (crdev->req_num > MAX_REQ_NUM){
        pr_err("CRYPTO REACHES MAX_SIZE\n");
        return -1;
    }

    spin_lock_bh(&crdev->agent_lock);
    xfer_req->id = crdev->xfer_idex;
    crdev->xfer_idex = crdev->xfer_idex % 1024;
    crdev->req_num++;
    list_add_tail(&xfer_req->list, req_queue);
    spin_unlock_bh(&crdev->agent_lock);
    
    wake_up_interruptible(&crdev->crypto_wq);
    return -EINPROGRESS;
}

EXPORT_SYMBOL_GPL(xdma_xfer_submit_queue);

struct xfer_req *alloc_xfer_req(void)
{
    struct xfer_req *xfer;
    xfer = (struct xfer_req *)kzalloc(sizeof(struct xfer_req), GFP_KERNEL);
    if (!xfer)
        return 0;
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

int set_sg_in(struct xfer_req *req, struct scatterlist *sg)
{
    if (!req){
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
        return -1;
    }
    if (!sg){
        pr_err("%s:%d: sg NULL\n", __func__, __LINE__);
        return -1;
    }
    req->sg_in = sg;
    return 0;
}
EXPORT_SYMBOL_GPL(set_sg_in);

int set_sg_out(struct xfer_req *req, struct scatterlist *sg)
{
    if (!req){
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
        return -1;
    }
    if (!sg){
        pr_err("%s:%d: sg NULL\n", __func__, __LINE__);
        return -1;
    }
    req->sg_out = sg;
    return 0;
}
EXPORT_SYMBOL_GPL(set_sg_out);

int set_callback(struct xfer_req *req, int (*cb)(struct xfer_req *req, int res))
{
    if (req == NULL){
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
        return -1;
    }
    req->crypto_complete = cb;
    return 0;
}
EXPORT_SYMBOL_GPL(set_callback);


int set_ctx(struct xfer_req *req, struct mycrypto_context ctx)
{
    if (!req){
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
        return -1;
    }
    req->ctx = ctx;
    return 0;
}
EXPORT_SYMBOL_GPL(set_ctx);

int get_result(struct xfer_req *req, int *res)
{
    if (!req){
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
        return -1;
    }
    *res = req->res;
    return 0;
}
EXPORT_SYMBOL_GPL(get_result);

void free_xfer_req(struct xfer_req *req)
{
    struct xdma_crdev *crdev = get_crdev();
    if (req){
        kfree(req);
        spin_lock_bh(&crdev->agent_lock);
        crdev->req_num--;
        spin_unlock_bh(&crdev->agent_lock);
    }
    else{
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
    }
    
}
EXPORT_SYMBOL_GPL(free_xfer_req);

// void print_xmit_list(void)
// {
    // struct xdma_crdev *crdev = get_crdev();
    // struct list_head *p;
    // int j;
    // for (j = 0 ; j < CHANNEL_NUM; j++)
    // {
    //     list_for_each(p, &crdev->agent[0].xmit.xmit_queue[j]){
    //         struct xfer_req *req = list_entry(p, struct xfer_req, list);
    //         // pr_err("%s: agent = %d id = %d channel = %d\n",__func__,0, req->id, j);
    //     }
    // }
    
// }
// EXPORT_SYMBOL_GPL(print_xmit_list);



// DEBUG FUNCTIONS


// void print_rcv_list(void)
// {
    // struct xdma_crdev *crdev = get_crdev();
    // struct list_head *p;
    // int j;
    // for (j = 0 ; j < CHANNEL_NUM; j++)
    // {
    //     list_for_each(p, &crdev->agent[0].rcv.rcv_queue[j]){
    //         struct xfer_req *req = list_entry(p, struct xfer_req, list);
    //         // pr_err("%s: agent = %d id = %d channel = %d\n",__func__,0, req->id, j);
    //     }
    // }
    
// }
// EXPORT_SYMBOL_GPL(print_rcv_list);

// void print_callback_list(void)
// {
    // struct xdma_crdev *crdev = get_crdev();
    // struct list_head *p;
    // int j;
    // for (j = 0 ; j < CHANNEL_NUM; j++)
    // {
    //     list_for_each(p, &crdev->agent[0].rcv.rcv_callback_queue[j]){
    //         struct xfer_req *req = list_entry(p, struct xfer_req, list);
    //         // pr_err("%s: agent = %d id = %d channel = %d\n",__func__,0, req->id, j);
    //     }
    // }
    
// }
// EXPORT_SYMBOL_GPL(print_callback_list);

// void print_xmit_deliver_list(void)
// {
    // struct xdma_crdev *crdev = get_crdev();
    // struct list_head *p;
    // // int i = 0;
    // list_for_each(p, &crdev->agent[0].xmit.deliver_list){
    //     struct xfer_req *req = list_entry(p, struct xfer_req, list);
    //     // pr_err("%s: deliver_list id = %d \n",__func__, req->id);
    // }
// }
// EXPORT_SYMBOL_GPL(print_xmit_deliver_list);


// void print_processing_list(void)
// {
    // struct xdma_crdev *crdev = get_crdev();
    // struct list_head *p;
    // // int i = 0;
    // list_for_each(p, &crdev->agent[0].processing_queue){
    //     struct xfer_req *req = list_entry(p, struct xfer_req, list);
    //     // pr_err("%s: id = %d \n",__func__ ,req->id);
    // }
// }
// EXPORT_SYMBOL_GPL(print_processing_list);

void debug_mem_in(void )
{
    int i;
    u32 *p;
    for (i = 0; i < (0x300); i+= 8)
    {
        p = (u32 *)get_region_in() + i;
        pr_err("debug inbound addr %p: %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x\n", p,
            *(p + 7), *(p + 6), *(p + 5), *(p + 4), *(p + 3), *(p + 2),*(p + 1), *p); 
    }
}

void debug_mem_out(void )
{
    int i;
    u32 *p;
    for (i = 0; i < (0x300); i+= 8)
    {
        p = (u32 *)get_region_out() + i;
        pr_err("debug outbound addr %p: %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x\n", p,
            *(p + 7), *(p + 6), *(p + 5), *(p + 4), *(p + 3), *(p + 2),*(p + 1), *p); 
    }
}