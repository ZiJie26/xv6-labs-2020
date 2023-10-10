# MIT 6.S081 Fall 2020 Lab 2

****System calls****

- [MIT 6.S081 Fall 2020 Lab 2](#mit-6s081-fall-2020-lab-2)
  - [开始前](#开始前)
  - [System call tracing](#system-call-tracing)
    - [实验代码部分](#实验代码部分)
      - [`kernel/syscall.h`](#kernelsyscallh)
      - [`user/usys.pl`](#userusyspl)
      - [`user/user.h`](#useruserh)
      - [`kernel/proc.h`](#kernelproch)
      - [`kernel/sysproc.c`](#kernelsysprocc)
      - [`kernel/proc.c`](#kernelprocc)
      - [`kernel/syscall.c`](#kernelsyscallc)
      - [`trace.c`](#tracec)
      - [编译执行](#编译执行)
  - [Sysinfo（moderate）](#sysinfomoderate)
    - [实验代码部分](#实验代码部分-1)
      - [`kernel/defs.h`](#kerneldefsh)
      - [`kernel/kalloc.c`](#kernelkallocc)
      - [`kernel/proc.c`](#kernelprocc-1)
      - [`kernel/syscall.c`](#kernelsyscallc-1)
      - [`kernel/syscall.h`](#kernelsyscallh-1)
      - [`kernel/sysproc.c`](#kernelsysprocc-1)
      - [`user/user.h`](#useruserh-1)
      - [`user/usys.pl`](#userusyspl-1)
      - [编译运行](#编译运行)
  - [make grade](#make-grade)
  - [Note](#note)
    - [系统调用全流程](#系统调用全流程)
    - [空闲内存页](#空闲内存页)

## 开始前

先切到syscall分支。

```linux
git fetch
git checkout syscall
make clean
```

注意不要使用`git checkout -b`（这是从当前分支创建新分支），我们需要从origin获取syscall分支。

## System call tracing

![picture 1](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%202/IMG_20230925-233432.png)  

```linux
$ trace 32 grep hello README
3: syscall read -> 1023
3: syscall read -> 966
3: syscall read -> 70
3: syscall read -> 0
$
$ trace 2147483647 grep hello README
4: syscall trace -> 0
4: syscall exec -> 3
4: syscall open -> 3
4: syscall read -> 1023
4: syscall read -> 966
4: syscall read -> 70
4: syscall read -> 0
4: syscall close -> 0
$
$ grep hello README
$
$ trace 2 usertests forkforkfork
usertests starting
test forkforkfork: 407: syscall fork -> 408
408: syscall fork -> 409
409: syscall fork -> 410
410: syscall fork -> 411
409: syscall fork -> 412
410: syscall fork -> 413
409: syscall fork -> 414
411: syscall fork -> 415
...
$
```

![picture 2](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%202/IMG_20230925-233513.png)  

### 实验代码部分

#### `kernel/syscall.h`

在 syscall.h 中加入新 system call 的序号：

```c
 #define SYS_mkdir  20
 #define SYS_close  21
 #define SYS_trace  22 // 在这里
```

#### `user/usys.pl`

在 usys.pl 中，加入用户态到内核态的跳板函数。

```python
 entry("sbrk");
 entry("sleep");
 entry("uptime");
 entry("trace");  # 这里
```

>这个脚本在运行后会生成 usys.S 汇编文件，里面定义了每个 system call 的用户态跳板函数：

```python
 trace:  # 定义用户态跳板函数
 li a7, SYS_trace # 将系统调用 id 存入 a7 寄存器
 ecall    # ecall，调用 system call ，跳到内核态的统一系统调用处理函数 syscall()  (syscall.c)
 ret
```

#### `user/user.h`

在用户态的头文件加入定义，使得用户态程序可以找到这个跳板入口函数。

```c
 char* sbrk(int);
 int sleep(int);
 int uptime(void);
 int trace(int);  // 这里
```

#### `kernel/proc.h`

在 proc.h 中修改 proc 结构的定义，添加 syscall_trace field，用 mask 的方式记录要 trace 的 system call

```c
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  struct proc *parent;         // Parent process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  uint64 syscall_trace;        // Mask for syscall tracing (新添加的用于标识追踪哪些 system call 的 mask)
};
```

#### `kernel/sysproc.c`

在文件中加入

```c
 uint64
 sys_trace(void)
 {
 int mask;

 if(argint(0, &mask) < 0)
     return -1;

 myproc()->syscall_trace = mask;
 return 0;
 }
```

>这里因为我们的系统调用会对进程进行操作，所以放在 sysproc.c 较为合适。

在 sysproc.c 中，实现 system call 的具体代码，也就是设置当前进程的 syscall_trace mask：

```c
uint64
sys_trace(void)
{
  int mask;

  if(argint(0, &mask) < 0) // 通过读取进程的 trapframe，获得 mask 参数
    return -1;
  
  myproc()->syscall_trace = mask; // 设置调用进程的 syscall_trace mask
  return 0;
}
```

#### `kernel/proc.c`

在 proc.c 中，创建新进程的时候，为新添加的 syscall_trace 附上默认值 0（否则初始状态下可能会有垃圾数据）。

``` c
static struct proc*
allocproc(void)
{
  ......

  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  p->syscall_trace = 0; // (newly added) 为 syscall_trace 设置一个 0 的默认值

  return p;
}
```

修改 fork 函数，使得子进程可以继承父进程的 syscall_trace mask：

```c
int
fork(void)
{
  ......

  safestrcpy(np->name, p->name, sizeof(p->name));

  np->syscall_trace = p->syscall_trace; // HERE!!! 子进程继承父进程的 syscall_trace

  pid = np->pid;

  np->state = RUNNABLE;

  release(&np->lock);

  return pid;
}
```

#### `kernel/syscall.c`

用 extern 全局声明新的内核调用函数，并且在 syscalls 映射表中，加入从前面定义的编号到系统调用函数指针的映射

```c
 extern uint64 sys_wait(void);
 extern uint64 sys_write(void);
 extern uint64 sys_uptime(void);
 extern uint64 sys_trace(void);   // 这里
```

```c
 [SYS_link]    sys_link,
 [SYS_mkdir]   sys_mkdir,
 [SYS_close]   sys_close,
 [SYS_trace]   sys_trace,  // 还有这里
```

由于需要打印系统调用的名称, 所以我们还要创建一个字符串数组,写入以下内容：

```c
const char *syscall_names[] = {
[SYS_fork]    "fork",
[SYS_exit]    "exit",
[SYS_wait]    "wait",
[SYS_pipe]    "pipe",
[SYS_read]    "read",
[SYS_kill]    "kill",
[SYS_exec]    "exec",
[SYS_fstat]   "fstat",
[SYS_chdir]   "chdir",
[SYS_dup]     "dup",
[SYS_getpid]  "getpid",
[SYS_sbrk]    "sbrk",
[SYS_sleep]   "sleep",
[SYS_uptime]  "uptime",
[SYS_open]    "open",
[SYS_write]   "write",
[SYS_mknod]   "mknod",
[SYS_unlink]  "unlink",
[SYS_link]    "link",
[SYS_mkdir]   "mkdir",
[SYS_close]   "close",
[SYS_trace]   "trace",
};
```

>这里 [SYS_trace] sys_trace 是 C 语言数组的一个语法，表示以方括号内的值作为元素下标。比如 int arr[] = {[3] 2333, [6] 6666} 代表 arr 的下标 3 的元素为 2333，下标 6 的元素为 6666，其他元素填充 0 的数组。（该语法在 C++ 中已不可用）

根据上方提到的系统调用的全流程，可以知道，所有的系统调用到达内核态后，都会进入到 syscall() 这个函数进行处理，所以要跟踪所有的内核函数，只需要在 syscall() 函数里埋点就行了。

```c
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) { // 如果系统调用编号有效
    p->trapframe->a0 = syscalls[num](); // 通过系统调用编号，获取系统调用处理函数的指针，调用并将返回值存到用户进程的 a0 寄存器中
 // 如果当前进程设置了对该编号系统调用的 trace，则打出 pid、系统调用名称和返回值。
    if((p->syscall_trace >> num) & 1) {
      printf("%d: syscall %s -> %d\n",p->pid, syscall_names[num], p->trapframe->a0); // syscall_names[num]: 从 syscall 编号到 syscall 名的映射表
    }
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
```

上面打出日志的过程还需要知道系统调用的名称字符串，在这里定义一个字符串数组进行映射：

```c
const char *syscall_names[] = {
...
[SYS_mkdir]   "mkdir",
[SYS_close]   "close",
[SYS_trace]   "trace",
};
```

#### `trace.c`

增加这些头文件。

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
```

#### 编译执行

最后别忘了在Makefile加上`$U/_trace\`。

![picture 0](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%202/IMG_20231005-144644.png)  

成功追踪并打印出相应的系统调用。

## Sysinfo（moderate）

![picture 2](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%202/IMG_20231010-085044.png)  

### 实验代码部分

#### `kernel/defs.h`

在内核的头文件中声明计算空闲内存和获取运行的进程数
的函数，因为是内存相关的，所以放在 kalloc、kfree 等函数的的声明之后。

```c
void*           kalloc(void);
void            kfree(void *);
void            kinit(void);
uint64          count_free_mem(void);//here
uint64          count_process(void);//and here

// log.c
void            initlog(int, struct superblock*);
```

#### `kernel/kalloc.c`

在 kalloc.c 中添加计算空闲内存的函数：

```c
uint64
count_free_mem(void) // added for counting free memory in bytes (lab2)
{
  acquire(&kmem.lock);// 必须先锁内存管理结构，防止竞态条件出现
  
  // 统计空闲页数，乘上页大小 PGSIZE 就是空闲的内存字节数
  uint64 mem_bytes = 0;
  struct run *r = kmem.freelist;
  while(r){
    mem_bytes += PGSIZE;
    r = r->next;
  }
  release(&kmem.lock);
  return mem_bytes;
}
```

#### `kernel/proc.c`

在 proc.c 中实现获取运行的进程数的函数：

```c
uint64
count_process(void) { // added function for counting used process slots (lab2)
  uint64 cnt = 0;
  for(struct proc *p = proc; p < &proc[NPROC]; p++) {
    // acquire(&p->lock);
    // no need to lock since all we do is reading, no writing will be done to the proc.
    // 不需要锁进程 proc 结构，因为我们只需要读取进程列表，不需要写
    if(p->state != UNUSED) {// 不是 UNUSED 的进程位，就是已经分配的
      cnt++;
    }
  }
  return cnt;
}
```

#### `kernel/syscall.c`

```c
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);
extern uint64 sys_trace(void);
extern uint64 sys_sysinfo(void);//here

static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
```

```c
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_sysinfo]   sys_sysinfo,//here
};
```

```c
[SYS_mkdir]   "mkdir",
[SYS_close]   "close",
[SYS_trace]   "trace",
[SYS_sysinfo]   "sysinfo",//here
};
```

#### `kernel/syscall.h`

```c
#define SYS_mkdir  20
#define SYS_close  21
#define SYS_trace  22
#define SYS_sysinfo  23//add
```

#### `kernel/sysproc.c`

```c
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"//add
```

这是具体系统信息函数的实现，其中调用了前面实现的 count_free_mem() 和 count_process()：

```c
uint64
sys_sysinfo(void)
{
  uint64 addr;

  if(argaddr(0, &addr) < 0)
    return -1;
  
  struct sysinfo sinfo;
  sinfo.freemem = count_free_mem(); // kalloc.c
  sinfo.nproc = count_process(); // proc.c
  
  // copy sysinfo to user space
  if(copyout(myproc()->pagetable, addr, (char *)&sinfo, sizeof(sinfo)) < 0)
    return -1;
  return 0;
}//add at last
```

#### `user/user.h`

在 user.h 提供用户态入口：

```c
int sleep(int);
int uptime(void);
int trace(int);
struct sysinfo;//add
int sysinfo(struct sysinfo *);//add
```

#### `user/usys.pl`

```c
entry("sleep");
entry("uptime");
entry("trace");
entry("sysinfo");//add
```

#### 编译运行

![picture 3](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%202/IMG_20231010-192604.png)  

## make grade

![picture 0](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%202/IMG_20230928-142533.png)  

## Note

### 系统调用全流程

```c
user/user.h  用户态程序调用跳板函数 trace()
user/usys.S  跳板函数 trace() 使用 CPU 提供的 ecall 指令，调用到内核态
kernel/syscall.c 到达内核态统一系统调用处理函数 syscall()，所有系统调用都会跳到这里来处理。
kernel/syscall.c syscall() 根据跳板传进来的系统调用编号，查询 syscalls[] 表，找到对应的内核函数并调用。
kernel/sysproc.c 到达 sys_trace() 函数，执行具体内核操作
```

这么繁琐的调用流程的主要目的是实现用户态和内核态的良好隔离。

并且由于内核与用户进程的页表不同，寄存器也不互通，所以参数无法直接通过 C 语言参数的形式传过来，而是需要使用 argaddr、argint、argstr 等系列函数，从进程的 trapframe 中读取用户进程寄存器中的参数。

同时由于页表不同，指针也不能直接互通访问（也就是内核不能直接对用户态传进来的指针进行解引用），而是需要使用 copyin、copyout 方法结合进程的页表，才能顺利找到用户态指针（逻辑地址）对应的物理内存地址。（在本 lab 第二个实验会用到）

```text
struct proc *p = myproc(); // 获取调用该 system call 的进程的 proc 结构
copyout(p->pagetable, addr, (char *)&data, sizeof(data)); // 将内核态的 data 变量（常为struct），结合进程的页表，写到进程内存空间内的 addr 地址处。
```

### 空闲内存页

xv6 中，空闲内存页的记录方式是，将空虚内存页本身直接用作链表节点，形成一个空闲页链表，每次需要分配，就把链表根部对应的页分配出去。每次需要回收，就把这个页作为新的根节点，把原来的 freelist 链表接到后面。注意这里是直接使用空闲页本身作为链表节点，所以不需要使用额外空间来存储空闲页链表，在 kalloc() 里也可以看到，分配内存的最后一个阶段，是直接将 freelist 的根节点地址（物理地址）返回出去了：

```c
// kernel/kalloc.c
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist; // 获得空闲页链表的根节点
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r; // 把空闲页链表的根节点返回出去，作为内存页使用（长度是 4096）
}
```

**常见的记录空闲页的方法有：空闲表法、空闲链表法、位示图法（位图法）、成组链接法**。这里 xv6 采用的是空闲链表法。
