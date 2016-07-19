/*
 * 標準的なマクロの定義
 */
#ifndef __STDMACROS_H__
#define __STDMACROS_H__

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define RANGE(val,min,max) (((min)>(val))?(min):((max)<(val))?(max):(val))

#define INLINE static __attribute__((always_inline)) __attribute__((unused))

#endif/*__STDMACROS_H__*/
