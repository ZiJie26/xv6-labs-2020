#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int getline(char *buf) {
    int cnt = 0;
    while (read(0, buf, 1)) {
        if (++cnt >= 256) {
            fprintf(2, "xargs: line too long\n");
            exit(1);
        }
        if (*buf++ == '\n')
            break;
    }
    *(buf-1) = 0;
    return cnt;
}

int prase(char *buf, char **nargv, const int max) {
    int cnt = 0;
    while (*buf) {
        char *last = buf - 1;
        if (*last == 0 && *buf != ' ') {
            if (cnt >= max) {
                fprintf(2, "xargs: args too many\n");
                exit(1);
            }
            nargv[cnt++] = buf;
        }
        else if (*buf == ' ')
            *buf = 0;
        buf++;
    }
    nargv[cnt] = 0;
    return cnt;
}

int main(int argc, char const *argv[]) {
    if (argc == 1) {
        fprintf(2, "Usage: xargs COMMAND ...\n");
        exit(1);
    }
    char buf[257] = {0};
    while (getline(buf + 1)) {
        char *nargv[MAXARG + 1];
        memcpy(nargv, argv + 1, sizeof(char *) * (argc - 1));
        prase(buf + 1, nargv + argc - 1, MAXARG - argc + 1);
        int pid = fork();
        if (pid > 0) {
            int status;
            wait(&status);
        }
        else if (pid == 0) {
            char cmd[128];
            strcpy(cmd, argv[1]);
            exec(cmd, nargv);
        }
        else {
            fprintf(2, "fork error\n");
            exit(1);
        }
    }
    exit(0);
}
