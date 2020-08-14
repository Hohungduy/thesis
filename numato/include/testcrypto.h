# ifndef _TESTCRYPTO_H_
# define _TESTCRYPTO_H_
// This module is used for test crypto driver.
//DEBUG 
#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <crypto/scatterwalk.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/skcipher.h>
#include <crypto/internal/aead.h>
#include <asm/uaccess.h>
#include <asm/smp.h>
#include <crypto/skcipher.h>
#include <crypto/akcipher.h>
#include <crypto/acompress.h>
#include <crypto/rng.h>
#include <crypto/drbg.h>
#include <crypto/kpp.h>
#include <crypto/internal/simd.h>
#include <crypto/aead.h>
#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/fips.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/scatterlist.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/zlib.h>
#include <linux/once.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/string.h>

//#define MODE_DIR_ENCRYPT 1;
//#define MODE_DIR_DECRYPT 2;

/* length of data field in each test case */
struct field_length {
    unsigned int AAD_len;
    unsigned int key_len;
    unsigned int scratchpad_len;
    unsigned int ivdata_len;
};
// for gcm-aes
const char AAD_const[8] = {0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef};
const char key_const[16] = {0xfe,0xff,0xe9,0x92,0x86,0x65,0x73,0x1c,0x6d,0x6a,0x8f,0x94,0x67,0x30,0x83,0x08};
const char scratchpad_const[60] =  {0xd9,0x31,0x32,0x25,0xf8,0x84,0x06,0xe5,
                                    0xa5,0x59,0x09,0xc5,0xaf,0xf5,0x26,0x9a,
                                    0x86,0xa7,0xa9,0x53,0x15,0x34,0xf7,0xda,0x2e, 
                                    0x4c,0x30,0x3d,0x8a,0x31,0x8a,0x72,0x1c,0x3c, 
                                    0x0c,0x95,0x95,0x68,0x09,0x53,0x2f,0xcf,0x0e,
                                    0x24,0x49,0xa6,0xb5,0x25,0xb1,0x6a,0xed,0xf5,
                                    0xaa,0x0d,0xe6,0x57,0xba,0x63,0x7b,0x39};
const char ivdata_const[12]={0xca,0xfe,0xba,0xbe,0xfa,0xce,0xdb,0xad,0xde,0xca,0xf8,0x88};

// for rfc4106
char AAD_const2 [12];
char key_const2[20];
char scratchpad_const2[320];// variable length
char ivdata_const2[8];
char authentag_const2[16];
char ciphertext_const2[320];//variable length = scratchpad_data
/* test case 1*/
const char AAD_test1[8] = {0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef};
const char key_test1[20] = {0xfe,0xff,0xe9,0x92,0x86,0x65,0x73,0x1c,0x6d,0x6a,0x8f,0x94,0x67,0x30,0x83,0x08,0xde,0xca,0xf8,0x88};
const char scratchpad_test1[68] = { 0xca,0xfe,0xba,0xbe,0xfa,0xce,0xdb,0xad,
                                    0xd9,0x31,0x32,0x25,0xf8,0x84,0x06,0xe5,
                                    0xa5,0x59,0x09,0xc5,0xaf,0xf5,0x26,0x9a,
                                    0x86,0xa7,0xa9,0x53,0x15,0x34,0xf7,0xda,0x2e, 
                                    0x4c,0x30,0x3d,0x8a,0x31,0x8a,0x72,0x1c,0x3c, 
                                    0x0c,0x95,0x95,0x68,0x09,0x53,0x2f,0xcf,0x0e,
                                    0x24,0x49,0xa6,0xb5,0x25,0xb1,0x6a,0xed,0xf5,
                                    0xaa,0x0d,0xe6,0x57,0xba,0x63,0x7b,0x39};
const char ivdata_test1[8]={0xca,0xfe,0xba,0xbe,0xfa,0xce,0xdb,0xad};
const char authentag_test1[16]={0xb4,0xfd,0x2d,0x56,0x96,0x47,0xd0,0x2f,0x72,0xbe,0x6c,0x8f,0x14,0x18,0x90,0x45};
struct field_length test1_len = {
    .AAD_len = 8,
    .key_len = 20,
    .scratchpad_len = 68,// include 4 bytes (octects) for Salt value
    .ivdata_len = 8,
};

