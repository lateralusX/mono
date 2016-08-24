/*
 * mono-typecast-helper.h: Helper macros to perform "safe" type cast in unmanaged code, primarly to reduce warnings and
 * add validation and safty to down and/or signed/unsigned casts of scalar/primitive data types.
 *
 * Authors:
 *  - Johan Lorensson <johan.lorensson@xamarin.com>
 *
 * Copyright 2016 Dot net foundation.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#ifndef __MONO_TYPECAST_HELPER_H__
#define __MONO_TYPECAST_HELPER_H__

#include <sys/types.h>
#include <config.h>
#include <assert.h>

/**
* Currently, type cast helper macros are only defined for Visual Studio compiler in order to reduce warnings
* in a controlled and checked way. NOTE, possible to define type cast helper macros on other platforms as well, if needed.
* By default, the type cast macros just makes the requested cast, but there is an option to enable safe type cast checks
* by defining MONO_ENABLE_SAFE_TYPECAST_HELPERS.
*/
#ifdef _MSC_VER
	#define MONO_ENABLE_TYPECAST_HELPERS
	//#define MONO_ENABLE_SAFE_TYPECAST_HELPERS
#endif

/**
* Set of helper macros that can be used in order to do "controlled" type casts in order to reduce warnings and/or detect type cast error cases at runtime.
* If a new type cast is needed, add a new macro's bellow following MONO_TARGET_TO_SOURCE format. In case where the type cast might just be needed
* on a specific platform, define that as well using the MONO_PLATFORM_TARGET_TO_SOURCE format.
*/
#ifdef MONO_ENABLE_TYPECAST_HELPERS
	#ifdef MONO_ENABLE_SAFE_TYPECAST_HELPERS
		inline long mono_uint64_to_long_safe_cast_func (uint64_t x);
		inline long mono_uint64_to_long_safe_cast_func (uint64_t x) { assert (x <= LONG_MAX); return ((long)x); }

		inline int mono_uint64_to_int_safe_cast_func (uint64_t x);
		inline int mono_uint64_to_int_safe_cast_func (uint64_t x) { assert (x <= INT_MAX); return ((int)x); }
	
		inline unsigned int mono_uint64_to_uint_safe_cast_func (uint64_t x);
		inline unsigned int mono_uint64_to_uint_safe_cast_func (uint64_t x) { assert (x <= UINT_MAX); return ((unsigned int)x); }
	
		inline off_t mono_uint64_to_off_t_safe_cast_func (uint64_t x);
		inline off_t mono_uint64_to_off_t_safe_cast_func (uint64_t x) { assert (x <= LONG_MAX); return ((off_t)x); }
		
		#define MONO_UINT64_TO_LONG(x) (mono_uint64_to_long_safe_cast_func (x))
		#define MONO_UINT64_TO_INT(x) (mono_uint64_to_int_safe_cast_func (x))
		#define MONO_UINT64_TO_UINT(x) (mono_uint64_to_uint_safe_cast_func (x))
		#define MONO_UINT64_TO_OFF_T(x) (mono_uint64_to_off_t_safe_cast_func (x))
	#else
		#define MONO_UINT64_TO_LONG(x) ((long)x)
		#define MONO_UINT64_TO_INT(x) ((int)x)
		#define MONO_UINT64_TO_UINT(x) ((unsigned int)x)
		#define MONO_UINT64_TO_OFF_T(x) ((off_t)x)
	#endif

	#define MONO_WIN32_UINT64_TO_LONG(x) MONO_UINT64_TO_LONG (x)
	#define MONO_WIN32_UINT64_TO_INT(x) MONO_UINT64_TO_INT (x)
	#define MONO_WIN32_UINT64_TO_UINT(x) MONO_UINT64_TO_UINT (x)
	#define MONO_WIN32_UINT64_TO_OFF_T(x) MONO_UINT64_TO_OFF_T (x)
#else
	#define MONO_UINT64_TO_LONG(x) x
	#define MONO_UINT64_TO_INT(x) x
	#define MONO_UINT64_TO_UINT(x) x
	#define MONO_UINT64_TO_OFF_T(x) x
	#define MONO_WIN32_UINT64_TO_LONG(x) x
	#define MONO_WIN32_UINT64_TO_INT(x) x
	#define MONO_WIN32_UINT64_TO_UINT(x) x
	#define MONO_WIN32_UINT64_TO_OFF_T(x) x
#endif


#endif

