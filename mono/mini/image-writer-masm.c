/*
 * image-writer-masm.c: MASM image writer targeting VS toolchain.
 *
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#include <config.h>
#include <glib.h>

#include "mono/mini/image-writer.h"
#include "mono/mini/image-writer-internals.h"

#if defined(TARGET_ASM_MASM)
#include "mono/utils/mono-digest.h"

static const char * g_gas_section_names [] = {".text", ".data", ".bss", ".rdata", ""};
static const char * g_masm_section_names [] = {".code", ".data", ".data?", ".const"};
static const char * g_masm_segment_names [] = {"_TEXT", "_DATA", "_BSS", "CONST"};


static const char * g_masm_byte_emit_format_table [] = {",00H",  ",01H",  ",02H",  ",03H",  ",04H",  ",05H",  ",06H",  ",07H",  ",08H",  ",09H",
							",0AH",  ",0BH",  ",0CH",  ",0DH",  ",0EH",  ",0FH",  ",010H", ",011H", ",012H", ",013H",
							",014H", ",015H", ",016H", ",017H", ",018H", ",019H", ",01AH", ",01BH", ",01CH", ",01DH",
							",01EH", ",01FH", ",020H", ",021H", ",022H", ",023H", ",024H", ",025H", ",026H", ",027H",
							",028H", ",029H", ",02AH", ",02BH", ",02CH", ",02DH", ",02EH", ",02FH", ",030H", ",031H",
							",032H", ",033H", ",034H", ",035H", ",036H", ",037H", ",038H", ",039H", ",03AH", ",03BH",
							",03CH", ",03DH", ",03EH", ",03FH", ",040H", ",041H", ",042H", ",043H", ",044H", ",045H",
							",046H", ",047H", ",048H", ",049H", ",04AH", ",04BH", ",04CH", ",04DH", ",04EH", ",04FH",
							",050H", ",051H", ",052H", ",053H", ",054H", ",055H", ",056H", ",057H", ",058H", ",059H",
							",05AH", ",05BH", ",05CH", ",05DH", ",05EH", ",05FH", ",060H", ",061H", ",062H", ",063H",
							",064H", ",065H", ",066H", ",067H", ",068H", ",069H", ",06AH", ",06BH", ",06CH", ",06DH",
							",06EH", ",06FH", ",070H", ",071H", ",072H", ",073H", ",074H", ",075H", ",076H", ",077H",
							",078H", ",079H", ",07AH", ",07BH", ",07CH", ",07DH", ",07EH", ",07FH", ",080H", ",081H",
							",082H", ",083H", ",084H", ",085H", ",086H", ",087H", ",088H", ",089H", ",08AH", ",08BH",
							",08CH", ",08DH", ",08EH", ",08FH", ",090H", ",091H", ",092H", ",093H", ",094H", ",095H",
							",096H", ",097H", ",098H", ",099H", ",09AH", ",09BH", ",09CH", ",09DH", ",09EH", ",09FH",
							",0A0H", ",0A1H", ",0A2H", ",0A3H", ",0A4H", ",0A5H", ",0A6H", ",0A7H", ",0A8H", ",0A9H",
							",0AAH", ",0ABH", ",0ACH", ",0ADH", ",0AEH", ",0AFH", ",0B0H", ",0B1H", ",0B2H", ",0B3H",
							",0B4H", ",0B5H", ",0B6H", ",0B7H", ",0B8H", ",0B9H", ",0BAH", ",0BBH", ",0BCH", ",0BDH",
							",0BEH", ",0BFH", ",0C0H", ",0C1H", ",0C2H", ",0C3H", ",0C4H", ",0C5H", ",0C6H", ",0C7H",
							",0C8H", ",0C9H", ",0CAH", ",0CBH", ",0CCH", ",0CDH", ",0CEH", ",0CFH", ",0D0H", ",0D1H",
							",0D2H", ",0D3H", ",0D4H", ",0D5H", ",0D6H", ",0D7H", ",0D8H", ",0D9H", ",0DAH", ",0DBH",
							",0DCH", ",0DDH", ",0DEH", ",0DFH", ",0E0H", ",0E1H", ",0E2H", ",0E3H", ",0E4H", ",0E5H",
							",0E6H", ",0E7H", ",0E8H", ",0E9H", ",0EAH", ",0EBH", ",0ECH", ",0EDH", ",0EEH", ",0EFH",
							",0F0H", ",0F1H", ",0F2H", ",0F3H", ",0F4H", ",0F5H", ",0F6H", ",0F7H", ",0F8H", ",0F9H",
							",0FAH", ",0FBH", ",0FCH", ",0FDH", ",0FEH", ",0FFH"};

//#define MASM_EMIT_PROC
#define MASM_EMIT_BINARY_DATA
#define MASM_EMIT_STRING_DATA

#ifdef MASM_EMIT_PROC
#define MASM_LABEL_PREFIX "$"
#else
#define MASM_LABEL_PREFIX ""
#endif /* MASM_EMIT_PROC */

