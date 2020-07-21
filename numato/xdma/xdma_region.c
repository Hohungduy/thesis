#include "xdma_region.h"

static struct base region_base;

void trigger_engine(int engine_idx)
{
    u32 trigger_val = ioread32(&region_base.engine.status->trigger);
    iowrite32(trigger_val ^ 0x00000001, &region_base.engine.status->trigger);
}

void *get_base(void)
{
    return &region_base;
}
EXPORT_SYMBOL_GPL(get_base);

int set_engine_base(struct crypto_engine base, int engine_idx)
{
#ifdef BUFFER
    if (engine_idx >= ENGINE_NUM)
        return -1;
    region_base.engine[engine_idx] = base;
    return 0;
#else 
    region_base.engine = base;
    return 0;
#endif
}

int clear_usr_irq(int irq_no)
{
#ifdef BUFFER

#else 
    u32 irq_state;
    if (irq_no)
        return -1;
    irq_state = ioread32(&region_base.engine.irq->deassert);
    pr_err("address = %p value = %x %x", &region_base.engine.irq->deassert, irq_state, irq_state ^ 1);
    iowrite32(irq_state ^ 1, &region_base.engine.irq->deassert);
    return 0;
#endif
}

int set_led_base(void *base)
{
    region_base.led.red = base + LED_RED_OFFSET;
    region_base.led.blue= base + LED_BLUE_OFFSET;
    return 0;
}

void toggle_red_led(void)
{
    iowrite32(ioread32(LED_RED_ADDR) ^ LED_RED_MASK, LED_RED_ADDR);
}
void toggle_blue_led(void)
{
    iowrite32(ioread32(LED_BLUE_ADDR) ^ LED_BLUE_MASK, LED_BLUE_ADDR);
}

/** BUFER ZONE */

void *get_next_region_ep_addr(int engine_idx)
{
#ifdef BUFFER
    struct crypto_engine base = region_base.engine[engine_idx];

    u32 head = ioread32(&base.comm->head_inb);

    return &base.in->region[head + 1];
#else 
    return SINGLE_PACKET_INBOUND_RAM_ADDR;
#endif
}
u32 get_tail_inb_idx(int engine_idx)
{
#ifdef BUFFER
    struct crypto_engine base = region_base.engine[engine_idx];
    return ioread32(&base.comm->tail_inb);
#else 
    return 0;
#endif
}

u32 get_head_outb_idx(int engine_idx)
{
#ifdef BUFFER
    struct crypto_engine base = region_base.engine[engine_idx];
    return ioread32(&base.comm->head_outb);
#else
    return 0;
#endif
}

u32 get_xfer_id_outb_idx(int engine_idx, int region_num)
{
#ifdef BUFFER
    struct crypto_engine base = region_base.engine[engine_idx];
    struct region_out *region = &base.out->region[region_num];
    return ioread32(&region->xfer_id);
#else  
    return ioread32(&region_base.engine.in->region.xfer_id);
#endif
}


