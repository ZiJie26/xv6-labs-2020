#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

void find(const char *path, const char *name) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        exit(1);
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot fstat %s\n", path);
        close(fd);
        exit(1);
    }
    if (st.type == T_FILE) {
        int i;
        for (i = strlen(path); path[i-1] != '/'; i--);
        if (strcmp(path + i, name) == 0)
            printf("%s\n", path);
    }
    else if (st.type == T_DIR) {
        char buf[256];
        strcpy(buf, path);
        char *p = buf + strlen(buf);
        *p++ = '/';
        struct dirent de;
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if(de.inum == 0)
                continue;
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("find: path too long\n");
                close(fd);
                exit(1);
            }
            memmove(p, de.name, DIRSIZ);
            find(buf, name);
        }
    }
    close(fd);
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: find DIR FILENAME\n");
        exit(1);
    }
    char dir[256];
    if (strlen(argv[1]) > 255) {
        printf("find: path too long\n");
        exit(1);
    }
    strcpy(dir, argv[1]);
    char *p = dir + strlen(dir) - 1;
    while (*p == '/')
        *p-- = 0;
    int fd = open(dir, O_RDONLY);
    if (fd < 0) {
        fprintf(2, "find: cannot open %s\n", dir);
        exit(1);
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot fstat %s\n", dir);
        close(fd);
        exit(1);
    }
    if (st.type != T_DIR) {
        fprintf(2, "'%s' is not a directory\n", argv[1]);
        close(fd);
        exit(1);
    }
    find(dir, argv[2]);
    exit(0);
}
