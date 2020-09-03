#include "xdma_crypto.h"

struct xdma_pci_dev *g_xpdev;

#define BAR_0_ADDR (g_xpdev->xdev->bar[0])
#define get_crdev() (g_xpdev->crdev)


#ifdef DEBUG_LINE
#define debug_line() pr_err("%s:%d\n", __func__, __LINE__)
#else
#define debug_line() 
#endif


void blinky(struct timer_list *timer)
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
    mod_timer(&blinky->timer, jiffies + (unsigned int)(blinky->interval_s*HZ));
}

int callback_task(void *data)
{
    struct xdma_crdev* crdev = get_crdev();
    struct xfer_req *req;
    struct engine_data *engine = (struct engine_data *)data;
    spinlock_t *cb_lock = &engine->cb_lock;
    struct list_head *cb_queue = &engine->cb_queue;

    while (true) {
        wait_event_interruptible(crdev->cb_wq, 
          ( !list_empty(cb_queue) ));
start:
        spin_lock(cb_lock);
        req = list_first_entry(cb_queue, 
            struct xfer_req, list);
        list_del(&req->list);
        spin_unlock(cb_lock);
        
        atomic_dec(&crdev->req_num);

        if (req->crypto_complete)
                req->crypto_complete(req, get_result(req));
        if (!list_empty(cb_queue))
            goto start;
    }
    do_exit(0);
}


int crypto_task(void *data)
{
    struct xdma_crdev* crdev = get_crdev();
    struct engine_data *engine = (struct engine_data *)data;
    int engine_idex = engine->engine_idex;
    spinlock_t *req_lock;
    spinlock_t *cb_lock ;   
    struct xfer_req *req;
    struct sg_table sgt;
    int timeout_ms;
    void *in_region ;
    void *out_region;
    int result;

    timeout_ms = 10000;
    in_region  = get_region_in(engine_idex);
    out_region = get_region_out(engine_idex);
    req_lock = &engine->req_lock;
    cb_lock = &engine->cb_lock;
    struct list_head *req_queue = &engine->req_queue;
    struct list_head *cb_queue = &engine->cb_queue;
    struct completion *encrypt_done = &engine->encrypt_done;
    int in_ep, out_ep;
    switch (engine_idex)
    {
    case 0:
        in_ep = IN_REGION_0_OFFSET;
        out_ep = OUT_REGION_0_OFFSET;
        break;
    case 1:
        in_ep = IN_REGION_1_OFFSET;
        out_ep = OUT_REGION_1_OFFSET;
        break;
    case 2:
        in_ep = IN_REGION_2_OFFSET;
        out_ep = OUT_REGION_2_OFFSET;
        break;
    case 3:
        in_ep = IN_REGION_3_OFFSET;
        out_ep = OUT_REGION_3_OFFSET;
        break;
    default:
        in_ep = IN_REGION_0_OFFSET;
        out_ep = OUT_REGION_0_OFFSET;
        break;
    }

    while (true) {
        wait_event_interruptible(crdev->crypto_wq,
            ( !list_empty(req_queue) ));
start:

        spin_lock(req_lock);
        req = list_first_entry(req_queue, 
            struct xfer_req, list);
        list_del(&req->list);
        spin_unlock(req_lock);

        reinit_completion(encrypt_done);

        sgt.sgl = req->sg_in;
        sgt.orig_nents = sg_nents(req->sg_in);
        result = xdma_xfer_submit(crdev->xdev, req->channel, 
            XFER_WRITE, in_ep + 96, &sgt, FALSE, timeout_ms);
        if (result < 0)
            goto xmit_failed;

        // Write crypto info
        write_crypto_info(in_region, req);

        // add to tail of processing queue
        trigger_engine(engine_idex);

        // Wait 
        wait_for_completion(encrypt_done);
	
        // Encrypt Done!
        sgt.sgl = req->sg_out;
        sgt.orig_nents = sg_nents(req->sg_out);
        result = xdma_xfer_submit(crdev->xdev, req->channel,
            XFER_READ, out_ep + 16, &sgt, FALSE, timeout_ms);
        if (result < 0)
            goto rcv_failed;

        result = get_tag_from_card(req, engine_idex);
        if (result < 0)
            goto rcv_failed;

        spin_lock(cb_lock);
        list_add_tail(&req->list, cb_queue);
        spin_unlock(cb_lock);

        req->res = DONE;
        goto go_back;

xmit_failed:
        pr_err("------------------  XMIT FAILED ---------------\n");
        spin_lock(cb_lock);
        list_add_tail(&req->list, cb_queue);
        spin_unlock(cb_lock);
        debug_mem_in(engine_idex);
        pr_err("------------------  XMIT FAILED ---------------\n");
        req->res = -BUSY_XMIT;
        goto go_back;

rcv_failed:
        pr_err("------------------  RCV FAILED ---------------\n");
        spin_lock(cb_lock);
        list_add_tail(&req->list, cb_queue);
        spin_unlock(cb_lock);
        debug_mem_in(engine_idex);
        debug_mem_out(engine_idex);
        pr_err("------------------  RCV FAILED ---------------\n");
        req->res = -BUSY_RCV;
        goto go_back;
        
go_back:
        wake_up(&crdev->cb_wq);
        if (!list_empty(req_queue))
            goto start;
    }
    do_exit(0);
}

