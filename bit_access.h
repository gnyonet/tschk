#ifndef __BIT_ACCESS_H__
#define __BIT_ACCESS_H__

#include "stdmacros.h"
#include <stdint.h>

#define CEIL_LOG2I(s,d) for(d=1; (s-1)>>d; d++)
#define LOOR_LOG2I(s,d) for(d=0; (s)>>(d+1); d++)

#ifdef __cplusplus
extern "C" {
#endif

INLINE int bit_read64(const void * vp, int * byte, int * bit, int64_t * val, int len)
{
	int read=len;
	const unsigned char * p=(const unsigned char *)vp;
	p+=*byte;
	*val=0;
	while(len>0)
	{
		if(8-*bit<=len)
		{
			*val|=(*p&((1<<(8-*bit))-1))<<(len-(8-*bit));
			len-=8-*bit;
			*bit=0, ++*byte, ++p;
#ifdef _FOR_H264_RBSP
			if(*byte>=2 && *(p-0)==3 && *(p-1)==0 && *(p-2)==0)
			{
				++*byte, ++p;
			}
#endif
		}
		else
		{
			*val|=(*p>>((8-*bit)-len))&((1<<len)-1);
			*bit+=len;
			break;
		}
	}
	return read;
}
INLINE int bit_read(const void * vp, int * byte, int * bit, int * val, int len)
{
	int64_t res;
	int ret;
	ret=bit_read64(vp,byte,bit,&res,len);
	*val=res;
	return ret;
}

INLINE int gol_read64(const void * vp, int * byte, int * bit, int64_t * val)
{
	int read=0;
	int len=0;
	int64_t tmp;
	while(1)
	{
		read+=bit_read64(vp,byte,bit,&tmp,1);
		if(tmp==1)
			break;
		len++;
	}
	read+=bit_read64(vp,byte,bit,&tmp,len);
	*val=(1<<len)+tmp-1;

	return read;
}
INLINE int gol_read(const void * vp, int * byte, int * bit, int * val)
{
	int64_t res;
	int ret;
	ret=gol_read64(vp,byte,bit,&res);
	*val=res;
	return ret;
}
INLINE int seg_read64(const void * vp, int * byte, int * bit, int64_t * val)
{
	int read = gol_read64(vp,byte,bit,val);
	*val=(((*val)&1)?1:-1)*((*val+1)>>1);

	return read;
}
INLINE int seg_read(const void * vp, int * byte, int * bit, int * val)
{
	int64_t res;
	int ret;
	ret=seg_read64(vp,byte,bit,&res);
	*val=res;
	return ret;
}

INLINE int bit_write64(void * vp, int * byte, int * bit, int64_t val, int len)
{
	int write=0;
	unsigned char * p=(unsigned char*)vp;
	p+=*byte;
	while(len>0)
	{
		int left=8-*bit;
		if(left<=len)
		{
			const int mask=(1<<left)-1;
			*p&=~mask;	/* 上書きに対応する */
			*p|=(val>>(len-left))&mask;
			len-=left;
			*bit=0;
#ifdef _FOR_H264_RBSP
			if(*byte>=2 && *(p-0)<=3 && *(p-1)==0 && *(p-2)==0)
			{
				*(p+1)=*(p-0);
				*(p-0)=0x03;
				p++;
				*byte+=1;
			}
#endif
			p++;
			*byte+=1;
		}
		else
		{
			const int mask=(1<<len)-1;
			const int shift=8-*bit-len;
			*p&=~(mask<<shift);	/* 上書きに対応する */
			*p|=(val&mask)<<shift;
			*bit+=len;
			break;
		}
	}
	return write;
}

INLINE int bit_write(void * vp, int * byte, int * bit, int val, int len)
{
	return bit_write64(vp,byte,bit,val,len);
}

/* 指数ゴロム書き込み */
INLINE int gol_write64(void * vp, int * byte, int * bit, int64_t val)
{
	int write=0;
	int len=0;
	LOOR_LOG2I(val+1,len);

	write+=bit_write64(vp,byte,bit,0,len);
	write+=bit_write64(vp,byte,bit,val+1,len+1);

	return write;
}
INLINE int gol_write(void * vp, int * byte, int * bit, int val)
{
	return gol_write64(vp,byte,bit,val);
}
INLINE int seg_write64(void * vp, int * byte, int * bit, int64_t val)
{
	int lastbit=0;
	if(val>=0)
		lastbit=1;
	else
		val=-val;
	if(val>0)
	{
		val<<=1;
		val -=1;
		val  =(val&~0x01)|lastbit;
	}
	return gol_write64(vp,byte,bit,val);
}
INLINE int seg_write(void * vp, int * byte, int * bit, int val)
{
	return seg_write64(vp,byte,bit,val);
}

#ifdef __cplusplus
}/*extern "C"*/
#endif
#endif/*__BIT_ACCESS_H__*/
