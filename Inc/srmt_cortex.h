#ifndef srmt_cortex_h
#define srmt_cortex_h
#include "srmt_types.h"
#include "rtx_os.h"
#include "RTE_Components.h"             /* Component selection */
#include CMSIS_device_header
#pragma no_anon_unions

typedef struct
{
	SCB_Type * scb;
	SysTick_Type * sys_tick;
	NVIC_Type * nvic;
	ITM_Type * itm;	
	DWT_Type * dwt;
	TPI_Type * tpi;
	CoreDebug_Type *core_debug;
#if (defined (__FPU_USED   ) && (__FPU_USED    == 1U))  
	FPU_Type *fpu;
#endif	
}OS_CORE;

#pragma anon_unions

typedef union
{
 struct
 {
  OS_VARIANT32 sp,pc;
 };
 struct
 {
  U32 SP,PC;
 };
}OS_CONTEXT;

typedef union
{
 U32 l;
 struct
 {
  U32 isr_number:9;
  U32 align8:1;
  U32 ICI:6;
  U32 GE:4;
  U32 :7;
  U32 Q:1;
  U32 V:1;
  U32 C:1;
  U32 N:1;
  U32 Z:1;
 };
}OS_XPSR;

typedef union
{
	U32 reg[8];
	struct
	{
	OS_VARIANT32 r0,r1,r2,r3,r12,lr,pc;
 OS_XPSR xpsr;
	};
}OS_ISR_CONTEXT;

typedef union
{
 struct
 {
  U32 reg[8];
  float regf[16];
 }; 
	struct
	{
	OS_VARIANT32 r0,r1,r2,r3,r12,lr,pc,xpsr;
 float s[16];
 OS_VARIANT32 fpscr; 
 U32 dummy; 
	};
}OS_ISR_EXTENDED_CONTEXT;

typedef union
{
	OS_VARIANT32 reg[8];
	struct
	{
	OS_VARIANT32 r4,r5,r6,r7,r8,r9,r10,r11;
	OS_VARIANT32 s[16];
	};
}OS_THREAD_EXTENDED_CONTEXT;

typedef union
{
	OS_VARIANT32 reg[8];
	struct
	{
	OS_VARIANT32 r4,r5,r6,r7,r8,r9,r10,r11;
	};
}OS_THREAD_CONTEXT;

typedef union
{
 U32 l;
 struct
 {
  U8 :2;
  U8 psp:1;
  U8 thread_mode:1;
  U8 no_fpu:1;
  U8 :3;
 };
}OS_CORTEX_EXCEPTION_BEHAVIOR;

typedef enum
{
 EX_NO=0,
 EX_FLAG,
 EX_HARD_FAULT,
 EX_NOT_ENOUGH_MEMORY,
 EX_MUTEX_NOT_CREATED,
 EX_MUTEX_NOT_INIT,
 EX_MUTEX_TIMEOUT,
 EX_EVENT_NOT_CREATED,
 EX_THREAD_NOT_CREATED,
 EX_QUEUE_NOT_CREATED,
 
 EX_END,
}OS_CORTEX_EXCEPTION;

typedef struct OS_RTOS2_TRY_TREE
{
 struct OS_RTOS2_TRY_TREE *next;
 struct OS_RTOS2_TRY_TREE *pred;
 OS_CONTEXT ct;
 U8* function;
 U8 atomic_context_lifo_level;
 OS_VARIANT32 src;
}OS_RTOS2_EXCEPTION_TREE;

typedef struct
{
 OS_RTOS2_EXCEPTION_TREE *raise;
 OS_RTOS2_EXCEPTION_TREE *pred;
 OS_RTOS2_EXCEPTION_TREE *deque;
}OS_RTOS2_EXCEPTION; 

typedef struct
{
 U8 name[16];
 OS_RTOS2_EXCEPTION exception;
 OS_VARIANT32 param;
 struct
 {
  U32 msp,psp,xpsr;
  U8 lr,lifo_level;
 }atomic_context;
}OS_RTOS2_EXTENDED_DATA;

typedef __packed struct
{
 U32 old_value;
 U8 overrun;
}OS_CORTEX_TIMESTAMP;

#define OS_CORTEX_EXCEPTION_FLAG (1<<30)

#define DECLARE_THREAD_ATTR(name1,size,prio) __align(8) U8 os_##name1##_thread_stack[size];\
OS_RTOS2_EXTENDED_DATA os_##name1##_thread_edata;\
const osThreadAttr_t os_##name1##_thread_attr=\
{\
 .name=OS_VOID(&os_##name1##_thread_edata),\
	.stack_mem=os_##name1##_thread_stack,\
 .priority=prio,\
	.stack_size=sizeof(os_##name1##_thread_stack)}\

#define EXTERN_DECLARE_THREAD_ATTR(name) extern U8 os_##name##_thread_stack[];\
extern OS_RTOS2_EXTENDED_DATA os_##name##_thread_edata;\
extern const osThreadAttr_t os_##name##_thread_attr;\

#define ATOMIC(a) os_atomic();\
                  {a;}\
                  os_deatomic()\
                  
extern OS_CORTEX_TIMESTAMP os_cortex_timestamp;
extern const OS_CORE os_core;

extern U32 os_timeout_start(U32 tval);
extern U32 os_timeout_check(U32* tout,U32 tval);
extern U32 os_cycle_start(U32 cval);
extern U32 os_cycle_check(U32* cc,U32 cval);
extern void os_cycle_count(U32 *cc);
extern void os_cycle_pause(U32 cycles);

extern U32 os_timestamp_get(void);

extern U32 os_cortex_try(void);
extern void os_cortex_raise( U8* function,OS_CORTEX_EXCEPTION exception,U32 src);
#define RAISE(a,b) os_cortex_raise(OS_VOID(__FUNCTION__),(OS_CORTEX_EXCEPTION)(a),(U32)(b))

extern U32 os_cortex_wait_flag(osRtxEventFlags_t* ef,U32 f,U32 tout);

extern U32 os_setjmp(OS_CONTEXT* ct,OS_THREAD_CONTEXT* tct);
extern void os_longjmp(OS_CONTEXT* ct,OS_THREAD_CONTEXT* tct,U32 branch);
extern void os_thread_goto(osRtxThread_t* th,OS_CONTEXT* ct,OS_THREAD_CONTEXT* tct,U8 branch);
extern void os_return_from_hardware_exception(OS_CONTEXT* c,U8 branch);
extern U32 os_atomic(void);
extern U32 os_deatomic(void);
#endif
