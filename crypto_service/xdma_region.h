#ifndef XDMA_REGION
#define XDMA_REGION

#include <linux/version.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/workqueue.h>
#include "libxdma.h"
/*
#define H2C_CH0_BUFFER_SIZE_OFFSET (0x00)
#define H2C_CH1_BUFFER_SIZE_OFFSET (0x04)
#define C2H_CH0_BUFFER_SIZE_OFFSET (0x08)
#define C2H_CH1_BUFFER_SIZE_OFFSET (0x0C)
#define REMAINING_REQUEST_OFFSET   (0x20)
#define NEXT_DSC_REQUEST_ADDRESS_OFFSET   (0x24)
#define FINAL_DSC_REQUEST_ADDRESS_OFFSET  (0x28)
#define DSC_REQ_SIZ                       (0x10)
#define DSC_REQ_OFFSET                    (0x40)
#define DATA_REQ_SIZ                      (0x5F0)
#define DATA_REQ_OFF_SET                  (0xD0)
#define MAGIC_BTYE                        (0x3D)
#define MAX_REGION_ADDR                   (0x3100)
#define MAX_REGION_ADDR_TEST              (0x00003100)
#define DEBUG_REGION 1
#define XFER_C2H                          (1 << 0)
#define CRYPTO_RES_DONE                   (1 << 0)
#define CRYPTO_RES_AUTH_FAILED            (1 << 1)
#define REGION_STATE_EMPTY                (0xAAAAAAAA)
#define REGION_STATE_NEED_PROCESS         (0xBBBBBBBB)
#define REGION_STATE_PROCESSING           (0xCCCCCCCC)
#define REGION_STATE_PROCESSED            (0xDDDDDDDD)
#define REGION_STATE_UNKNOWN              (0xFFFFFFFF)
#define DATA_MAX_SIZE                     (1520/32)
#define MAX_REGION_INB                    (8) 
#define MAX_REGION_OUTB                   (8)
*/

#define INB_BASE                          (0x40)
#define OUTB_BASE                         (0x3200)
#define INB_BUFF_SIZE                     (08)
#define OUTB_BUFF_SIZE                    (08)
#define REGION_SIZE                       (1600)

struct common_base {
    u32 H2C_BUFFER_SIZE;
    u32 pad;
    u32 C2H_BUFFER_SIZE;
    u32 pad_;
    u32 padding[2];
    u32 NEXT_INB_REGION;
    u32 FINAL_INB_REGION;
    u32 padding_;
    u32 NEXT_OUTB_REGION;
    u32 FINAL_OUTB_REGION;
};

struct region {
    union {
        u32 region_descriptor;
        u32 xfer_id;
        u8 xfer_descriptor;
        u8 crypto_result;
        u8 data_length;
        u8 padding;
        u32 padding_;
    } region_description;
    union {
        u32 padding[7];
    } crypto_description;
    u32 data [DATA_MAX_SIZE];
};

struct global_mem {
    struct common_base cb;
    union {
        struct region region[MAX_REGION_INB];
    } ib;
    union {
        struct region region[MAX_REGION_OUTB];
    } ob;
};

struct dsc;

extern void set_base(void __iomem* base);

static inline u32 get_next_inb_region_addr(void);

static inline u32 get_next_outb_region_addr(void);

static inline u32 get_final_inb_region_addr(void);

static inline u32 get_final_outb_region_addr(void);

inline u32 get_h2c_buffer_size(void);

inline u32 get_c2h_buffer_size(void);

inline u32 get_inb_base(void);


int is_real_mem_available(void);
int is_buff_available(void);
int is_backlog_available(void);


#ifdef DEBUG_REGION
#define debug_mem debug_mem

void debug_mem(void);

#else
#define debug_mem(...)
#endif

#endif

// int process_next_req(void);
// void create_global_region_for_testing(void);



