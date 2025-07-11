#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser_fasta.h"

#define MAX_NAME_LENGTH 256
#define MAX_SEQ_LENGTH 10000000

// Helper function to trim whitespace from both ends of a string
static char *trim_whitespace(char *str) {
    if (!str) return str;
    
    // Trim leading whitespace
    while (isspace(*str)) str++;
    
    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';
    
    return str;
}

// Helper function to check if a line is empty or contains only whitespace
static int is_empty_line(const char *line) {
    if (!line) return 1;
    for (size_t i = 0; i < strlen(line); i++) {
        if (!isspace(line[i])) return 0;
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

// Main PHY parser function
SeqList *parse_phy(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", path);
        return NULL;
    }

    SeqList *sl = calloc(1, sizeof(SeqList));
    char *line = NULL;
    size_t len = 0;
    int line_num = 0;
    int num_sequences = 0;
    int num_sites = 0;
    
    // Read header line
    if (getline(&line, &len, f) == -1) {
        fprintf(stderr, "Error: Empty file '%s'\n", path);
        fclose(f);
        free(line);
        free(sl);
        return NULL;
    }
    
    line_num++;
    
    // Parse header: number of sequences and number of sites
    if (sscanf(line, "%d %d", &num_sequences, &num_sites) != 2 || num_sequences <= 0 || num_sites <= 0) {
        fprintf(stderr, "Error: Invalid PHY header at line %d. Expected: <num_sequences> <num_sites>\n", line_num);
        fclose(f);
        free(line);
        free(sl);
        return NULL;
    }
    
    // Initialize sequence list
    sl->capacity = num_sequences;
    sl->count = 0;
    sl->items = calloc(num_sequences, sizeof(Sequence));
    
    // Try to detect if this is interleaved format by checking if we have enough sequences
    // in the first block or if sequences are incomplete
    char **sequence_names = calloc(num_sequences, sizeof(char*));
    char **sequence_data = calloc(num_sequences, sizeof(char*));
    int *sequence_lengths = calloc(num_sequences, sizeof(int));
    
    for (int i = 0; i < num_sequences; i++) {
        sequence_data[i] = calloc(num_sites + 1, sizeof(char));
        sequence_lengths[i] = 0;
    }
    
    int current_seq = 0;
    int in_first_block = 1;
    
    while (getline(&line, &len, f) != -1) {
        line_num++;
        
        // Remove trailing newline
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
        }
        if (line_len > 1 && line[line_len - 2] == '\r') {
            line[line_len - 2] = '\0';
        }
        
        // Skip empty lines (they may separate interleaved blocks)
        if (is_empty_line(line)) {
            if (in_first_block && current_seq == num_sequences) {
                in_first_block = 0;
                current_seq = 0;
            }
            continue;
        }
        
        // Parse sequence line
        char name_buffer[MAX_NAME_LENGTH];
        char *seq_buffer = malloc(MAX_SEQ_LENGTH);
        if (!seq_buffer) {
            fprintf(stderr, "Error: Failed to allocate memory for sequence buffer\n");
            goto cleanup_error;
        }
        
        if (in_first_block) {
            // First block - extract name and sequence
            
            // Try strict PHY format first (10 character names)
            if (strlen(line) >= 10) {
                strncpy(name_buffer, line, 10);
                name_buffer[10] = '\0';
                char *name = trim_whitespace(name_buffer);
                
                // Extract sequence data (everything after position 10)
                char *seq_start = line + 10;
                while (*seq_start && isspace(*seq_start)) seq_start++; // Skip whitespace
                
                if (*seq_start) {
                    // Remove spaces from sequence
                    char *seq_clean = seq_buffer;
                    for (char *p = seq_start; *p; p++) {
                        if (!isspace(*p)) {
                            *seq_clean++ = *p;
                        }
                    }
                    *seq_clean = '\0';
                    
                    // Store name and sequence
                    sequence_names[current_seq] = strdup(name);
                    strcpy(sequence_data[current_seq] + sequence_lengths[current_seq], seq_buffer);
                    sequence_lengths[current_seq] += strlen(seq_buffer);
                    current_seq++;
                    
                    if (current_seq > num_sequences) {
                        fprintf(stderr, "Error: Too many sequences at line %d\n", line_num);
                        free(seq_buffer);
                        goto cleanup_error;
                    }
                    free(seq_buffer);
                    continue;
                }
            }
            
            // Try relaxed format (space-separated name and sequence)
            char *line_copy = strdup(line);
            char *name = strtok(line_copy, " \t");
            char *seq_part = strtok(NULL, "");
            
            if (name && seq_part) {
                // Remove spaces from sequence
                char *seq_clean = seq_buffer;
                for (char *p = seq_part; *p; p++) {
                    if (!isspace(*p)) {
                        *seq_clean++ = *p;
                    }
                }
                *seq_clean = '\0';
                
                // Store name and sequence
                sequence_names[current_seq] = strdup(name);
                strcpy(sequence_data[current_seq] + sequence_lengths[current_seq], seq_buffer);
                sequence_lengths[current_seq] += strlen(seq_buffer);
                current_seq++;
                
                if (current_seq > num_sequences) {
                    fprintf(stderr, "Error: Too many sequences at line %d\n", line_num);
                    free(line_copy);
                    free(seq_buffer);
                    goto cleanup_error;
                }
                
                free(line_copy);
                free(seq_buffer);
                continue;
            }
            
            free(line_copy);
            fprintf(stderr, "Error: Invalid sequence format at line %d\n", line_num);
            free(seq_buffer);
            goto cleanup_error;
            
        } else {
            // Subsequent blocks in interleaved format - just sequence data
            char *seq_clean = seq_buffer;
            for (char *p = line; *p; p++) {
                if (!isspace(*p)) {
                    *seq_clean++ = *p;
                }
            }
            *seq_clean = '\0';
            
            strcpy(sequence_data[current_seq] + sequence_lengths[current_seq], seq_buffer);
            sequence_lengths[current_seq] += strlen(seq_buffer);
            current_seq++;
            
            if (current_seq >= num_sequences) {
                current_seq = 0;
            }
        }
        
        free(seq_buffer);
    }
    
    // Validate that we have all sequences
    if (current_seq != 0 && current_seq != num_sequences) {
        fprintf(stderr, "Error: Incomplete sequence data. Expected %d sequences, got %d\n", num_sequences, current_seq);
        goto cleanup_error;
    }
    
    // Create final sequences
    for (int i = 0; i < num_sequences; i++) {
        if (!sequence_names[i]) {
            fprintf(stderr, "Error: Missing sequence name for sequence %d\n", i + 1);
            goto cleanup_error;
        }
        
        if (sequence_lengths[i] != num_sites) {
            fprintf(stderr, "Error: Sequence '%s' has length %d, expected %d\n", 
                    sequence_names[i], sequence_lengths[i], num_sites);
            goto cleanup_error;
        }
        
        sl->items[i].id = sequence_names[i];
        sl->items[i].seq = sequence_data[i];
        sl->items[i].len = sequence_lengths[i];
        sl->items[i].type = detect_sequence_type(sequence_data[i], sequence_lengths[i]);
        sl->count++;
    }
    
    // Clean up temporary arrays
    free(sequence_names);
    free(sequence_data);
    free(sequence_lengths);
    free(line);
    fclose(f);
    
    return sl;
    
cleanup_error:
    // Clean up on error
    if (sequence_names) {
        for (int i = 0; i < num_sequences; i++) {
            free(sequence_names[i]);
        }
        free(sequence_names);
    }
    if (sequence_data) {
        for (int i = 0; i < num_sequences; i++) {
            free(sequence_data[i]);
        }
        free(sequence_data);
    }
    free(sequence_lengths);
    free(line);
    fclose(f);
    
    if (sl) {
        free(sl->items);
        free(sl);
    }
    
    return NULL;
} 