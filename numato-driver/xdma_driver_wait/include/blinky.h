#ifndef BLINKY
#define BLINKY

#include "xdma_crypto_service.h"

#define LED_BASE (0x00010000)
#define RED_LED_MASK (1 << 0)
#define BLUE_LED_MASK (1 << 1)
#define CONFIG_BAR_NUM (0)

extern struct test_data_struct test_data;

void init_blinky(struct test_data_struct *data);
void work_blinky_handler(struct work_struct *work);


#endif