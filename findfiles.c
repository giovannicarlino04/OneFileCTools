#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#else
#include <unistd.h>
#include <fnmatch.h>
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#endif

typedef struct {
    char *pattern;
    int case_sensitive;
    int recursive;
    int show_dirs;
    int show_files;
    int max_depth;
    int current_depth;
} FindOptions;

typedef struct {
    char *path;
    int is_dir;
    size_t size;
    time_t mtime;
} FileInfo;

// Simple pattern matching (supports * and ? wildcards)
int match_pattern(const char *text, const char *pattern, int case_sensitive) {
    if (!text || !pattern) return 0;
    
    if (!case_sensitive) {
        // Convert to lowercase for case-insensitive comparison
        char *text_lower = strdup(text);
        char *pattern_lower = strdup(pattern);
        for (int i = 0; text_lower[i]; i++) text_lower[i] = tolower(text_lower[i]);
        for (int i = 0; pattern_lower[i]; i++) pattern_lower[i] = tolower(pattern_lower[i]);
        
        int result = match_pattern(text_lower, pattern_lower, 1);
        free(text_lower);
        free(pattern_lower);
        return result;
    }
    
    // Simple wildcard matching
    if (*pattern == '\0') return *text == '\0';
    if (*pattern == '*') {
        if (*(pattern + 1) == '\0') return 1; // * matches everything
        for (int i = 0; text[i]; i++) {
            if (match_pattern(text + i, pattern + 1, 1)) return 1;
        }
        return 0;
    }
    if (*pattern == '?') {
        if (*text == '\0') return 0;
        return match_pattern(text + 1, pattern + 1, 1);
    }
    if (*text != *pattern) return 0;
    return match_pattern(text + 1, pattern + 1, 1);
}

// Get file information
FileInfo get_file_info(const char *path) {
    FileInfo info = {0};
    info.path = strdup(path);
    
    struct stat st;
    if (stat(path, &st) == 0) {
        info.is_dir = S_ISDIR(st.st_mode);
        info.size = st.st_size;
        info.mtime = st.st_mtime;
    }
    
    return info;
}

// Free file info
void free_file_info(FileInfo *info) {
    if (info && info->path) {
        free(info->path);
        info->path = NULL;
    }
}

// Print file information
void print_file_info(const FileInfo *info) {
    if (!info || !info->path) return;
    
    char time_str[64];
    struct tm *tm_info = localtime(&info->mtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    if (info->is_dir) {
        printf("[DIR]  %s\n", info->path);
    } else {
        printf("[FILE] %s (%zu bytes, %s)\n", info->path, info->size, time_str);
    }
}

// Search directory recursively
void search_directory(const char *dir_path, const FindOptions *opts) {
    if (opts->current_depth > opts->max_depth) return;
    
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error opening directory '%s': %s\n", dir_path, strerror(errno));
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        
        // Build full path
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s%s%s", 
                dir_path, 
                (dir_path[strlen(dir_path)-1] == PATH_SEP) ? "" : PATH_SEP_STR,
                entry->d_name);
        
        // Check if it's a directory
        int is_dir = 0;
#ifdef _WIN32
        // On Windows, we need to use stat() to determine if it's a directory
        struct stat st;
        if (stat(full_path, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode);
        }
#else
        is_dir = (entry->d_type == DT_DIR);
        if (entry->d_type == DT_UNKNOWN) {
            struct stat st;
            if (stat(full_path, &st) == 0) {
                is_dir = S_ISDIR(st.st_mode);
            }
        }
#endif
        
        // Check if we should process this entry
        int should_process = 0;
        if (is_dir && opts->show_dirs) should_process = 1;
        if (!is_dir && opts->show_files) should_process = 1;
        
        if (should_process) {
            // Check pattern match
            if (match_pattern(entry->d_name, opts->pattern, opts->case_sensitive)) {
                FileInfo info = get_file_info(full_path);
                print_file_info(&info);
                free_file_info(&info);
            }
        }
        
        // Recursively search subdirectories
        if (is_dir && opts->recursive) {
            FindOptions sub_opts = *opts;
            sub_opts.current_depth++;
            search_directory(full_path, &sub_opts);
        }
    }
    
    closedir(dir);
}

// Print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s [options] <pattern> [directory]\n", program_name);
    printf("Options:\n");
    printf("  -r, --recursive    Search subdirectories recursively\n");
    printf("  -d, --directories  Show directories\n");
    printf("  -f, --files        Show files (default)\n");
    printf("  -i, --ignore-case  Case-insensitive pattern matching\n");
    printf("  -m, --max-depth N  Maximum recursion depth (default: 10)\n");
    printf("  -h, --help         Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s *.txt                    # Find all .txt files in current directory\n", program_name);
    printf("  %s -r *.c src/             # Find all .c files recursively in src/\n", program_name);
    printf("  %s -d -i *test*            # Find directories containing 'test' (case-insensitive)\n", program_name);
}

// Parse command line arguments
int parse_args(int argc, char *argv[], FindOptions *opts, char **pattern, char **search_dir) {
    // Set defaults
    opts->case_sensitive = 1;
    opts->recursive = 0;
    opts->show_dirs = 0;
    opts->show_files = 1;
    opts->max_depth = 10;
    opts->current_depth = 0;
    
    *pattern = NULL;
    *search_dir = ".";
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            opts->recursive = 1;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--directories") == 0) {
            opts->show_dirs = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--files") == 0) {
            opts->show_files = 1;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--ignore-case") == 0) {
            opts->case_sensitive = 0;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--max-depth") == 0) {
            if (i + 1 < argc) {
                opts->max_depth = atoi(argv[++i]);
                if (opts->max_depth < 0) opts->max_depth = 10;
            }
        } else if (argv[i][0] != '-') {
            if (!*pattern) {
                *pattern = strdup(argv[i]);
            } else if (!*search_dir) {
                *search_dir = strdup(argv[i]);
            } else {
                fprintf(stderr, "Too many arguments\n");
                return 0;
            }
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 0;
        }
    }
    
    if (!*pattern) {
        fprintf(stderr, "Pattern is required\n");
        print_usage(argv[0]);
        return 0;
    }
    
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    FindOptions opts;
    char *pattern, *search_dir;
    
    if (!parse_args(argc, argv, &opts, &pattern, &search_dir)) {
        return 1;
    }
    
    printf("Searching for pattern '%s' in '%s'\n", pattern, search_dir);
    if (opts.recursive) {
        printf("Recursive search enabled (max depth: %d)\n", opts.max_depth);
    }
    printf("Showing: %s%s\n", 
           opts.show_files ? "files " : "",
           opts.show_dirs ? "directories" : "");
    printf("---\n");
    
    // Set the pattern in the options struct
    opts.pattern = pattern;
    
    search_directory(search_dir, &opts);
    
    // Cleanup allocated memory
    if (pattern) free(pattern);
    if (search_dir && search_dir != ".") free(search_dir);
    
    return 0;
}
