/*
 * mini-posix.c: POSIX signal handling support for Mono.
 *
 * Authors:
 *   Mono Team (mono-list@lists.ximian.com)
 *
 * Copyright 2001-2003 Ximian, Inc.
 * Copyright 2003-2008 Ximian, Inc.
 *
 * See LICENSE for licensing information.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#include <config.h>
#include <signal.h>
#include <math.h>

#include <mono/metadata/assembly.h>
#include <mono/metadata/loader.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/io-layer/io-layer.h>
#include "mono/metadata/profiler.h"
#include <mono/metadata/profiler-private.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/gc-internals.h>
#include <mono/metadata/threads-types.h>
#include <mono/metadata/verify.h>
#include <mono/metadata/verify-internals.h>
#include <mono/metadata/mempool-internals.h>
#include <mono/metadata/attach.h>
#include <mono/utils/mono-math.h>
#include <mono/utils/mono-compiler.h>
#include <mono/utils/mono-counters.h>
#include <mono/utils/mono-logger-internals.h>
#include <mono/utils/mono-mmap.h>
#include <mono/utils/dtrace.h>

#include "mini.h"
#include <string.h>
#include <ctype.h>
#include "trace.h"
#include "version.h"

#include "jit-icalls.h"

#if defined(HOST_WIN32) && G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)
#include <mmsystem.h>
#endif

void
mono_runtime_install_handlers (void)
{
#ifndef MONO_CROSS_COMPILE
	win32_seh_init();
	win32_seh_set_handler(SIGFPE, mono_sigfpe_signal_handler);
	win32_seh_set_handler(SIGILL, mono_sigill_signal_handler);
	win32_seh_set_handler(SIGSEGV, mono_sigsegv_signal_handler);
	if (mini_get_debug_options ()->handle_sigint)
		win32_seh_set_handler(SIGINT, mono_sigint_signal_handler);
#endif
}

void
mono_runtime_cleanup_handlers (void)
{
#ifndef MONO_CROSS_COMPILE
	win32_seh_cleanup();
#endif
}


/* mono_chain_signal:
 *
 *   Call the original signal handler for the signal given by the arguments, which
 * should be the same as for a signal handler. Returns TRUE if the original handler
 * was called, false otherwise.
 */
gboolean
MONO_SIG_HANDLER_SIGNATURE (mono_chain_signal)
{
	MonoJitTlsData *jit_tls = mono_native_tls_get_value (mono_jit_tls_id);
	jit_tls->mono_win_chained_exception_needs_run = TRUE;
	return TRUE;
}

static VOID
thread_timer_expired (HANDLE thread)
{
	CONTEXT context;

	context.ContextFlags = CONTEXT_CONTROL;
	if (GetThreadContext (thread, &context)) {
#ifdef _WIN64
		mono_profiler_stat_hit ((guchar *) context.Rip, &context);
#else
		mono_profiler_stat_hit ((guchar *) context.Eip, &context);
#endif
	}
}

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)
static MMRESULT	g_timer_event = 0;
static HANDLE g_timer_main_thread = INVALID_HANDLE_VALUE;

static VOID CALLBACK
timer_event_proc (UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	thread_timer_expired ((HANDLE)dwUser);
}

static VOID
stop_profiler_timer_event (void)
{
	if (g_timer_event != 0) {

		timeKillEvent (g_timer_event);
		g_timer_event = 0;
	}

	if (g_timer_main_thread != INVALID_HANDLE_VALUE) {

		CloseHandle (g_timer_main_thread);
		g_timer_main_thread = INVALID_HANDLE_VALUE;
	}
}

static VOID
start_profiler_timer_event (void)
{
	g_return_if_fail (g_timer_main_thread == INVALID_HANDLE_VALUE && g_timer_event == 0);

	TIMECAPS timecaps;

	if (timeGetDevCaps (&timecaps, sizeof (timecaps)) != TIMERR_NOERROR)
		return;

	if ((g_timer_main_thread = OpenThread (READ_CONTROL | THREAD_GET_CONTEXT, FALSE, GetCurrentThreadId ())) == NULL)
		return;

	if (timeBeginPeriod (1) != TIMERR_NOERROR)
		return;

	if ((g_timer_event = timeSetEvent (1, 0, (LPTIMECALLBACK)timer_event_proc, (DWORD_PTR)g_timer_main_thread, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS)) == 0) {
		timeEndPeriod (1);
		return;
	}
}

#else /* G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) */

static PTP_TIMER g_timer_event = NULL;
static HANDLE g_timer_main_thread = INVALID_HANDLE_VALUE;

static VOID CALLBACK
threadpool_timer_callback (PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer)
{
	thread_timer_expired ((HANDLE)context);
	return;
}