/* test case 2*/
//fix theo fpga
// const char AAD_test2[12] = {0x00,0x00,0x00,0x00,0x21,0x43,0x65,0x87,0x21,0x43,0x00,0x00};
// // const char key_test2[20] = {0x4c,0x80,0xcd,0xef,0xbb,0x5d,0x10,0xda,0x90,0x6a,0xc7,0x3c,
// //                             0x36,0x13,0xa6,0x34,0x2e,0x44,0x3b,0x68};
// const char key_test2[20] = {0x34,0xa6,0x13,0x36,0x3c,0xc7,0x6a,0x90,
//                             0xda,0x10,0x5d,0xbb,0xef,0xcd,0x80,0x4c,
//                             0x68,0x3b,0x44,0x2e};
// const char scratchpad_test2[80] = { 0xfe,0x4c,0x24,0x3b,0x7e,0xed,0x56,0x49,
//                                     0x02,0x01,0xa8,0xc0,0xb7,0x4d,0x11,0x80,0x00,0x00,0x9a,0x69,0x48,0x00,0x00,0x45,
//                                     0x00,0x00,0x01,0x00,0x00,0x01,0xd3,0x38,0x56,0xf1,0x9b,0x0a,0x01,0x01,0xa8,0xc0,
//                                     0x73,0x03,0x70,0x64,0x75,0x5f,0x04,0x70,0x69,0x73,0x5f,0x04,0x00,0x00,0x00,0x00,
//                                     0x00,0x6b,0x64,0x02,0x79,0x74,0x69,0x63,0x72,0x65,0x62,0x79,0x63,0x09,0x70,0x69,
//                                     0x01,0x02,0x02,0x01,0x01,0x00,0x21,0x00};
// const char ivdata_test2[8]={0xfe,0x4c,0x24,0x3b,0x7e,0xed,0x56,0x49};
// const char authentag_test2[16]={0xb4,0xfd,0x2d,0x56,0x96,0x47,0xd0,0x2f,0x72,0xbe,0x6c,0x8f,0x14,0x18,0x90,0x45};
// //Outbound
// const char ciphertext_test2[80] = {0xfe,0x4c,0x24,0x3b,0x7e,0xed,0x56,0x49,
//                                    0x76,0x2b,0xd2,0x8d,0x52,0xdf,0x30,0xdc,0x07,0x5b,0x9d,0x72,0x7e,0x53,0xcf,0xfe,
// 	                               0x34,0xac,0xce,0x13,0xfa,0x09,0x85,0x34,0xfd,0xa6,0x96,0x66,0x73,0x98,0x1b,0x8d,
// 	                               0x5d,0x3c,0xa1,0xf4,0xf1,0x5b,0x92,0x65,0xcf,0xf3,0xa3,0x14,0x6f,0x43,0xa2,0xcf,
// 	                               0xce,0x3b,0xb9,0x86,0xb7,0xab,0xae,0x47,0x62,0xff,0xf5,0x84,0x18,0x1e,0xb2,0x15,
// 	                               0x32,0x97,0xfd,0x68,0xd7,0x17,0xbc,0x61
//                                 };
// struct field_length test2_len = {
//     .AAD_len = 12,
//     .key_len = 20,// include 4 bytes (octects) for Salt value
//     .scratchpad_len = 80,
//     .ivdata_len = 8,
// };

// theo LSB->MSB (big edian)

const char AAD_test2[12] = {0x00,0x00,0x43,0x21,0x87,0x65,0x43,0x21,0x00,0x00,0x00,0x00};
const char key_test2[20] = {0x4c,0x80,0xcd,0xef,0xbb,0x5d,0x10,0xda,0x90,0x6a,0xc7,0x3c,
                            0x36,0x13,0xa6,0x34,0x2e,0x44,0x3b,0x68};
