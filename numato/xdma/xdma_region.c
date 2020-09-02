#include "xdma_region.h"

struct mem_base mem_base;

void trigger_engine(int engine_idx)
{
    u32 trigger_val = ioread32(&mem_base.engine.status->trigger);

    iowrite32(trigger_val ^ 0x00000001, &mem_base.engine.status->trigger);
}

void *get_region_in(void)
{
    return mem_base.engine.in;
}
void *get_region_out(void)
{
    return mem_base.engine.out;
}

int set_mem_base(struct mem_base base)
{
    mem_base = base;
    return 0;
}

int clear_usr_irq(int irq_no)
{

    u32 irq_state;
    if (irq_no)
        return -1;
    irq_state = ioread32(&mem_base.engine.irq->deassert);
    iowrite32(irq_state ^ 1, &mem_base.engine.irq->deassert);
    return 0;
}

void toggle_red_led(void)
{
    iowrite32(ioread32(mem_base.red_led) ^ LED_RED_MASK, mem_base.red_led);
}
void toggle_blue_led(void)
{
    iowrite32(ioread32(mem_base.blue_led) ^ LED_BLUE_MASK, mem_base.blue_led);
}

