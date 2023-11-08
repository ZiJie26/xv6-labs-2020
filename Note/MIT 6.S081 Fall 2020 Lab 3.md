# MIT 6.S081 Fall 2020 Lab 3

****Page Tables****

***by ZJay/楚梓杰***

[toc]
要启动实验，请切换到pgtbl分支：

```linux
git fetch
git checkout pgtbl
make clean
```

## Print a page table

![picture 0](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%203/IMG_20231023_151549.png)  

![picture 1](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%203/IMG_20231023_151606.png)

### `kernel/defs.h`

```C
// kernel/defs.h
int             copyout(pagetable_t, uint64, char *, uint64);
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);
int             vmprint(pagetable_t pagetable);//ADD
```

### `kernel/exec.c`

```C
// kernel/exec.c
  p->trapframe->sp = sp; // initial stack pointer
  proc_freepagetable(oldpagetable, oldsz);

  vmprint(p->pagetable);// --ADD 按照实验要求，在 exec 返回之前打印一下页表。
```

### `kernel/vm.c`

添加到最后，因为需要递归打印页表，而 xv6 已经有一个递归释放页表的函数 freewalk()，将其复制一份，并将释放部分代码改为打印即可：

```C
// kernel/vm.c
int pgtblprint(pagetable_t pagetable, int depth) {
    // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if(pte & PTE_V) {
      // print
      printf("..");
      for(int j=0;j<depth;j++) {
        printf(" ..");
      }
      printf("%d: pte %p pa %p\n", i, pte, PTE2PA(pte));

      // if not a leaf page table, recursively print out the child table
      if((pte & (PTE_R|PTE_W|PTE_X)) == 0){
        // this PTE points to a lower-level page table.
        uint64 child = PTE2PA(pte);
        pgtblprint((pagetable_t)child,depth+1);
      }
    }
  }
  return 0;
}

int vmprint(pagetable_t pagetable) {
  printf("page table %p\n", pagetable);
  return pgtblprint(pagetable, 0);
}
```

## A kernel page table per process

![picture 2](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%203/IMG_20231023_152722.png)  

![picture 3](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%203/IMG_20231023_152744.png)

xv6 原本的设计是，用户进程在用户态使用各自的用户态页表，但是一旦进入内核态（例如使用了系统调用），则切换到内核页表（通过修改 satp 寄存器，trampoline.S）。然而这个内核页表是全局共享的，也就是全部进程进入内核态都共用同一个内核态页表。

本 Lab 目标是让每一个进程进入内核态后，都能有自己的独立内核页表，为第三个实验做准备。

### `kernel/proc.h`

在进程的结构体 proc 中，添加一个 kernelpgtbl，用于存储进程专享的内核态页表。

```C
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  pagetable_t kernelpgtbl;      // ADD
};
```

### `kernel/vm.c`

暴改 kvminit。内核需要依赖内核页表内一些固定的映射的存在才能正常工作，例如 UART 控制、硬盘界面、中断控制等。而 kvminit 原本只为全局内核页表 kernel_pagetable 添加这些映射。我们抽象出来一个可以为任何我们自己创建的内核页表添加这些映射的函数 kvm_map_pagetable()。

```C
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "spinlock.h" // ADD
#include "proc.h" //ADD
```

---
将这一段：

```C
/*
 * create a direct-map page table for the kernel.
 */
void
kvminit()
{
  kernel_pagetable = (pagetable_t) kalloc();
  memset(kernel_pagetable, 0, PGSIZE);
  // uart registers
  kvmmap(UART0, UART0, PGSIZE, PTE_R | PTE_W);
  // virtio mmio disk interface
  kvmmap(VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);
  // CLINT
  kvmmap(CLINT, CLINT, 0x10000, PTE_R | PTE_W);
  // PLIC
  kvmmap(PLIC, PLIC, 0x400000, PTE_R | PTE_W);
  // map kernel text executable and read-only.
  kvmmap(KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);
  // map kernel data and the physical RAM we'll make use of.
  kvmmap((uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);
  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
}
```

