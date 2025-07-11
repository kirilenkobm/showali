#ifndef VERSION_H
#define VERSION_H

#define VERSION_MAJOR 0
#define VERSION_MINOR 9
#define VERSION_PATCH 5

// Build version string from components
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define VERSION_STRING TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH)

#define VERSION_BUILD __DATE__ " " __TIME__

// Numeric version for comparisons
#define VERSION_NUMBER ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH)

// Program info
#define PROGRAM_NAME "showali"
#define PROGRAM_DESCRIPTION "TUI alignment viewer"

#endif // VERSION_H