#define MASM_TEMP_LABEL_PREFIX "$L"

#define MASM_LOCATION_COUNTER "$"
#define GAS_LOCATION_COUNTER "."

#define MASM_UINT8_DIRECTIVE "BYTE"
#define MASM_INT8_DIRECTIVE "SBYTE"
#define MASM_UINT16_DIRECTIVE "WORD"
#define MASM_INT16_DIRECTIVE "SWORD"
#define MASM_UINT32_DIRECTIVE "DWORD"
#define MASM_INT32_DIRECTIVE "SDWORD"
#define MASM_INT64_DIRECTIVE "QWORD"
#define MASM_INT64_DIRECTIVE "SQWORD"

#if defined(TARGET_AMD64)
#define MASM_POINTER_DIRECTIVE "QWORD"
#else
#define MASM_POINTER_DIRECTIVE "DWORD"
#endif

#define MASM_MAX_SYMBOL_SIZE 247

#define MASM_SEGMENT_PREFIX "_"
#define GAS_SECTION_PREFIX "."

struct _MASMImageWriterExtension {
#ifdef MASM_EMIT_PROC
	char current_function [MASM_MAX_SYMBOL_SIZE];
#endif
	char symbol_temp_buffer [MASM_MAX_SYMBOL_SIZE];
};

typedef struct _MASMImageWriterExtension MASMImageWriterExtension;

static inline
const char*
masm_get_label_prefix (void)
{
	return MASM_LABEL_PREFIX;
}

static inline gboolean
masm_include_label_prefix (const char *label_name)
{
	return (label_name[0] == MASM_LABEL_PREFIX [0]);
}

static inline const char*
masm_current_location_counter (void)
{
	return MASM_LOCATION_COUNTER;
}

static inline gboolean
masm_is_location_counter (const char *label_name)
{
	return (label_name [0] == MASM_LOCATION_COUNTER [0]);
}

static inline gboolean
masm_is_gas_location_counter (const char *label_name)
{
	return (label_name [0] == GAS_LOCATION_COUNTER [0] && label_name [1] == '\0');
}

static inline const char*
masm_get_temporary_label_prefix (void)
{
	return MASM_TEMP_LABEL_PREFIX;
}

static inline gboolean
masm_include_temporary_label_prefix (const char *label_name)
{
	return (label_name [0] == MASM_TEMP_LABEL_PREFIX [0] &&
		label_name [1] == MASM_TEMP_LABEL_PREFIX [1]);
}

static inline const char*
masm_get_int32_directive (void)
{
	return MASM_INT32_DIRECTIVE;
}

static inline const char *
masm_get_int64_directive (void)
{
	return MASM_INT64_DIRECTIVE;
}

static const char*
masm_mangle_symbol_name (MonoImageWriter *acfg, const char *symbol)
{
	const char *result = symbol;

	size_t symbol_size = strlen (symbol);
	if (symbol_size >= MASM_MAX_SYMBOL_SIZE) {
		// Symbol exceeds max MASM supported symbol size. As long as this is an internal
		// symbol we can convert it to and MD5 hash and use that as symbol.
		guchar md5_buffer [16];
		mono_md5_get_digest (symbol, symbol_size, md5_buffer);

		g_assert (acfg->asm_writer_ext != NULL);
		
		MASMImageWriterExtension *writer_ext = acfg->asm_writer_ext;
		size_t max_symbol_size = G_N_ELEMENTS (writer_ext->symbol_temp_buffer) - (G_N_ELEMENTS (md5_buffer) * 2) - 1 - 1;
		
		strncpy_s (writer_ext->symbol_temp_buffer, G_N_ELEMENTS (writer_ext->symbol_temp_buffer), symbol, max_symbol_size);
		strncat_s (writer_ext->symbol_temp_buffer, G_N_ELEMENTS (writer_ext->symbol_temp_buffer), "@", 1);

		for (int i = 0; i < G_N_ELEMENTS (md5_buffer); ++i)
			sprintf (writer_ext->symbol_temp_buffer + max_symbol_size + 1 + (i * 2), "%.2X", md5_buffer [i]);

		result = writer_ext->symbol_temp_buffer;
	}

	return result;
}