// const char key_test2[20] = {0x34,0xa6,0x13,0x36,0x3c,0xc7,0x6a,0x90,
//                             0xda,0x10,0x5d,0xbb,0xef,0xcd,0x80,0x4c,
//                             0x68,0x3b,0x44,0x2e};
const char scratchpad_test2[80] = { 0x49,0x56,0xed,0x7e,0x3b,0x24,0x4c,0xfe,
                                    0x45,0x00,0x00,0x48,0x69,0x9a,0x00,0x00,0x80,0x11,0x4d,0xb7,0xc0,0xa8,0x01,0x02,                                    
                                    0xc0,0xa8,0x01,0x01,0x0a,0x9b,0xf1,0x56,0x38,0xd3,0x01,0x00,0x00,0x01,0x00,0x00,                                   
                                    0x00,0x00,0x00,0x00,0x04,0x5f,0x73,0x69,0x70,0x04,0x5f,0x75,0x64,0x70,0x03,0x73,                      
                                    0x69,0x70,0x09,0x63,0x79,0x62,0x65,0x72,0x63,0x69,0x74,0x79,0x02,0x64,0x6b,0x00,       
                                    0x00,0x21,0x00,0x01,0x01,0x02,0x02,0x01};
// const char scratchpad_test2[80] = { 0x49,0x56,0xed,0x7e,0x3b,0x24,0x4c,0xfe,
//                                     0x02,0x01,0xa8,0xc0,0xb7,0x4d,0x11,0x80,0x00,0x00,0x9a,0x69,0x48,0x00,0x00,0x45,
//                                     0x00,0x00,0x01,0x00,0x00,0x01,0xd3,0x38,0x56,0xf1,0x9b,0x0a,0x01,0x01,0xa8,0xc0,
//                                     0x73,0x03,0x70,0x64,0x75,0x5f,0x04,0x70,0x69,0x73,0x5f,0x04,0x00,0x00,0x00,0x00,
//                                     0x00,0x6b,0x64,0x02,0x79,0x74,0x69,0x63,0x72,0x65,0x62,0x79,0x63,0x09,0x70,0x69,
//                                     0x01,0x02,0x02,0x01,0x01,0x00,0x21,0x00};
const char ivdata_test2[8]={0x49,0x56,0xed,0x7e,0x3b,0x24,0x4c,0xfe};
// const char ivdata_test2[8]={0xfe,0x4c,0x24,0x3b,0x7e,0xed,0x56,0x49};
// const char authentag_test2[16] = {0xb4,0xfd,0x2d,0x56,0x96,0x47,0xd0,0x2f,0x72,0xbe,0x6c,0x8f,0x14,0x18,0x90,0x45};
const char authentag_test2[16] = {0x45,0x90,0x18,0x14,0x8f,0x6c,0xbe,0x72,0x2f,0xd0,0x47,0x96,0x56,0x2d,0xfd,0xb4};
//Outbound
const char ciphertext_test2[80] = {0x49,0x56,0xed,0x7e,0x3b,0x24,0x4c,0xfe,
                                   0x76,0x2b,0xd2,0x8d,0x52,0xdf,0x30,0xdc,0x07,0x5b,0x9d,0x72,0x7e,0x53,0xcf,0xfe,
	                               0x34,0xac,0xce,0x13,0xfa,0x09,0x85,0x34,0xfd,0xa6,0x96,0x66,0x73,0x98,0x1b,0x8d,
	                               0x5d,0x3c,0xa1,0xf4,0xf1,0x5b,0x92,0x65,0xcf,0xf3,0xa3,0x14,0x6f,0x43,0xa2,0xcf,
	                               0xce,0x3b,0xb9,0x86,0xb7,0xab,0xae,0x47,0x62,0xff,0xf5,0x84,0x18,0x1e,0xb2,0x15,
	                               0x32,0x97,0xfd,0x68,0xd7,0x17,0xbc,0x61
                                   };
struct field_length test2_len = {
    .AAD_len = 12,
    .key_len = 20,// include 4 bytes (octects) for Salt value
    .scratchpad_len = 80,
    .ivdata_len = 8,
};
/* test case 3*/
const char AAD_test3[8] = {0x00,0x00,0xa5,0xf8,0x00,0x00,0x00,0x0a};
const char key_test3[20] = {0xfe,0xff,0xe9,0x92,0x86,0x65,0x73,0x1c,
                            0x6d,0x6a,0x8f,0x94,0x67,0x30,0x83,0x08,
                            0xca,0xfe,0xba,0xbe};
