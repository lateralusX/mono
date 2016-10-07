/*
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#ifndef __MONO_METADATA_MARSHAL_INTERNALS_H__
#define __MONO_METADATA_MARSHAL_INTERNALS_H__

#include <config.h>
#include <glib.h>
#include <mono/metadata/object-internals.h>

void*
mono_marshal_alloc_co_task_mem (size_t size);

void
mono_marshal_free_co_task_mem (void *ptr);

gpointer
mono_marshal_realloc_co_task_mem (gpointer ptr, size_t size);

void*
mono_marshal_alloc_hglobal (size_t size);

gpointer
mono_marshal_realloc_hglobal (gpointer ptr, size_t size);

void
mono_marshal_free_hglobal (void *ptr);

gpointer
mono_string_to_lpstr (MonoString *s);

#endif /* __MONO_METADATA_MARSHAL_INTERNALS_H__ */
