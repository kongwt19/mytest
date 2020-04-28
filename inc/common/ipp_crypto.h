#ifndef __IPP_CRYPTO_H__
#define __IPP_CRYPTO_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int ipp_aes_encrypt(uint8_t *in,int inlen,uint8_t** out,int *outlen,int format);
int ipp_aes_decrypt(uint8_t *in,int inlen,uint8_t** out,int *outlen,int *format);

#ifdef __cplusplus
}
#endif

#endif

