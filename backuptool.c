#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#define mkdir _mkdir
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

int copy_file(const char *src_path, const char *dest_path) {
    FILE *src = fopen(src_path, "rb");
    if (!src) {
        perror("Failed to open source file");
        return 1;
    }

    FILE *dest = fopen(dest_path, "wb");
    if (!dest) {
        perror("Failed to open destination file");
        fclose(src);
        return 1;
    }

    char buffer[4096];
    size_t bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dest) != bytes) {
            perror("Failed to write to destination file");
            fclose(src);
            fclose(dest);
            return 1;
        }
    }

    fclose(src);
    fclose(dest);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <src_folder>\n", argv[0]);
        return 1;
    }

    const char *src_folder = argv[1];

    mkdir("backup");  // Create backup directory if it doesn't exist

#ifdef _WIN32
    WIN32_FIND_DATA findData;
    HANDLE hFind;
    char search_path[512];
    snprintf(search_path, sizeof(search_path), "%s\\*", src_folder);

    hFind = FindFirstFile(search_path, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        perror("Failed to open directory");
        return 1;
    }

    do {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
            continue;

        // Get current time
        time_t rawtime = time(NULL);
        struct tm *timeinfo = localtime(&rawtime);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d_%H%M%S", timeinfo);

        // Build paths
        char src_path[512], dest_path[512];
        snprintf(src_path, sizeof(src_path), "%s\\%s", src_folder, findData.cFileName);
        snprintf(dest_path, sizeof(dest_path), "backup\\%s_%s", findData.cFileName, timebuf);

        printf("Copying %s to %s\n", src_path, dest_path);
        copy_file(src_path, dest_path);

    } while (FindNextFile(hFind, &findData));
    FindClose(hFind);

#else
    DIR *d = opendir(src_folder);
    if (d == NULL) {
        perror("Failed to open directory");
        return 1;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;

        // Get current time
        time_t rawtime = time(NULL);
        struct tm *timeinfo = localtime(&rawtime);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d_%H%M%S", timeinfo);

        // Build paths
        char src_path[512], dest_path[512];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_folder, dir->d_name);
        snprintf(dest_path, sizeof(dest_path), "backup/%s_%s", dir->d_name, timebuf);

        printf("Copying %s to %s\n", src_path, dest_path);
        copy_file(src_path, dest_path);
    }

    closedir(d);
#endif

    return 0;
}
