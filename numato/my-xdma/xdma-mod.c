#include "xdma-mod.h"

MODULE_LICENSE("GPL");



static int xdma_init(void)
{





    return 0;
}

static void xdma_exit(void)
{

}

module_init(xdma_init);
module_exit(xdma_exit);