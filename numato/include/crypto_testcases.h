#ifndef __crypto_testcases__
#define __crypto_testcases__

#include "xdma_region.h"

#define TESTCASE_1_ICV_SIZE (4*4)

struct region_in testcase_1_in = {
    .crypto_dsc = {
        .info = {
            // 0c091000
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000, 
            .free_space_ = 0,
            .direction   = 1,
            .length = 0b00001001000,
            .aadsize = 12,
            .keysize = 0
        },
        .icv  = {0x562dfdb4, 0x2fd04796, 0x8f6cbe72, 0x45901814},
        .key  = {0x00000000, 0x00000000, 0x00000000, 0x00000000,
                 0x3613a634, 0x906ac73c, 0xbb5d10da, 0x4c80cdef
                 },
        .iv   = {
            .nonce = 0x2e443b68, 
            .iv[1] = 0x4956ed7e, 
            .iv[0] = 0x3b244cfe, 
            .tail  = 0x00000001
        },
        .aad  = {0x00000000, 0x87654321, 0x00004321, 0x00000000}
    },
    .data = {
        0xc0a80102, 0x80114db7, 0x699a0000, 0x45000048,      
        0x00010000, 0x38d30100, 0x0a9bf156, 0xc0a80101,      
        0x64700373, 0x70045f75, 0x045f7369, 0x00000000,      
        0x02646b00, 0x63697479, 0x79626572, 0x69700963,      
        0x01020201, 0x00210001 
        }
};
struct region_out testcase_1_out = {
    .data = {
        0xfecf537e, 0x729d5b07, 0xdc30df52, 0x8dd22b76, 
        0x8d1b9873, 0x6696a6fd, 0x348509fa, 0x13ceac34, 
        0xcfa2436f, 0x14a3f3cf, 0x65925bf1, 0xf4a13c5d, 
        0x15b21e18, 0x84f5ff62, 0x47aeabb7, 0x86b93bce, 
        0x61bc17d7, 0x68fd9732, 0x00000000, 0x00000000, 
        0x45901814, 0x8f6cbe72, 0x2fd04796, 0x562dfdb4 
        }
};

struct region_in testcase_2_in = {
    .crypto_dsc = {
        .info = {
            // 0c091000
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000, 
            .free_space_ = 0,
            .direction   = 1,
            .length = 0b00001001000,
            .aadsize = 12,
            .keysize = 0
        },
        .icv  = {0x562dfdb4, 0x2fd04796, 0x8f6cbe72, 0x45901814},
        .key  = {0x00000000, 0x00000000, 0x00000000, 0x00000000,
                 0x3613a634, 0x906ac73c, 0xbb5d10da, 0x4c80cdef
                 },
        .iv   = {
            .nonce = 0x2e443b68, 
            .iv[1] = 0x4956ed7e, 
            .iv[0] = 0x3b244cfe, 
            .tail  = 0x00000001
        },
        .aad  = {0x00000000, 0x87654321, 0x00004321, 0x00000000}
    },
    .data = {
        0xc0a80102, 0x80114db7, 0x699a0000, 0x45000048,      
        0x00010000, 0x38d30100, 0x0a9bf156, 0xc0a80101,      
        0x64700373, 0x70045f75, 0x045f7369, 0x00000000,      
        0x02646b00, 0x63697479, 0x79626572, 0x69700963,      
        0x01020201, 0x00210001 
        }
};

// struct region_in testcase_3_in = {
//     .crypto_dsc = {
//         .info = {0x00000000, 0x00000000, 0x00000000, 0x00083482},
//         .icv  = {0xb335f4ee, 0xcfdbf831, 0x824b4c49, 0x15956c96},
//         .key  = {0xabbccdde, 0xf0011223, 0x34455667, 0x78899aab, 
//                  0xabbccdde, 0xf0011223, 0x34455667, 0x78899aab},
//         .iv   = {0x11223344, 0x01020304, 0x05060708, 0x00000001},
//         .aad  = {0x00000000, 0x00000000, 0x4a2cbfe3, 0x00000002}
//     },
//     .data = {
//         0x45000030, 0x69a64000, 0x80062690, 0xc0a80102,
//         0x9389155e, 0x0a9e008b, 0x2dc57ee0, 0x00000000,
//         0x70024000, 0x20bf0000, 0x020405b4, 0x01010402,
//         0x01020201
//         }
// };
// struct region_out testcase_3_out = {
//     .data = {
//         0xff425c9b, 0x724599df, 0x7a3bcd51, 0x0194e00d,
//         0x6a78107f, 0x1b0b1cbf, 0x06efae9d, 0x65a5d763,
//         0x748a6379, 0x85771d34, 0x7f054565, 0x9f14e99d,
//         0xef842d8e, 0x00000000, 0x00000000, 0x00000000,
//         0xb335f4ee, 0xcfdbf831, 0x824b4c49, 0x15956c96
//         }
// };

