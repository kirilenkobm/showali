#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser_fasta.h"

// simple growable arrays
#define MAX_SEQ 1000000

// Helper function to check if a line looks like a FASTA header
static int is_fasta_header(const char *line) {
    return line && line[0] == '>' && strlen(line) > 1;
}

// Helper function to check if a line looks like a sequence
static int is_sequence_line(const char *line) {
    if (!line || strlen(line) == 0) return 0;
    
    // Check if line contains only valid nucleotide/protein characters
    for (size_t i = 0; i < strlen(line); i++) {
        char c = toupper(line[i]);
        if (c == '\n' || c == '\r') continue;  // Skip newlines
        if (c == '-' || c == 'N' || c == 'X') continue;  // Allow gaps and unknown bases
        if (c >= 'A' && c <= 'Z') continue;  // Allow amino acids/nucleotides
        if (isspace(c)) continue;  // Allow whitespace
        return 0;  // Invalid character found
    }
    return 1;
}

SeqList *parse_fasta(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { 
        fprintf(stderr, "Error: Cannot open file '%s'\n", path);
        return NULL;
    }

    SeqList *sl = calloc(1, sizeof(*sl));
    sl->capacity = 16;
    sl->items    = calloc(sl->capacity, sizeof(Sequence));

    char *line = NULL;
    size_t  len = 0;
    Sequence *cur = NULL;
    int line_num = 0;
    int found_header = 0;
    int found_sequence = 0;

    while (getline(&line, &len, f) != -1) {
        line_num++;
        
        // Skip empty lines and whitespace-only lines
        if (strlen(line) == 0 || strspn(line, " \t\n\r") == strlen(line)) {
            continue;
        }
        
        if (line[0] == '>') {
            // Found a header
            found_header = 1;
            if (!is_fasta_header(line)) {
                fprintf(stderr, "Error: Invalid FASTA header at line %d\n", line_num);
                free(line);
                fclose(f);
                free(sl->items);
                free(sl);
                return NULL;
            }
            
            if (sl->count == sl->capacity) {
                sl->capacity *= 2;
                sl->items = realloc(sl->items, sl->capacity * sizeof(Sequence));
            }
            cur = &sl->items[sl->count++];
            cur->id = strdup(line + 1);
            cur->seq = calloc(1, MAX_SEQ);
            cur->len = 0;
        } else if (cur) {
            // We have a current sequence, so this should be sequence data
            if (!is_sequence_line(line)) {
                fprintf(stderr, "Error: Invalid sequence data at line %d\n", line_num);
                free(line);
                fclose(f);
                free(sl->items);
                free(sl);
                return NULL;
            }
            
            found_sequence = 1;
            // strip newline & append
            char *p = strchr(line, '\n');
            if (p) *p = 0;
            p = strchr(line, '\r');
            if (p) *p = 0;
            
            size_t l = strlen(line);
            if (l > 0 && cur->len + l < MAX_SEQ) {
                memcpy(cur->seq + cur->len, line, l);
                cur->len += l;
            }
        } else {
            // We found sequence data before any header - not a valid FASTA file
            fprintf(stderr, "Error: Found sequence data before FASTA header at line %d\n", line_num);
            fprintf(stderr, "This doesn't appear to be a valid FASTA file\n");
            free(line);
            fclose(f);
            free(sl->items);
            free(sl);
            return NULL;
        }
    }
    
    free(line);
    fclose(f);
    
    // Validate that we found at least one header
    if (!found_header) {
        fprintf(stderr, "Error: No FASTA headers found in file '%s'\n", path);
        fprintf(stderr, "This doesn't appear to be a valid FASTA file\n");
        free(sl->items);
        free(sl);
        return NULL;
    }
    
    // Validate that we found at least one sequence
    if (!found_sequence || sl->count == 0) {
        fprintf(stderr, "Error: No sequences found in file '%s'\n", path);
        fprintf(stderr, "The FASTA file appears to be empty or corrupted\n");
        free(sl->items);
        free(sl);
        return NULL;
    }
    
    // Check for sequences without data
    for (size_t i = 0; i < sl->count; i++) {
        if (sl->items[i].len == 0) {
            fprintf(stderr, "Error: Found sequence header without sequence data\n");
            fprintf(stderr, "The FASTA file appears to be corrupted\n");
            free(sl->items);
            free(sl);
            return NULL;
        }
    }
    
    return sl;
}
