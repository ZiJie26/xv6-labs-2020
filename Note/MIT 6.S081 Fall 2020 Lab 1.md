# MIT 6.S081 Fall 2020 Lab 1

****Xv6 and Unix utilities****  

***by ZJay/楚梓杰***

- [MIT 6.S081 Fall 2020 Lab 1](#mit-6s081-fall-2020-lab-1)
  - [项目构建](#项目构建)
    - [sleep](#sleep)
    - [pingpong](#pingpong)
    - [primes](#primes)
    - [find](#find)
    - [xargs](#xargs)
  - [time](#time)
  - [完成实验](#完成实验)
    - [提交到GitHub](#提交到github)
    - [提交到MIT](#提交到mit)

## 项目构建

### sleep

突然发现用VSCode管理文件和写代码更方便，可以进入xv6的文件夹输入`code .`调用VSCode。

我们的任务：

![picture 12](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-084827.png)  

在user文件夹里创建sleep.c文件 :

```c
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
```

由于 atoi 没有对非数字返回错误，这里我们这里处理一下 (虽然并不需要)，然后编辑 Makefile, 在 `UPROGS` 里加一行 `$U/_sleep\`。

![picture 13](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-090259.png)  

`make clean; make qemu`编译运行

![picture 14](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-091603.png)  

可以看到sleep程序已经存在了，运行`sleep 10`，可以发现`$`这个符号是过了一段时间才出现的，说明运行成功。下面是这位大佬的[实验记录](http://xv6.dgs.zone/)的描述。

![picture 15](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-092053.png)  

退出 QEMU, 运行 `./grade-lab-util sleep` 测试:

![picture 16](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-093257.png)  

但是这里又有问题了，根据[这位大佬](https://blog.csdn.net/John_Snowww/article/details/129972288)的文章，发现是因为没有evn这个文件夹，python这个文件直接就在bin文件夹里面了。
![picture 17](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-094209.png)  

我们设置一个软连接：`sudo ln -s /usr/bin/python3 /usr/bin/python` （把grade-lab-util文件第一行的python改成python3也可以）

![picture 18](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-094544.png)  

还是报错，我发现有个\r，于是在网上搜索，根据[这篇文章](https://blog.csdn.net/qq_45779334/article/details/114040911)，原因是

![picture 19](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-095044.png)  

我们`vim grade-lab-util`然后输入`:set ff=unix`，随后保存退出`:wq`即可。

再次运行命令，即可看到测试结果：

![picture 20](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-095327.png)  

下面的流程与之相似

### pingpong

![picture 21](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-101045.png)  

![picture 22](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-101137.png)  

创建user/pingpong.c

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ        0
#define WRITE       1

int p[2];

int main() {
    pipe(p);
    int pid = fork();

    if (pid > 0) {
        int status;
        write(p[WRITE], "ping", 4);
        wait(&status);
        char buf[10];
        read(p[READ], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        exit(0);
    }

    else if (pid == 0) {
        char buf[10];
        read(p[READ], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        write(p[WRITE], "pong", 4);
        exit(0);
    }

    else {
        fprintf(2, "fork error");
        exit(1);
    }
}
```

将`$U/_pingpong\`加入Makefile。

用`./grade-lab-util pingpong`测试：

![picture 23](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-101842.png)  

以下操作都类似，就只放代码了。

### primes

![picture 24](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-103906.png)  

![picture 25](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-103928.png)  

![picture 26](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-103946.png)  

**引用自[Wings](https://blog.wingszeng.top/)大佬的博客**

>先简单说明一下 CSP 并发模型.

>我们熟知的并发模型是共享内存, 通过共享的信号量和锁来进行临界资源的原子操作. 而 通信顺序过程Communicating Sequential Processes 模型是通过一个 没有缓存区的命名管道 进行的. 由于其没有缓存区, 且发送永远先于接收, 接收必须等发送完成才能进行. 于是整个多并发体系可以同步.

>有趣的是, 模型里的发送方和接收方并不知道对方, 所以消息传递可以不是线性的. (虽然这里用不到, 只是觉得很好玩)

>头大… 想了好久, 想到递归就找到路了.

>第一个进程创建一个管道, 父进程输入数字, 子进程接收并进行筛法. 子进程采用递归的做法. 递归的参数是 left neighbor.

>从其中读取第一个数, 筛法保证一定是素数. 建立一个新的管道 right neighbor 用来和它的子进程通信并同步. 父进程 (当前进程) 从 left neighbor 继续取, 如果不能被筛掉, 则写入 right neighbor 管道. 子进程 (当前进程的子进程) 调用筛法, 参数是 right neighbor 管道, 递归函数内进行下一个筛法处理.

>说不明白… 写完就有 流水线 的感觉了. 从前面接收一个, 处理这一个, 将结果交由后面处理.

>写代码的时候需要注意, 由于文件描述符有限, 所以不用的管道要 close 掉. 比如父进程只会向管道里写东西, 而不会从管道里读东西, 所以可以把读取端给 close 掉. 这个操作不会影响子进程, 因为每个进程虽然共享文件描述符, 但是都维护自己的文件打开列表, 并不共享.

>若管道的输入端不被任何一个进程打开, 则 read 不会阻塞, 而是返回 EOF. (当然 xv6 里没有 EOF 这个宏, 这里可以认为 EOF = 0)

>fork 只会在当前处产生分支, 父子进程走不同的路.

代码：

```c
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
```

进行测试

![picture 27](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-104154.png)  

### find

![picture 28](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-104811.png)  

![picture 29](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-104827.png)  

同样引用自Wings大佬：

>~~大模拟~~!很容易想到递归. 参考 `ls.c`, 递归的参数是 file path, 这样就需要在 main 调用递归函数之前判断第一个参数是不是目录. fstat 查看文件状态, 可以看到是目录还是文件. 目录可以 open, 里面存的是 struct dirent[], 表示这个目录下面的每个文件. 这里面才有文件名. 处理一下这些文件的 path, 递归调用即可. 注意 . 和 .. 不要进去就行.

```c
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
```

测试结果：

![picture 30](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-105622.png)  

### xargs

![picture 31](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-110012.png)  

![picture 32](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-110038.png)  

>这题主要是运用 fork exec 那套逻辑

代码：

```c
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
```

测试结果：

![picture 33](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-110719.png)  

## time

>不是官方上面的 uptime.

>本来写了个 time, 想着验证一下 CSP 并发筛法的时间. 结果由于文件描述符太少了, 数据跑不到大, 普通筛法和并发筛法小数据都是 1 tricks 跑完. 放一下 time 吧. 非常简单的 fork exec. 父进程调用 uptime 系统调用获取当前 tricks, 然后 wait 子进程 exec 完, 再调用一次 uptime 算时间差即可.

>UPD: 发现 main 的 argv 参数类型不用 const 限制会方便很多…

```c
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
```

## 完成实验

每次完成一个实验都可以用`make grade`对自己的这次实验进行评分

![picture 0](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230926-203235.png)  

### 提交到GitHub

![picture 34](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-115139.png)  

### 提交到MIT

>这一步不是必须，交了其实也没有分数，但是你能在MIT的网站里留下自己的痕迹hahah

![picture 35](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-141131.png)  

**提交网站在**[这里](https://6828.scripts.mit.edu/2020/handin.py/)

先填邮箱申请API然后可以用API登录（登录怎么都不能粘贴，最后手打了一遍API）

最后提交完是这样的：

![picture 36](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-141333.png)