static VOID
stop_profiler_timer_event (void)
{
	if (g_timer_event != NULL) {

		// Cancel timer and any running callbacks and wait for pending callbacks to complete.
		SetThreadpoolTimer (g_timer_event, NULL, 0, 0);
		WaitForThreadpoolTimerCallbacks (g_timer_event, TRUE);

		// Now it's safe to close down the timer.
		CloseThreadpoolTimer (g_timer_event);
		g_timer_event = INVALID_HANDLE_VALUE;
	}

	if (g_timer_main_thread != INVALID_HANDLE_VALUE) {

		// Done with main thread as well.
		CloseHandle (g_timer_main_thread);
		g_timer_main_thread = INVALID_HANDLE_VALUE;
	}

	return;
}

static VOID
start_profiler_timer_event (void)
{
	g_return_if_fail (g_timer_main_thread == INVALID_HANDLE_VALUE && g_timer_event == NULL);

	BOOL result = FALSE;

	g_timer_main_thread = OpenThread (READ_CONTROL | THREAD_GET_CONTEXT, FALSE, GetCurrentThreadId ());
	if (g_timer_main_thread != INVALID_HANDLE_VALUE)
	{
		g_timer_event = CreateThreadpoolTimer (threadpool_timer_callback, (PVOID)g_timer_main_thread, NULL);
		if (g_timer_event != NULL) {

			ULARGE_INTEGER due_time;
			FILETIME timer_due;

			// Use a precision of 1ms (expressed in number of 100 ns blocks). Negative value
			// means relative current time.
			due_time.QuadPart = (ULONGLONG) -(10 * 1000);
			timer_due.dwHighDateTime = due_time.HighPart;
			timer_due.dwLowDateTime  = due_time.LowPart;

			// Schedule periodic timer.
			SetThreadpoolTimer (g_timer_event, &timer_due, 1, 0);

			result = TRUE;
		}
	}

	if (result == FALSE) {

		stop_profiler_timer_event ();
	}

	return;
}
#endif /* G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) */

void
mono_runtime_setup_stat_profiler (void)
{
	start_profiler_timer_event ();
	return;
}

void
mono_runtime_shutdown_stat_profiler (void)
{
	stop_profiler_timer_event ();
	return;
}

gboolean
mono_thread_state_init_from_handle (MonoThreadUnwindState *tctx, MonoThreadInfo *info)
{
	DWORD id = mono_thread_info_get_tid (info);
	HANDLE handle;
	CONTEXT context;
	MonoContext *ctx;
	MonoJitTlsData *jit_tls;
	void *domain;
	MonoLMF *lmf = NULL;
	gpointer *addr;

	tctx->valid = FALSE;
	tctx->unwind_data [MONO_UNWIND_DATA_DOMAIN] = NULL;
	tctx->unwind_data [MONO_UNWIND_DATA_LMF] = NULL;
	tctx->unwind_data [MONO_UNWIND_DATA_JIT_TLS] = NULL;

	g_assert (id != GetCurrentThreadId ());

	handle = OpenThread (THREAD_ALL_ACCESS, FALSE, id);
	g_assert (handle);

	context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;

	if (!GetThreadContext (handle, &context)) {
		CloseHandle (handle);
		return FALSE;
	}

	g_assert (context.ContextFlags & CONTEXT_INTEGER);
	g_assert (context.ContextFlags & CONTEXT_CONTROL);

	ctx = &tctx->ctx;

	memset (ctx, 0, sizeof (MonoContext));
	mono_sigctx_to_monoctx (&context, ctx);

	/* mono_set_jit_tls () sets this */
	jit_tls = mono_thread_info_tls_get (info, TLS_KEY_JIT_TLS);
	/* SET_APPDOMAIN () sets this */
	domain = mono_thread_info_tls_get (info, TLS_KEY_DOMAIN);

	/*Thread already started to cleanup, can no longer capture unwind state*/
	if (!jit_tls || !domain)
		return FALSE;

	/*
	 * The current LMF address is kept in a separate TLS variable, and its hard to read its value without
	 * arch-specific code. But the address of the TLS variable is stored in another TLS variable which
	 * can be accessed through MonoThreadInfo.
	 */
	/* mono_set_lmf_addr () sets this */
	addr = mono_thread_info_tls_get (info, TLS_KEY_LMF_ADDR);
	if (addr)
		lmf = *addr;

	tctx->unwind_data [MONO_UNWIND_DATA_DOMAIN] = domain;
	tctx->unwind_data [MONO_UNWIND_DATA_JIT_TLS] = jit_tls;
	tctx->unwind_data [MONO_UNWIND_DATA_LMF] = lmf;
	tctx->valid = TRUE;

	return TRUE;
}

