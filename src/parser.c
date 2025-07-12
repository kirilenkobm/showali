#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include "parser.h"

// Helper function to read first few lines of a file for format detection
static char **read_file_lines(const char *filename, int max_lines, int *lines_read) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        *lines_read = 0;
        return NULL;
    }

    char **lines = calloc(max_lines, sizeof(char*));
    char *line = NULL;
    size_t len = 0;
    int count = 0;

    while (count < max_lines && getline(&line, &len, f) != -1) {
        // Remove trailing newline
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
        }
        if (line_len > 1 && line[line_len - 2] == '\r') {
            line[line_len - 2] = '\0';
        }
        
        lines[count] = strdup(line);
        count++;
    }

    free(line);
    fclose(f);
    *lines_read = count;
    return lines;
}

// Helper function to free lines array
static void free_lines(char **lines, int count) {
    if (!lines) return;
    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);
}

// Helper function to check if a string looks like a sequence (DNA/RNA/Protein)
static int looks_like_sequence(const char *line) {
    if (!line || strlen(line) == 0) return 0;
    
    int valid_chars = 0;
    int total_chars = 0;
    
    for (size_t i = 0; i < strlen(line); i++) {
        char c = toupper(line[i]);
        if (isalpha(c) || c == '-' || c == '.' || c == 'N' || c == 'X' || c == '*') {
            valid_chars++;
        }
        if (!isspace(c)) {
            total_chars++;
        }
    }
    
    // At least 70% of non-space characters should be valid sequence characters
    return total_chars > 0 && (valid_chars * 100 / total_chars) >= 70;
}

// Auto-detection logic
AlignmentFormat detect_format(const char *filename) {
    if (!filename) return FORMAT_UNKNOWN;
    
    // First, check file extension as a hint
    const char *ext = strrchr(filename, '.');
    if (ext) {
        ext++; // Skip the dot
        if (strcasecmp(ext, "fasta") == 0 || strcasecmp(ext, "fa") == 0 || 
            strcasecmp(ext, "fas") == 0 || strcasecmp(ext, "fna") == 0) {
            return FORMAT_FASTA;
        }
        if (strcasecmp(ext, "phy") == 0 || strcasecmp(ext, "phylip") == 0) {
            return FORMAT_PHY;
        }
        if (strcasecmp(ext, "maf") == 0) {
            return FORMAT_MAF;
        }
        if (strcasecmp(ext, "aln") == 0 || strcasecmp(ext, "clustal") == 0) {
            return FORMAT_ALN;
        }
    }
    
    // Read first few lines to analyze content
    int lines_read;
    char **lines = read_file_lines(filename, 10, &lines_read);
    
    if (!lines || lines_read == 0) {
        free_lines(lines, lines_read);
        return FORMAT_UNKNOWN;
    }
    
    // Check for FASTA format (starts with >)
    if (lines[0] && lines[0][0] == '>') {
        free_lines(lines, lines_read);
        return FORMAT_FASTA;
    }
    
    // Check for ALN/Clustal format (starts with CLUSTAL)
    if (lines[0] && strncasecmp(lines[0], "CLUSTAL", 7) == 0) {
        free_lines(lines, lines_read);
        return FORMAT_ALN;
    }
    
    // Check for MAF format (starts with ##maf or has alignment blocks)
    if (lines[0] && strncmp(lines[0], "##maf", 5) == 0) {
        free_lines(lines, lines_read);
        return FORMAT_MAF;
    }
    
    // Look for MAF alignment blocks (lines starting with 'a', 's', etc.)
    for (int i = 0; i < lines_read; i++) {
        if (lines[i] && strlen(lines[i]) > 0) {
            char first_char = lines[i][0];
            if (first_char == 'a' || first_char == 's' || first_char == 'i' || first_char == 'e' || first_char == 'q') {
                // Check if this looks like MAF format
                char *space_pos = strchr(lines[i], ' ');
                if (space_pos && space_pos > lines[i] && space_pos[1] != '\0') {
                    free_lines(lines, lines_read);
                    return FORMAT_MAF;
                }
            }
        }
    }
    
    // Check for PHY format (first line should be two numbers: num_sequences num_sites)
    if (lines[0] && strlen(lines[0]) > 0) {
        // Try to parse first line as two integers
        char *line_copy = strdup(lines[0]);
        char *token1 = strtok(line_copy, " \t");
        char *token2 = strtok(NULL, " \t");
        char *token3 = strtok(NULL, " \t");
        
        if (token1 && token2 && !token3) {
            // Check if both tokens are numbers
            char *endptr1, *endptr2;
            long num1 = strtol(token1, &endptr1, 10);
            long num2 = strtol(token2, &endptr2, 10);
            
            if (*endptr1 == '\0' && *endptr2 == '\0' && num1 > 0 && num2 > 0) {
                // First line looks like PHY format header
                // Check if subsequent lines look like sequences
                if (lines_read > 1 && lines[1] && strlen(lines[1]) > 0) {
                    // Check if second line has a name followed by sequence data
                    char *line2_copy = strdup(lines[1]);
                    char *name = strtok(line2_copy, " \t");
                    char *seq_data = strtok(NULL, "");
                    
                    if (name && seq_data && looks_like_sequence(seq_data)) {
                        free(line2_copy);
                        free(line_copy);
                        free_lines(lines, lines_read);
                        return FORMAT_PHY;
                    }
                    free(line2_copy);
                }
            }
        }
        free(line_copy);
    }
    
    free_lines(lines, lines_read);
    return FORMAT_UNKNOWN;
}