static inline const char*
masm_get_label (const char *s)
{
	// MASM doesn't support .L for local labels, so jump past in
	// original string.
	if (s [0] == '.' && s [1] == 'L')
		s += 2;
	return s;
}

static const char*
masm_format_label (MonoImageWriter *acfg, const char *label, char *buffer, size_t buffer_size)
{
	const char *label_prefix = "";
	const char *label_name = masm_mangle_symbol_name(acfg, label);

	if (masm_is_location_counter (label) || masm_is_gas_location_counter (label)) {
		label_name = masm_current_location_counter ();
	} else if (!masm_include_label_prefix (label)) {
		label_prefix = masm_get_label_prefix ();
		label_name = masm_get_label (label);
	}

	sprintf_s (buffer, buffer_size, "%s%s", label_prefix, label_name);
	return buffer;
}

static const char*
masm_format_temporary_label (MonoImageWriter *acfg, const char *label, char *buffer, size_t buffer_size)
{
	const char *label_prefix = "";
	const char *label_name = masm_mangle_symbol_name(acfg, label);

	if (!masm_include_temporary_label_prefix (label)) {
		label_prefix = masm_get_temporary_label_prefix ();
		label_name = masm_get_label (label);
	}

	sprintf_s (buffer, buffer_size, "%s%s%d", label_prefix, label_name, acfg->label_gen);
	acfg->label_gen ++;
	return buffer;
}

static const char*
masm_find_section_segment_replacement (const char *section_names[], size_t section_names_len, const char *section_name)
{
	const char *result = NULL;

	// Search array for matching section, but only if appears to be a section
	// that we need a replacement for.
	if (section_name [0] == GAS_SECTION_PREFIX [0]) {
		for (size_t i = 0; i < section_names_len; ++i) {
			if (_strnicmp (section_name, section_names [i], MASM_MAX_SYMBOL_SIZE) == 0) {
				assert (i < G_N_ELEMENTS (g_masm_segment_names));
				result = g_masm_segment_names [i];
				break;
			}
		}
	}
	
	return result;
}

static const char*
masm_section_to_segment_name (const char *section_name, char *buffer, size_t buffer_size, gboolean *needs_alias)
{
	const char * result = NULL;
	*needs_alias = FALSE;
	
	// Check for known GAS section names and get corresponding MASM segment name.
	result = masm_find_section_segment_replacement (g_gas_section_names, G_N_ELEMENTS (g_gas_section_names), section_name);
	if (result == NULL) {
		// Not a known section, make sure to escape custom sections, replace
		// characters not supported in MASM.
		size_t section_name_length = strlen (section_name);
		size_t next_char_to_escape = 0;

		// Section names can not be mangled.
		g_assert (section_name_length <= MASM_MAX_SYMBOL_SIZE);

		next_char_to_escape = strcspn (section_name, GAS_SECTION_PREFIX);
		if (next_char_to_escape != section_name_length) {
			strncpy (buffer, section_name, buffer_size);
			while (next_char_to_escape != section_name_length) {
				buffer [next_char_to_escape] = MASM_SEGMENT_PREFIX [0];
				next_char_to_escape = strcspn (buffer + next_char_to_escape, GAS_SECTION_PREFIX);
			}

			*needs_alias = TRUE;
			result = buffer;
		}
	}

	return result;
}

static void
masm_writer_emit_symbol_type (MonoImageWriter *acfg, const char *name, gboolean func, gboolean global)
{
	g_assert (acfg->asm_writer_ext != NULL);
	MASMImageWriterExtension *writer_ext = acfg->asm_writer_ext;
	
	asm_writer_emit_unset_mode (acfg);

#ifdef MASM_EMIT_PROC
	if (func) {
		// End any previous PROC before emitting a new function.
		if (writer_ext->current_function[0] != '\0') {
			fprintf (acfg->fp, "%s ENDP\n", writer_ext->current_function);
			writer_ext->current_function[0] = '\0';
		}

		strncpy_s (writer_ext->current_function, MASM_MAX_SYMBOL_SIZE, name, MASM_MAX_SYMBOL_SIZE);
		fprintf (acfg->fp, "%s PROC\n", name);
	} else {
		// None function symbols are emitted as labels.
		fprintf (acfg->fp, "%s:\n", name);
	}
#endif /* MASM_EMIT_PROC */
	return;
}