// struct region_in testcase_4_in = {
//     .crypto_dsc = {
//         .info = {0x00000000, 0x00000000, 0x00000000, 0x00084080},
//         .icv  = {0xf821d496, 0xeeb096e9, 0x8ad2b69e, 0x4799c71d},
//         .key  = {0x00000000, 0x00000000, 0x00000000, 0x00000000, 
//                  0x00000000, 0x00000000, 0x00000000, 0x00000000},
//         .iv   = {0x00000000, 0x00000000, 0x00000000, 0x00000001},
//         .aad  = {0x00000000, 0x00000000, 0x00000000, 0x00000001}
//     },
//     .data = {
//         0x4500003c, 0x99c50000, 0x8001cb7a, 0x40679318,
//         0x01010101, 0x0800075c, 0x02004400, 0x61626364,
//         0x65666768, 0x696a6b6c, 0x6d6e6f70, 0x71727374,
//         0x75767761, 0x62636465, 0x66676869, 0x01020201
//         }
// };
// struct region_out testcase_4_out = {
//     .data = {
//         0x4688daf2, 0xf973a392, 0x732909c3, 0x31d56d60,
//         0xf694abaa, 0x414b5e7f, 0xf5fdcdff, 0xf5e9a284,
//         0x45647649, 0x2719ffb6, 0x4de7d9dc, 0xa1e1d894,
//         0xbc3bd578, 0x73ed4d18, 0x1d19d4d5, 0xc8c18af3,
//         0xf821d496, 0xeeb096e9, 0x8ad2b69e, 0x4799c71d
//         }
// };

// struct region_in testcase_5_in = {
//     .crypto_dsc = {
//         .info = {0x00000000, 0x00000000, 0x00000000, 0x000840C0},
//         .icv  = {0xe0d731cc, 0x978ecafa, 0xeae88f00, 0xe80d6e48},
//         .key  = {0x3de09874, 0xb388e649, 0x1988d0c3, 0x607eae1f, 
//                  0x00000000, 0x00000000, 0x00000000, 0x00000000},
//         .iv   = {0x57690e43, 0x4e280000, 0xa2fca1a3, 0x00000001},
//         .aad  = {0x00000000, 0x42f67e3f, 0x10101010, 0x10101010}
//     },
//     .data = {
//         0x4500003c, 0x99c30000, 0x8001cb7c, 0x40679318,
//         0x01010101, 0x0800085c, 0x02004300, 0x61626364,
//         0x65666768, 0x696a6b6c, 0x6d6e6f70, 0x71727374,
//         0x75767761, 0x62636465, 0x66676869, 0x01020201
//         }
// };
// struct region_out testcase_5_out = {
//     .data = {
//         0xfba2caa4, 0x853cf9f0, 0xf22cb10d, 0x86dd83b0,
//         0xfec75691, 0xcf1a04b0, 0x0d1138ec, 0x9c357917,
//         0x65acbd87, 0x01ad7984, 0x5bf9fe3f, 0xba487bc9,
//         0x1755e666, 0x2b4c8d0d, 0x1f5e2273, 0x9530320a,
//         0xe0d731cc, 0x978ecafa, 0xeae88f00, 0xe80d6e48
//         }
// };

