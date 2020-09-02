#ifndef _COMMON_
#define _COMMON_

#include "bkcrypto.h"
#include "cipher.h"
/* struct bkcrypto_context
*  -  @bkcrypto_cipher_op: contains the transformation objects 
*   (it is stored in tfm ->__crt_ctx and tfm = req->base.tfm )
*  -  @bkcrypto_cipher_req: contains the context of request 
*   which is stored in req ->_ctx
*  - error_code: contains bits which represents for error code.

*/
struct bkcrypto_context{
    struct bkcrypto_cipher_op ctx_op; 
    struct bkcrypto_cipher_req ctx_req;
    u32 error_code:7;
};


//debug
#endif