替换为：

```C
void kvm_map_pagetable(pagetable_t pgtbl) {
  
  // uart registers
  kvmmap(pgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);
  // virtio mmio disk interface
  kvmmap(pgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);
  // CLINT
  kvmmap(pgtbl, CLINT, CLINT, 0x10000, PTE_R | PTE_W);
  // PLIC
  kvmmap(pgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W);
  // map kernel text executable and read-only.
  kvmmap(pgtbl, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);
  // map kernel data and the physical RAM we'll make use of.
  kvmmap(pgtbl, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);
  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(pgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
}
pagetable_t
kvminit_newpgtbl()
{
  pagetable_t pgtbl = (pagetable_t) kalloc();
  memset(pgtbl, 0, PGSIZE);
  kvm_map_pagetable(pgtbl);
  return pgtbl;
}
/*
 * create a direct-map page table for the kernel.
 */
void
kvminit()
{
  kernel_pagetable = kvminit_newpgtbl();
}
```

---

```C
kvmmap(uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kernel_pagetable, va, sz, pa, perm) != 0)
    panic("kvmmap");
}
```

换成：

```C
kvmmap(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(pgtbl, va, sz, pa, perm) != 0)
    panic("kvmmap");
}
```

---

```C
kvmpa(uint64 va)
{
  uint64 off = va % PGSIZE;
  pte_t *pte;
  uint64 pa;
  
  pte = walk(kernel_pagetable, va, 0);
```

换成：

```C
kvmpa(pagetable_t kernelpgtbl, uint64 va)
{
  uint64 off = va % PGSIZE;
  pte_t *pte;
  uint64 pa;
  pte = walk(kernelpgtbl, va, 0); // read from the process-specific kernel pagetable instead
```

---

kvm_free_kernelpgtbl() 用于递归释放整个多级页表树，也是从 freewalk() 修改而来。

```C
  kfree((void*)pagetable);
}

// --ADD--
// Free a process-specific kernel page-table,
// without freeing the underlying physical memory
void
kvm_free_kernelpgtbl(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    uint64 child = PTE2PA(pte);
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      kvm_free_kernelpgtbl((pagetable_t)child);
      pagetable[i] = 0;
    }
  }
  kfree((void*)pagetable);
}
// --ADD--

// Free user memory pages,
// then free page-table pages.
void
```

### `kernel/proc.c`

在 xv6 原来的设计中，内核页表本来是只有一个的，所有进程共用，所以需要为不同进程创建多个内核栈，并 map 到不同位置（见 procinit() 和 KSTACK 宏）。而我们的新设计中，每一个进程都会有自己独立的内核页表，并且每个进程也只需要访问自己的内核栈，而不需要能够访问所有 64 个进程的内核栈。所以可以将所有进程的内核栈 map 到其各自内核页表内的固定位置（不同页表内的同一逻辑地址，指向不同物理内存）。

删除以下代码：

```c
char *pa = kalloc();
      if(pa == 0)
        panic("kalloc");
      uint64 va = KSTACK((int) (p - proc));
      kvmmap(va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
      p->kstack = va;
```

这里删除了为所有进程预分配内核栈的代码，变为创建进程的时候再创建内核栈

---

在创建进程的时候，为进程分配独立的内核页表，以及内核栈。
添加：

```c
    return 0;
  }

  // --add--
  p->kernelpgtbl = kvminit_newpgtbl();
  char *pa = kalloc();
  if(pa == 0)
    panic("kalloc");
  uint64 va = KSTACK((int)0);
  kvmmap(p->kernelpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  p->kstack = va;
  // --add--

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
```

到这里进程独立的内核页表就创建完成了，但是目前只是创建而已，用户进程进入内核态后依然会使用全局共享的内核页表，因此还需要在 scheduler() 中进行相关修改。