static inline void
masm_writer_emit_byte (MonoImageWriter *acfg, const guint8 value, gboolean new_line)
{
	fprintf (acfg->fp, (new_line == TRUE) ? "\n\t%s 0%XH" : "\t%s 0%XH", MASM_UINT8_DIRECTIVE, value);
}

static inline void
masm_writer_emit_change_column_mode (MonoImageWriter *acfg, int mode)
{
	if (acfg->mode != mode) {
		asm_writer_emit_unset_mode (acfg);
		acfg->mode = mode;
		acfg->col_count = 0;
	}
	return;
}

static inline void
masm_writer_emit_reset_column_mode (MonoImageWriter *acfg, int mode)
{
	if (acfg->mode != mode || acfg->col_count != 0) {
		asm_writer_emit_unset_mode (acfg);
		acfg->mode = mode;
		acfg->col_count = 0;
	}
	return;
}

static inline const char*
masm_writer_get_masm_data_type (int type)
{
	if (type == EMIT_WORD)
		return MASM_INT16_DIRECTIVE;
	else if (type == EMIT_LONG)
		return MASM_INT32_DIRECTIVE;
	else if (type == EMIT_BYTE)
		return MASM_INT8_DIRECTIVE;
	
	return MASM_INT64_DIRECTIVE;
}

static void
masm_writer_emit_data (MonoImageWriter *acfg, int value, int type)
{
#ifdef MASM_EMIT_BINARY_DATA
	masm_writer_emit_change_column_mode (acfg, type);

	if ((acfg->col_count % 8) == 0) {
		fprintf (acfg->fp, (acfg->col_count != 0) ? "\n\t%s " : "\t%s ", masm_writer_get_masm_data_type (type));
	} else {
		fprintf (acfg->fp, ", ");
	}

	fprintf (acfg->fp, "0%XH", value);
	acfg->col_count++;
#endif /* MASM_EMIT_BINARY_DATA */
	return;
}

static void
masm_writer_emit_comment (MonoImageWriter *acfg, const char * format, const char *comment)
{
	assert (acfg);
	assert (format);
	assert (comment);

	size_t comment_len = strlen (comment);
	if (comment_len > MASM_MAX_SYMBOL_SIZE) {
		// Must be splitted over multiple lines since MASM won't support that long lines.
		const char *comment_end = comment + comment_len;
		char buffer[MASM_MAX_SYMBOL_SIZE];

		while (comment < comment_end) {
			strncpy_s (buffer, G_N_ELEMENTS (buffer), comment, _TRUNCATE);
			fprintf (acfg->fp, format, buffer);
			comment += strlen (buffer);
		}
	} else {
		fprintf (acfg->fp, format, comment);
	}

	return;
}

void
asm_writer_emit_start (MonoImageWriter *acfg)
{
	g_assert (acfg->asm_writer_ext == NULL);

	acfg->asm_writer_ext = g_new0 (MASMImageWriterExtension, 1);
	return;
}

int
asm_writer_emit_writeout (MonoImageWriter *acfg)
{
	asm_writer_emit_unset_mode (acfg);

	fprintf (acfg->fp, "END\n");
	fclose (acfg->fp);
	
	if (acfg->asm_writer_ext != NULL)
		g_free ((MASMImageWriterExtension *)acfg->asm_writer_ext);
	
	return 0;
}

void
asm_writer_emit_unset_mode (MonoImageWriter *acfg)
{
	if (acfg->mode == EMIT_NONE)
		return;
	fprintf (acfg->fp, "\n");
	acfg->mode = EMIT_NONE;
}

