#include "args.h"
#include "version.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Args parse_args(int argc, char **argv) {
    Args args = {
        .no_color = false,
        .filename = NULL,
        .show_help = false,
        .show_version = false,
        .has_error = false,
        .error_message = NULL
    };
    
    // Handle version flag
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        args.show_version = true;
        return args;
    }
    
    // Handle help flag
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        args.show_help = true;
        return args;
    }
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0 || strcmp(argv[i], "-n") == 0) {
            args.no_color = true;
        } else if (args.filename == NULL) {
            args.filename = argv[i];
        } else {
            args.has_error = true;
            args.error_message = "Too many arguments";
            return args;
        }
    }
    
    if (args.filename == NULL) {
        args.has_error = true;
        args.error_message = "No input file specified";
        return args;
    }
    
    return args;
}

void show_help(const char *program_name) {
    printf("Usage: %s [options] <alignment_file>\n", program_name);
    printf("Options:\n");
    printf("  -v, --version      Show version information\n");
    printf("  -h, --help         Show this help message\n");
    printf("  -n, --no-color     Disable ANSI color codes\n");
    printf("\nControls:\n");
    printf("  Arrow keys         Navigate (hold for acceleration)\n");
    printf("  WASD               Navigate (jump half-screen)\n");
    printf("  Q                  Quit\n");
    printf("  J                  Jump to position\n");
    printf("  F                  Find\n");
    printf("  Mouse              Drag to select rectangular area\n");
    printf("  Right-click or C   Copy selection to clipboard\n");
    printf("  ESC                Clear selection\n");
    printf("                        \n");
    printf("Made in Berlin by Krlnk\n");
}

void show_version(void) {
    printf("%s %s\n", PROGRAM_NAME, VERSION_STRING);
    printf("%s\n", PROGRAM_DESCRIPTION);
    printf("Built: %s\n", VERSION_BUILD);
}

void free_args(Args *args) {
    // Currently no dynamic memory to free, but keeping for future extensibility
    (void)args;
}