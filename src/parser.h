#pragma once
#include <stddef.h>
#include "parser_fasta.h"  // Include existing FASTA parser definitions

// Format types supported by the parser
typedef enum {
    FORMAT_UNKNOWN,
    FORMAT_FASTA,
    FORMAT_PHY,
    FORMAT_MAF,
    FORMAT_ALN
} AlignmentFormat;

// Parser result structure
typedef struct {
    AlignmentFormat format;
    SeqList *sequences;
    char *error_message;
} ParseResult;

// Main parser functions
AlignmentFormat detect_format(const char *filename);
ParseResult *parse_alignment(const char *filename);
ParseResult *parse_alignment_with_format(const char *filename, AlignmentFormat format);
void free_parse_result(ParseResult *result);

// Format-specific parsers
SeqList *parse_fasta(const char *path);  // Already exists
SeqList *parse_phy(const char *path);
SeqList *parse_maf(const char *path);
SeqList *parse_aln(const char *path);

// Helper functions
const char *format_to_string(AlignmentFormat format);
const char *format_to_extension(AlignmentFormat format); 