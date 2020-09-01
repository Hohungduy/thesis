# Thesis
#### Description
This is my thesis.

#### What is this for ?

#### What is inside ?
    1. original_dma_xillinx
        This is example driver for pcie made by xilinx.
        I modified some small detail so that it can run on my computers
    2. netfpga-10g
        This is a mess. I change some details from netfpga projects.
    3. numato
        This driver is for numato aller board

#### Where to put this source code ?

    <openwrt_dir>/packet/kernel

#### Build
```bash
    make package/kernel/thesis/{clean,compile} -j4 V=s
```
