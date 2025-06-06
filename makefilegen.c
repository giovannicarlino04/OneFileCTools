#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_FILES 128
#define MAX_LINE 512

int has_main_function(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;

    char line[MAX_LINE];
    int result = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "main(")) {
            result = 1;
            break;
        }
    }

    fclose(f);
    return result;
}

int str_end_cmp(const char *string, const char* endstring) {
    const char *ext = strrchr(string, '.');
    return (ext && strcmp(ext, endstring) == 0);
}

typedef struct {
    char name[256];
    int is_cpp;
} SourceFile;

int main(int argc, char** argv) {
    
    if(argc != 2){
        printf("Usage: %s <src_folder>", argv[0]);
        return 1;
    }


    DIR *d = opendir(argv[1]);
    struct dirent *dir;

    SourceFile sources[MAX_FILES];
    int file_count = 0;
    int has_cpp = 0;

    while ((dir = readdir(d)) != NULL) {
        if (str_end_cmp(dir->d_name, ".c") || str_end_cmp(dir->d_name, ".cpp")) {
            if (file_count >= MAX_FILES) break;

            strncpy(sources[file_count].name, dir->d_name, sizeof(sources[file_count].name));
            sources[file_count].is_cpp = str_end_cmp(dir->d_name, ".cpp");
            if (sources[file_count].is_cpp) has_cpp = 1;
            file_count++;
        }
    }
    closedir(d);

    if (file_count == 0) {
        fprintf(stderr, "No .c or .cpp files found.\n");
        return 1;
    }

    FILE *out = fopen("Makefile", "w");
    if (!out) {
        perror("Cannot write Makefile");
        return 1;
    }

    fprintf(out, "CC=gcc\n");
    if (has_cpp)
        fprintf(out, "CXX=g++\n");
    fprintf(out, "CFLAGS=-Wall -O2\n");
    if (has_cpp)
        fprintf(out, "CXXFLAGS=-Wall -O2 -std=c++17\n");

    // Object files
    fprintf(out, "OBJ=");
    for (int i = 0; i < file_count; i++) {
        const char *dot = strrchr(sources[i].name, '.');
        if (!dot) continue;
        size_t len = dot - sources[i].name;
        fprintf(out, "%.*s.o ", (int)len, sources[i].name);
    }
    fprintf(out, "\n\n");

    fprintf(out, "all: my_program\n\n");

    // Use CXX for linking if we have any .cpp
    fprintf(out, "my_program: $(OBJ)\n");
    if (has_cpp)
        fprintf(out, "\t$(CXX) $(CXXFLAGS) -o $@ $(OBJ)\n\n");
    else
        fprintf(out, "\t$(CC) $(CFLAGS) -o $@ $(OBJ)\n\n");

    // Compilation rules
    fprintf(out, "%%.o: %%.c\n");
    fprintf(out, "\t$(CC) $(CFLAGS) -c $< -o $@\n\n");

    if (has_cpp) {
        fprintf(out, "%%.o: %%.cpp\n");
        fprintf(out, "\t$(CXX) $(CXXFLAGS) -c $< -o $@\n\n");
    }

    fprintf(out, "clean:\n");
    fprintf(out, "\trm -f *.o my_program\n");

    fclose(out);
    printf("Makefile generated successfully.\n");
    return 0;
}
