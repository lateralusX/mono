/*
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#ifndef __EGLIB_REMAP_POSIX_WIN32_H__
#define __EGLIB_REMAP_POSIX_WIN32_H__

#if _MSC_VER
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <string.h>

	/* POSIX names have been deprecated in Visual Studio for quite some time and generates C4996 compiler warnings:
	*  https://msdn.microsoft.com/en-us/library/ttcz0bys.aspx
	*  On desktop, code using deprecated POSIX API names will still build, but under other WINAPI families
	*  the build will fail, since the deprecated symbols are no longer defined. This header file can be used in cross platform
	*  sources to redefine the deprecated POSIX names on Windows to avoid extensive ifdef-ing.
	*/
	#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		#undef read
		#define read _read

		#undef write
		#define write _write

		#undef open
		#define open _open

		#undef close
		#define close _close

		#undef unlink
		#define unlink _unlink

		#undef access
		#define access _access

		#undef getcwd
		#define getcwd _getcwd

		#undef mktemp
		#define mktemp _mktemp

		#undef strdup
		#define strdup _strdup
	#endif // !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#endif // _MCS_VER

#endif // __EGLIB_REMAP_POSIX_WIN32_H__
