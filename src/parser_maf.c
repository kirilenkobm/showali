#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser_fasta.h"

#define MAX_LINE_LENGTH 100000
#define MAX_SPECIES_NAME 256
#define MAX_ALIGNMENT_LENGTH 10000000

// Structure to hold a single MAF alignment block
typedef struct {
    char *species;
    int start;
    int size;
    char strand;
    int src_size;
    char *sequence;
} MAFSequence;

typedef struct {
    MAFSequence *sequences;
    int sequence_count;
    int alignment_length;
} MAFBlock;

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

// Free a MAF block
static void free_maf_block(MAFBlock *block) {
    if (!block) return;
    
    if (block->sequences) {
        for (int i = 0; i < block->sequence_count; i++) {
            free(block->sequences[i].species);
            free(block->sequences[i].sequence);
        }
        free(block->sequences);
    }
    free(block);
}

// Parse a single MAF alignment block
static MAFBlock *parse_maf_block(FILE *f, char **current_line, size_t *len, int *line_num) {
    MAFBlock *block = calloc(1, sizeof(MAFBlock));
    int sequences_capacity = 16;
    block->sequences = calloc(sequences_capacity, sizeof(MAFSequence));
    
    // Skip to alignment header (line starting with 'a')
    while (getline(current_line, len, f) != -1) {
        (*line_num)++;
        
        // Remove trailing newline
        size_t line_len = strlen(*current_line);
        if (line_len > 0 && (*current_line)[line_len - 1] == '\n') {
            (*current_line)[line_len - 1] = '\0';
        }
        if (line_len > 1 && (*current_line)[line_len - 2] == '\r') {
            (*current_line)[line_len - 2] = '\0';
        }
        
        // Skip empty lines and comments
        if (is_empty_line(*current_line) || (*current_line)[0] == '#') {
            continue;
        }
        
        // Found alignment header
        if ((*current_line)[0] == 'a') {
            break;
        }
        
        // If we hit a line that doesn't start with 'a', this might not be MAF format
        free_maf_block(block);
        return NULL;
    }
    
    // Parse sequence lines (lines starting with 's')
    while (getline(current_line, len, f) != -1) {
        (*line_num)++;
        
        // Remove trailing newline
        size_t line_len = strlen(*current_line);
        if (line_len > 0 && (*current_line)[line_len - 1] == '\n') {
            (*current_line)[line_len - 1] = '\0';
        }
        if (line_len > 1 && (*current_line)[line_len - 2] == '\r') {
            (*current_line)[line_len - 2] = '\0';
        }
        
        // End of block if we hit an empty line or another block
        if (is_empty_line(*current_line) || (*current_line)[0] == 'a') {
            // Put the line back for the next block
            fseek(f, -((long)strlen(*current_line) + 1), SEEK_CUR);
            (*line_num)--;
            break;
        }
        
        // Skip non-sequence lines
        if ((*current_line)[0] != 's') {
            continue;
        }
        
        // Parse sequence line: s species start size strand src_size sequence
        char species_name[MAX_SPECIES_NAME];
        int start, size, src_size;
        char strand;
        char *sequence = malloc(MAX_ALIGNMENT_LENGTH);
        if (!sequence) {
            fprintf(stderr, "Error: Failed to allocate memory for sequence buffer\n");
            free_maf_block(block);
            return NULL;
        }
        
        if (sscanf(*current_line, "s %s %d %d %c %d %s", 
                  species_name, &start, &size, &strand, &src_size, sequence) != 6) {
            fprintf(stderr, "Error: Invalid MAF sequence line at line %d\n", *line_num);
            free(sequence);
            free_maf_block(block);
            return NULL;
        }
        
        // Expand sequences array if needed
        if (block->sequence_count >= sequences_capacity) {
            sequences_capacity *= 2;
            block->sequences = realloc(block->sequences, sequences_capacity * sizeof(MAFSequence));
        }
        
        // Store sequence information
        MAFSequence *seq = &block->sequences[block->sequence_count];
        seq->species = strdup(species_name);
        seq->start = start;
        seq->size = size;
        seq->strand = strand;
        seq->src_size = src_size;
        seq->sequence = strdup(sequence);
        
        // Update alignment length
        int seq_len = strlen(sequence);
        if (seq_len > block->alignment_length) {
            block->alignment_length = seq_len;
        }
        
        block->sequence_count++;
        
        free(sequence);
    }
    
    if (block->sequence_count == 0) {
        free_maf_block(block);
        return NULL;
    }
    
    return block;
}

