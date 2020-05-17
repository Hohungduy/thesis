#include "blinky.h"

void blinky(struct timer_list *blinky_timer)
{
    struct test_data_struct *data;
    enum LED_STATE state;
    struct xdma_dev *hndl;
    void *config_space;
    u32 led_read;
    u32 red, blue;

    data = &test_data;
    state = data->led;
    hndl  = (struct xdma_dev *)data->dev_handler;
    config_space = (void *)hndl->bar[CONFIG_BAR_NUM];

    led_read = ioread32(config_space + LED_BASE);
    red = (led_read & RED_LED_MASK) == 0;
    blue= (led_read & BLUE_LED_MASK) == 0;
    // pr_info("led_read %x \n", led_read);
    // pr_info("config_space %p \n", config_space );


    if (red || blue){
        iowrite32(0x0F, config_space + LED_BASE);
        mod_timer(blinky_timer, jiffies + (unsigned int)(data->interval * HZ) );
        return;
    }

    switch (state)
    {
    case RED_ON_BLUE_OFF:
        iowrite32(!RED_LED_MASK, config_space + LED_BASE);
        break;
    case RED_ON_BLUE_ON:
        iowrite32(!(BLUE_LED_MASK || RED_LED_MASK),
                 config_space + LED_BASE);
        break;
    case RED_OFF_BLUE_ON:
        iowrite32(!BLUE_LED_MASK, config_space + LED_BASE);
        break;
    default:
        break;
    }
    // dbg_desc("Timer blinky function\n");
    mod_timer(&test_data.blinky_timer, jiffies + (unsigned int)(test_data.interval*HZ));
}

void init_blinky(struct test_data_struct *data)
{
    data->interval = 1;
    data->led = RED_ON_BLUE_OFF;
    timer_setup(&data->blinky_timer, blinky, 0);
    mod_timer(&data->blinky_timer, jiffies + data->interval*HZ);
}

void work_blinky_handler(struct work_struct *work)
{
    struct xdma_dev *xdev;
    struct test_data_struct *data;

    data = container_of(work, struct test_data_struct, work_blinky);
    xdev = (struct xdma_dev *)data->dev_handler;
    
    if (data->interval == 2){
        data->interval = 1;
        pr_info("interval = 1");
    }   
    else{
        data->interval = 2;
        pr_info("interval = 2");
    }

    debug_mem();
    xdma_user_isr_enable(xdev, 1 << 0);
}