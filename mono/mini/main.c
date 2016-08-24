#include <config.h>
#include <fcntl.h>
#include <mono/metadata/assembly.h>
#include <mono/utils/mono-mmap.h>
#include <mono/utils/mono-typecast-helper.h>
#include "mini.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HOST_WIN32
#  include <io.h>
#else
#  ifndef BUILDVER_INCLUDED
#    include "buildver-boehm.h"
#  endif
#endif

/*
 * If the MONO_ENV_OPTIONS environment variable is set, it uses this as a
 * source of command line arguments that are passed to Mono before the
 * command line arguments specified in the command line.
 */
static int
mono_main_with_options (int argc, char *argv [])
{
	mono_parse_env_options (&argc, &argv);

	return mono_main (argc, argv);
}

#define STREAM_INT(x) (*(uint32_t*)x)
#define STREAM_LONG(x) (*(uint64_t*)x)

static gboolean
probe_embedded (const char *program, int *ref_argc, char **ref_argv [])
{
	MonoBundledAssembly last = { NULL, 0, 0 };
	char sigbuffer [16+sizeof (uint64_t)];
	gboolean status = FALSE;
	uint64_t directory_location;
	off_t sigstart, baseline = 0;
	uint64_t directory_size;
	char *directory, *p;
	int items, i;
	unsigned char *mapaddress = NULL;
	void *maphandle = NULL;
	GArray *assemblies;
	char *entry_point = NULL;
	char **new_argv;
	int j;

	int fd = open (program, O_RDONLY);
	if (fd == -1)
		return FALSE;
	if ((sigstart = lseek (fd, -24, SEEK_END)) == -1)
		goto doclose;
	if (read (fd, sigbuffer, sizeof (sigbuffer)) == -1)
		goto doclose;
	if (memcmp (sigbuffer+sizeof(uint64_t), "xmonkeysloveplay", 16) != 0)
		goto doclose;
	directory_location = GUINT64_FROM_LE ((*(uint64_t *) &sigbuffer [0]));
	if (lseek (fd, MONO_WIN32_UINT64_TO_LONG(directory_location), SEEK_SET) == -1)
		goto doclose;
	directory_size = sigstart-directory_location;
	directory = g_malloc (directory_size);
	if (directory == NULL)
		goto doclose;
	if (read (fd, directory, MONO_WIN32_UINT64_TO_UINT(directory_size)) == -1)
		goto dofree;

	items = STREAM_INT (directory);
	p = directory+4;

	assemblies = g_array_new (0, 0, sizeof (MonoBundledAssembly*));
	for (i = 0; i < items; i++){
		char *kind;
		int strsize = STREAM_INT (p);
		uint64_t offset, item_size;
		kind = p+4;
		p += 4 + strsize;
		offset = STREAM_LONG(p);
		p += 8;
		item_size = STREAM_INT (p);
		p += 4;
		
		if (mapaddress == NULL){
			mapaddress = mono_file_map (directory_location-offset, MONO_MMAP_READ | MONO_MMAP_PRIVATE, fd, offset, &maphandle);
			if (mapaddress == NULL){
				perror ("Error mapping file");
				exit (1);
			}
			baseline = MONO_UINT64_TO_OFF_T(offset);
		}
		if (strncmp (kind, "assembly:", strlen ("assembly:")) == 0){
			char *aname = kind + strlen ("assembly:");
			MonoBundledAssembly mba = { aname, mapaddress + offset - baseline, MONO_UINT64_TO_UINT(item_size) }, *ptr;
			ptr = g_new (MonoBundledAssembly, 1);
			memcpy (ptr, &mba, sizeof (MonoBundledAssembly));
			g_array_append_val  (assemblies, ptr);
			if (entry_point == NULL)
				entry_point = aname;
		} else if (strncmp (kind, "config:", strlen ("config:")) == 0){
			char *config = kind + strlen ("config:");
			char *aname = g_strdup (config);
			aname [strlen(aname)-strlen(".config")] = 0;
			mono_register_config_for_assembly (aname, config);
		} else if (strncmp (kind, "system_config:", strlen ("system_config:")) == 0){
			printf ("TODO s-Found: %s %llx\n", kind, (long long)offset);
		} else if (strncmp (kind, "options:", strlen ("options:")) == 0){
			mono_parse_options_from (kind + strlen("options:"), ref_argc, ref_argv);
		} else if (strncmp (kind, "config_dir:", strlen ("config_dir:")) == 0){
			printf ("TODO Found: %s %llx\n", kind, (long long)offset);
		} else {
			fprintf (stderr, "Unknown stream on embedded package: %s\n", kind);
			exit (1);
		}
	}
	g_array_append_val (assemblies, last);
	
	mono_register_bundled_assemblies ((const MonoBundledAssembly **) assemblies->data);
	new_argv = g_new (char *, (*ref_argc)+1);
	for (j = 0; j < *ref_argc; j++)
		new_argv [j] = (*ref_argv)[j];
	new_argv [j] = entry_point;
	*ref_argv = new_argv;
	(*ref_argc)++;
	
	return TRUE;
	
dofree:
	g_free (directory);
doclose:
	if (!status)
		close (fd);
	return status;
}

