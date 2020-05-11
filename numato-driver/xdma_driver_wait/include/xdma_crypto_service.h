#ifndef __HEADER_CRYPTO_SERVICE__
#define __HEADER_CRYPTO_SERVICE__

#include "xdma_mod.h"
#include "libxdma.h"
#include "libxdma_api.h"
#include <linux/mm.h>
#include <asm/cacheflush.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>

#include "xdma_region.h"
#include "blinky.h"

#define MAX_BACKLOG 100
#define MAX_ID (MAX_BACKLOG + 10)

struct xdma_pci_dev;
struct xfer_callback_struct {
    int xfer_id;
};
struct xcrypto_dev {
    spinlock_t lock;
    struct xdma_dev *xdev;
    int load[2];
    int backlog[2];
    void *id_buff[MAX_ID];
    u32 id_buf_idx;
};

struct xfer_req
{
    int (*complete)(void *data, int res);
    void *data;
    struct scatterlist *sg;
};


enum LED_STATE {
    // RED_BLUE
    RED_OFF_BLUE_OFF,
    RED_OFF_BLUE_ON,
    RED_ON_BLUE_OFF,
    RED_ON_BLUE_ON
};

int xpdev_create_crypto_service(struct xdma_pci_dev *xpdev);

struct test_data_struct {
    struct work_struct work;
    struct work_struct work_blinky;
    struct timer_list blinky_timer;
    enum LED_STATE led;
    u8 interval;
    void *dev_handler;
};

extern struct test_data_struct test_data;

enum USER_IRQ_TYPE {
    IRQ_0_TEST,
    IRQ_1_,
    IRQ_2_,
    IRQ_3_,
    IRQ_4_,
    IRQ_5_,
    IRQ_6_,
    IRQ_7_,
    IRQ_8_,
    IRQ_9_,
    IRQ_10_,
    IRQ_11_,
    IRQ_12_,
    IRQ_13_,
    IRQ_14_,
    IRQ_15_
};

int update_load(int channel, int num_load);
int choose_channel(void);
int is_full_backlog(int channel);
int submit_xfer(struct xfer_req * xfer_req);
struct xfer_callback_struct *alloc_xfer_callback(void);

#define MAX_CHANNEL (2)
#define MAX_LOAD (100)
#endif

