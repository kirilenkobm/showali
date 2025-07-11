#include <stdio.h>
#include "args.h"
#include "app.h"

int main(int argc, char **argv) {
    // Parse command-line arguments
    Args args = parse_args(argc, argv);
    
    // Handle help flag
    if (args.show_help) {
        show_help(argv[0]);
        free_args(&args);
        return 0;
    }
    
    // Handle version flag
    if (args.show_version) {
        show_version();
        free_args(&args);
        return 0;
    }
    
    // Handle argument parsing errors
    if (args.has_error) {
        fprintf(stderr, "Error: %s\n", args.error_message);
        fprintf(stderr, "Usage: %s [options] <alignment_file>\n", argv[0]);
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        free_args(&args);
        return 1;
    }
    
    // Run the main application
    int result = run_app(&args);
    
    // Cleanup
    free_args(&args);
    return result;
}