// Main MAF parser function
SeqList *parse_maf(const char *path) {
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
    int line_num = 0;
    
    // Parse MAF blocks and collect all unique species
    MAFBlock **blocks = NULL;
    int block_count = 0;
    int block_capacity = 16;
    blocks = calloc(block_capacity, sizeof(MAFBlock*));
    
    // Read all blocks
    while (!feof(f)) {
        MAFBlock *block = parse_maf_block(f, &line, &len, &line_num);
        if (!block) break;
        
        // Expand blocks array if needed
        if (block_count >= block_capacity) {
            block_capacity *= 2;
            blocks = realloc(blocks, block_capacity * sizeof(MAFBlock*));
        }
        
        blocks[block_count++] = block;
    }
    
    if (block_count == 0) {
        fprintf(stderr, "Error: No valid MAF blocks found in file '%s'\n", path);
        free(blocks);
        free(line);
        fclose(f);
        free(sl->items);
        free(sl);
        return NULL;
    }
    
    // Collect all unique species names
    char **species_names = calloc(256, sizeof(char*));
    int species_count = 0;
    
    for (int b = 0; b < block_count; b++) {
        MAFBlock *block = blocks[b];
        for (int s = 0; s < block->sequence_count; s++) {
            char *species = block->sequences[s].species;
            
            // Check if this species is already in our list
            int found = 0;
            for (int i = 0; i < species_count; i++) {
                if (strcmp(species_names[i], species) == 0) {
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                species_names[species_count++] = strdup(species);
            }
        }
    }
    
    // Calculate total alignment length
    int total_length = 0;
    for (int b = 0; b < block_count; b++) {
        total_length += blocks[b]->alignment_length;
    }
    
    // Create sequences for each species
    for (int i = 0; i < species_count; i++) {
        if (sl->count >= sl->capacity) {
            sl->capacity *= 2;
            sl->items = realloc(sl->items, sl->capacity * sizeof(Sequence));
        }
        
        Sequence *seq = &sl->items[sl->count];
        seq->id = strdup(species_names[i]);
        seq->seq = calloc(total_length + 1, sizeof(char));
        seq->len = 0;
        seq->type = SEQ_UNKNOWN;
        
        // Concatenate sequences from all blocks
        for (int b = 0; b < block_count; b++) {
            MAFBlock *block = blocks[b];
            int found_in_block = 0;
            
            // Find this species in the current block
            for (int s = 0; s < block->sequence_count; s++) {
                if (strcmp(block->sequences[s].species, species_names[i]) == 0) {
                    strcpy(seq->seq + seq->len, block->sequences[s].sequence);
                    seq->len += strlen(block->sequences[s].sequence);
                    found_in_block = 1;
                    break;
                }
            }
            
            // If species not found in this block, add gaps
            if (!found_in_block) {
                memset(seq->seq + seq->len, '-', block->alignment_length);
                seq->len += block->alignment_length;
                seq->seq[seq->len] = '\0';  // Ensure null termination
            }
        }
        
        // Detect sequence type
        seq->type = detect_sequence_type(seq->seq, seq->len);
        sl->count++;
    }
    
    // Clean up
    for (int i = 0; i < species_count; i++) {
        free(species_names[i]);
    }
    free(species_names);
    
    for (int b = 0; b < block_count; b++) {
        free_maf_block(blocks[b]);
    }
    free(blocks);
    
    free(line);
    fclose(f);
    
    return sl;
} 