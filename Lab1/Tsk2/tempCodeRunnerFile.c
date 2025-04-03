#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <linux/limits.h>

enum errors {
    SUCCESS = 0,
    ERROR_OPEN_DIR = 1,
};

int list_files(const char *dirpath) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    if ((dir = opendir(dirpath)) == NULL) {
        return ERROR_OPEN_DIR;
    }

    while ((entry = readdir(dir)) != NULL) {
        char filepath[PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

        if (stat(filepath, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        printf("%s", entry->d_name);

        if (S_ISDIR(file_stat.st_mode)) {
            printf(" (directory)");
        } else if (S_ISREG(file_stat.st_mode)) {
            printf(" (regular file)");
        } else if (S_ISLNK(file_stat.st_mode)) {
            printf(" (symbolic link)");
        } else {
            printf(" (other)");
        }

        printf(" [inode: %lu]", (unsigned long)file_stat.st_ino);

        printf(" [size: %ld bytes]", (long)file_stat.st_size);

        char timebuf[80];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
        printf(" [modified: %s]", timebuf);

        struct passwd *pw = getpwuid(file_stat.st_uid);
        struct group *gr = getgrgid(file_stat.st_gid);
        printf(" [owner: %s]", pw ? pw->pw_name : "unknown");
        printf(" [group: %s]", gr ? gr->gr_name : "unknown");

        printf("\n");
    }

    closedir(dir);
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory1> [directory2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        printf("Listing files in directory: %s\n", argv[i]);
        if (list_files(argv[i])) {
            printf("error while opening dir");
        }
        printf("\n");
    }

    return SUCCESS;
}