const char scratchpad_test3[72] = { 0xfa,0xce,0xdb,0xad,0xde,0xca,0xf8,0x88,
                                    0x45,0x00,0x00,0x3e,0x69,0x8f,0x00,0x00,
                                    0x80,0x11,0x4d,0xcc,0xc0,0xa8,0x01,0x02,
                                    0xc0,0xa8,0x01,0x01,0x0a,0x98,0x00,0x35,
                                    0x00,0x2a,0x23,0x43,0xb2,0xd0,0x01,0x00,
                                    0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x03,0x73,0x69,0x70,0x09,0x63,0x79,0x62,
                                    0x65,0x72,0x63,0x69,0x74,0x79,0x02,0x64,
                                    0x6b,0x00,0x00,0x01,0x00,0x01,0x00,0x01};
const char ivdata_test3[8]={0xfa,0xce,0xdb,0xad,0xde,0xca,0xf8,0x88};
const char authentag_test3[16]={0xb4,0xfd,0x2d,0x56,0x96,0x47,0xd0,0x2f,0x72,0xbe,0x6c,0x8f,0x14,0x18,0x90,0x45};
struct field_length test3_len = {
    .AAD_len = 8,
    .key_len = 20,// include 4 bytes (octects) for Salt value
    .scratchpad_len = 72,
    .ivdata_len = 8,
};

/* test case 4*/
const char AAD_test4[8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
const char key_test4[20] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00};
const char scratchpad_test4[72] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x45,0x00,0x00,0x3c,0x99,0xc5,0x00,0x00,
                                    0x80,0x01,0xcb,0x7a,0x40,0x67,0x93,0x18,
                                    0x01,0x01,0x01,0x01,0x08,0x00,0x07,0x5c,
                                    0x02,0x00,0x44,0x00,0x61,0x62,0x63,0x64,
                                    0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,
                                    0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,
                                    0x75,0x76,0x77,0x61,0x62,0x63,0x64,0x65,
                                    0x66,0x67,0x68,0x69,0x01,0x02,0x02,0x01};
