/*
 * メッセージ表示ユーティリティのマクロ宣言
 */
#ifndef __MESSAGE_TRACE_H__
#define __MESSAGE_TRACE_H__
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS	/* C++でのPRId64に対応 */
#include <inttypes.h>
#ifdef __cplusplus
extern "C" {
#endif

#define _MACRO_TO_DATA(a) _DATA_TO_VAL(a)
#define _DATA_TO_VAL(a) #a
#define MESSAGE(...) fprintf(stdout,"#|"__FILE__":"_MACRO_TO_DATA(__LINE__)"|\t" __VA_ARGS__),fflush(stdout)
#define ERROR(...) fprintf(stdout,"#|"__FILE__":"_MACRO_TO_DATA(__LINE__)"|\tERROR:" __VA_ARGS__),fflush(stdout)
#define PERROR(a) ERROR(a" : %s(%d)\n", strerror(errno), errno)
#define DUMPI(a) MESSAGE(#a"=%"PRId64"\n",( int64_t)(a))
#define DUMPU(a) MESSAGE(#a"=%"PRIu64"\n",(uint64_t)(a))
#define DUMPX(a) MESSAGE(#a"=%"PRIx64"\n",(uint64_t)(a))
#define DUMPZ(a) MESSAGE(#a"=%zd\n",(a))
#define DUMPP(a) MESSAGE(#a"=%p\n",a)
#define DUMPD(a) MESSAGE(#a"=%f\n",a)
#define DUMPLI(a) DUMPI(a)
#define DUMPLU(a) DUMPU(a)
#define DUMPLX(a) DUMPX(a)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif/*__MESSAGE_TRACE_H__*/