在调度器将 CPU 交给进程执行之前，切换到该进程对应的内核页表：

```c
 // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;

// --add--
        w_satp(MAKE_SATP(p->kernelpgtbl));
        sfence_vma();
// --add--

        swtch(&c->context, &p->context);

        kvminithart();// add

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
```

到这里，每个进程执行的时候，就都会在内核态采用自己独立的内核页表了。

最后需要做的事情就是在进程结束后，应该释放进程独享的页表以及内核栈，回收资源，否则会导致内存泄漏。

（如果 usertests 在 reparent2 的时候出现了 panic: kvmmap，大概率是因为大量内存泄漏消耗完了内存，导致 kvmmap 分配页表项所需内存失败，这时候应该检查是否正确释放了每一处分配的内存，尤其是页表是否每个页表项都释放干净了，）

```c
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  // --add--
  void *kstack_pa = (void *)kvmpa(p->kernelpgtbl, p->kstack);
  kfree(kstack_pa);
  p->kstack = 0;
  kvm_free_kernelpgtbl(p->kernelpgtbl);
  p->kernelpgtbl = 0;
  // --add--
  p->state = UNUSED;
}
```

### `kernel/defs.h`

如下图对代码进行更改，删除红色的添加绿色的。

![picture 7](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%203/IMG_20231023_160111.png)  

添加的代码：

```c
// kernel/defs.h
pagetable_t     kvminit_newpgtbl(void);

+uint64          kvmpa(pagetable_t, uint64);
void            kvmmap(pagetable_t, uint64, uint64, uint64, int);
void            kvm_free_kernelpgtbl(pagetable_t);
```

### `kernel/kalloc.c`

```c
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
  printf("PHYSTOP is %p\n", PHYSTOP);// add
}
```

### `kernel/virtio_disk.c`

注意到我们的修改影响了其他代码： virtio 磁盘驱动 virtio_disk.c 中调用了 kvmpa() 用于将虚拟地址转换为物理地址，这一操作在我们修改后的版本中，需要传入进程的内核页表。对应修改即可。

```C
#include "fs.h"
#include "buf.h"
#include "virtio.h"
#include "proc.h"// ADD

// the address of virtio mmio register r.
#define R(r) ((volatile uint32 *)(VIRTIO0 + (r)))
```

```C
  // buf0 is on a kernel stack, which is not direct mapped,
  // thus the call to kvmpa().
  //disk.desc[idx[0]].addr = (uint64) kvmpa((uint64) &buf0); //REMOVE 删除这一行
  disk.desc[idx[0]].addr = (uint64) kvmpa(myproc()->kernelpgtbl, (uint64) &buf0);// ADD
  disk.desc[idx[0]].len = sizeof(buf0);
  disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
  disk.desc[idx[0]].next = idx[1];
```

## Simplify `copyin/copyinstr`

![picture 4](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%203/IMG_20231023_152956.png)  

![picture 5](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%203/IMG_20231023_153009.png)  

### `kernel/defs.h`

```C
void            kvmmap(pagetable_t, uint64, uint64, uint64, int);
// void *kget_freelist(void); // used for tracing purposes in exp2
void   kvm_free_kernelpgtbl(pagetable_t);

// --ADD--
int    kvmcopymappings(pagetable_t src, pagetable_t dst, uint64 start, uint64 sz);
uint64    kvmdealloc(pagetable_t kernelpgtbl, uint64 oldsz, uint64 newsz);
// --ADD--

int             mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t     uvmcreate(void);
void            uvminit(pagetable_t, uchar *, uint);
```

### `kernel/exec.c`

```C
    uint64 sz1;
    if((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;

    // --ADD--
    if(sz1 >= PLIC) {
      goto bad;
    }
    // --ADD--

    sz = sz1;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
```

