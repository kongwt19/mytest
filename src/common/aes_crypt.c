#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "aes.h"

#include "platform.h"

// Enable ECB, CTR and CBC mode. Note this can be done before including aes.h or at compile-time.
// E.g. with GCC by using the -D flag: gcc -c aes.c -DCBC=0 -DCTR=1 -DECB=1
#define CBC 1
#define CTR 1
#define ECB 1

int ch_aes_encrypt(unsigned char *in, int inlen, const unsigned char *key,unsigned int keylen, unsigned char *iv, unsigned int ivlen, unsigned char **out, int *out_len)
{
	unsigned char *new_in = NULL;
	unsigned char *p;
	int i;
	int ac_inlen, pad_len;
	if (in == NULL || key == NULL || iv == NULL || 16 != keylen || 16 != ivlen) {
		return -1;
	}
	pad_len = 16 - (inlen % 16);
	ac_inlen = inlen + pad_len;
	new_in = (unsigned char *)malloc(ac_inlen);
	if(NULL == new_in)
	{
		printf("new_in malloc error(%d)\r\n",ac_inlen);
		return -2;
	}
	*out = (unsigned char *)(unsigned char **)malloc(ac_inlen);
	if (NULL == *out) {
		free(new_in);
		new_in = NULL;
		printf("out malloc error\r\n");
		return -2;
	}
	p = *out;
	for (i = 0; i<inlen; i++)
		new_in[i] = in[i];
	//memcpy(new_in,in,inlen);
	for (i = 0; i<pad_len; i++) {
		new_in[inlen + i] = (unsigned char)pad_len;
	}

	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key, iv);
	AES_CBC_encrypt_buffer(&ctx, new_in, ac_inlen);
	*out_len = ac_inlen;
	for (i = 0; i < ac_inlen; i++)
	{
		p[i] = new_in[i];
	}
	free(new_in);
	return 0;
}
int ch_aes_decrypt(unsigned char *in, int inlen, const unsigned char *key,unsigned int keylen,unsigned char *iv, unsigned int ivlen, unsigned char **out, int *outlen)
{
	int ret = 0, temp;
	unsigned char *p;
	p = NULL;
	if (in == NULL || key == NULL || iv == NULL || (inlen % 16) || 16 != keylen || 16 != ivlen) {
		return -1;
	}
	*out = (unsigned char *)(unsigned char **)malloc(inlen);
	p = *out;
	if (NULL == *out) {
		return -2;
	}
	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key, iv);
	AES_CBC_decrypt_buffer(&ctx, in, inlen);
	int i;
	for (i = 0; i < inlen; i++)
	{
		p[i] = in[i];
	}
	temp = (int)p[inlen - 1];
	if (temp<1 || temp>16) {
		free(*out);
		*out = NULL;
		ret = -3;		
	}
	*outlen = inlen - temp;
	return ret;

}

int hex_encrypt(unsigned char *in, int inlen, unsigned char *out, int buffer_len, int *outlen) {
	int i;
	unsigned char *p;
	int olen = 2 * inlen;
	if (olen > buffer_len) return -1;
	p = out;
	for (i = 0; i < inlen; i++) {
		unsigned char c1 = *in++;
		unsigned char temp = (c1 >> 4) & 0x0F;
		if (temp<10) temp = 48 + temp;
		else temp = 87 + temp;
		*p++ = temp;
		temp = c1 & 0x0F;
		if (temp<10) temp = 48 + temp;
		else temp = 87 + temp;
		*p++ = temp;
	}
	*outlen = olen;
	return 0;
}

int hex_decrypt(unsigned char *in, int inlen, unsigned char *out, int buffer_len, int *outlen) {
	int i;
	unsigned char *p;
	p = out;
	int olen = inlen / 2;
	if (olen > buffer_len) return -1;
	for (i = 0; i < olen; i++) {
		unsigned char c1 = *in++;
		if (c1>47 && c1<58) c1 = c1 - 48;
		else if (c1>96 && c1<103) c1 = c1 - 87;
		else return -2;
		unsigned char c2 = *in++;
		if (c2>47 && c2<58) c2 = c2 - 48;
		else if (c2>96 && c2<103) c2 = c2 - 87;
		else return -3;
		*p++ = c1 * 16 + c2;
	}
	*outlen = olen;
	return 0;
}