// Main parsing function with auto-detection
ParseResult *parse_alignment(const char *filename) {
    AlignmentFormat format = detect_format(filename);
    return parse_alignment_with_format(filename, format);
}

// Parse with specified format
ParseResult *parse_alignment_with_format(const char *filename, AlignmentFormat format) {
    ParseResult *result = calloc(1, sizeof(ParseResult));
    result->format = format;
    result->sequences = NULL;
    result->error_message = NULL;
    
    switch (format) {
        case FORMAT_FASTA:
            result->sequences = parse_fasta(filename);
            if (!result->sequences) {
                result->error_message = strdup("Failed to parse FASTA file");
            }
            break;
            
        case FORMAT_PHY:
            result->sequences = parse_phy(filename);
            if (!result->sequences) {
                result->error_message = strdup("Failed to parse PHY file");
            }
            break;
            
        case FORMAT_MAF:
            result->sequences = parse_maf(filename);
            if (!result->sequences) {
                result->error_message = strdup("Failed to parse MAF file");
            }
            break;
            
        case FORMAT_ALN:
            result->sequences = parse_aln(filename);
            if (!result->sequences) {
                result->error_message = strdup("Failed to parse ALN file");
            }
            break;
            
        default:
            result->error_message = strdup("Unknown or unsupported file format");
            break;
    }
    
    return result;
}

// Free parse result
void free_parse_result(ParseResult *result) {
    if (!result) return;
    
    if (result->sequences) {
        // Free sequences (assuming the existing SeqList has proper cleanup)
        for (size_t i = 0; i < result->sequences->count; i++) {
            free(result->sequences->items[i].id);
            free(result->sequences->items[i].seq);
        }
        free(result->sequences->items);
        free(result->sequences);
    }
    
    free(result->error_message);
    free(result);
}

// Helper functions
const char *format_to_string(AlignmentFormat format) {
    switch (format) {
        case FORMAT_FASTA: return "FASTA";
        case FORMAT_PHY: return "PHYLIP";
        case FORMAT_MAF: return "MAF";
        case FORMAT_ALN: return "CLUSTAL";
        default: return "Unknown";
    }
}

const char *format_to_extension(AlignmentFormat format) {
    switch (format) {
        case FORMAT_FASTA: return "fasta";
        case FORMAT_PHY: return "phy";
        case FORMAT_MAF: return "maf";
        case FORMAT_ALN: return "aln";
        default: return "txt";
    }
} 