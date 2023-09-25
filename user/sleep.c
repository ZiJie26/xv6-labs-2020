#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(2, "Usage: sleep NUMBER\n");
        exit(1);
    }

    uint len = strlen(argv[1]);
    for (int i = 0; i < len; i++)
        if (!('0' <= argv[1][i] && argv[1][i] <= '9')) {
            fprintf(2, "Invalid time interval '%s'", argv[1][i]);
            exit(1);
        }

    uint n = atoi(argv[1]);
    sleep(n);

    exit(0);
}
