#ifndef __HEADER_XDEBUG__
#define __HEADER_XDEBUG__

#include <linux/printk.h>

#define x_info(s) pr_info("%s    %s %d ", s, __FILE__, __LINE__)
#define x_info_failed(s) pr_info("%s    %s %s %d ", "FAILED ", s, __FILE__, __LINE__)

#endif