void write_inb_xfer_id(int engine_idx, int region_idx, u32 xfer_id)
{
#ifdef BUFFER
    struct crypto_engine base = region_base.engine[engine_idx];
    iowrite32(xfer_id, &base.in->region[region_idx].xfer_id);
#else  
    pr_err("%d \n %s\n", __LINE__, __FILE__);
    iowrite32(xfer_id, &region_base.engine.in->region.xfer_id);
#endif
}
void *get_region_ep_addr_out(int engine_idx)
{
#ifdef BUFFER

    struct crypto_engine base = region_base.engine[engine_idx];

    u32 tail = ioread32(&base.comm->tail_outb);

    return &base.out->region[tail];
#else 
    return (void *)(0x00010000);
#endif

}
void *get_region_ep_addr(int engine_idx, int region_idx)
{
#ifdef BUFFER
    struct crypto_engine base = region_base.engine[engine_idx];

    return &base.in->region[region_idx];
#else 
    return SINGLE_PACKET_INBOUND_RAM_ADDR;
#endif
}
u64 get_next_data_ep_addr(int engine_idx)
{

#ifdef BUFFER
    struct crypto_engine base = region_base.engine[engine_idx];

    u32 head = ioread32(&base.comm->head_inb);

    return offsetof(struct crypto_engine, in) + 
        offsetof(struct inbound, region) + 
        sizeof(struct region_in) * head +
        offsetof(struct region_in, data) ;    // return (void *)(&base->in.region[head + 1].data) - (void *)base;
#else
    return SINGLE_PACKET_INBOUND_RAM_ADDR + 
            offsetof(struct region_in, data);
#endif
}
u64 get_data_ep_addr_out(int engine_idx)
{
#ifdef BUFFER
    struct crypto_engine base = region_base.engine[engine_idx];

    u32 tail = ioread32(&base.comm->tail_outb);

    return offsetof(struct crypto_engine, out) + 
        offsetof(struct outbound, region) + 
        sizeof(struct region_out) * tail +
        offsetof(struct region_out, data) ;    // return (void *)(&base->in.region[head + 1].data) - (void *)base;
#else 
    return SINGLE_PACKET_OUTBOUND_RAM_ADDR 
            + offsetof(struct region_out, data);
#endif
}
u64 get_region_data_ep_addr(int engine_idx, int region_idx)
{
#ifdef BUFFER
    return offsetof(struct crypto_engine, in) + 
        offsetof(struct outbound, region) + 
        sizeof(struct region_in) * region_idx +
        offsetof(struct region_in, data) ;    // return (void *)(&base->in.region[head + 1].data) - (void *)base;
#else   
    return SINGLE_PACKET_INBOUND_RAM_ADDR 
            + offsetof(struct region_in, data[0]);
#endif
}

int is_engine_full(int engine_idx)
{
#ifdef BUFFER
    u32 head = ioread32(&region_base.engine[engine_idx].comm->head_inb);
    u32 tail = ioread32(&region_base.engine[engine_idx].comm->tail_inb);
    
    // pr_err("head = %x", head);
    // pr_err("tail = %x", tail);

    if (head < 0 || tail < 0)
    {
        return -1;
    }
        
    if (((head + 1) % REGION_NUM) == (tail % REGION_NUM))
        return 1;
    else
        return 0;

#else 
    return ioread32(&region_base.engine.status->busy) 
                    == CRYPTO_STATUS_BUSY;
#endif

}

int is_engine_full_out(int engine_idx)
{
#ifdef BUFFER
    u32 head = ioread32(&region_base.engine[engine_idx].comm->head_outb);
    u32 tail = ioread32(&region_base.engine[engine_idx].comm->tail_outb);
    
    // pr_err("head = %x", head);
    // pr_err("tail = %x", tail);

    if (head < 0 || tail < 0)
    {
        return -1;
    }
        
    if (((head + 1) % REGION_NUM) == (tail % REGION_NUM))
        return 1;
    else
        return 0;
#else 
    return ioread32(&region_base.engine.status->busy) 
                    == CRYPTO_STATUS_BUSY;
#endif
}

int is_engine_empty_out(int engine_idx)
{
#ifdef BUFFER
    u32 head = ioread32(&region_base.engine[engine_idx].comm->head_outb);
    u32 tail = ioread32(&region_base.engine[engine_idx].comm->tail_outb);
    if (head < 0 || tail < 0)
        return -1;
    
    if (((head) % REGION_NUM) == (tail % REGION_NUM))
        return 1;
    else
        return 0;
#else 

    return ioread32(&region_base.engine.status->busy) 
                    != CRYPTO_STATUS_BUSY;
#endif
}

void *get_region_addr(int engine_idx)
{
    // pr_err("%d \n %s\n", __LINE__, __FILE__);
    return &region_base.engine.in->region;
}
// void active_inb_region(int engine_idx, int region_idx)
// {
//     struct crypto_engine base = region_base.engine[engine_idx];

