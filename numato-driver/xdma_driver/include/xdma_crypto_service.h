#ifndef __HEADER_CRYPTO_SERVICE__
#define __HEADER_CRYPTO_SERVICE__

#include "xdma_mod.h"
#include "libxdma.h"
#include "libxdma_api.h"

struct xdma_pci_dev;

int xpdev_create_crypto_service(struct xdma_pci_dev *xpdev);


#endif

