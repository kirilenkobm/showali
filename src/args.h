#pragma once
#include <stdbool.h>

// Structure to hold parsed command-line arguments
typedef struct {
    bool no_color;
    char *filename;
    bool show_help;
    bool show_version;
    bool has_error;
    char *error_message;
} Args;

// Parse command-line arguments
Args parse_args(int argc, char **argv);

// Display help message
void show_help(const char *program_name);

// Display version information
void show_version(void);

// Free args structure
void free_args(Args *args); 