irqreturn_t xfer_rcv(int irq_no, void *dev)
{
    struct engine_data *engine = (struct engine_data *)dev;
    int engine_idex = engine->engine_idex;
    struct completion *encrypt_done = &engine->encrypt_done;
    clear_usr_irq(engine_idex);
    complete(encrypt_done);
    return IRQ_HANDLED;
}


int crdev_create(struct xdma_pci_dev *xpdev)
{
    struct xdma_crdev* crdev;
    struct mem_base mem_base;
    int i;

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
    mem_base.red_led       = BAR_0_ADDR + LED_RED_OFFSET;
    mem_base.blue_led      = BAR_0_ADDR + LED_BLUE_OFFSET;

    mem_base.engine[0].comm   = BAR_0_ADDR + COMM_REGION_0_OFFSET;
    mem_base.engine[0].in     = BAR_0_ADDR + IN_REGION_0_OFFSET;
    mem_base.engine[0].out    = BAR_0_ADDR + OUT_REGION_0_OFFSET;
    mem_base.engine[0].status = BAR_0_ADDR + STATUS_REGION_0_OFFSET;
    mem_base.engine[0].irq    = BAR_0_ADDR + IRQ_REIGON_0_OFFSET;

    mem_base.engine[1].comm   = BAR_0_ADDR + COMM_REGION_1_OFFSET;
    mem_base.engine[1].in     = BAR_0_ADDR + IN_REGION_1_OFFSET;
    mem_base.engine[1].out    = BAR_0_ADDR + OUT_REGION_1_OFFSET;
    mem_base.engine[1].status = BAR_0_ADDR + STATUS_REGION_1_OFFSET;
    mem_base.engine[1].irq    = BAR_0_ADDR + IRQ_REIGON_1_OFFSET;

    mem_base.engine[2].comm   = BAR_0_ADDR + COMM_REGION_2_OFFSET;
    mem_base.engine[2].in     = BAR_0_ADDR + IN_REGION_2_OFFSET;
    mem_base.engine[2].out    = BAR_0_ADDR + OUT_REGION_2_OFFSET;
    mem_base.engine[2].status = BAR_0_ADDR + STATUS_REGION_2_OFFSET;
    mem_base.engine[2].irq    = BAR_0_ADDR + IRQ_REIGON_2_OFFSET;

    mem_base.engine[3].comm   = BAR_0_ADDR + COMM_REGION_3_OFFSET;
    mem_base.engine[3].in     = BAR_0_ADDR + IN_REGION_3_OFFSET;
    mem_base.engine[3].out    = BAR_0_ADDR + OUT_REGION_3_OFFSET;
    mem_base.engine[3].status = BAR_0_ADDR + STATUS_REGION_3_OFFSET;
    mem_base.engine[3].irq    = BAR_0_ADDR + IRQ_REIGON_3_OFFSET;

    set_mem_base(mem_base);
    // Timer
    crdev->blinky.interval_s = 1;
    timer_setup(&crdev->blinky.timer, blinky, 0);
    crdev->blinky.led = RED;
    
    
    init_waitqueue_head(&crdev->crypto_wq);
    init_waitqueue_head(&crdev->cb_wq);
    spin_lock_init(&crdev->agent_lock);

    atomic_set(&crdev->req_num, 0);
#define USE_ENGINE (1)
    for (i = 0; i < USE_ENGINE; i++)
    {
        crdev->engine[i].engine_idex = i;
        xdma_user_isr_register(xpdev->xdev, 0x1 << i, xfer_rcv, &crdev->engine[i]);
        spin_lock_init(&crdev->engine[i].cb_lock);
        spin_lock_init(&crdev->engine[i].req_lock);
        
        INIT_LIST_HEAD(&crdev->engine[i].req_queue);
        INIT_LIST_HEAD(&crdev->engine[i].cb_queue);

        init_completion(&crdev->engine[i].encrypt_done);

        crdev->engine[i].crypto_task = kthread_create_on_node(crypto_task, 
            &crdev->engine[i], cpu_to_node(DEFAULT_CORE), "crypto_task_%d", i);
        crdev->engine[i].callback_task = kthread_create_on_node(callback_task, 
            &crdev->engine[i], cpu_to_node(DEFAULT_CORE), "cb_task_%d", i);
    }

    // // Wake up routine
    for (i = 0; i < USE_ENGINE; i++)
    {
        wake_up_process(crdev->engine[i].crypto_task);
        wake_up_process(crdev->engine[i].callback_task);
        // //init user_irq
        xdma_user_isr_enable(xpdev->xdev, 0x1 << i);
    }
    // timer
    mod_timer(&crdev->blinky.timer, 
        jiffies + (unsigned int)(crdev->blinky.interval_s*HZ));
    return 0;
free:
    // TODO:
    return -1;
}