// struct region_in testcase_6_in = {
//     .crypto_dsc = {
//         .info = {0x00000000, 0x00000000, 0x00000000, 0x00081CC0},
//         .icv  = {0x369f071f, 0x35e034be, 0x95f112e4, 0xe7d05d35},
//         .key  = {0x3de09874, 0xb388e649, 0x1988d0c3, 0x607eae1f, 
//                  0x00000000, 0x00000000, 0x00000000, 0x00000000},
//         .iv   = {0x57690e43, 0x4e280000, 0xa2fca1a3, 0x00000001},
//         .aad  = {0x00000000, 0x42f67e3f, 0x10101010, 0x10101010}
//     },
//     .data = {
//         0x4500001c, 0x42a20000, 0x8001441f, 0x406793b6,
//         0xe0000002, 0x0a00f5ff, 0x01020201
//         }
// };
// struct region_out testcase_6_out = {
//     .data = {
//         0xfba2ca84, 0x5e5df9f0, 0xf22c3e6e, 0x86dd831e,
//         0x1fc65792, 0xcd1af913, 0x0e1379ed, 0x00000000,
//         0x369f071f, 0x35e034be, 0x95f112e4, 0xe7d05d35
//         }
// };

// struct region_in testcase_7_in = {
//     .crypto_dsc = {
//         .info = {0x00000000, 0x00000000, 0x00000000, 0x00082882},
//         .icv  = {0x95457b96, 0x52037f53, 0x18027b5b, 0x4cd7a636},
//         .key  = {0xfeffe992, 0x8665731c, 0x6d6a8f94, 0x67308308, 
//                  0xfeffe992, 0x8665731c, 0x00000000, 0x00000000},
//         .iv   = {0xcafebabe, 0xfacedbad, 0xdecaf888, 0x00000001},
//         .aad  = {0x00000000, 0x00000000, 0x0000a5f8, 0x0000000a}
//     },
//     .data = {
//         0x45000028, 0xa4ad4000, 0x40067880, 0x0a01038f,
//         0x0a010612, 0x802306b8, 0xcb712602, 0xdd6bb03e,
//         0x501016d0, 0x75680001
//         }
// };
// struct region_out testcase_7_out = {
//     .data = {
//         0xa5b1f806, 0x6029aea4, 0x0e598b81, 0x22de0242,
//         0x0938b3ab, 0x33f828e6, 0x87b8858b, 0x5bfbdbd0,
//         0x315b2745, 0x2144cc77, 0x00000000, 0x00000000,
//         0x95457b96, 0x52037f53, 0x18027b5b, 0x4cd7a636
//         }
// };

// struct region_in testcase_8_in = {
//     .crypto_dsc = {
//         .info = {0x00000000, 0x00000000, 0x00000000, 0x00082840},
//         .icv  = {0x651f57e6, 0x5f354f75, 0xff170157, 0x69623436},
//         .key  = {0xabbccdde, 0xf0011223, 0x34455667, 0x78899aab, 
//                  0x00000000, 0x00000000, 0x00000000, 0x00000000},
//         .iv   = {0xdecaf888, 0xcafedeba, 0xceface74, 0x00000001},
//         .aad  = {0x00000000, 0x00000100, 0x00000000, 0x00000001}
//     },
//     .data = {
//         0x45000049, 0x33ba0000, 0x7f119106, 0xc3fb1d10,
//         0xc2b1d326, 0xc02831ce, 0x0035dd7b, 0x800302d5,
//         0x00004e20, 0x001e8c18, 0xd75b81dc, 0x91baa047,
//         0x6b91b924, 0xb280389d, 0x92c963ba, 0xc046ec95,
//         0x9b6266c0, 0x4722b149, 0x23010101
//         }
// };
// struct region_out testcase_8_out = {
//     .data = {
//         0x18a6fd42, 0xf72cbf4a, 0xb2a2ea90, 0x1f73d814,
//         0xe3e7f243, 0xd95412e1, 0xc349c1d2, 0xfbec168f,
//         0x9190feeb, 0xaf2cb019, 0x84e65863, 0x965d7472,
//         0xb79da345, 0xe0e78019, 0x1f0d2f0e, 0x0f496c22,
//         0x6f2127b2, 0x7db35724, 0xe7845d68, 0x00000000,
//         0x651f57e6, 0x5f354f75, 0xff170157, 0x69623436
//         }
// };

#endif // !__crypto_testcases__