/*******************************************************************************
 *
 *  NetFPGA-10G http://www.netfpga.org
 *
 *  File:
 *        nf10driver.c
 *
 *  Project:
 *        nic
 *
 *  Author:
 *        Mario Flajslik
 *
 *  Description:
 *        Top level file for the nic driver. Contains functions that are called
 *        when module is loaded/unloaded to initialize/remove PCIe device and
 *        nic datastructures.
 *
 *  Copyright notice:
 *        Copyright (C) 2010, 2011 The Board of Trustees of The Leland Stanford
 *                                 Junior University
 *
 *  Licence:
 *        This file is part of the NetFPGA 10G development base package.
 *
 *        This file is free code: you can redistribute it and/or modify it under
 *        the terms of the GNU Lesser General Public License version 2.1 as
 *        published by the Free Software Foundation.
 *
 *        This package is distributed in the hope that it will be useful, but
 *        WITHOUT ANY WARRANTY; without even the implied warranty of
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *        Lesser General Public License for more details.
 *
 *        You should have received a copy of the GNU Lesser General Public
 *        License along with the NetFPGA source package.  If not, see
 *        http://www.gnu.org/licenses/.
 *
 */





#include "nu.h"

// These attributes have been removed since Kernel 3.8.x. Keep them here for backward
// compatibility.
// See https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=54b956b903607
#ifndef __devinit
  #define __devinit
#endif
#ifndef __devexit
  #define __devexit
#endif
#ifndef __devexit_p
  #define __devexit_p
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Le Thanh Long");
MODULE_DESCRIPTION("numato driver");


#define PCI_VENDOR_ID_NUMATO 0x10ee
#define PCI_DEVICE_ID_NUMATO 0x7024
#define BAR0_SIZE 0x10000ULL
#define BAR0 0
#define BAR1 1
#define PCIE_BITS 32
#define RAM_ZISE 10
#define DEVICE_NAME "NUMATO"
#define NVEC 1



static struct pci_device_id pci_id[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_NUMATO, PCI_DEVICE_ID_NUMATO)},
    {0}
};
MODULE_DEVICE_TABLE(pci, pci_id);

void aller_numato_test(struct nf10_card *card){
    volatile void *tx_dsc = card->tx_dsc;
    struct mydata {
        uint64_t a;
        uint64_t b;
    };

    struct mydata read[RAM_ZISE], write[RAM_ZISE];
    uint16_t i;
    // read.a = 0x00;
    // read.b = 0x00;
    // write.a = 0xabcdef;
    // write.b = 0x123456;
    /*
        Test TX region
    */

    /*
    (((uint64_t)tx_dsc)) = 0x12345672;

    if ( ((((uint64_t)tx_dsc))) == 0x12345672 ){
        printk("NUMATO TEST OKAY %llx\n", ((((uint64_t)tx_dsc) )));
    }
    else{
        printk("NUMATO TEST FAILED\n");
    }
    */

    
    
    for (i =0 ; i < RAM_ZISE; i ++){
        read[i].a = 0;
        read[i].b = 0;
        write[i].a = i;
        write[i].b = i + 1;
    }

    for (i =0 ; i < RAM_ZISE; i ++){
        
        printk(KERN_ERR "NUMATO: TEST FUNCTION WRITE size = %ld: Test byte = %llx %llx \n", 
            sizeof(write[i]), write[i].a, write[i].b);
        memcpy_toio( ((uint64_t *)tx_dsc + 2*i), &write[i], sizeof(struct mydata));
    }
    msleep(1);

    for (i =0 ; i < RAM_ZISE; i ++){
        memcpy_fromio( &read[i] , (uint64_t *)tx_dsc + 2*i, sizeof(struct mydata) ); 
        printk(KERN_ERR "NUMATO: TEST FUNCTION READ: Test byte = %llx %llx \n", 
            read[i].a, read[i].b);
    }

    // for (i =0 ; i < RAM_ZISE; i ++){
        
    //     printk(KERN_ERR "NUMATO: TEST FUNCTION WRITE size = %ld: Test byte = %llx , %llx \n", 
    //         sizeof(write[i]), write[i].a, write[i].b);
    //     memcpy_toio( (struct mydata *)tx_dsc + i, &write[i], sizeof(struct mydata));
    // }
    // msleep(1);

    // for (i =0 ; i < RAM_ZISE; i ++){
    //     memcpy_fromio( &read[i] , (struct mydata *)tx_dsc + i, sizeof(struct mydata) ); 
    //     printk(KERN_ERR "NUMATO: TEST FUNCTION READ: Test byte = %llx , %llx \n", 
    //         read[i].a, read[i].b);
    // }
        

}

