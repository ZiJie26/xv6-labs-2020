#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char const *argv[]) {
    if (argc == 1) {
        fprintf(2, "Usage: time COMMAND ...\n");
        exit(1);
    }
    int pid = fork();
    if (pid > 0) {
        int start = uptime();
        int status;
        wait(&status);
        int end = uptime();
        printf("==============\ntime ticks: %d\n", end - start);
    }
    else if (pid == 0) {
        char cmd[128];
        strcpy(cmd, argv[1]);
        char *nargv[MAXARG + 1];
        memcpy(nargv, argv + 1, sizeof(char *) * (argc - 1));
        exec(cmd, nargv);
    }
    exit(0);
}
