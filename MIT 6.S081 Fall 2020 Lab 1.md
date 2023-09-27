# MIT 6.S081 Fall 2020 Lab 1

****Xv6 and Unix utilities****  

[toc]

by 楚梓杰

**源码可以访问我的[GitHub仓库](https://github.com/ZiJie3726/xv6-labs-2020)**

建议先看Git版本控制教程，在[这里](http://xv6.dgs.zone/labs/use_git/git1.html)

## 环境搭建

之前尝试过给笔记本装Ubuntu来一个沉浸式Linux学习，但是由于我有Onedrive多平台同步的需求，但Linux版的Onedrive用起来又太蛋疼，每次同步都把所有文件都下载下来，同步过程中中断也很麻烦，还是装回Windows采用虚拟机来进行学习。

如果之前没装过WSL的话，可以用**管理员模式**打开PowerShell或者命令行（**如果可以代理请用`wsl --install -d Ubuntu-20.04`安装**），直接用`wsl --install`命令安装。默认安装Ubuntu22.04版本，可以在微软应用商店安装其他发行版。**强烈建议本次实验用20.04版本，部署环境能省去一大部分步骤，之前用22.04非常麻烦**。

安装完别忘记**重启**！重启之后去微软应用商店搜索**Ubuntu20.04**版本安装。

>必须运行 Windows 10 版本 2004 及更高版本（内部版本 19041 及更高版本）或 Windows 11 才能使用此命令。

其他情况可以访问[官方文档](https://learn.microsoft.com/zh-cn/windows/wsl/install#prerequisites)。

VSCode可以自行百度下载，下载后安装WSL插件：

![picture 11](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-084317.png)  

打开你的Ubuntu 20.04，输入`git clone git://g.csail.mit.edu/xv6-labs-2020`把代码克隆到本地。

`cd xv6-labs-2020` ,首先更新系统

```
sudo apt update
sudo apt upgrade
```

然后一步安装所有必备工具

`sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu 
`

切换到第一个实验util分支

`git checkout util`

编译xv6并开启模拟器

`make qemu`

成功会出现这些代码

```
xv6 kernel is booting

hart 1 starting
hart 2 starting
init: starting sh
$ 
```

按下ctrl+a松开后再按x退出qemu

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

![picture 35](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-141131.png)  

**提交网站在**[这里](https://6828.scripts.mit.edu/2020/handin.py/)

先填邮箱申请API然后可以用API登录（登录怎么都不能粘贴，最后手打了一遍API）

最后提交完是这样的：

![picture 36](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-141333.png)  

## 这是之前使用22.04的环境配置过程

### 开始配置环境

首先更换阿里云的镜像，这里就不再阐述，百度搜索全是教程。
然后建立一个文件夹`MIT-6S081`用于放实验材料，最终的目录结构是这样的

```linux
MIT-6S081/
├── qemu-5.1.0
├── riscv64-toolchain
└── xv6-labs-2020
```

#### 安装QEMU 5

>为什么要用 5?

一开始凭着能简单就不要麻烦的思想，刚好apt能安装，就直接用apt装了QEMU，结果装完发现用不了，在网上搜索才知道

>If you run make qemu and the script appears to hang after
`qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0`
you’ll need to uninstall that package and install an older version

因为这个问题搜索的时候恰好找到了这位大佬的[博客](https://blog.wingszeng.top/series/learning-mit-6-s081/)，所以本实验基于这位大佬的博客和我的老师推荐的[教程](http://xv6.dgs.zone/labs/requirements/lab1.html)同时辅以我个人遇到的种种问题在网络上查的的方法，最后汇总成这篇个人学习笔记（我仅仅是做一个汇总和记录，更建议看大佬的[原文](https://blog.wingszeng.top/series/learning-mit-6-s081/))。

**继续**：进入你的文件夹，执行`wget https://download.qemu.org/qemu-5.1.0.tar.xz`(建议挂代理)，然后执行`tar -xf qemu-5.1.0.tar.xz`进行解压，进入解压后的文件夹执行`./configure --disable-werror --target-list=riscv64-softmmu
make`编译。

然而在编译的时候我遇到了问题，WSL2默认安装的这个Ubuntu是缺少我们会用到的一些工具的，所以我出现了如下的报错：

ERROR: "cc" either does not exist or does not work  

![picture 0](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-162234.png)  

ERROR: pkg-config binary 'pkg-config' not found  

![picture 1](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-162429.png)  

ERROR: glib-2.48 gthread-2.0 is required to compile QEMU  

![picture 2](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-162655.png)  

ERROR: pixman >= 0.21.8 not present. Please install the pixman devel package

这些问题可以通过以下代码解决：

```linux
sudo apt install gcc
sudo apt-get install pkg-config
sudo apt-get install  libglib2.0-dev 
sudo apt-get install libpixman-1-dev
```

没装make会出现：

![picture 3](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-163923.png)  

按他说的`sudo apt install make`就行了。

装完之后执行`make`

最后可以使用`./riscv64-softmmu/qemu-system-riscv64 --version`查看版本：

![picture 4](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-164125.png)  

#### riscv64 gnu toolchain

这里我们使用 sifive 编译好的:

```linux
cd ~/MIT-6S081/
wget https://static.dev.sifive.com/dev-tools/freedom-tools/v2020.12/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14.tar.gz
tar -zxf riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14.tar.gz -C ./riscv64-toolchain
```

![picture 5](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-164633.png)  

又出现问题了，发现解压过后子文件夹里还有一个子文件夹，好在WSL可以用Win的资源管理器，我们修改一下：

![picture 6](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-171028.png)  

从里面的文件都移出到riscv64-toolchain文件夹即可：

![picture 7](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-171240.png)  

用`./riscv64-toolchain/bin/riscv64-unknown-elf-gcc --version`检查版本：

![picture 8](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-171412.png)  

#### Boot xv6

```linux
cd ~/MIT-6S081/
git clone git://g.csail.mit.edu/xv6-labs-2020
cd xv6-labs-2020
git checkout util
```

然后需要修改一下 Makefile（`vim Makefile`编辑）, 让它使用刚刚装好的 QEMU 和 riscv toolchain.

将 46 行的 `#TOOLPREFIX =` 设置 riscv toolchain 的位置, 并带上前缀:

`TOOLPREFIX = ../riscv64-toolchain/bin/riscv64-unknown-elf-`

将 62 行的 `QEMU = qemu-system-riscv64` 设置 qemu 的位置:

`QEMU = ../qemu-5.1.0/riscv64-softmmu/qemu-system-riscv64
`

![picture 9](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-172922.png)  

输入命令
`make qemu`
在一大串代码过后成功运行

![picture 10](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230924-173554.png)  

可按`Ctrl + a,x` 退出 QEMU。
