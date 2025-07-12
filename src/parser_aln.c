#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include "parser_fasta.h"

#define MAX_NAME_LENGTH 256
#define MAX_SEQ_BUFFER 1000000

// Helper function to check if a line is empty or contains only whitespace
static int is_empty_line(const char *line) {
    if (!line) return 1;
    for (size_t i = 0; i < strlen(line); i++) {
        if (!isspace(line[i])) return 0;
    }
    return 1;
}

// Helper function to check if a line is a consensus line (contains *, :, ., or spaces)
static int is_consensus_line(const char *line) {
    if (!line || strlen(line) == 0) return 0;
    
    // Skip leading whitespace to find the actual content
    const char *content = line;
    while (*content && isspace(*content)) content++;
    
    if (!*content) return 0; // Empty line
    
    // Check if line contains only consensus characters
    for (const char *p = content; *p && *p != '\n' && *p != '\r'; p++) {
        if (*p != '*' && *p != ':' && *p != '.' && *p != ' ') {
            return 0; // Found non-consensus character
        }
    }
    return 1;
}

// Helper function to detect sequence type (reused from FASTA parser)
static SequenceType detect_sequence_type(const char *seq, size_t len) {
    if (len == 0) return SEQ_UNKNOWN;
    
    int dna_chars = 0;
    int rna_chars = 0;
    int total_chars = 0;
    
    for (size_t i = 0; i < len; i++) {
        char c = toupper(seq[i]);
        
        if (c == '-' || c == 'N' || c == 'X' || c == '<' || c == '>' || c == '*' || c == '.' || isspace(c)) {
            continue;
        }
        
        total_chars++;
        
        if (c == 'A' || c == 'T' || c == 'G' || c == 'C') {
            dna_chars++;
        }
        
        if (c == 'A' || c == 'U' || c == 'G' || c == 'C') {
            rna_chars++;
        }
    }
    
    if (total_chars < 4) return SEQ_UNKNOWN;
    
    double dna_pct = (double)dna_chars / total_chars;
    double rna_pct = (double)rna_chars / total_chars;
    
    if (dna_pct >= 0.95) {
        for (size_t i = 0; i < len; i++) {
            if (toupper(seq[i]) == 'U') {
                return SEQ_RNA;
            }
        }
        return SEQ_DNA;
    }
    
    if (rna_pct >= 0.95) {
        for (size_t i = 0; i < len; i++) {
            if (toupper(seq[i]) == 'U') {
                return SEQ_RNA;
            }
        }
    }
    
    return SEQ_PROTEIN;
}

// Main ALN parser function
SeqList *parse_aln(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", path);
        return NULL;
    }

    SeqList *sl = calloc(1, sizeof(SeqList));
    sl->capacity = 16;
    sl->items = calloc(sl->capacity, sizeof(Sequence));
    
    char *line = NULL;
    size_t len = 0;
    
    // Skip header line (should start with CLUSTAL)
    if (getline(&line, &len, f) == -1) {
        fprintf(stderr, "Error: Empty file '%s'\n", path);
        fclose(f);
        free(line);
        free(sl->items);
        free(sl);
        return NULL;
    }
    
    if (strncasecmp(line, "CLUSTAL", 7) != 0) {
        fprintf(stderr, "Error: Not a valid CLUSTAL file - missing CLUSTAL header\n");
        fclose(f);
        free(line);
        free(sl->items);
        free(sl);
        return NULL;
    }
    
    // Storage for sequences - we'll discover sequence names as we go
    char **sequence_names = NULL;
    char **sequence_data = NULL;
    int sequence_count = 0;
    int sequence_capacity = 0;
    
    // Parse CLUSTAL blocks
    
    while (getline(&line, &len, f) != -1) {
        
        // Remove trailing newline
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
        }
        if (line_len > 1 && line[line_len - 2] == '\r') {
            line[line_len - 2] = '\0';
        }
        
        // Skip empty lines and consensus lines
        if (is_empty_line(line) || is_consensus_line(line)) {
            continue;
        }
        
        // Parse sequence line: name followed by sequence data
        char *seq_buffer = malloc(MAX_SEQ_BUFFER);
        if (!seq_buffer) {
            fprintf(stderr, "Error: Failed to allocate memory for sequence buffer\n");
            goto cleanup_error;
        }
        
        char name_buffer[MAX_NAME_LENGTH];
        if (sscanf(line, "%s %s", name_buffer, seq_buffer) != 2) {
            free(seq_buffer);
            continue; // Skip lines that don't match expected format
        }
        
        // Found sequence line
        
        // Find or create this sequence
        int seq_index = -1;
        for (int i = 0; i < sequence_count; i++) {
            if (strcmp(sequence_names[i], name_buffer) == 0) {
                seq_index = i;
                break;
            }
        }
        
        if (seq_index == -1) {
            // New sequence - add it
            if (sequence_count >= sequence_capacity) {
                sequence_capacity = (sequence_capacity == 0) ? 16 : sequence_capacity * 2;
                sequence_names = realloc(sequence_names, sequence_capacity * sizeof(char*));
                sequence_data = realloc(sequence_data, sequence_capacity * sizeof(char*));
                if (!sequence_names || !sequence_data) {
                    fprintf(stderr, "Error: Failed to allocate memory for sequences\n");
                    free(seq_buffer);
                    goto cleanup_error;
                }
            }
            
            seq_index = sequence_count;
            sequence_names[seq_index] = strdup(name_buffer);
            sequence_data[seq_index] = calloc(MAX_SEQ_BUFFER, sizeof(char));
            sequence_count++;
            
            // Added new sequence
        }
        
        // Append sequence data (remove any spaces)
        char *clean_seq = sequence_data[seq_index] + strlen(sequence_data[seq_index]);
        for (char *p = seq_buffer; *p; p++) {
            if (!isspace(*p)) {
                *clean_seq++ = *p;
            }
        }
        *clean_seq = '\0';
        
        free(seq_buffer);
    }
    
    // Finished parsing sequences
    
    if (sequence_count == 0) {
        fprintf(stderr, "Error: No sequences found in ALN file '%s'\n", path);
        goto cleanup_error;
    }
    
    // Create final sequences
    for (int i = 0; i < sequence_count; i++) {
        if (sl->count >= sl->capacity) {
            sl->capacity *= 2;
            sl->items = realloc(sl->items, sl->capacity * sizeof(Sequence));
        }
        
        Sequence *seq = &sl->items[sl->count];
        seq->id = sequence_names[i];  // Transfer ownership
        seq->seq = sequence_data[i];  // Transfer ownership
        seq->len = strlen(seq->seq);
        seq->type = detect_sequence_type(seq->seq, seq->len);
        sl->count++;
        
        // Created sequence successfully
    }
    
    // Clean up temporary arrays (but not the data inside, which was transferred)
    free(sequence_names);
    free(sequence_data);
    free(line);
    fclose(f);
    
    return sl;
    
cleanup_error:
    // Clean up on error
    if (sequence_names) {
        for (int i = 0; i < sequence_count; i++) {
            free(sequence_names[i]);
        }
        free(sequence_names);
    }
    if (sequence_data) {
        for (int i = 0; i < sequence_count; i++) {
            free(sequence_data[i]);
        }
        free(sequence_data);
    }
    free(line);
    fclose(f);
    
    if (sl) {
        free(sl->items);
        free(sl);
    }
    
    return NULL;
} 