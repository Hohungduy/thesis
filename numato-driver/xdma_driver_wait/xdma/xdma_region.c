#include "xdma_region.h"

static struct base region_base;

int set_engine_base(void *base, int engine_idx)
{
    if (engine_idx >= ENGINE_NUM)
        return -1;
    region_base.engine[engine_idx] = (struct crypto_engine *)base;
    return 0;
}

int set_led_base(void *base)
{
    region_base.led = (struct led_region *)base;
    return 0;
}

void toggle_red_led(void)
{
    u32 led_register = ioread32(region_base.led);
    u32 red = led_register ^ 0x00000001;
    iowrite32(red, region_base.led);
}
void toggle_blue_led(void)
{
    u32 led_register = ioread32(region_base.led);
    u32 blue = led_register ^ 0x00000002;
    iowrite32(blue, region_base.led);
}

/** BUFER ZONE */

void *get_next_region_ep_addr(int engine_idx)
{
    struct crypto_engine *base = region_base.engine[engine_idx];

    u32 head = ioread32(&base->comm.head_inb);

    return &base->in.region[head + 1];
}
void *get_next_data_ep_addr(int engine_idx)
{
    struct crypto_engine *base = region_base.engine[engine_idx];

    u32 head = ioread32(&base->comm.head_inb);

    return &base->in.region[head + 1].data;
}

int is_engine_full(int engine_idx)
{

    u32 head = ioread32(&region_base.engine[engine_idx]->comm.head_inb);
    u32 tail = ioread32(&region_base.engine[engine_idx]->comm.tail_inb);
    
    
    if (head < 0 || tail < 0)
        return -1;
    if (((head + 1) % REGION_NUM) == (tail % REGION_NUM))
        return 1;
    else
        return 0;
}

int is_engine_empty(int engine_idx)
{

    u32 head = ioread32(&region_base.engine[engine_idx]->comm.head_inb);
    u32 tail = ioread32(&region_base.engine[engine_idx]->comm.tail_inb);
    if (head < 0 || tail < 0)
        return -1;
    
    if (((head) % REGION_NUM) == (tail % REGION_NUM))
        return 1;
    else
        return 0;
}

int increase_head_idx(int engine_idx)
{
    u32 head_idx;
    void *head_addr = &region_base.engine[engine_idx]->comm.head_inb;
    head_idx = ioread32(head_addr);
    iowrite32(head_idx + 1, head_addr);

    return 0;
}

int active_next_region(int engine_idx)
{
    struct region *region = get_next_region_ep_addr(engine_idx);
    iowrite32(0xABCDABCD, &region->region_dsc);
    return 0;
}

void test_mem(void)
{

    int i;
    printk("head_inb   = %d", ioread32(&region_base.engine[0]->comm.head_inb));
    printk("tail_inb   = %d", ioread32(&region_base.engine[0]->comm.tail_inb));
    
    for (i =0; i < REGION_NUM; i++){
        printk("region_dsc = %d", ioread32(&region_base.engine[0]->in.region[i].region_dsc));
        printk("region_dsc = %d", ioread32(&region_base.engine[0]->in.region[i].data_len));
    }
    

}