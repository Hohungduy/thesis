#ifndef __xdma_crypto_h__
#define __xdma_crypto_h__

#include "xdma_mod.h"
#include <linux/module.h>
#include "xdma_region.h"
#include "libxdma_api.h"

#define BUFF_LENGTH (REGION_NUM * 2)
#define BACKLOG_MAX_LENGTH (50)

struct xdma_pci_dev;

/** BUFFER */
// #include "common.h"

struct mycrypto_context {

};

struct xfer_req{
    int (*complete)(void *data, int res);
    void *data;
    struct scatterlist *sg;
    struct mycrypto_context ctx; 
    struct list_head list;
    spinlock_t lock;
};

/** LED */

enum led_state {
    RED,
    BLUE,
    RED_BLUE,
};

struct blinky {
    struct timer_list timer;
    unsigned int interval;
    enum led_state led;
};

struct xdma_crdev {
    struct xdma_pci_dev* xpdev;
    struct xdma_dev *xdev;
    spinlock_t lock;
    struct blinky blinky;
    struct xfer_req *req_buff[BUFF_LENGTH];
    int head;
    int tail;

    struct list_head req_backlog;
};


 

int crdev_create(struct xdma_pci_dev *xpdev);
void crdev_cleanup(void);

#endif