const char ivdata_test4[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const char authentag_test4[16]={0xb4,0xfd,0x2d,0x56,0x96,0x47,0xd0,0x2f,0x72,0xbe,0x6c,0x8f,0x14,0x18,0x90,0x45};
struct field_length test4_len = {
    .AAD_len = 8,
    .key_len = 20,// include 4 bytes (octects) for Salt value
    .scratchpad_len = 72,
    .ivdata_len = 8,
};


/*test case 5*/
const char AAD_test5[8] = { 0xc7,0xb9,0x52,0x4c,0x00,0x00,0x00,0x01};
const char key_test5[20] = {0x8d,0xc3,0xd9,0x91,0x21,0x9f,0xd8,0xc8,
                            0xe5,0x20,0x2a,0x31,0x1c,0x92,0x71,0xee,
                            0xb4,0x49,0x7f,0xd2};
const char scratchpad_test5[52] = { 0x58,0x6c,0x32,0xd9,0xbf,0x24,0x34,0x39,
                                    0x45,0x00,0x00,0x28,0x00,0x00,0x40,0x00,
                                    0x40,0x06,0x0c,0xed,0x4a,0x7d,0x44,0xbc,
                                    0xc0,0xa8,0xde,0x01,0x14,0x6c,0xed,0x64,
                                    0x00,0x00,0x00,0x00,0x87,0x56,0x71,0xc9,
                                    0x50,0x14,0x00,0x00,0x86,0xfc,0x00,0x00,
                                    0x01,0x02,0x02,0x04};

const char ivdata_test5[12]={0x58,0x6c,0x32,0xd9,0xbf,0x24,0x34,0x39,};
const char authentag_test5[16]={0xb4,0xfd,0x2d,0x56,0x96,0x47,0xd0,0x2f,0x72,0xbe,0x6c,0x8f,0x14,0x18,0x90,0x45};
struct field_length test5_len = {
    .AAD_len = 8,
    .key_len = 20,// include 4 bytes (octects) for Salt value
    .scratchpad_len = 52,
    .ivdata_len = 8,
};

/* test case 6*/
const char AAD_test6[8] = { 0xc4,0x1c,0x22,0x5b,0x00,0x00,0x00,0x01};
const char key_test6[20] = {0x09,0xfc,0x4d,0x10,0x76,0x4a,0x7f,0xdc,
                            0x56,0x30,0xaa,0xd6,0xea,0x57,0x9a,0xa8,
                            0x5d,0x8b,0x0d,0x13};
const char scratchpad_test6[52] = {0xbe,0xb4,0xc9,0x26,0xff,0xb6,0x08,0x00,
                                   0x45,0x00,0x00,0x28,0x00,0x00,0x40,0x00,
                                   0x40,0x06,0x0c,0xed,0x4a,0x7d,0x44,0xbc,
                                   0xc0,0xa8,0xde,0x01,0x01,0xbb,0xa1,0x84,
                                   0x00,0x00,0x00,0x00,0x5e,0xc0,0x63,0xcd,
                                   0x50,0x14,0x00,0x00,0x1c,0x20,0x00,0x00,
                                   0x01,0x02,0x02,0x04};

const char ivdata_test6[8]={0x16,0x77,0xc1,0x86,0xdb,0xd5,0x32,0x5a};
const char authentag_test6[16]={0xb4,0xfd,0x2d,0x56,0x96,0x47,0xd0,0x2f,0x72,0xbe,0x6c,0x8f,0x14,0x18,0x90,0x45};
struct field_length test6_len = {
    .AAD_len = 8,
    .key_len = 20,// include 4 bytes (octects) for Salt value
    .scratchpad_len = 52,
    .ivdata_len = 8,
};

/* test case 9 - long data*/
// const char AAD_test7[8] = { 0xc8,0xb5,0x9f,0x82,0x00,0x00,0x00,0x0a};
// const char key_test7[20] = {0x09,0x36,0x59,0x36,0xd9,0xb6,0x42,0x82,
//                             0x69,0xb6,0xdf,0x4a,0x19,0xd8,0x9e,0xa8,
//                             0x5d,0x8b,0x0d,0x13};
// const char scratchpad_test7[52] = {0xbe,0xb4,0xc9,0x26,0xff,0xb6,0x08,0x00,
//                                    0x45,0x00,0x00,0x28,0x00,0x00,0x40,0x00,
//                                    0x40,0x06,0x0c,0xed,0x4a,0x7d,0x44,0xbc,
//                                    0xc0,0xa8,0xde,0x01,0x01,0xbb,0xa1,0x84,
//                                    0x00,0x00,0x00,0x00,0x5e,0xc0,0x63,0xcd,
//                                    0x50,0x14,0x00,0x00,0x1c,0x20,0x00,0x00,
//                                    0x01,0x02,0x02,0x04};

// const char ivdata_test7[8]={0x16,0x77,0xc1,0x86,0xdb,0xd5,0x32,0x5a};

// struct field_length test7_len = {
//     .AAD_len = 8,
//     .key_len = 20,// include 4 bytes (octects) for Salt value
//     .scratchpad_len = 52,
//     .ivdata_len = 8,
// };



/* test case 7 -ver2*/
//inbound
// const char AAD_test7[8] = {0xc6,0xcb,0x41,0x5c,0x00,0x00,0x00,0x02};//Big edian: LSB ->MSB (0x02: LSB)

// // const char key_test2[20] = {0x4c,0x80,0xcd,0xef,0xbb,0x5d,0x10,0xda,0x90,0x6a,0xc7,0x3c,
// //                             0x36,0x13,0xa6,0x34,0x2e,0x44,0x3b,0x68};
// const char key_test7[20] = {0x52,0xac,0x8d,0x0c,0x6a,0x12,0x2c,0xff,0x69,0x6c,0xbe,0xae,0x83,0x83,0x44,0x04,
//                             0xe6,0x46,0x2a,0x50};//Litte Edian : MSB ->LSB (0x76: MSB - 0x38:MSB)
// const char scratchpad_test7[52] = { 0x6f,0x47,0xe5,0x3f,0x17,0xfd,0x5e,0x98,
//                                     0x45,0x00,0x00,0x28,0x00,0x00,0x40,0x00,0x40,0x06,0xf1,0x27,0x9d,0xf0,0x0d,0x0e,
//                                     0xc0,0xa8,0xde,0x01,0x01,0xbb,0xa2,0xf6,0x00,0x00,0x00,0x00,0xf7,0x57,0x04,0x67,
//                                     0x50,0x14,0x00,0x00,0xC5,0xB7,0x00,0x00,0x01,0x02,0x02,0x04
//                                     };// Big edian: LSB->MSB (0x19: LSB, 0xb3: LSB)
// const char ivdata_test7[8]={0x6f,0x47,0xe5,0x3f,0x17,0xfd,0x5e,0x98};// Big edian: LSB->MSB (0x98:LSB)
// const char authentag_test7[16]={0xc8,0x33,0xbc,0x69,0x20,0x2d,0xff,0xda,0x67,0x04,0x85,0x62,0x2c,0x8e,0x29,0x9b};// Big edian: LSB->MSB (0x58 LSB)
// //Outbound
// const char ciphertext_test7[52] = {0x6f,0x47,0xe5,0x3f,0x17,0xfd,0x5e,0x98,
//                                    0x5a,0x43,0x35,0x02,0xee,0x14,0xc2,0xa5,0xb9,0xba,0xb1,0x42,0x94,0x75,0xd6,0x46,
// 	                               0x3c,0x6c,0xd0,0xdb,0x98,0xf3,0x3b,0x59,0xd1,0x47,0x71,0xa4,0xd2,0x8f,0x64,0x9b,
// 	                               0x3b,0xbc,0x63,0xc4,0x7b,0x5e,0xd4,0x6e,0x5e,0xb2,0x73,0xcf
//                                 };
// struct field_length test7_len = {
//     .AAD_len = 8,
//     .key_len = 20,// include 4 bytes (octects) for Salt value
//     .scratchpad_len = 52,
//     .ivdata_len = 8,
// };

/* test case 7 -ver3*/
//inbound
const char AAD_test7[8] = {0xc7,0x86,0x32,0x64,0x00,0x00,0x00,0x02};//Big edian: LSB ->MSB (0x02: LSB)

// const char key_test2[20] = {0x4c,0x80,0xcd,0xef,0xbb,0x5d,0x10,0xda,0x90,0x6a,0xc7,0x3c,
//                             0x36,0x13,0xa6,0x34,0x2e,0x44,0x3b,0x68};
const char key_test7[20] = {0x1d,0xfc,0x10,0x87,0x16,0x45,0x96,0xac,0xfd,0x97,0x5f,0xd5,0x3c,0x63,0x3e,0x34,
                            0xbc,0x4f,0xe1,0xb1};//Litte Edian : MSB ->LSB (0x76: MSB - 0x38:MSB)
const char scratchpad_test7[72] = { 0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9,
                                    0x45,0x00,0x00,0x3c,0x89,0x97,0x40,0x00,0x40,0x06,0xff,0x40,0xc0,0xa8,0xde,0x01,
                                    0x4a,0x7d,0xc8,0xbc,0xb9,0x5e,0x14,0x6c,0x36,0x84,0xa1,0x1e,0x00,0x00,0x00,0x00,
                                    0xa0,0x02,0xfa,0xf0,0x42,0x8f,0x00,0x00,0x01,0x03,0x03,0x07,0x01,0x02,0x02,0x04,
                                    0xbc,0x1a,0xf7,0x13,0x00,0x00,0x00,0x00,0x01,0x03,0x03,0x07,0x01,0x02,0x02,0x04
                                    };// Big edian: LSB->MSB (0x19: LSB, 0xb3: LSB)
                                //,0xad,0xc0,0xbd,0xad,0xa4,0x07,0xd9,0xf6,0xc7,0x98,0xe1,0xc4,0x95,0x5d,0x1c,0x6e
const char ivdata_test7[8]={0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9};// Big edian: LSB->MSB (0x98:LSB)
const char authentag_test7[16]={0xf5,0x16,0x61,0x81,0xec,0x8c,0x5e,0x65,0x8b,0x75,0x5d,0x9e,0xb3,0x55,0xde,0xcc};// Big edian: LSB->MSB (0x58 LSB)
//0xf5,0x16,0x61,0x81,0xec,0x8c,0x5e,0x65,0x8b,0x75,0x5d,0x9e,0xb3,0x55,0xde,0xcc
//Outbound
const char ciphertext_test7[72] = {0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9,
                                   0xc9,0x36,0xb3,0x24,0x66,0xc2,0x22,0xf5,0xfa,0xb7,0x33,0xef,0x89,0xcc,0x6a,0x04,
	                               0x57,0x9c,0x46,0xe8,0xa5,0x14,0x8c,0xe4,0x4f,0x52,0xe1,0x35,0x3a,0x5e,0x64,0x9a,
	                               0xe5,0x2e,0x75,0x3f,0x04,0xba,0xae,0x0a,0xc9,0x95,0xa3,0x07,0xcd,0xa9,0x72,0xfb,
                                   0xb4,0x6e,0x3a,0xde,0x17,0x4b,0x75,0x33,0xcb,0x18,0xab,0x7b,0xe4,0x99,0x7b,0xae
                                };
struct field_length test7_len = {
    .AAD_len = 8,
    .key_len = 20,// include 4 bytes (octects) for Salt value
    .scratchpad_len = 72,
    .ivdata_len = 8,
};


// /* test case 7 -ver3*/
// //inbound
// const char AAD_test7[8] = {0xc7,0x86,0x32,0x64,0x00,0x00,0x00,0x02};//Big edian: LSB ->MSB (0x02: LSB)

// // const char key_test2[20] = {0x4c,0x80,0xcd,0xef,0xbb,0x5d,0x10,0xda,0x90,0x6a,0xc7,0x3c,
// //                             0x36,0x13,0xa6,0x34,0x2e,0x44,0x3b,0x68};
// const char key_test7[20] = {0x1d,0xfc,0x10,0x87,0x16,0x45,0x96,0xac,0xfd,0x97,0x5f,0xd5,0x3c,0x63,0x3e,0x34,
//                             0xbc,0x4f,0xe1,0xb1};//Litte Edian : MSB ->LSB (0x76: MSB - 0x38:MSB)
// const char scratchpad_test7[72] = { 0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9,
// /* test case 7 -ver3*/
// //inbound
// const char AAD_test7[8] = {0xc7,0x86,0x32,0x64,0x00,0x00,0x00,0x02};//Big edian: LSB ->MSB (0x02: LSB)

// // const char key_test2[20] = {0x4c,0x80,0xcd,0xef,0xbb,0x5d,0x10,0xda,0x90,0x6a,0xc7,0x3c,
// //                             0x36,0x13,0xa6,0x34,0x2e,0x44,0x3b,0x68};
// const char key_test7[20] = {0x1d,0xfc,0x10,0x87,0x16,0x45,0x96,0xac,0xfd,0x97,0x5f,0xd5,0x3c,0x63,0x3e,0x34,
//                             0xbc,0x4f,0xe1,0xb1};//Litte Edian : MSB ->LSB (0x76: MSB - 0x38:MSB)
// const char scratchpad_test7[72] = { 0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9,
//                                     0x45,0x00,0x00,0x3c,0x89,0x97,0x40,0x00,0x40,0x06,0xff,0x40,0xc0,0xa8,0xde,0x01,
//                                     0x4a,0x7d,0xc8,0xbc,0xb9,0x5e,0x14,0x6c,0x36,0x84,0xa1,0x1e,0x00,0x00,0x00,0x00,
//                                     0xa0,0x02,0xfa,0xf0,0x42,0x8f,0x00,0x00,0x01,0x03,0x03,0x07,0x01,0x02,0x02,0x04,
//                                     0xbc,0x1a,0xf7,0x13,0x00,0x00,0x00,0x00,0x01,0x03,0x03,0x07,0x01,0x02,0x02,0x04
//                                     };// Big edian: LSB->MSB (0x19: LSB, 0xb3: LSB)
//                                 //,0xad,0xc0,0xbd,0xad,0xa4,0x07,0xd9,0xf6,0xc7,0x98,0xe1,0xc4,0x95,0x5d,0x1c,0x6e
// const char ivdata_test7[8]={0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9};// Big edian: LSB->MSB (0x98:LSB)
// const char authentag_test7[16]={0xf5,0x16,0x61,0x81,0xec,0x8c,0x5e,0x65,0x8b,0x75,0x5d,0x9e,0xb3,0x55,0xde,0xcc};// Big edian: LSB->MSB (0x58 LSB)
// //0xf5,0x16,0x61,0x81,0xec,0x8c,0x5e,0x65,0x8b,0x75,0x5d,0x9e,0xb3,0x55,0xde,0xcc
// //Outbound
// const char ciphertext_test7[72] = {0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9,
//                                    0xc9,0x36,0xb3,0x24,0x66,0xc2,0x22,0xf5,0xfa,0xb7,0x33,0xef,0x89,0xcc,0x6a,0x04,
// 	                               0x57,0x9c,0x46,0xe8,0xa5,0x14,0x8c,0xe4,0x4f,0x52,0xe1,0x35,0x3a,0x5e,0x64,0x9a,
// 	                               0xe5,0x2e,0x75,0x3f,0x04,0xba,0xae,0x0a,0xc9,0x95,0xa3,0x07,0xcd,0xa9,0x72,0xfb,
//                                    0xb4,0x6e,0x3a,0xde,0x17,0x4b,0x75,0x33,0xcb,0x18,0xab,0x7b,0xe4,0x99,0x7b,0xae
//                                 };
// struct field_length test7_len = {
//     .AAD_len = 8,
//     .key_len = 20,// include 4 bytes (octects) for Salt value
//     .scratchpad_len = 72,
//     .ivdata_len = 8,
// };
//                                     };// Big edian: LSB->MSB (0x19: LSB, 0xb3: LSB)
//                                 //,0xad,0xc0,0xbd,0xad,0xa4,0x07,0xd9,0xf6,0xc7,0x98,0xe1,0xc4,0x95,0x5d,0x1c,0x6e
// const char ivdata_test7[8]={0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9};// Big edian: LSB->MSB (0x98:LSB)
// const char authentag_test7[16]={0xf5,0x16,0x61,0x81,0xec,0x8c,0x5e,0x65,0x8b,0x75,0x5d,0x9e,0xb3,0x55,0xde,0xcc};// Big edian: LSB->MSB (0x58 LSB)
// //0xf5,0x16,0x61,0x81,0xec,0x8c,0x5e,0x65,0x8b,0x75,0x5d,0x9e,0xb3,0x55,0xde,0xcc
// //Outbound
// const char ciphertext_test7[72] = {0x33,0x8e,0xfd,0xd8,0x3a,0xbf,0x85,0xa9,
//                                    0xc9,0x36,0xb3,0x24,0x66,0xc2,0x22,0xf5,0xfa,0xb7,0x33,0xef,0x89,0xcc,0x6a,0x04,
// 	                               0x57,0x9c,0x46,0xe8,0xa5,0x14,0x8c,0xe4,0x4f,0x52,0xe1,0x35,0x3a,0x5e,0x64,0x9a,
// 	                               0xe5,0x2e,0x75,0x3f,0x04,0xba,0xae,0x0a,0xc9,0x95,0xa3,0x07,0xcd,0xa9,0x72,0xfb,
//                                    0xb4,0x6e,0x3a,0xde,0x17,0x4b,0x75,0x33,0xcb,0x18,0xab,0x7b,0xe4,0x99,0x7b,0xae
//                                 };
// struct field_length test7_len = {
//     .AAD_len = 8,
//     .key_len = 20,// include 4 bytes (octects) for Salt value
//     .scratchpad_len = 72,
//     .ivdata_len = 8,
// };
#endif






// {
//     "name": "My Server",
//     "host": "192.168.1.7",
//     "protocol": "sftp",
//     "port": 22,
//     "username": "ptnb3",
//     "remotePath": "/home/ptnb3/USAMI/linuxsytemforrouter/package/kernel/thesis/",
//     "uploadOnSave": true
// }