void
asm_writer_emit_section_change (MonoImageWriter *acfg, const char *section_name, int subsection_index)
{
	char name_buffer[MASM_MAX_SYMBOL_SIZE];
	gboolean needs_alias = FALSE;

	g_assert (acfg->asm_writer_ext != NULL);
	MASMImageWriterExtension *writer_ext = acfg->asm_writer_ext;
		
	asm_writer_emit_unset_mode (acfg);

#ifdef MASM_EMIT_PROC
	// Any open PROC that must be ended before changing section?
	if (writer_ext->current_function[0] != '\0') {
		fprintf (acfg->fp, "%s ENDP\n", writer_ext->current_function);
		writer_ext->current_function[0] = '\0';
	}
#endif /* MASM_EMIT_PROC */

	// If we already have the same segment open, continue to use that segment, if not start a new.
	if (0 != strcmp (section_name, (acfg->current_section != NULL) ? acfg->current_section : "")) {
		// Any open segment that needs to be ended before changing section?
		if (acfg->current_section != NULL && acfg->current_section[0] != '\0')
			fprintf (acfg->fp, "%s ENDS\n", masm_section_to_segment_name (acfg->current_section, name_buffer, G_N_ELEMENTS (name_buffer), &needs_alias));

		// Get MASM segment to emit.
		const char * escaped_section_name = masm_section_to_segment_name (section_name, name_buffer, G_N_ELEMENTS (name_buffer), &needs_alias);

		// MASM have some reserved characters that can't go into segment names. It is still possible to
		// emit the full name using ALIAS and that's what will go into the COFF information.
		if (needs_alias == TRUE)
			fprintf (acfg->fp, "%s SEGMENT ALIAS('%s')\n", escaped_section_name, section_name);
		else
			fprintf (acfg->fp, "%s SEGMENT\n", escaped_section_name);
	}

	return;
}

void
asm_writer_emit_global (MonoImageWriter *acfg, const char *name, gboolean func)
{
	// Public symbols can not be mangled to shorter version.
	g_assert (strlen (name) <= MASM_MAX_SYMBOL_SIZE);
	
	asm_writer_emit_unset_mode (acfg);
	fprintf (acfg->fp, "PUBLIC %s\n", name);
	masm_writer_emit_symbol_type (acfg, name, func, TRUE);
}

void
asm_writer_emit_local_symbol (MonoImageWriter *acfg, const char *name, const char *end_label, gboolean func)
{
	const char *symbol = masm_mangle_symbol_name (acfg, name);

	asm_writer_emit_unset_mode (acfg);
	if (symbol != name)
		// Emit comment with unmangled name.
		masm_writer_emit_comment (acfg, "; %s\n", name);
	masm_writer_emit_symbol_type (acfg, symbol, func, FALSE);
}

void
asm_writer_emit_symbol_size (MonoImageWriter *acfg, const char *name, const char *end_label)
{
	asm_writer_emit_unset_mode (acfg);
}

void
asm_writer_emit_label (MonoImageWriter *acfg, const char *name)
{
	const char *symbol = masm_mangle_symbol_name (acfg, name);

	asm_writer_emit_unset_mode (acfg);

	if (symbol != name)
		// Emit comment with unmangled name.
		masm_writer_emit_comment (acfg, "; %s\n", name);
	
	// Label syntax used, $[label_name] for standrad or $L[label_name] for "local".
	if (masm_include_label_prefix (symbol))
		fprintf (acfg->fp, "%s LABEL NEAR\n", symbol);
	else
		fprintf (acfg->fp, "%s%s LABEL NEAR\n", masm_get_label_prefix (), masm_get_label (symbol));
}

void
asm_writer_emit_bytes (MonoImageWriter *acfg, const guint8* buf, int size)
{
#ifdef MASM_EMIT_BINARY_DATA
	masm_writer_emit_change_column_mode (acfg, EMIT_BYTE);

	for (int i = 0; i < size; ++i, ++acfg->col_count) {
		if ((acfg->col_count % 32) == 0)
			masm_writer_emit_byte (acfg, buf [i], acfg->col_count != 0);
		else
			fputs (g_masm_byte_emit_format_table [buf [i]], acfg->fp);
	}
#endif /* MASM_EMIT_BINARY_DATA */
	return;
}

void
asm_writer_emit_string (MonoImageWriter *acfg, const char *value)
{
#ifdef MASM_EMIT_STRING_DATA
	size_t value_len = strlen (value);

	asm_writer_emit_unset_mode (acfg);
	
	if (value_len >= MASM_MAX_SYMBOL_SIZE) {
		// Emit string as bytes (to long as literal), including NULL terminator.
		// Emit comment with unmangled name.
		masm_writer_emit_comment (acfg, "\t; &s\n", value);
		asm_writer_emit_bytes (acfg, value, value_len + 1);
		asm_writer_emit_unset_mode (acfg);
	} else if (strlen (value) != 0) {
		fprintf (acfg->fp, "\t%s '%s', 00H\n", MASM_UINT8_DIRECTIVE, value);
	} else {
		fprintf (acfg->fp, "\t%s 00H\n", MASM_UINT8_DIRECTIVE, value);
	}
#endif /* MASM_EMIT_STRING_DATA */
	return;
}