#ifdef HOST_WIN32

#include <shellapi.h>
#include <conio.h>

#define MONO_ATEXIT_COMMAND "--atexit="
#define MONO_ATEXIT_COMMAND_LEN G_N_ELEMENTS(MONO_ATEXIT_COMMAND)-1

#define MONO_ATEXIT_COMMAND_DELIMITER ','
#define MONO_ATEXIT_COMMAND_DELIMITER_LEN G_N_ELEMENTS(MONO_ATEXIT_COMMAND_DELIMITER)-1

#define MONO_ATEXIT_COMMAND_WAIT_KEYPRESS "waitkeypress"
#define MONO_ATEXIT_COMMAND_WAIT_KEYPRESS_LEN G_N_ELEMENTS(MONO_ATEXIT_COMMAND_WAIT_KEYPRESS)-1

#define MONO_ENABLE_ALL_COMMANDS_ATEXIT "--atexit=" MONO_ATEXIT_COMMAND_WAIT_KEYPRESS

// Define used to install commands atexit. Default value is to look in current argv for the presence of --atexit argument. It is also possible to always
// include --atexit. This can be handy when running from within debugger eliminating the need to change debug command arguments.
//#define MONO_INSTALL_DEFAULT_ATEXIT_COMMANDS
#ifndef MONO_INSTALL_DEFAULT_ATEXIT_COMMANDS
	#define MONO_INSTALL_COMMANDS_ATEXIT(argc, argv) install_commands_atexit_ex (argc, argv, TRUE)
#else
	#define MONO_INSTALL_COMMANDS_ATEXIT(argc, argv) install_default_atexit_commands ()
#endif

// Typedefs used to setup atexit handler table.
typedef void (*atexit_command_handler)(void);

typedef struct {
	const char * cmd;
	const int cmd_len;
	atexit_command_handler cmd_handler;
} CommandHandlerItem;

/**
* wait_keypress_atexit:
*
* This function is installed as an atexit function making sure that the console is not terminated before the end user has a chance to read the result.
* This can be handy in debug scenarios (running from within the debugger) since an exit of the process will close the console window
* without giving the end user a chance to look at the output before closed.
*/
static void
wait_keypress_atexit (void)
{
	fflush (stdin);

	printf ("Press any key to continue . . . ");
	fflush (stdout);

	_getch ();

	return;
}

/**
* install_wait_keypress_atexit:
*
* This function installs the wait keypress exit handler.
*/
static void
install_wait_keypress_atexit (void)
{
	atexit (wait_keypress_atexit);
	return;
}

// Table describing exit handlers that can be installed at process startup. Adding a new exit handler can be done by adding a new item to the table together with an install handler function.
const CommandHandlerItem g_command_handler_items[] =	{
															{ MONO_ATEXIT_COMMAND_WAIT_KEYPRESS, MONO_ATEXIT_COMMAND_WAIT_KEYPRESS_LEN, install_wait_keypress_atexit },
															{ NULL, 0, NULL }
														};

