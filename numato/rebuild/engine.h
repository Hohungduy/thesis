#ifndef __HEADER_ENGINE__
#define __HEADER_ENGINE__

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>

#include "xdebug.h"

enum engine_type {
    ENGINE_TYPE_UNKNOWN,
    ENGINE_TYPE_AXILITE,
    ENGINE_TYPE_BYPASS,
    ENGINE_TYPE_DMA
};

struct engine_ops {

};

struct engine_sgdma_regs {
	u32 identifier;
	u32 reserved_1[31];	/* padding */

	/* bus address to first descriptor in Root Complex Memory */
	u32 first_desc_lo;
	u32 first_desc_hi;
	/* number of adjacent descriptors at first_desc */
	u32 first_desc_adjacent;
	u32 credits;
} __packed;

struct engine_regs {
	u32 identifier;
	u32 control;
	u32 control_w1s;
	u32 control_w1c;
	u32 reserved_1[12];	/* padding */

	u32 status;
	u32 status_rc;
	u32 completed_desc_count;
	u32 alignments;
	u32 reserved_2[14];	/* padding */

	u32 poll_mode_wb_lo;
	u32 poll_mode_wb_hi;
	u32 interrupt_enable_mask;
	u32 interrupt_enable_mask_w1s;
	u32 interrupt_enable_mask_w1c;
	u32 reserved_3[9];	/* padding */

	u32 perf_ctrl;
	u32 perf_cyc_lo;
	u32 perf_cyc_hi;
	u32 perf_dat_lo;
	u32 perf_dat_hi;
	u32 perf_pnd_lo;
	u32 perf_pnd_hi;
} __packed;

enum shutdown_state {
	ENGINE_SHUTDOWN_NONE = 0,	/* No shutdown in progress */
	ENGINE_SHUTDOWN_REQUEST = 1,	/* engine requested to shutdown */
	ENGINE_SHUTDOWN_IDLE = 2	/* engine has shutdown and is idle */
};

#define MAX_ENGINE_NAME_LENGTH (5)

struct engine_struct {
    struct xpcie *xpcie_dev; /** Parent device */ 
    char name[MAX_ENGINE_NAME_LENGTH]; /** name of this engine */
    
    /** HW register addess offsets */
    struct engine_regs *regs; /** Control reg BAR offset */
    struct engine_sgdma_regs *sgdma_regs; /** SGDMA reg BAR offset */
    u32 bypass_offset; /** Bypass mode BAR offset */

    /** Engine state, configuration & flags */
    enum shutdown_state shutdown; /** engine shutdown mode */
    enum dma_data_direction dir;
    int device_open; 

    spinlock_t lock;
};



int engine_setup_routine(struct engine_struct *engine);

#endif