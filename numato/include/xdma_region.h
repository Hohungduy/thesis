#ifndef _xdma_region_
#define _xdma_region_

#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
// #include <stdio.h>
// #include <linux/iomap.h>
// #include <asm-generic/iomap.h>
#include <linux/pci.h>
// #include "xdma_crypto.h"

#define REGION_NUM (1)
#define DATA_IN_MAX_LEN (1888/4)
#define DATA_OUT_MAX_LEN (1680/4)

#define ENGINE_NUM (1)
#define ENGINE_OFFSET(x) (0 + x*1)
#define LED_RED_OFFSET (0x30000)
#define LED_BLUE_OFFSET (0x30008)
#define CRYPTO_STATUS_ADDR (0x20000)
#define CRYPTO_STATUS_BUSY (0x00000001)

#define SINGLE_PACKET_INBOUND_RAM_ADDR (0x00000000)
#define SINGLE_PACKET_OUTBOUND_RAM_ADDR (0x00010000)

#define LED_BLUE_ADDR (region_base.led.blue)
#define LED_RED_ADDR (region_base.led.red)
#define LED_BLUE_MASK (0x1)
#define LED_RED_MASK  (0x1)


struct common_base {
    u32 h2c_buff_size;
    u32 h2c_buff_size_;

    u32 c2h_buff_size;
    u32 c2h_buff_size_;

    u32 padding[3];

    u32 head_inb;
    u32 tail_inb;

    u32 padding_;

    u32 head_outb;
    u32 tail_outb;

    u32 padding_last[6];
};

struct info {
    u32 keysize     : 4;
    u32 aadsize     : 4;
    u32 length      : 11;
    u32 direction   : 1;
    u32 free_space_ : 12;
    u32 free_space[3];
};

struct iv {
    u32 tail;
    u32 iv[2];
    u32 nonce;
};

struct crypto_dsc_in {
    struct info info;
    u32 icv[4];
    u32 key[8];
    struct iv iv;
    u32 aad[4];
};

struct inbound {
    u32 region_dsc;
    u32 xfer_id;
    u8 xfer_dsc;
    u8 crypto_result;
    u16 data_len;
    u32 data_len__;
    struct crypto_dsc_in crypto_dsc;
    u32 data[DATA_IN_MAX_LEN];
};

struct outbound {
    u32 region_dsc;
    u32 xfer_id;
    u8 xfer_dsc;
    u8 crypto_result;
    u16 data_len;
    u32 data_len__;
    u32 data[DATA_OUT_MAX_LEN];
};

struct crypto_status {
    u32 busy;
    u32 freespace;
    u32 trigger;
};

struct interrupt_gpio {
    u32 interrupt;
    u32 free;
    u32 deassert;
};

struct crypto_engine {
    struct common_base *comm;
    struct inbound *in;
    struct outbound *out;
    struct crypto_status *status;
    struct interrupt_gpio *irq;
};

struct mem_base {
    struct crypto_engine engine;
    void *red_led;
    void *blue_led;
};

void trigger_engine(int engine_idx);
int clear_usr_irq(int irq_no);
void *get_region_in(void);
void *get_region_out(void);
int set_mem_base(struct mem_base base);
int set_led_base(void *base);
void toggle_red_led(void);
void toggle_blue_led(void);



#endif