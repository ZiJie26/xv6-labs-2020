# MIT 6.S081 Fall 2020 Lab 4

****traps****

***by ZJay/楚梓杰***

[toc]

![picture 0](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231106_142229.png)  

```linux
git fetch
git checkout traps
make clean
```

## RISC-V assembly (easy)

![picture 1](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231106_142841.png)  

创建文件`answers-traps.txt`,在里面写上回答：

```txt
Q: Which registers contain arguments to functions? For example, which register holds 13 in main's call to printf?
A: a0-a7; a2;

Q: Where is the call to function f in the assembly code for main? Where is the call to g? (Hint: the compiler may inline functions.)
A: There is none. g(x) is inlined within f(x) and f(x) is further inlined into main()

Q: At what address is the function printf located?
A: 0x0000000000000628, main calls it with pc-relative addressing.

Q: What value is in the register ra just after the jalr to printf in main?
A: 0x0000000000000038, next line of assembly right after the jalr

Q: Run the following code.

 unsigned int i = 0x00646c72;
 printf("H%x Wo%s", 57616, &i);      

What is the output?
If the RISC-V were instead big-endian what would you set i to in order to yield the same output?
Would you need to change 57616 to a different value?
A: "He110 World"; 0x726c6400; no, 57616 is 110 in hex regardless of endianness.

Q: In the following code, what is going to be printed after 'y='? (note: the answer is not a specific value.) Why does this happen?

 printf("x=%d y=%d", 3);

A: A random value depending on what codes there are right before the call.Because printf tried to read more arguments than supplied.
The second argument `3` is passed in a1, and the register for the third argument, a2, is not set to any specific value before the
call, and contains whatever there is before the call.
```

简单翻译：

```txt
Q: 哪些寄存器存储了函数调用的参数？举个例子，main 调用 printf 的时候，13 被存在了哪个寄存器中？
A: a0-a7; a2;

Q: main 中调用函数 f 对应的汇编代码在哪？对 g 的调用呢？ (提示：编译器有可能会内链(inline)一些函数)
A: 没有这样的代码。 g(x) 被内链到 f(x) 中，然后 f(x) 又被进一步内链到 main() 中

Q: printf 函数所在的地址是？
A: 0x0000000000000628, main 中使用 pc 相对寻址来计算得到这个地址。

Q: 在 main 中 jalr 跳转到 printf 之后，ra 的值是什么？
A: 0x0000000000000038, jalr 指令的下一条汇编指令的地址。

Q: 运行下面的代码

 unsigned int i = 0x00646c72;
 printf("H%x Wo%s", 57616, &i);      

输出是什么？
如果 RISC-V 是大端序的，要实现同样的效果，需要将 i 设置为什么？需要将 57616 修改为别的值吗？
A: "He110 World"; 0x726c6400; 不需要，57616 的十六进制是 110，无论端序（十六进制和内存中的表示不是同个概念）

Q: 在下面的代码中，'y=' 之后会答应什么？ (note: 答案不是一个具体的值) 为什么?

 printf("x=%d y=%d", 3);

A: 输出的是一个受调用前的代码影响的“随机”的值。因为 printf 尝试读的参数数量比提供的参数数量多。
第二个参数 `3` 通过 a1 传递，而第三个参数对应的寄存器 a2 在调用前不会被设置为任何具体的值，而是会
包含调用发生前的任何已经在里面的值。
```

## Backtrace(moderate)

![picture 2](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231106_144150.png)  

![picture 3](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231106_150124.png)  

![picture 4](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231106_150141.png)  

### `kernel/defs.h`

在 defs.h 中添加声明

```c
// defs.h
void            printf(char*, ...);
void            panic(char*) __attribute__((noreturn));
void            printfinit(void);
void            backtrace(void); // new
```

### `kernel/riscv.h`

在 riscv.h 中添加获取当前 fp（frame pointer）寄存器的方法：

```c
// riscv.h
static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x));
  return x;
}
```

fp 指向当前栈帧的开始地址，sp 指向当前栈帧的结束地址。 （栈从高地址往低地址生长，所以 fp 虽然是帧开始地址，但是地址比 sp 高）
栈帧中从高到低第一个 8 字节 fp-8 是 return address，也就是当前调用层应该返回到的地址。
栈帧中从高到低第二个 8 字节 fp-16 是 previous address，指向上一层栈帧的 fp 开始地址。
剩下的为保存的寄存器、局部变量等。一个栈帧的大小不固定，但是至少 16 字节。
在 xv6 中，使用一个页来存储栈，如果 fp 已经到达栈页的上界，则说明已经到达栈底。

查看 call.asm，可以看到，一个函数的函数体最开始首先会扩充一个栈帧给该层调用使用，在函数执行完毕后再回收，例子：

```c
int g(int x) {
   0: 1141                  addi  sp,sp,-16  // 扩张调用栈，得到一个 16 字节的栈帧
   2: e422                  sd    s0,8(sp)   // 将返回地址存到栈帧的第一个 8 字节中
   4: 0800                  addi  s0,sp,16
  return x+3;
}
   6: 250d                  addiw a0,a0,3
   8: 6422                  ld    s0,8(sp)   // 从栈帧读出返回地址
   a: 0141                  addi  sp,sp,16   // 回收栈帧
   c: 8082                  ret              // 返回
```

注意栈的生长方向是从高地址到低地址，所以扩张是 -16，而回收是 +16。

### `kernel/printf.c`

实现 backtrace 函数：

```c
// printf.c

void backtrace() {
  uint64 fp = r_fp();
  while(fp != PGROUNDUP(fp)) { // 如果已经到达栈底
    uint64 ra = *(uint64*)(fp - 8); // return address
    printf("%p\n", ra);
    fp = *(uint64*)(fp - 16); // previous fp
  }
}
```

### `kernel/sysproc.c`

在 sys_sleep 的开头调用一次 backtrace()

```c
// sysproc.c
uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  backtrace(); // print stack backtrace.

  if(argint(0, &n) < 0)
    return -1;
  
  // ......

  return 0;
}
```

### 编译运行

![picture 12](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231108_163058.png)  

## Alarm(Hard)

![picture 5](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231107_083358.png)  

```linux
$ alarmtest
test0 start
........alarm!
test0 passed
test1 start
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
test1 passed
test2 start
................alarm!
test2 passed
$ usertests
...
ALL TESTS PASSED
$
```

![picture 6](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231107_083642.png)  

## test0: invoke handler(调用处理程序)

![picture 7](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231107_083850.png)  

```linux
int sigalarm(int ticks, void (*handler)());
int sigreturn(void);
```

![picture 8](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231107_084106.png)  

```linux
if(which_dev == 2) ...
```

![picture 9](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231107_084337.png)  

```linux
make CPUS=1 qemu-gdb
```

![picture 10](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231107_084405.png)  

## test1/test2(): resume interrupted code(恢复被中断的代码)

![picture 11](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%204/IMG_20231107_084654.png)  