irqreturn_t handler(int irq_no, void *card){

    return IRQ_HANDLED;
}

static int __devinit nu_probe(struct pci_dev *pdev, const struct pci_device_id *id){
	int err;
    int irq_no;

    int ret = -ENODEV;
    struct nf10_card *card;

    printk(KERN_ERR "NUMATO: Probe function started! \n");
	// Step 1: Enable device
	printk(KERN_ERR "NUMATO: Try enabling PCI device!\n");
    if((err = pci_enable_device(pdev))) {
		printk(KERN_ERR "NUMATO: Unable to enable the PCI device!\n");
        ret = -ENODEV;
		goto err_out_none;
	}
    printk(KERN_ERR "NUMATO: Enable PCI device done!\n");

    // enable BusMaster (enables generation of pcie requests)
	pci_set_master(pdev);

    //Step 2: Request MMIO.IO resources
    // if (!request_mem_region(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0), DEVICE_NAME)) {
    //     printk(KERN_ERR "NUMATO: Reserving memory region failed\n");
    //     ret = -ENOMEM;
    //     goto err_out_clear_master;
    //     //goto err_out_msi;
    // }

    //Step 3: Set the DMA mask size *** Need CHeck

    // set DMA addressing masks (full 64bit)
    if(dma_set_mask(&pdev->dev, DMA_BIT_MASK(PCIE_BITS)) < 0){
        printk(KERN_ERR "NUMATO: dma_set_mask fail!\n");
        ret = -EFAULT;
        goto err_out_clear_master;

        if(dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(PCIE_BITS)) < 0){
            printk(KERN_ERR "NUMATO: dma_set_mask fail!\n");
            ret = -EFAULT;
            goto err_out_clear_master;
        }
    }

    // Step 4: Setup shared control data
	
    printk(KERN_ERR "NUMATO: Setup card struct \n");

	card = (struct nf10_card*)kmalloc(sizeof(struct nf10_card), GFP_KERNEL);
	if (card == NULL) {
		printk(KERN_ERR "NUMATO: Private card memory alloc failed\n");
		ret = -ENOMEM;
		goto err_out_iounmap;
	}
    memset(card, 0, sizeof(struct nf10_card));
    pci_set_drvdata(pdev, card);
	
    card->pdev = pdev;

	printk(KERN_ERR "NUMATO: Try mapping mem memory\n");

    //card->tx_dsc = ioremap_nocache(pci_resource_start(pdev, 0) , pci_resource_len(pdev, 0));
	card->tx_dsc = pci_iomap(pdev, BAR0, (long unsigned)pci_resource_len(pdev, BAR0) );
    if (!card->tx_dsc )
	{
		printk(KERN_ERR "NUMATO: cannot map region tx len:%lx start:%lx , end:%lx\n",
			(long unsigned)pci_resource_len(pdev, BAR0),
			(long unsigned)pci_resource_start(pdev, BAR0),
            (long unsigned)pci_resource_end(pdev, BAR0));
		goto err_out_iounmap;
	}
	printk(KERN_ERR "NUMATO: mapping memory tx done %p \n",card->tx_dsc );
    printk(KERN_ERR "NUMATO: Mem region len tx :%lx start:%lx , end: %lx mapped\n",
                (long unsigned)pci_resource_len(pdev, BAR0),
                (long unsigned)pci_resource_start(pdev, BAR0),
                (long unsigned)pci_resource_end(pdev, BAR0));

    printk(KERN_ERR "NUMATO: BAR  1 length %ld \n",(long unsigned)pci_resource_len(pdev, 1) );
    printk(KERN_ERR "NUMATO: BAR  2 length %ld \n",(long unsigned)pci_resource_len(pdev, 2) );
    printk(KERN_ERR "NUMATO: BAR  3 length %ld \n",(long unsigned)pci_resource_len(pdev, 3) );
    printk(KERN_ERR "NUMATO: BAR  4 length %ld \n",(long unsigned)pci_resource_len(pdev, 4) );

    // card->rx_dsc = pci_iomap(pdev, BAR1, (long unsigned)pci_resource_len(pdev, BAR1) );
    // if (!card->rx_dsc )
	// {
	// 	printk(KERN_ERR "NUMATO: cannot map region rx len:%lx start:%lx\n",
	// 		(long unsigned)pci_resource_len(pdev, BAR1),
	// 		(long unsigned)pci_resource_start(pdev, BAR1));
	// 	goto err_out_iounmap;
	// }
	// printk(KERN_ERR "NUMATO: mapping memory rx done %p \n",card->rx_dsc );
    // printk(KERN_ERR "NUMATO: Mem region len rx :%lx start:%lx mapped\n",
    //             (long unsigned)pci_resource_len(pdev, BAR1),
    //             (long unsigned)pci_resource_start(pdev, BAR1));
    // Step 5: Initialize device register

    /*
     * Step 6: Register IRQ handler
     * enable MSI
     */

    // ret = pci_alloc_irq_vectors(pdev, NVEC, NVEC, PCI_IRQ_MSI);
    // if (ret < 0){
    //     printk(KERN_ERR "NUMATO: MSI Failed\n");
    //     goto err_out_msi;
    // }
    // printk(KERN_ERR "NUMATO: MSI Done\n");
    // irq_no = pci_irq_vector(pdev, 0);
    // ret = request_irq(irq_no, handler, IRQF_SHARED, DEVICE_NAME, (void*)card);
    // if (ret < 0){
    //     printk(KERN_ERR "NUMATO: MSI request IRQ Failed\n");
    // }
    msleep(1);

    aller_numato_test(card);


    return 0;

 err_out_msi:
    pci_free_irq_vectors(pdev);
 err_out_iounmap:
    if(card->tx_dsc) pci_iounmap(pdev, card->tx_dsc);
    // if(card->tx_dsc) pci_iounmap(pdev, card->rx_dsc);
	pci_set_drvdata(pdev, NULL);
	kfree(card);
 err_out_clear_master:
    pci_clear_master(pdev);
