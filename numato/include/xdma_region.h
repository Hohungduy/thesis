#ifndef _xdma_region_
#define _xdma_region_

#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
// #include <stdio.h>
// #include <linux/iomap.h>
// #include <asm-generic/iomap.h>
#include <linux/pci.h>

#define REGION_NUM (1)
#define DATA_MAX_LEN (1624)
#define ENGINE_NUM (1)
#define ENGINE_OFFSET(x) (0 + x*1)
#define LED_OFFSET (0x30000)

static struct base region_base;

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

struct crypto_dsc {
    union {
        u32 data[90];

    } crypto_dsc;
};

struct region_in {
    u32 region_dsc;
    u32 xfer_id;
    u8 xfer_dsc;
    u8 crypto_result;
    u16 data_len;
    u32 data_len__;
    struct crypto_dsc crypto_dsc;
    u8 data[DATA_MAX_LEN];
};
struct region_out {
    u32 region_dsc;
    u32 xfer_id;
    u8 xfer_dsc;
    u8 crypto_result;
    u16 data_len;
    u32 data_len__;
    struct crypto_dsc crypto_dsc;
    u8 data[DATA_MAX_LEN];
};

struct inbound {
    struct region_in region[REGION_NUM];
};

struct outbound {
    struct region_out region[REGION_NUM];
};

struct crypto_engine {
    struct common_base *comm;
    struct inbound *in;
    struct outbound *out;
};

struct led_region {
    u32 *red;
    u32 *blue;
};

struct base {
    struct crypto_engine engine[ENGINE_NUM];
    struct led_region led;
};


int set_engine_base(struct crypto_engine base, int engine_idx);
int set_led_base(void *base);
void toggle_red_led(void);
void toggle_blue_led(void);
int is_engine_full(int engine_idx);
int is_engine_empty(int engine_idx);
void *get_next_region_ep_addr(int engine_idx);
u64 get_next_data_ep_addr(int engine_idx);
void active_outb_region(int engine_idx, int region_idx);

int increase_head_inb_idx(int engine_idx, int booking);
int increase_tail_outb_idx(int engine_idx, int booking);

int active_next_region(int engine_idx);
void *get_region_ep_addr_out(int engine_idx);
u64 get_data_ep_addr_out(int engine_idx);
int increase_tail_idx_out(int engine_idx);
int is_engine_empty_out(int engine_idx);
void *get_region_ep_addr(int engine_idx, int region_idx);
u32 get_tail_inb_idx(int engine_idx);
u32 get_head_outb_idx(int engine_idx);
u32 get_xfer_id_outb_idx(int engine_idx, int region_num);
void write_inb_xfer_id(int engine_idx, int region_idx, u32 xfer_id);
void active_inb_region(int engine_idx, int region_idx);
u64 get_region_data_ep_addr(int engine_idx, int region_idx);


#endif