void crdev_cleanup(void)
{
    struct list_head *p, *n;
    struct xdma_crdev *crdev = get_crdev();
    int i;
    // Stop timer
    del_timer_sync(&crdev->blinky.timer);
    
    pr_err("crdev cleanup\n");
    spin_lock_irq(&crdev->agent_lock);
    for (i = 0; i < USE_ENGINE; i++)
    {
        list_for_each_safe(p, n, &crdev->engine[i].req_queue){
            struct xfer_req* req = list_entry(p, struct xfer_req, list);
            list_del(p);
            kfree(req);
        }
        list_for_each_safe(p, n, &crdev->engine[i].cb_queue){
            struct xfer_req* req = list_entry(p, struct xfer_req, list);
            list_del(p);
            kfree(req);
        }

        kthread_stop(crdev->engine[i].crypto_task);
        kthread_stop(crdev->engine[i].callback_task);
    }
    spin_unlock_irq(&crdev->agent_lock);

}

#define MAX_CHANNEL (2)

int xdma_xfer_submit_queue(struct xfer_req * xfer_req)
{
    struct xdma_crdev *crdev = get_crdev();
    int engine_idex;

    if (atomic_read(&crdev->req_num) > MAX_REQ_NUM){
        pr_err("CRYPTO REACHES MAX_SIZE\n");
        return -1;
    }
    engine_idex = atomic_inc_return(&crdev->req_num) % USE_ENGINE;	
    xfer_req->channel = atomic_read(&crdev->req_num) % MAX_CHANNEL;
    
    spin_lock(&crdev->engine[engine_idex].req_lock);
    list_add_tail(&xfer_req->list, &crdev->engine[engine_idex].req_queue);
    spin_unlock(&crdev->engine[engine_idex].req_lock);


    wake_up(&crdev->crypto_wq);
    return -EINPROGRESS;
}

EXPORT_SYMBOL_GPL(xdma_xfer_submit_queue);

struct xfer_req *alloc_xfer_req(void)
{
    struct xfer_req *xfer;
    xfer = (struct xfer_req *)kzalloc(sizeof(struct xfer_req), GFP_ATOMIC);
    if (!xfer)
        return 0;
    return xfer;
}
EXPORT_SYMBOL_GPL(alloc_xfer_req);

void free_xfer_req(struct xfer_req *req)
{
    // struct xdma_crdev *crdev = get_crdev();
    if (req){
        kfree(req);
    }
    else{
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
    }
}
EXPORT_SYMBOL_GPL(free_xfer_req);

int set_tag(struct xfer_req *req, int length, int offset)
{
    if (!req)
        return -1;
    req->tag_length = length;
    req->tag_offset = offset;
    return 0;
}
EXPORT_SYMBOL_GPL(set_tag);

int get_tag(struct xfer_req *req, void *buf)
{
    if (!req)
        return -1;
    memcpy(buf, req->tag_buff, req->tag_length);
    return 0;
}
EXPORT_SYMBOL_GPL(get_tag);

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


int set_ctx(struct xfer_req *req, struct bkcrypto_context ctx)
{
    if (!req){
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
        return -1;
    }
    req->ctx = ctx;
    return 0;
}
EXPORT_SYMBOL_GPL(set_ctx);

enum xfer_result_t get_result(struct xfer_req *req)
{
    if (!req){
        pr_err("%s:%d: req NULL\n", __func__, __LINE__);
        return -1;
    }
    return req->res;
}
EXPORT_SYMBOL_GPL(get_result);


void debug_mem_in(int engine_idx)
{
    int i;
    u32 *p;
    for (i = 0; i < (0x300); i+= 8)
    {
        p = (u32 *)get_region_in(engine_idx) + i;
        pr_err("IN %p: %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x\n", 
            p, *(p + 7), *(p + 6), *(p + 5), *(p + 4), 
            *(p + 3), *(p + 2),*(p + 1), *p); 
    }
}

void debug_mem_out(int engine_idx )
{
    int i;
    u32 *p;
    for (i = 0; i < (0x300); i+= 8)
    {
        p = (u32 *)get_region_out(engine_idx) + i;
        pr_err("OUT %p: %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x %8.0x\n", 
            p, *(p + 7), *(p + 6), *(p + 5), *(p + 4), 
            *(p + 3), *(p + 2),*(p + 1), *p); 
    }
}

inline struct crypto_dsc_in *get_dsc_in(struct xfer_req *req)
{
    return &req->crypto_dsc;
}

void write_crypto_info(void *mem_in_base, struct xfer_req *req)
{
    memcpy_toio(mem_in_base + offsetof(struct inbound, crypto_dsc),
        get_dsc_in(req), sizeof(struct crypto_dsc_in));
}

int get_tag_from_card(struct xfer_req *req, int engine_idx)
{
    if (!req)
        return -1;
    memcpy_fromio(req->tag_buff,
        get_region_out(engine_idx) + req->tag_offset, req->tag_length);
    return 0;
}