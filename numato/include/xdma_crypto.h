#ifndef __xdma_crypto_h__
#define __xdma_crypto_h__

#include <linux/module.h>
#include "linux/atomic.h"
#include "linux/workqueue.h"
#include "linux/kthread.h"
#include "linux/delay.h"
#include "linux/list.h"
#include "linux/kernel.h"
#include "linux/mutex.h"
#include "linux/scatterlist.h"

#include "xdma_mod.h"
#include "libxdma_api.h"
#include "xdma_region.h"
#include "stdbool.h"

#define BUFF_LENGTH (REGION_NUM)
#define BACKLOG_MAX_LENGTH (50)
#define AGENT_NUM (1)
#define CORE_NUM (1)

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define COMM_REGION_0_OFFSET (0x11000)
#define IN_REGION_0_OFFSET   (0x00000)
#define OUT_REGION_0_OFFSET  (0x10000)
#define STATUS_REGION_0_OFFSET (0x20000)
#define IRQ_REIGON_0_OFFSET  (0x30000)

#define COMM_REGION_1_OFFSET (0x11000)
#define IN_REGION_1_OFFSET   (0x01000)
#define OUT_REGION_1_OFFSET  (0x11000)
#define STATUS_REGION_1_OFFSET (0x21000)
#define IRQ_REIGON_1_OFFSET  (0x31000)

#define COMM_REGION_2_OFFSET (0x11000)
#define IN_REGION_2_OFFSET   (0x02000)
#define OUT_REGION_2_OFFSET  (0x12000)
#define STATUS_REGION_2_OFFSET (0x22000)
#define IRQ_REIGON_2_OFFSET  (0x32000)

#define COMM_REGION_3_OFFSET (0x11000)
#define IN_REGION_3_OFFSET   (0x00000)
#define OUT_REGION_3_OFFSET  (0x10000)
#define STATUS_REGION_3_OFFSET (0x20000)
#define IRQ_REIGON_3_OFFSET  (0x40000)

#define H2C_TARGET (0)
#define STATUS_OFFSET (0x40)
#define CONTROL_OFFSET (0x04)

#define DEFAULT_CHANNEL_IDX (0)
#define DEFAULT_CORE (1)
#define MAX_REQ_NUM (512000)
#define CRYTO_DONE_IRQ (0)
#define XFER_WRITE (1)
#define XFER_READ (0)
#define DEFAULT_ENGINE (0)
#define TAG_MAX_SIZE (16)
struct xdma_pci_dev;

#include "common.h"

enum xfer_result_t {
    DONE,
    BUSY_XMIT,
    BUSY_RCV,
    INVALID_ADDSIZE
};

struct xfer_req{
// from user

    int (*crypto_complete)(struct xfer_req *data, int res);
    struct bkcrypto_context ctx;
    struct crypto_async_request *base;
    struct scatterlist *sg_in;
    struct scatterlist *sg_out;
    struct crypto_dsc_in crypto_dsc;
    int tag_offset;
    int tag_length;
    
// Dont touch
    u8 tag_buff[TAG_MAX_SIZE];
    enum xfer_result_t res;
    struct list_head list;
    int channel;
};

/** LED */

enum led_state {
    RED,
    BLUE,
    RED_BLUE,
};

struct blinky {
    struct timer_list timer;
    unsigned int interval_s;
    enum led_state led;
};

struct engine_data {
    int engine_idex;
    struct list_head req_queue;
    spinlock_t req_lock;
    struct list_head cb_queue;
    spinlock_t cb_lock;
    struct completion encrypt_done;
    struct task_struct *crypto_task;
    struct task_struct *callback_task;
};

struct xdma_crdev {
    struct xdma_pci_dev* xpdev;
    struct xdma_dev *xdev;
    struct blinky blinky;

    atomic_t req_num;
    spinlock_t agent_lock;

    struct engine_data engine[MAX_ENGINE];

    struct wait_queue_head crypto_wq;
    struct wait_queue_head cb_wq;

};



int crdev_create(struct xdma_pci_dev *xpdev);
void crdev_cleanup(void);

struct xfer_req *alloc_xfer_req(void);
int set_sg_in(struct xfer_req *req, struct scatterlist *sg);
int set_sg_out(struct xfer_req *req, struct scatterlist *sg);
int set_callback(struct xfer_req *req, int (*cb)(struct xfer_req *req, int res));
int set_ctx(struct xfer_req *req, struct bkcrypto_context ctx);
enum xfer_result_t get_result(struct xfer_req *req);

int set_tag(struct xfer_req *req, int length, int offset);
int get_tag(struct xfer_req *req, void *buf);

int xdma_xfer_submit_queue(struct xfer_req *xfer_req);
void free_xfer_req(struct xfer_req *req);
void debug_mem_in(int engine_idx);
void debug_mem_out(int engine_idx);
inline struct crypto_dsc_in *get_dsc_in(struct xfer_req *req);
void write_crypto_info(void *mem_in_base, struct xfer_req *req);
int get_tag_from_card(struct xfer_req *req, int engine_idx);

#endif