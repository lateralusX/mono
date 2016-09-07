/* 
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#ifndef __POSIX_NAMES_WIN32_H__
#define __POSIX_NAMES_WIN32_H__

#if _MSC_VER 
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <string.h>
	
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

#endif // __POSIX_NAMES_WIN32_H__
