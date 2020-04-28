#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ipp_crypto.h"


/*int main()
{
	uint8_t *in = "hello"; //a4d4a2ed587540c429efdd80c4a3e22e
	int format = 2;     
	int inlen, outlen;	
	
	inlen = strlen(in);

	printf("yuanwen: %s\n", in);

	uint8_t *out = NULL;
	out = (uint8_t *)malloc( (inlen+1) * sizeof(uint8_t) );
	if(NULL == out)
	{
		return -1;
	}
		

    	ipp_aes_encrypt( in, inlen, &out, &outlen, format );
	printf("encrypt: %s\nlen: %d\n", out, outlen);


    	ipp_aes_decrypt(out,  outlen, &in, &inlen, &format);
	printf("decrypt: %s\nlen: %d\n", in, inlen);


	free(out);
	out = NULL;

	return 0;
}*/
