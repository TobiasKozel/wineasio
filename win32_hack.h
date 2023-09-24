#ifndef _WIN32_HACK
#define _WIN32_HACK

/* ASIO drivers (breaking the COM specification) use the Microsoft variety of
 * thiscall calling convention which gcc is unable to produce.  These macros
 * add an extra layer to fixup the registers. Borrowed from config.h and the
 * wine source code.
 */

/* From config.h */
#define __ASM_DEFINE_FUNC(name,suffix,code) asm(".text\n\t.align 4\n\t.globl " #name suffix "\n\t.type " #name suffix ",@function\n" #name suffix ":\n\t.cfi_startproc\n\t" code "\n\t.cfi_endproc\n\t.previous");
#define __ASM_GLOBAL_FUNC(name,code) __ASM_DEFINE_FUNC(name,"",code)
#define __ASM_NAME(name) name
#define __ASM_STDCALL(args) ""

/* From wine source */
#ifdef __i386__  /* thiscall functions are i386-specific */

	#define THISCALL(func) __thiscall_ ## func
	#define THISCALL_NAME(func) __ASM_NAME("__thiscall_" #func)
	#define __thiscall __stdcall
	#define DEFINE_THISCALL_WRAPPER(func,args) \
		extern void THISCALL(func)(void); \
		__ASM_GLOBAL_FUNC(__thiscall_ ## func, \
						  "popl %eax\n\t" \
						  "pushl %ecx\n\t" \
						  "pushl %eax\n\t" \
						  "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )

	#ifndef HIDDEN
		/* Hide ELF symbols for the COM members - No need to to export them */
		#define HIDDEN __attribute__ ((visibility("hidden")))
	#endif

	/*
	 * thiscall wrappers for the vtbl (as seen from app side 32bit)
	 */

	HIDDEN void __thiscall_Init(void);
	HIDDEN void __thiscall_GetDriverName(void);
	HIDDEN void __thiscall_GetDriverVersion(void);
	HIDDEN void __thiscall_GetErrorMessage(void);
	HIDDEN void __thiscall_Start(void);
	HIDDEN void __thiscall_Stop(void);
	HIDDEN void __thiscall_GetChannels(void);
	HIDDEN void __thiscall_GetLatencies(void);
	HIDDEN void __thiscall_GetBufferSize(void);
	HIDDEN void __thiscall_CanSampleRate(void);
	HIDDEN void __thiscall_GetSampleRate(void);
	HIDDEN void __thiscall_SetSampleRate(void);
	HIDDEN void __thiscall_GetClockSources(void);
	HIDDEN void __thiscall_SetClockSource(void);
	HIDDEN void __thiscall_GetSamplePosition(void);
	HIDDEN void __thiscall_GetChannelInfo(void);
	HIDDEN void __thiscall_CreateBuffers(void);
	HIDDEN void __thiscall_DisposeBuffers(void);
	HIDDEN void __thiscall_ControlPanel(void);
	HIDDEN void __thiscall_Future(void);
	HIDDEN void __thiscall_OutputReady(void);
#else /* __i386__ */

#define THISCALL(func) func
#define THISCALL_NAME(func) __ASM_NAME(#func)
#define __thiscall __stdcall
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */

#endif /* __i386__ */

#endif // _WIN32_HACK