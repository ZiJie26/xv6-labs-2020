#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ            0
#define WRITE           1
#define EOF             0

void sieve(int pleft[]) {
    close(pleft[WRITE]);
    int prime;
    if (read(pleft[READ], &prime, sizeof(prime)) == EOF)
        exit(0);
    printf("prime %d\n", prime);
    int num;
    int pright[2];
    pipe(pright);
    int pid = fork();
    if (pid > 0) {
        close(pright[READ]);
        while (read(pleft[READ], &num, sizeof(num)) != EOF) {
            if (num % prime != 0)
                write(pright[WRITE], &num, sizeof(num));
        }
        close(pleft[READ]);
        close(pright[WRITE]);
        int status;
        wait(&status);
        exit(0);
    }
    else if (pid == 0) {
        sieve(pright);
        exit(0);
    }
    else {
        fprintf(2, "fork error");
        exit(1);
    }
}

int main() {
    int p[2];
    pipe(p);
    int pid = fork();
    if (pid > 0) {
        close(p[READ]);
        for (int i = 2; i <= 35; i++)
            write(p[WRITE], &i, sizeof(i));
        close(p[WRITE]);
        int status;
        wait(&status);
        exit(0);
    }
    else if (pid == 0) {
        sieve(p);
        exit(0);
    }
    else {
        fprintf(2, "fork error\n");
        exit(1);
    }
}