//err_out_disable_device:
	pci_disable_device(pdev);
 err_out_none:
	return ret;
}

static void __devexit nu_remove(struct pci_dev *pdev){
    struct nf10_card *card;

    // free private data
    printk(KERN_INFO "NUMATO: releasing private memory\n");
    card = (struct nf10_card*)pci_get_drvdata(pdev);
    if(card){

        //nf10fops_remove(pdev, card);
        //nf10iface_remove(pdev, card);
        if(card->tx_dsc) iounmap(card->tx_dsc);
    }

    pci_set_drvdata(pdev, NULL);
    // release memory
    printk(KERN_INFO "NUMATO: releasing mem region\n");
	// pci_iounmap(pdev, card->tx_dsc);
    // pci_iounmap(pdev, card->rx_dsc);

    // disabling device
    printk(KERN_INFO "NUMATO: disabling device\n");
    pci_free_irq_vectors(pdev);
    pci_clear_master(pdev);
	pci_disable_device(pdev);
}

pci_ers_result_t nu_pcie_error(struct pci_dev *dev, enum pci_channel_state state){
    printk(KERN_ALERT "NUMATO: PCIe error: %d\n", state);
    return PCI_ERS_RESULT_RECOVERED;
}

static struct pci_error_handlers pcie_err_handlers = {
    .error_detected = nu_pcie_error
};

static struct pci_driver pci_driver = {
	.name = DEVICE_NAME,
	.id_table = pci_id,
	.probe = nu_probe,
	.remove = __devexit_p(nu_remove),
    .err_handler = &pcie_err_handlers
};

static int __init nf10_init(void)
{
	printk(KERN_ERR "\n\n\nNUMATO: module loaded\n");
	return pci_register_driver(&pci_driver);
}

static void __exit nf10_exit(void)
{
    pci_unregister_driver(&pci_driver);
	printk(KERN_ERR "NUMATO: module unloaded \n\n\n\n");
    printk(KERN_ERR "NUMATO: module unloaded \n\n\n\n");
}

module_init(nf10_init);
module_exit(nf10_exit);