//     struct region_in *region = &base.in->region[region_idx];
//     iowrite32(0xABCDABCD, &region->region_dsc);
// }
// void active_outb_region(int engine_idx, int region_idx)
// {
//     struct crypto_engine base = region_base.engine[engine_idx];

//     struct region_out *region = &base.out->region[region_idx];
//     iowrite32(0xABCDABCD, &region->region_dsc);
// }

// int increase_head_inb_idx(int engine_idx, int booking)
// {
//     u32 head_idx, tail_idx;
//     u32 region_dsc;
//     int i;
//     void *head_addr = &region_base.engine[engine_idx].comm->head_inb;
//     void *tail_addr = &region_base.engine[engine_idx].comm->tail_inb;
//     head_idx = ioread32(head_addr);
//     tail_idx = ioread32(tail_addr);
//     pr_err("increase_head_inb_idx head = %d\n", head_idx);
    
//     for (i = head_idx; i != (booking % REGION_NUM); i = (i + 1) % REGION_NUM)
//     {
//         region_dsc = ioread32(&region_base.engine[engine_idx].in->region[i].region_dsc);
//         pr_err("region_dsc %x", region_dsc);
//         if (region_dsc == 0xABCDABCD)
//         {
//             active_inb_region(engine_idx, i);
//             pr_err("write head_dx = %d\n", (i+1)%REGION_NUM);
//             iowrite32((i + 1) % REGION_NUM, head_addr);
//         }
//         else
//         {
//             return 0;
//         }
//     }

//     return 0;
// }

// int increase_tail_outb_idx(int engine_idx, int booking)
// {
//     u32 head_idx, tail_idx;
//     u32 region_dsc;
//     int i;
//     void *head_addr = &region_base.engine[engine_idx].comm->head_outb;
//     void *tail_addr = &region_base.engine[engine_idx].comm->tail_outb;
//     head_idx = ioread32(head_addr);
//     tail_idx = ioread32(tail_addr);
//     pr_err("increase_head_inb_idx head = %d\n", head_idx);
    
//     for (i = tail_idx; i != (booking % REGION_NUM); i = (i + 1) % REGION_NUM)
//     {
//         region_dsc = ioread32(&region_base.engine[engine_idx].out->region[i].region_dsc);
//         pr_err( "region_dsc %x", region_dsc );
//         if (region_dsc == 0xABCDABCD)
//         {
//             // active_outb_region(engine_idx, i);
//             pr_err( "write head_dx = %d\n", (i + 1) % REGION_NUM ) ;
//             iowrite32( (i + 1) % REGION_NUM, tail_addr );
//         }
//         else
//         {
//             return 0;
//         }
//     }

//     return 0;
// }

// int increase_tail_idx_out(int engine_idx)
// {
//     u32 tail_idx;
//     void *tail_addr = &region_base.engine[engine_idx].comm->tail_outb;
//     tail_idx = ioread32(tail_addr);
//     iowrite32(tail_idx + 1, tail_addr);

//     return 0;
// }

// int active_next_region(int engine_idx)
// {
//     struct region_in *region = get_next_region_ep_addr(engine_idx);
//     iowrite32(0xABCDABCD, &region->region_dsc);
//     return 0;
// }
// int active_next_region_out(int engine_idx)
// {
//     struct region_out *region = get_region_ep_addr_out(engine_idx);
//     iowrite32(0xABCDABCD, &region->region_dsc);
//     return 0;
// }



void test_mem(void)
{
#ifdef BUFFER
    int i;
    pr_err("head_inb   = %d", ioread32(&region_base.engine[0].comm->head_inb));
    pr_err("tail_inb   = %d", ioread32(&region_base.engine[0].comm->tail_inb));
    
    for (i =0; i < REGION_NUM; i++){
        pr_err("region_dsc = %d", ioread32(&region_base.engine[0].in->region[i].region_dsc));
        pr_err("region_dsc = %d", ioread32(&region_base.engine[0].in->region[i].data_len));
    }
#else  

#endif
}
EXPORT_SYMBOL_GPL(test_mem);