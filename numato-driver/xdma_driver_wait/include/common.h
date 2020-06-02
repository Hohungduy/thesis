#ifndef _COMMON_
#define _COMMON_

#include "mycrypto.h"
#include "cipher.h"
/* struct mycrypto_context
*  -  @mycrypto_cipher_op: contains the transformation objects 
*   (it is stored in tfm ->__crt_ctx and tfm = req->base.tfm )
*  -  @mycrypto_cipher_req: contains the context of request 
*   which is stored in req ->_ctx
*  - error_code: contains bits which represents for error code.

*/
struct mycrypto_context{
    struct mycrypto_cipher_op ctx; 
    struct mycrypto_cipher_req req_ctx;
    u32 error_code:7;
};


//debug
#endif
