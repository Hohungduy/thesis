#ifndef __crypto_testcases__
#define __crypto_testcases__

#include "xdma_region.h"

#define TESTCASE_1_ICV_SIZE (4*4)


struct inbound testcase_1_in = {
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
struct outbound testcase_1_out = {
    .data = {
        0xfecf537e, 0x729d5b07, 0xdc30df52, 0x8dd22b76, 
        0x8d1b9873, 0x6696a6fd, 0x348509fa, 0x13ceac34, 
        0xcfa2436f, 0x14a3f3cf, 0x65925bf1, 0xf4a13c5d, 
        0x15b21e18, 0x84f5ff62, 0x47aeabb7, 0x86b93bce, 
        0x61bc17d7, 0x68fd9732, 0x00000000, 0x00000000, 
        0x45901814, 0x8f6cbe72, 0x2fd04796, 0x562dfdb4 
        }
};

struct inbound testcase_2_in = {
    .crypto_dsc = {
        .info = {
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000,
            .free_space_ = 0,
            .direction   = 1,
            .length = 0b00001000000,
            .aadsize = 8,
            .keysize = 0
        },
        .icv  = {0x5a41ad4a, 0xc309e9d8, 0xa8bc6ee4, 0x83b70d3a},
        .key  = {0x00000000, 0x00000000, 0x00000000, 0x00000000,
                 0x67308308, 0x6d6a8f94, 0x8665731c, 0xfeffe992  
                },
        .iv   = {
            .nonce = 0xcafebabe, 
            .iv[1] = 0xfacedbad, 
            .iv[0] = 0xdecaf888, 
            .tail = 0x00000001
        },
        .aad  = {0x0000000a, 0x0000a5f8, 0x00000000, 0x00000000 }
    },
    .data = {
        0xc0a80102, 0x80114dcc, 0x698f0000, 0x4500003e, 
        0xb2d00100, 0x002a2343, 0x0a980035, 0xc0a80101,
        0x09637962, 0x03736970, 0x00000000, 0x00010000,   
        0x00010001, 0x6b000001, 0x74790264, 0x65726369,   
        }
};
struct outbound testcase_2_out = {
    .data = {
        0xeb8df304, 0x6e3a65be, 0xb07c72c1, 0xdeb22cd9,   
        0x114d2a5c, 0x1ba76d5d, 0x33ae530f, 0xa5a5897d,   
        0xec416642, 0x51330d0e, 0xc10e9a4f, 0x3de81827,   
        0x5d918bd1, 0xec3b9ba9, 0xb47e48a4, 0xcfbb85a5,   
        0x5a41ad4a, 0xc309e9d8, 0xa8bc6ee4, 0x83b70d3a   
        }
};

struct inbound testcase_3_in = {
    .crypto_dsc = {
        .info = {
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000, 
            .free_space_ = 0,
            .direction   = 1,
            .length = 0b00000110100,
            .aadsize = 8,
            .keysize = 2
        },
        .icv  = {0x15956c96, 0x824b4c49, 0xcfdbf831, 0xb335f4ee},
        .key  = {0x78899aab, 0x34455667, 0xf0011223, 0xabbccdde,    
                 0x78899aab, 0x34455667, 0xf0011223, 0xabbccdde},
        .iv   = {
            .nonce = 0x11223344, 
            .iv[1] = 0x01020304, 
            .iv[0] = 0x05060708, 
            .tail = 0x00000001
        },
        .aad  = {0x00000002, 0x4a2cbfe3, 0x00000000, 0x00000000}
    },
    .data = {
        0xc0a80102, 0x80062690, 0x69a64000, 0x45000030,   
        0x00000000, 0x2dc57ee0, 0x0a9e008b, 0x9389155e,   
        0x01010402, 0x020405b4, 0x20bf0000, 0x70024000,   
        0x01020201
        }
};
struct outbound testcase_3_out = {
    .data = {
        0x0194e00d, 0x7a3bcd51, 0x724599df, 0xff425c9b,   
        0x65a5d763, 0x06efae9d, 0x1b0b1cbf, 0x6a78107f,   
        0x9f14e99d, 0x7f054565, 0x85771d34, 0x748a6379,   
        0x00000000, 0x00000000, 0x00000000, 0xef842d8e,   
        0x15956c96, 0x824b4c49, 0xcfdbf831, 0xb335f4ee,  
        }
};

struct inbound testcase_4_in = {
    .crypto_dsc = {
        .info = {
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000, 
            .free_space_ = 0,
            .direction   = 1,
            .length = 0b00001000000,
            .aadsize = 8,
            .keysize = 0
            },
        .icv  = {0x4799c71d, 0x8ad2b69e, 0xeeb096e9, 0xf821d496},
        .key  = {0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                 0x00000000, 0x00000000, 0x00000000, 0x00000000},
        .iv   = {
            .nonce = 0x00000000, 
            .iv[1] = 0x00000000, 
            .iv[0] = 0x00000000, 
            .tail = 0x00000001
        },
        .aad  = {0x00000001, 0x00000000, 0x00000000, 0x00000000}
    },
    .data = {
        0x40679318, 0x8001cb7a, 0x99c50000, 0x4500003c, 
        0x61626364, 0x02004400, 0x0800075c, 0x01010101,   
        0x71727374, 0x6d6e6f70, 0x696a6b6c, 0x65666768,   
        0x01020201, 0x66676869, 0x62636465, 0x75767761   
        }
};
struct outbound testcase_4_out = {
    .data = {
        0x31d56d60, 0x732909c3, 0xf973a392, 0x4688daf2,   
        0xf5e9a284, 0xf5fdcdff, 0x414b5e7f, 0xf694abaa,   
        0xa1e1d894, 0x4de7d9dc, 0x2719ffb6, 0x45647649,   
        0xc8c18af3, 0x1d19d4d5, 0x73ed4d18, 0xbc3bd578,   
        0x4799c71d, 0x8ad2b69e, 0xeeb096e9, 0xf821d496   
        }
};

struct inbound testcase_5_in = {
    .crypto_dsc = {
        .info = {
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000, 
            .free_space_ = 0, 
            .direction   = 1,
            .length = 0b00001000000,
            .aadsize = 12,
            .keysize = 0
        },
        .icv  = {0xe80d6e48, 0xeae88f00, 0x978ecafa, 0xe0d731cc},
        .key  = {0x00000000, 0x00000000, 0x00000000, 0x00000000,
                 0x607eae1f, 0x1988d0c3, 0xb388e649, 0x3de09874   
                 },
        .iv   = {
            .nonce = 0x57690e43, 
            .iv[1] = 0x4e280000, 
            .iv[0] = 0xa2fca1a3, 
            .tail  = 0x00000001
        },
        .aad  = {0x10101010, 0x10101010, 0x42f67e3f, 0x00000000}
    },
    .data = {
        0x40679318, 0x8001cb7c, 0x99c30000, 0x4500003c, 
        0x61626364, 0x02004300, 0x0800085c, 0x01010101,   
        0x71727374, 0x6d6e6f70, 0x696a6b6c, 0x65666768,   
        0x01020201, 0x66676869, 0x62636465, 0x75767761   
        }
};
struct outbound testcase_5_out = {
    .data = {
        0x86dd83b0, 0xf22cb10d, 0x853cf9f0, 0xfba2caa4,   
        0x9c357917, 0x0d1138ec, 0xcf1a04b0, 0xfec75691,   
        0xba487bc9, 0x5bf9fe3f, 0x01ad7984, 0x65acbd87,   
        0x9530320a, 0x1f5e2273, 0x2b4c8d0d, 0x1755e666,   
        0xe80d6e48, 0xeae88f00, 0x978ecafa, 0xe0d731cc   
        }
};

struct inbound testcase_6_in = {
    .crypto_dsc = {
        .info = {
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000, 
            .free_space_ = 0,
            .direction   = 1,
            .length = 0b00000011100,
            .aadsize = 12,
            .keysize = 0
        },
        .icv  = {0xe7d05d35, 0x95f112e4, 0x35e034be, 0x369f071f},
        .key  = {0x00000000, 0x00000000, 0x00000000, 0x00000000,
                 0x607eae1f, 0x1988d0c3, 0xb388e649, 0x3de09874    
                 },
        .iv   = {
            .nonce = 0x57690e43, 
            .iv[1] = 0x4e280000, 
            .iv[0] = 0xa2fca1a3, 
            .tail  = 0x00000001
            },
        .aad  = {0x10101010, 0x10101010, 0x42f67e3f, 0x00000000}
    },
    .data = {
        0x406793b6, 0x8001441f, 0x42a20000, 0x4500001c,   
        0x01020201, 0x0a00f5ff, 0xe0000002  
        }
};
struct outbound testcase_6_out = {
    .data = {
        0x86dd831e, 0xf22c3e6e, 0x5e5df9f0, 0xfba2ca84,   
        0x00000000, 0x0e1379ed, 0xcd1af913, 0x1fc65792,   
        0xe7d05d35, 0x95f112e4, 0x35e034be, 0x369f071f   
        }
};

struct inbound testcase_7_in = {
    .crypto_dsc = {
        .info = {
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000, 
            .free_space_ = 0,
            .direction   = 1,
            .length = 0b00000101000,
            .aadsize = 8,
            .keysize = 1
        },
        .icv  = {0x4cd7a636, 0x18027b5b, 0x52037f53, 0x95457b96},
        .key  = {0x00000000, 0x00000000, 0x8665731c, 0xfeffe992,  
                 0x67308308, 0x6d6a8f94, 0x8665731c, 0xfeffe992},
        .iv   = {
            .nonce = 0xcafebabe, 
            .iv[1] = 0xfacedbad, 
            .iv[0] = 0xdecaf888, 
            .tail  = 0x00000001
        },
        .aad  = {0x0000000a, 0x0000a5f8, 0x00000000, 0x00000000}
    },
    .data = {
        0x0a01038f, 0x40067880, 0xa4ad4000, 0x45000028,  
        0xdd6bb03e, 0xcb712602, 0x802306b8, 0x0a010612,     
        0x75680001, 0x501016d0 
        }
};
struct outbound testcase_7_out = {
    .data = {
        0x22de0242, 0x0e598b81, 0x6029aea4, 0xa5b1f806,   
        0x5bfbdbd0, 0x87b8858b, 0x33f828e6, 0x0938b3ab,   
        0x00000000, 0x00000000, 0x2144cc77, 0x315b2745,   
        0x4cd7a636, 0x18027b5b, 0x52037f53, 0x95457b96   
        }
};

struct inbound testcase_8_in = {
    .crypto_dsc = {
        .info = {
            .free_space[0] = 0x00000000, 
            .free_space[1] = 0x00000000, 
            .free_space[2] = 0x00000000, 
            .free_space_ = 0,
            .direction   = 1,
            .length = 0b00000101000,
            .aadsize = 8,
            .keysize = 0
            },
        .icv  = {0x69623436, 0xff170157, 0x5f354f75, 0x651f57e6},
        .key  = {0x00000000, 0x00000000, 0x00000000, 0x00000000,
                 0x78899aab, 0x34455667, 0xf0011223, 0xabbccdde},
        .iv   = {
            .nonce = 0xdecaf888, 
            .iv[1] = 0xcafedeba, 
            .iv[0] = 0xceface74, 
            .tail  = 0x00000001
        },
        .aad  = {0x00000001, 0x00000000, 0x00000100, 0x00000000 }
    },
    .data = {
        0xc3fb1d10, 0x7f119106, 0x33ba0000, 0x45000049,   
        0x800302d5, 0x0035dd7b, 0xc02831ce, 0xc2b1d326,   
        0x91baa047, 0xd75b81dc, 0x001e8c18, 0x00004e20,   
        0xc046ec95, 0x92c963ba, 0xb280389d, 0x6b91b924,  
        0x23010101, 0x4722b149, 0x9b6266c0  
        }
};
struct outbound testcase_8_out = {
    .data = {
        0x1f73d814, 0xb2a2ea90, 0xf72cbf4a, 0x18a6fd42,   
        0xfbec168f, 0xc349c1d2, 0xd95412e1, 0xe3e7f243,   
        0x965d7472, 0x84e65863, 0xaf2cb019, 0x9190feeb,   
        0x0f496c22, 0x1f0d2f0e, 0xe0e78019, 0xb79da345,   
        0x00000000, 0xe7845d68, 0x6f2127b2, 0x7db35724,    
        0x69623436, 0xff170157, 0x5f354f75, 0x651f57e6   
        }
};

#endif // !__crypto_testcases__