```C
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));

  // --ADD--
  uvmunmap(p->kernelpgtbl, 0, PGROUNDUP(oldsz)/PGSIZE, 0);
  kvmcopymappings(pagetable, p->kernelpgtbl, 0, sz);
  // --ADD--

  // Commit to the user image.
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
```

### `kernel/proc.c`

```C
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;
  kvmcopymappings(p->pagetable, p->kernelpgtbl, 0, p->sz);// --ADD
  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
```

```C

  sz = p->sz;
  if(n > 0){
    uint64 newsz;// --ADD
    if((newsz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }

    // --ADD--
    if(kvmcopymappings(p->pagetable, p->kernelpgtbl, sz, n) != 0) {
      uvmdealloc(p->pagetable, newsz, sz);
      return -1;
    }
    sz = newsz;
    // --ADD--

  } else if(n < 0){
    uvmdealloc(p->pagetable, sz, sz + n);// --ADD
    // synchronize kernel page-table's mapping of user memory
    sz = kvmdealloc(p->kernelpgtbl, sz, sz + n);
  }
  p->sz = sz;
  return 0;
```

```C

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0 || // --ADD
     kvmcopymappings(np->pagetable, np->kernelpgtbl, 0, p->sz) < 0){// --ADD
    freeproc(np);
    release(&np->lock);
    return -1;
```

### `kernel/vm.c`

删除：
`kvmmap(pgtbl, CLINT, CLINT, 0x10000, PTE_R | PTE_W);`

---

```C
kvminit()
{
  kernel_pagetable = kvminit_newpgtbl();
  // CLINT *is* however required during kernel boot up and
  // we should map it for the global kernel pagetable
  kvmmap(kernel_pagetable, CLINT, CLINT, 0x10000, PTE_R | PTE_W);// --ADD
}
```

```C
  return newsz;
}

// --ADD
// Just like uvmdealloc, but without freeing the memory.
// Used for syncing up kernel page-table's mapping of user memory.
uint64
kvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  if(newsz >= oldsz)
    return oldsz;
  if(PGROUNDUP(newsz) < PGROUNDUP(oldsz)){
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 0);
  }
  return newsz;
}
// --ADD

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
```

```C
  return -1;
}

// --ADD
// Copy some of the mappings from src into dst.
// Only copies the page table and not the physical memory.
// returns 0 on success, -1 on failure.
int
kvmcopymappings(pagetable_t src, pagetable_t dst, uint64 start, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  // PGROUNDUP: prevent re-mapping already mapped pages
  for(i = PGROUNDUP(start); i < start + sz; i += PGSIZE){
    if((pte = walk(src, i, 0)) == 0)
      panic("kvmcopymappings: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("kvmcopymappings: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    // `& ~PTE_U` marks the page as kernel page and not user page.
    // Required or else kernel can not access these pages.
    if(mappages(dst, i, PGSIZE, pa, flags & ~PTE_U) != 0){
      goto err;
    }
  }
  return 0;
 err:
  uvmunmap(dst, 0, i / PGSIZE, 0);
  return -1;
}
// --ADD

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void
```

```C
  return 0;
}

// --ADD
int copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);
int copyinstr_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max);
// --ADD

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  return copyin_new(pagetable, dst, srcva, len);// --CHANGE
}
// Copy a null-terminated string from user to kernel.
```

```C
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  // printf("trace: copyinstr %p\n", walk(pagetable, srcva, 0));
  return copyinstr_new(pagetable, dst, srcva, max);// --CHANGE
}
int pgtblprint(pagetable_t pagetable, int depth) {
```

### `kernel/vmcopyin.c`

```C
copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  struct proc *p = myproc();
  // printf("trace: copyin_new %p, %p, %p, %d\n", r_satp(), dst, srcva, len);
  if (srcva >= p->sz || srcva+len >= p->sz || srcva+len < srcva)
    return -1;
  memmove((void *) dst, (void *)srcva, len);
```

## make grade

![picture 6](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%203/IMG_20231023_153833.png)  
