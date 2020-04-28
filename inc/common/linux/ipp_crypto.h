#ifndef __IPP_CRYPTO_H__
#define __IPP_CRYPTO_H__

#include "platform.h"

#define IAMLOVELY 0
#define IAMHANDSOME	1
#define IAMSTRONG 2
#define IAMBOY 3
#define IAMFULL 4
#define IAMFINE 5
#define IAMOK 6
#define IAMGREAT 7
#define IAMNICE 8
#define IAMJOY 9

#ifdef __cplusplus
extern "C" {
#endif

int aes_encrypt(uint8_t *in,int inlen,uint8_t *out,int *outlen,int format);
int aes_decrypt(uint8_t *in,int inlen,uint8_t *out,int *outlen,int *format);

#ifdef __cplusplus
}
#endif

#endif

