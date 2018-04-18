/**
 * \file
 * Low-level TLS support
 *
 * Author:
 *	Rodrigo Kumpera (kumpera@gmail.com)
 *
 * Copyright 2011 Novell, Inc (http://www.novell.com)
 * Copyright 2011 Xamarin, Inc (http://www.xamarin.com)
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#ifndef __MONO_TLS_H__
#define __MONO_TLS_H__

#include <config.h>
#include <glib.h>

/* TLS entries used by the runtime */
typedef enum {
	/* mono_thread_internal_current () */
	TLS_KEY_THREAD = 0,
	TLS_KEY_JIT_TLS = 1,
	/* mono_domain_get () */
	TLS_KEY_DOMAIN = 2,
	TLS_KEY_SGEN_THREAD_INFO = 3,
	TLS_KEY_LMF_ADDR = 4,
	TLS_KEY_NUM = 5
} MonoTlsKey;

#ifdef HAVE_KW_THREAD
#define USE_KW_THREAD
#endif

#ifdef HOST_WIN32

#include <windows.h>

// Method working for both dynamic and static libraries to get a callback
// called similar to DLLMain but works for all scenarios. It should be possible to
// use this as a trigger to emulate pthread destructors on windows using VC compiler.

// For C++11 it is possible to use thread_local and RAII to complete similar. This will be portable
// between Windows C++ compilers as well, but then we will need to include C++11 sources and depend on C++
// compiler for that source file.

// If below works as expected, the /TLS switch on dumpbin should display callback in table:

//TLS Callbacks
//
//Address
//----------------
//000000014001156E  @ILT+1385(__dyn_tls_init)
//0000000140019250
//0000000140011631  @ILT+1580(tls_callback_test_func)
//0000000000000000

//extern "C"
//void NTAPI tls_callback_test_func (PVOID DllHandle, DWORD dwReason, PVOID)
//{
//	char outputBuffer[255];
//	sprintf_s (outputBuffer, "tls_callback_test_func called with dwReason = %d\n", dwReason);
//	OutputDebugStringA (outputBuffer);
//}
//
//#pragma comment (linker, "/INCLUDE:_tls_used")
//
//#pragma const_seg(".CRT$XLF")
//
//extern "C" const PIMAGE_TLS_CALLBACK tls_callback_test = tls_callback_test_func;
//
//#pragma const_seg()

// Add method where we can store the key + destructor.
// Use static declspec(thread), we could probably use a small set of array for this. Mono currently needs 2.
// Should it be a fixed size array of destructor slots?
//

typedef void (*TlsKeyDestructorFuncPtr)(DWORD);

typedef struct _TlsKeyDestructor {
	TlsKeyDestructorFuncPtr destructor_ptr;
	DWORD destructor_data;
} TlsKeyDestructor;

#define MAX_TLS_DESTRUCTORS 16

static __declspec(thread) TlsKeyDestructor tls_key_desctructors[MAX_TLS_DESTRUCTORS];

#define MonoNativeTlsKey DWORD
#define mono_native_tls_alloc(key,destructor) ((*(key) = TlsAlloc ()) != TLS_OUT_OF_INDEXES && destructor == NULL)
#define mono_native_tls_free TlsFree
#define mono_native_tls_set_value TlsSetValue
#define mono_native_tls_get_value TlsGetValue

#else

#include <pthread.h>

#define MonoNativeTlsKey pthread_key_t
#define mono_native_tls_get_value pthread_getspecific

static inline int
mono_native_tls_alloc (MonoNativeTlsKey *key, void *destructor)
{
	return pthread_key_create (key, (void (*)(void*)) destructor) == 0;
}

static inline void
mono_native_tls_free (MonoNativeTlsKey key)
{
	pthread_key_delete (key);
}

static inline int
mono_native_tls_set_value (MonoNativeTlsKey key, gpointer value)
{
	return !pthread_setspecific (key, value);
}

#endif /* HOST_WIN32 */

void mono_tls_init_gc_keys (void);
void mono_tls_init_runtime_keys (void);
void mono_tls_free_keys (void);
gint32 mono_tls_get_tls_offset (MonoTlsKey key);
gpointer mono_tls_get_tls_getter (MonoTlsKey key, gboolean name);
gpointer mono_tls_get_tls_setter (MonoTlsKey key, gboolean name);

gpointer mono_tls_get_thread (void);
gpointer mono_tls_get_jit_tls (void);
gpointer mono_tls_get_domain (void);
gpointer mono_tls_get_sgen_thread_info (void);
gpointer mono_tls_get_lmf_addr (void);

void mono_tls_set_thread (gpointer value);
void mono_tls_set_jit_tls (gpointer value);
void mono_tls_set_domain (gpointer value);
void mono_tls_set_sgen_thread_info (gpointer value);
void mono_tls_set_lmf_addr (gpointer value);

#endif /* __MONO_TLS_H__ */
