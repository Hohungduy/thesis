# linux
## Makefile
Top level makefile
## Documentations
## arch
architecture specific code is in this directory and in include/asm-<arch> directory
## crypto
cryptographic API for use by kernel itself
## drivers
Code to run peripheral devices. Ex: videos driver, network card drivers, low-level CSCI drivers, ... is in drivers/net
## fs
Greneric filesystem code & each different filesystem are found in this directory.
## include
Architecture specific include files are in asm-arch
symbolic link from asm to asm-arch sothat #include <asm/file.h> will get the proper file for that architecture without having to hard code it into the .c file
## init
contain main.c and version.c and code for creating "early userspace".
version.c defines the Linux version string.
main.c: kernel "glue"
Early user space: important
## ipc
code for shared memory, semaphores and other form of IPC
## kernel
kernel level code that doesnt fit anywhere else goes in here
## lib
Generic usefulness to all kernel code are put in here: string operation, debugging, command line parsing code
## mm
