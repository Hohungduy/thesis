#ifndef __HEADER_XDEBUG__
#define __HEADER_XDEBUG__

#include <linux/printk.h>

#define x_info(s) pr_info("%s %s %s", __FILE__, __LINE__, s)
#define x_info_failed(s) pr_info("%s %s %s %s", "FAILED ", __FILE__, __LINE__, s)

#endif