void
asm_writer_emit_line (MonoImageWriter *acfg)
{
	// Make MASM output compact, just emit new line if we are
	// in an emitting mode.
	asm_writer_emit_unset_mode (acfg);
}

void 
asm_writer_emit_alignment (MonoImageWriter *acfg, int size)
{
	asm_writer_emit_unset_mode (acfg);
	fprintf (acfg->fp, "\tALIGN 0%XH\n", size);
}

void 
asm_writer_emit_alignment_fill (MonoImageWriter *acfg, int size, int fill)
{
	asm_writer_emit_unset_mode (acfg);
	asm_writer_emit_alignment (acfg, size);
}

void
asm_writer_emit_pointer_unaligned (MonoImageWriter *acfg, const char *target)
{
	char buffer[MASM_MAX_SYMBOL_SIZE];

	asm_writer_emit_unset_mode (acfg);
	fprintf (acfg->fp, "\t%s %s\n", MASM_POINTER_DIRECTIVE, target ? masm_format_label (acfg, target, buffer, G_N_ELEMENTS (buffer)) : "0");
}

void
asm_writer_emit_pointer (MonoImageWriter *acfg, const char *target)
{
	asm_writer_emit_unset_mode (acfg);
	asm_writer_emit_alignment (acfg, sizeof (gpointer));
	asm_writer_emit_pointer_unaligned (acfg, target);
}

void
asm_writer_emit_int16 (MonoImageWriter *acfg, int value)
{
	masm_writer_emit_data (acfg, value, EMIT_WORD);
}

void
asm_writer_emit_int32 (MonoImageWriter *acfg, int value)
{
	masm_writer_emit_data (acfg, value, EMIT_LONG);
}

void
asm_writer_emit_symbol_diff (MonoImageWriter *acfg, const char *end, const char* start, int offset)
{
	char start_buffer [MASM_MAX_SYMBOL_SIZE];
	char end_buffer [MASM_MAX_SYMBOL_SIZE];
	char symbol [MASM_MAX_SYMBOL_SIZE];

	if (acfg->mode != EMIT_LONG) {
		asm_writer_emit_unset_mode (acfg);
		acfg->mode = EMIT_LONG;
		acfg->col_count = 0;
	}

	start = masm_format_label (acfg, start, start_buffer, G_N_ELEMENTS (start_buffer));
	end = masm_format_label (acfg, end, end_buffer, G_N_ELEMENTS (end_buffer));
	
	if (offset == 0 && strcmp (start, masm_current_location_counter ()) != 0) {
		masm_format_temporary_label (acfg, "DIFF_SYM", symbol, G_N_ELEMENTS (symbol));
		masm_writer_emit_reset_column_mode (acfg, EMIT_LONG);
		fprintf (acfg->fp, "%s=%s - %s", symbol, end, start);
		fprintf (acfg->fp, "\n\t%s %s", masm_get_int32_directive (), symbol);
		asm_writer_emit_unset_mode (acfg);
		return;
	}

	if ((acfg->col_count % 8) == 0)
		fprintf (acfg->fp, (acfg->col_count != 0) ? "\n\t%s " : "\t%s ", masm_get_int32_directive ());
	else
		fprintf (acfg->fp, ",");
	if (offset > 0)
		fprintf (acfg->fp, "%s - %s + %d\t; 0%XH", end, start, offset, offset);
	else if (offset < 0)
		fprintf (acfg->fp, "%s - %s %d\t; 0%XH", end, start, offset, offset);
	else
		fprintf (acfg->fp, "%s - %s", end, start);

	acfg->col_count++;
	return;
}

void
asm_writer_emit_zero_bytes (MonoImageWriter *acfg, int num)
{
	asm_writer_emit_unset_mode (acfg);
	fprintf (acfg->fp, "\t%s 0%XH DUP (00H)\n", MASM_UINT8_DIRECTIVE, num);
}

gboolean
asm_writer_subsections_supported (MonoImageWriter *acfg)
{
	return FALSE;
}

const char*
asm_writer_get_temp_label_prefix (MonoImageWriter *acfg)
{
	return MASM_TEMP_LABEL_PREFIX;
}
#endif /* TARGET_ASM_MASM */
