/*
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#ifndef __G_API_FAMILY_H__
#define __G_API_FAMILY_H__

#include <glib.h>
#include <eglib-config.h>

#define G_API_FAMILY_PARTITION(x) (x)

#ifdef G_OS_WIN32
#include <winapifamily.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	#define G_API_FAMILY_NAME "Default"
	#define G_API_PARTITION_DEFAULT 1
	#define G_API_PARTITION_WIN_APP 0
	#define G_API_PARTITION_WIN_TV_TITLE 0
#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
	#define G_API_FAMILY_NAME "WIN-APP"
	#define G_API_PARTITION_DEFAULT 0
	#define G_API_PARTITION_WIN_APP 1
	#define G_API_PARTITION_WIN_TV_TITLE 0
#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_TV_TITLE)
	#define G_API_FAMILY_NAME "WIN-TV-TITLE"
	#define G_API_PARTITION_DEFAULT 0
	#define G_API_PARTITION_WIN_APP 0
	#define G_API_PARTITION_WIN_TV_TITLE 1
#else
	#error Unsupported API family
#endif
#else
	#define G_API_FAMILY_NAME "Default"
	#define G_API_PARTITION_DEFAULT 1
	#define G_API_PARTITION_WIN_APP 0
	#define G_API_PARTITION_WIN_TV_TITLE 0
#endif

#define G_UNSUPPORTED_API "%s:%d: '%s' not supported by API Family '%s'.", __FILE__, __LINE__
#define g_unsupported_api(name) G_STMT_START { g_critical (G_UNSUPPORTED_API, name, G_API_FAMILY_NAME); } G_STMT_END

#endif // __G_API_FAMILY_H__