/**
 * get_command_atexit_arg_len:
 * @command_arg: Get length of next --atexit command.
 *
 * This function calculates the length of next command included in --atexit argument.
 *
 * Returns: The length of next command, if available.
 */
static size_t
get_command_atexit_arg_len (const char *command_arg)
{
	assert (command_arg != NULL);

	size_t total_len = strlen (command_arg);
	size_t current_len;
	for (current_len = 0; current_len < total_len; ++current_len) {
		if (command_arg [current_len] == MONO_ATEXIT_COMMAND_DELIMITER)
			break;
	}
	return current_len;
}

/**
 * install_command_atexit:
 * @command_arg: Commands included in --atexit argument, example "waitkeypress,someothercmd,yetanothercmd".
 *
 * This function installs the next command included in @command_arg parameter.
 *
 * Returns: The length of consumed command.
 */
static size_t
install_command_atexit (const char *command_arg)
{
	assert (command_arg != NULL);

	size_t command_arg_len = get_command_atexit_arg_len (command_arg);
	for (int current_item = 0; current_item < G_N_ELEMENTS (g_command_handler_items); ++current_item) {
		const CommandHandlerItem * command_handler_item = &g_command_handler_items [current_item];

		if (command_handler_item->cmd == NULL)
			continue;

		if (command_arg_len == command_handler_item->cmd_len && strncmp (command_arg, command_handler_item->cmd, command_arg_len) == 0) {
			assert (command_handler_item->cmd_handler != NULL);
			command_handler_item->cmd_handler ();
			break;
		}
	}
	return command_arg_len;
}

/**
 * install_commands_atexit:
 * @argc: Number of arguments.
 * @argv: Arguments.
 *
 * This function installs all atexit commands included in @argv.
 *
 * Returns: Number of atexit commands in @argv.
 */
static int
install_commands_atexit (int argc, char *argv [])
{
	assert (argv != NULL);

	int atexit_arg_count = 0;

	for (int current_arg=0; current_arg < argc; ++current_arg) {
		assert (NULL != argv [current_arg]);
		if (strncmp (argv [current_arg], MONO_ATEXIT_COMMAND, MONO_ATEXIT_COMMAND_LEN) == 0) {
			const char * command_arg = argv [current_arg];

			assert(command_arg != NULL);
			command_arg += MONO_ATEXIT_COMMAND_LEN;

			while (*command_arg != '\0') {
				command_arg += install_command_atexit (command_arg);
				if (*command_arg == MONO_ATEXIT_COMMAND_DELIMITER)
					command_arg++;
			}

			//One more atexit argument.
			atexit_arg_count++;
		}
	}

	return atexit_arg_count;
}

#ifndef MONO_INSTALL_DEFAULT_ATEXIT_COMMANDS

/**
 * validate_arguments:
 * @current_argc: current number of arguments in @current_argv.
 * @current_argv: current argument list.
 * @new_argc: new number of arguments in @new_argv.
 * @new_argv: new argument list.
 *
 * This is a debug validation function making sure that the modified argv list includes all arguments
 * except the ones that should have been removed. NOTE, it will only be included in debug builds.
 */
static void
validate_arguments (int current_argc, char *current_argv [], int new_argc, char *new_argv [])
{
#ifdef _DEBUG
	assert (current_argv != NULL);
	assert (new_argv != NULL);

	for (int current_argc_item = 0; current_argc_item < current_argc; ++current_argc_item) {
		if (strncmp (current_argv [current_argc_item], MONO_ATEXIT_COMMAND, MONO_ATEXIT_COMMAND_LEN) != 0) {
			gboolean arg_found = FALSE;
			for (int new_argc_item = 0; new_argc_item < new_argc; ++new_argc_item) {
				if (current_argv [current_argc_item] == new_argv [new_argc_item]) {
					arg_found = TRUE;
					break;
				}
			}
			assert (arg_found == TRUE);
		}
	}
#endif
	return;
}

