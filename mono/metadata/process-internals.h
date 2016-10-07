/*
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#ifndef __MONO_METADATA_PROCESS_INTERNALS_H__
#define __MONO_METADATA_PROCESS_INTERNALS_H__

#include <config.h>
#include <glib.h>

void
process_get_fileversion (MonoObject *filever, gunichar2 *filename, MonoError *error);

gchar*
mono_process_quote_path (const gchar *path);

gchar*
mono_process_unquote_application_name (gchar *path);

gboolean
mono_process_complete_path (const gunichar2 *appname, gchar **completed);

void
mono_process_init_startup_info (HANDLE stdin_handle, HANDLE stdout_handle,
								HANDLE stderr_handle,STARTUPINFO *startinfo);

gboolean
mono_process_get_shell_arguments (MonoProcessStartInfo *proc_start_info, gunichar2 **shell_path,
								MonoString **cmd);

gboolean
mono_process_create_process (MonoProcInfo *mono_process_info, gunichar2 *shell_path, MonoString *cmd,
							guint32 creation_flags, gchar *env_vars, gunichar2 *dir, STARTUPINFO *start_info,
							PROCESS_INFORMATION *process_info);

#endif /* __MONO_METADATA_PROCESS_INTERNALS_H__ */
