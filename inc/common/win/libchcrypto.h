#ifndef __LIB_CHCRYPTO_H
#define __LIB_CHCRYPTO_H

/*
in  	明文
inlen 	明文长度/字节
key		密钥 16字节长度
iv		向量 16字节长度
out		密文
outlen	密文长度/字节
*/
int ch_aes_encrypt(const unsigned char *in, int inlen, const unsigned char *key,unsigned char *iv,unsigned char **out,int *out_len);

/*
in  	密文
inlen 	密文长度/字节
key		密钥 16字节长度
iv		向量 16字节长度
out		明文
outlen	明文长度/字节
*/
int ch_aes_decrypt(const unsigned char *in,int inlen, const unsigned char *key,unsigned char *iv,unsigned char **out,int *outlen);

//buffer_len	给out分配的长度/字节
int hex_encrypt(unsigned char *in, int inlen, unsigned char *out,int buffer_len, int *outlen);
int hex_decrypt(unsigned char *in, int inlen, unsigned char *out, int buffer_len, int *outlen);

#endif
