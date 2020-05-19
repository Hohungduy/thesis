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

int is_engine_full(int channel)
{

    u32 head = ioread32(&region_base.engine[channel]->comm.head_inb);
    u32 tail = ioread32(&region_base.engine[channel]->comm.tail_inb);
    
    
    if (head < 0 || tail < 0)
        return -1;
    if (((head + 1) % REGION_NUM) == (tail % REGION_NUM))
        return 1;
    else
        return 0;
}

int is_engine_empty(int channel)
{

    u32 head = ioread32(&region_base.engine[channel]->comm.head_inb);
    u32 tail = ioread32(&region_base.engine[channel]->comm.tail_inb);
    if (head < 0 || tail < 0)
        return -1;
    
    if (((head) % REGION_NUM) == (tail % REGION_NUM))
        return 1;
    else
        return 0;
}