/**
 * remove_atexit_arguments:
 * @new_argc: new number of argc that should be allocated.
 * @ref_argc: pointer to the current argc variable that might be updated.
 * @ref_argv: pointer to the current argv string vector variable that might be updated.
 *
 * This function removes any --atexit argument(s) and updates @ref_argc and @ref_argv with new
 * values that doesn't include --atexit argument(s). Since --atexit argument(s) are consumed
 * they should not be passed along to other parts of the runtime.
 */
static void
remove_atexit_arguments (int new_argc, int *ref_argc, char **ref_argv [])
{
	assert (ref_argc != NULL);
	assert (ref_argv != NULL);

	if (new_argc != *ref_argc) {
		char ** new_argv = g_new0 (gchar *, new_argc);

		if (new_argv != NULL) {
			int next_arg = 0;

			// Look for atexit argument(s) that should be remove from argv.
			for (int current_arg = 0; current_arg < *ref_argc; ++current_arg) {
				if (strncmp ((*ref_argv) [current_arg], MONO_ATEXIT_COMMAND, MONO_ATEXIT_COMMAND_LEN) != 0) {
					// Not an atexit argument, keep argument in updated argv.
					assert (next_arg < new_argc);
					new_argv [next_arg++] = (*ref_argv) [current_arg];
				}
			}

			validate_arguments (*ref_argc, *ref_argv, new_argc, new_argv);

			g_free (*ref_argv);
			*ref_argv = new_argv;
			*ref_argc = new_argc;
		}
	}

	return;
}

/**
 * install_commands_atexit_ex:
 * @argc: Number of arguments, might be updated if @remove_atexit_args is set to TRUE.
 * @argv: Arguments, might be updated if @remove_atexit_args is set to TRUE.
 * @remove_atexit_args: When TRUE, all instances of --atexit argument will be removed from @argc, @argv. When FALSE, @argc and @argv will not be updated.
 *
 * This function installs all atexit commands included in @argv. If @remove_atexit_args is TRUE, @argc and @argv will be updated to not
 * include the consumed --atexit argument(s).
 */
static void
install_commands_atexit_ex (int *argc, char **argv [], gboolean remove_atexit_args)
{
	assert (argc != NULL);
	assert (argv != NULL);

	int new_argc = *argc;
	int atexit_arg_count = install_commands_atexit (*argc, *argv);

	assert (atexit_arg_count < new_argc);
	new_argc -= atexit_arg_count;

	// Remove any atexit arguments from argv since the argument(s) are already consumed
	// and unknown to the rest of the runtime.
	if (remove_atexit_args == TRUE)
		remove_atexit_arguments (new_argc, argc, argv);

	return;
}

#else

/**
 * install_default_atexit_commands:
 *
 * This function installs all default atexit commands.
 */
static void
install_default_atexit_commands (void)
{
	char * argv[1];
	argv[0] = g_strdup (MONO_ENABLE_ALL_COMMANDS_ATEXIT);
	if (argv[0] != NULL) {
		install_commands_atexit (1, argv);
		g_free (argv[0]);
	}

	return;
}

#endif

int
main (void)
{
	TCHAR szFileName[MAX_PATH];
	int argc;
	gunichar2** argvw;
	gchar** argv;
	int i;
	DWORD count;
	
	argvw = CommandLineToArgvW (GetCommandLine (), &argc);
	argv = g_new0 (gchar*, argc + 1);
	for (i = 0; i < argc; i++)
		argv [i] = g_utf16_to_utf8 (argvw [i], -1, NULL, NULL, NULL);
	argv [argc] = NULL;

	LocalFree (argvw);

	if ((count = GetModuleFileName (NULL, szFileName, MAX_PATH)) != 0){
		char *entry = g_utf16_to_utf8 (szFileName, count, NULL, NULL, NULL);
		probe_embedded (entry, &argc, &argv);
	}

	MONO_INSTALL_COMMANDS_ATEXIT (&argc, &argv);
	return mono_main_with_options  (argc, argv);
}

#else

int
main (int argc, char* argv[])
{
	mono_build_date = build_date;

	probe_embedded (argv [0], &argc, &argv);
	return mono_main_with_options (argc, argv);
}

#endif
