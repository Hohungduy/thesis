#ifndef __HEADER_XDEBUG__
#define __HEADER_XDEBUG__

#include <linux/printk.h>

#define x_info(s) pr_info("%s    %s %d ", s, __FILE__, __LINE__)
#define x_info_failed(s) pr_info("%s    %s %s %d ", "FAILED ", s, __FILE__, __LINE__)
#ifdef __LIBXDMA_DEBUG__
#define dbg_io		pr_err
#define dbg_fops	pr_err
#define dbg_perf	pr_err
#define dbg_sg		pr_err
#define dbg_tfr		pr_err
#define dbg_irq		pr_err
#define dbg_init	pr_err
#define dbg_desc	pr_err
#else
/* disable debugging */
#define dbg_io(...)
#define dbg_fops(...)
#define dbg_perf(...)
#define dbg_sg(...)
#define dbg_tfr(...)
#define dbg_irq(...)
#define dbg_init(...)
#define dbg_desc(...)
#endif


#endif
