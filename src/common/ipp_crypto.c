#include "ipp_defines.h"
#include "aes.h"
#include "platform.h"
#include "ipp_crypto.h"

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

uint8_t lbla(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=(in1*in2)*10+in1-in2;
	return hehe;
}
uint8_t lala(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=(in1*in2+in1-in2)*10+2*in1-in2;
	return hehe;
}
uint8_t l9la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=in1*in2+in1-in2;
	return hehe;
}
uint8_t l8la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=in1*in2*10+in1;
	return hehe;
}
uint8_t l7la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=in1*in2*10+in1+in2;
	return hehe;
}
uint8_t l6la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=in1*in2*(in1-in2)*in2;
	return hehe;
}

uint8_t l0la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=in1*in1*10+in2*in2;
	return hehe;
}

uint8_t l1la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=in1*in1*10+in2;
	return hehe;
}

uint8_t l2la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=(in1*in1+in2)*10+in2;
	return hehe;
}

uint8_t l3la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=(in1*in1-in2)*10+in1;
	return hehe;
}


uint8_t l4la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=(in1*in2+in1-in2)*10+in1+in2;
	return hehe;
}

uint8_t l5la(uint8_t in1,uint8_t in2)
{
	static uint8_t hehe;
	hehe=0xff;
	hehe&=(in1*in2)*10-in1;
	return hehe;
}
/**********************************************************************************
 *Function   :aes_key
 *Description:Get the aes key
 *Param      :uint8_t *key  address to store key
              int len       key length
 *Return     :void
**********************************************************************************/
void aes_key(uint8_t *key, int len)
{
    if(16 != len)
    {
        return;
    }
	key[0]=l0la(IAMFULL,IAMBOY);
	key[1]=l0la(IAMSTRONG,IAMBOY);
	key[2]=l1la(IAMBOY,IAMGREAT);
	key[3]=l2la(IAMBOY,IAMFULL);
	key[4]=l3la(IAMFULL,IAMHANDSOME);
	key[5]=l4la(IAMFULL,IAMBOY);
	key[6]=l1la(IAMSTRONG,IAMSTRONG);
	key[7]=l1la(IAMSTRONG,IAMHANDSOME);
	key[8]=l0la(IAMLOVELY,IAMHANDSOME);
	key[9]=l6la(IAMFINE,IAMSTRONG);
	key[10]=l5la(IAMBOY,IAMGREAT);
	key[11]=l8la(IAMOK,IAMSTRONG);
	key[12]=l7la(IAMBOY,IAMLOVELY);
	key[13]=l9la(IAMFULL,IAMBOY);
	key[14]=lala(IAMFULL,IAMBOY);
	key[15]=lbla(IAMGREAT,IAMSTRONG);
}
/**********************************************************************************
 *Function   :aes_encrypt
 *Description:Encry a content by AES
 *Param      :uint8_t *in       input content
              int inlen         input content length
              uint8_t *out      encrypted output buffer, malloc by caller, the 
                                length must calculated by: (inlen + 16) * 2 + 1
              int *outlen       encrypted output buffer length   
              int geshi         format, not used
 *Return     :IPP_OK         success
              Others         Fail
**********************************************************************************/
int ipp_aes_encrypt(uint8_t *in, int inlen, uint8_t** out, int *outlen, int format)
{
	(void)format;
    int ret = -1;
    int encry_len = 0;
    unsigned char *encry = NULL;
    unsigned char ipp_val[16] =  {0};
    unsigned char ipp_vv[17] = "bothareengineers";

 //   ipp_LogV_Prefix("Encrypt message begin\r\n");
    
    if(in == NULL)
    {
    	return ret;
    }

    aes_key(ipp_val, sizeof(ipp_val));
    
    int len = (inlen + 16) * 2 + 1;

    ret = ch_aes_encrypt(in, inlen, ipp_val,16, ipp_vv,16, &encry, &encry_len);
    if(0 > ret)
    {
		FREE_POINTER(encry);
        ipp_LogE_Prefix("Encrypt failed(%d)!\r\n",ret);
        return -1;
    }
	*out = (uint8_t *)malloc(len);
	if(NULL == *out)
	{
		FREE_POINTER(encry);
		ipp_LogE_Prefix("malloc error!\r\n");
		return -1;
	}

    ret = hex_encrypt(encry, encry_len, *out, len, outlen);
    if(0 > ret)
    {
        ipp_LogE_Prefix("Hex failed!\r\n");
        FREE_POINTER(encry);
        return -1;
    }
    (*out)[*outlen] = '\0';

    FREE_POINTER(encry);
    
 //   ipp_LogV_Prefix("Encrypt message end\r\n");

    return 0;
}
/**********************************************************************************
 *Function   :aes_decrypt
 *Description:Decyrpt a content by AES
 *Param      :uint8_t *in       input content
              int inlen         input content length
              uint8_t *out      decrypted output buffer, malloc by caller, the 
                                length must calculated by: inlen + 1
              int *outlen       decrypted output buffer length   
              int *geshi         format, not used
 *Return     :IPP_OK         success
              Others         Fail
**********************************************************************************/
int ipp_aes_decrypt(uint8_t *in,int inlen,uint8_t** out,int *outlen,int *format)
{
	(void)format;
    int ret = -1;
    int len = 0;
    int decry_len = 0;
    unsigned char *decry = NULL;
    unsigned char ipp_val[16] =  {0};
    unsigned char ipp_vv[17] = "bothareengineers";

    //ipp_LogV_Prefix("Decrypt message begin\r\n");

    if(in == NULL)
    {
    	return ret;
    }

    aes_key(ipp_val, sizeof(ipp_val));
    
    len = inlen + 1;
	*out = (uint8_t *)malloc(len);
	if(NULL == *out)
	{
		ipp_LogE_Prefix("malloc error!\r\n");
		return -2;
	}
    
    ret = hex_decrypt((unsigned char *)in, inlen, (unsigned char *)*out, len, outlen);
    if(0 > ret)
    {
    	FREE_POINTER(*out);
        //ipp_LogE_Prefix("dec-%d: Hex failed!(%d)\r\n",  __LINE__,ret);
        return -3;
    }
   ret = ch_aes_decrypt(*out, *outlen, ipp_val,16, ipp_vv,16, &decry,&decry_len);
	if(0 > ret)
    {
    	FREE_POINTER(*out);
        ipp_LogE_Prefix("dec-%d: Decrypt failed!(%d)\r\n",  __LINE__,ret);
        return -3;
    }
    memset(*out, 0, len);
    memcpy(*out, decry, decry_len);
    *outlen = decry_len;

    FREE_POINTER(decry);
    
    return 0;
}

