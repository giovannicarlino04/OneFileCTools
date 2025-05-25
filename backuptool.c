#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>


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

    DIR *d = opendir(argv[1]);
    if (d == NULL) {
        perror("Failed to open directory");
        return 1;
    }

    struct dirent *dir;

    while ((dir = readdir(d)) != NULL) {
        // Skip "." and ".."
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;

        // Get current time
        time_t rawtime;
        struct tm *timeinfo;
        char timebuf[64];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        // Format time as YYYY-MM-DD_HHMMSS without newline
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d_%H%M%S", timeinfo);

        // Print filename with timestamp
        printf("File: %s - Timestamp: %s\n", dir->d_name, timebuf);

        char src_path[512];
        char dest_path[512];
        mkdir("backup");

        snprintf(src_path, sizeof(src_path), "%s/%s", argv[1], dir->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s_%s", "./backup", dir->d_name, timebuf);
        printf("Copying %s to %s\n", src_path, dest_path);
        copy_file(src_path, dest_path);
    
    }
    
    closedir(d);
    return 0;
}
