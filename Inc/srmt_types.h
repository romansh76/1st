#ifndef srmt_types_H
#define srmt_types_H
#include <stdarg.h>

#ifdef __ARMCC_VERSION 
typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef signed short S16;
typedef unsigned int U32;
typedef signed int S32;
typedef unsigned long long U64;
typedef signed long long S64;
typedef float F32;
typedef double F64;

typedef U32 (*OS_VOIDF)(void);

#define OS_ADDR(a) __attribute__((at(a)))
#define __offset(struc,member) (__cpp(offsetof(struc, member)))
#define __size(a) __cpp(sizeof(a))

extern U32 Region$$Table$$Base[];
#endif

#define OS_TYPE(type,var) ((type)(var))
#define OS_VOID(var)     ((void*)(var))
#define OS_PROC_FRAME_BEGIN typedef __packed struct {

#define OS_PROC_FRAME_WATCH 1
#if (OS_PROC_FRAME_WATCH==1)
#define OS_PROC_FRAME_END }__LV; __LV *__lv; __lv=param;
#define OS_PROC_FRAME_ENDP }*__LV; __LV *__lv; __lv=param;
#define OS_LOCAL  (__lv[0])
#else
#define OS_PROC_FRAME_END }__LV;
#define OS_PROC_FRAME_ENDP }*__LV; 
#define OS_LOCAL  (((__LV*)(param))[0])
#endif

typedef union
{
	U8 *pchar;
	U16 *pword;
	U32 *plong;
	F32 *pfloat;
	void *pvoid;
	U8 uchar;
	U16 uword;
	U32 ulong;
	S8 schar;
	S16 sword;
	S32 slong;
	F32 vfloat;
	U8 auchar[4];
	U16 auword[2];
	U32 aulong[1];	
	S8 aschar[4];
	S16 asword[2];
	S32 aslong[1];
 va_list px;
}OS_VARIANT32;

typedef union
{
	U8 uchar;
	U16 uword;
	U8 auchar[2];
	U16 auword[1];
	S8 schar;
	S16 sword;
	S8 aschar[2];
	S16 asword[1];
}OS_VARIANT16;

typedef struct
{
 U32 size;
 U8 data[1];
}OS_STANDART_RESOURCE_DESC;

typedef struct
{
 OS_VARIANT32 key,value;
}OS_HASH_TABLE;


typedef enum
{
	OS_WRITE=0,
	OS_READ,
 OS_DISABLE=0,
 OS_ENABLE,
 OS_RIGHT_JUSTIFY=0,
 OS_LEFT_JUSTIFY,
 OS_RESET=0,
 OS_SET,
 OS_LED_NO=0,
 OS_LED_RED,
 OS_LED_GREEN,
 OS_LED_YELLOW,
}OS_SRMT_ENUM;

#endif
