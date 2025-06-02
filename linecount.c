#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

int count_lines_in_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("Could not open file: %s\n", filename);
        return -1;
    }
    int lines = 0;
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp)) {
        lines++;
    }
    fclose(fp);
    return lines;
}

int has_c_or_h_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot) return 0;
    return (strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0);
}

#ifdef _WIN32
int count_lines_in_directory(const char* dir, int* total) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char searchPath[1024];
    snprintf(searchPath, sizeof(searchPath), "%s\\*.*", dir);
    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Could not open directory: %s\n", dir);
        return 1;
    }
    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            continue;
        char path[1024];
        snprintf(path, sizeof(path), "%s\\%s", dir, findFileData.cFileName);
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            count_lines_in_directory(path, total);
        } else if (has_c_or_h_extension(findFileData.cFileName)) {
            int lines = count_lines_in_file(path);
            if (lines >= 0) {
                printf("%s: %d\n", path, lines);
                *total += lines;
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    FindClose(hFind);
    return 0;
}
#endif

int main(int argc, char** argv) 
{
    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
#ifdef _WIN32
        int total = 0;
        if (count_lines_in_directory(argv[2], &total) == 0) {
            printf("Total lines: %d\n", total);
            return 0;
        } else {
            return 1;
        }
#else
        printf("Recursive directory traversal is not implemented for non-Windows systems.\n");
        return 1;
#endif
    } 
    else if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        int lines = count_lines_in_file(argv[2]);
        if (lines >= 0) {
            printf("%s: %d\n", argv[2], lines);
            return 0;
        } else {
            return 1;
        }
    } else {
        printf("Usage: %s -d <directory>\n", argv[0]);
        printf("Alternative Usage: %s -f <file>\n", argv[0]);
        return 1;
    }
}