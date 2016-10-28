/*
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#ifndef __MONO_IMAGE_WRITER_INTERNALS_H__
#define __MONO_IMAGE_WRITER_INTERNALS_H__

#include <config.h>
#include <glib.h>
#include "mono/mini/mini.h"

/* emit mode */
enum {
	EMIT_NONE,
	EMIT_BYTE,
	EMIT_WORD,
	EMIT_LONG
};

struct _MonoImageWriter {
	MonoMemPool *mempool;
	char *outfile;
	gboolean use_bin_writer;
	const char *current_section;
	int current_subsection;
	const char *section_stack [16];
	int subsection_stack [16];
	int stack_pos;
	FILE *fp;
	/* Bin writer */
#ifdef USE_BIN_WRITER
	BinSymbol *symbols;
	BinSection *sections;
	BinSection *cur_section;
	BinReloc *relocations;
	GHashTable *labels;
	int num_relocs;
	guint8 *out_buf;
	int out_buf_size, out_buf_pos;
#endif
	/* Asm writer */
	char *tmpfname;
	int mode; /* emit mode */
	int col_count; /* bytes emitted per .byte line */
	int label_gen;
	void * asm_writer_ext;
};


#ifdef TARGET_ASM_MASM
void
asm_writer_emit_start (MonoImageWriter *acfg);

int
asm_writer_emit_writeout (MonoImageWriter *acfg);

void
asm_writer_emit_unset_mode (MonoImageWriter *acfg);

void
asm_writer_emit_section_change (MonoImageWriter *acfg, const char *section_name, int subsection_index);

void
asm_writer_emit_global (MonoImageWriter *acfg, const char *name, gboolean func);

void
asm_writer_emit_local_symbol (MonoImageWriter *acfg, const char *name, const char *end_label, gboolean func);

void
asm_writer_emit_symbol_size (MonoImageWriter *acfg, const char *name, const char *end_label);

void
asm_writer_emit_label (MonoImageWriter *acfg, const char *name);

void
asm_writer_emit_bytes (MonoImageWriter *acfg, const guint8* buf, int size);

void
asm_writer_emit_string (MonoImageWriter *acfg, const char *value);

void
asm_writer_emit_line (MonoImageWriter *acfg);

void 
asm_writer_emit_alignment (MonoImageWriter *acfg, int size);

void 
asm_writer_emit_alignment_fill (MonoImageWriter *acfg, int size, int fill);

void
asm_writer_emit_pointer_unaligned (MonoImageWriter *acfg, const char *target);

void
asm_writer_emit_pointer (MonoImageWriter *acfg, const char *target);

void
asm_writer_emit_int16 (MonoImageWriter *acfg, int value);

void
asm_writer_emit_int32 (MonoImageWriter *acfg, int value);

void
asm_writer_emit_symbol_diff (MonoImageWriter *acfg, const char *end, const char* start, int offset);

void
asm_writer_emit_zero_bytes (MonoImageWriter *acfg, int num);

gboolean
asm_writer_subsections_supported (MonoImageWriter *acfg);

const char *
asm_writer_get_temp_label_prefix (MonoImageWriter *acfg);

#endif

#endif /* __MONO_IMAGE_WRITER_INTERNALS_H__ */
