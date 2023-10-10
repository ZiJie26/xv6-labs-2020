# BuildingEnvironment

[toc]

***By ZJay***

**完成的实验代码可以访问我的[GitHub仓库](https://github.com/ZiJie3726/xv6-labs-2020)**

**建议先看Git版本控制教程，在[这里](http://xv6.dgs.zone/labs/use_git/git1.html)**

**现在Linux环境下只能使用ssh连接Github进行push，若push到Github出现问题请自行百度。**

之前尝试过给笔记本装Ubuntu来一个沉浸式Linux学习，但是由于我有Onedrive多平台同步的需求，但Linux版的Onedrive用起来又太蛋疼，每次同步都把所有文件都下载下来，同步过程中中断也很麻烦，还是装回Windows采用虚拟机来进行学习。

我自己是硬盘直接装Ubuntu22.04、WSL2+VSCode、Docker都使用过，最后我自己是选择了Docker，但是搭建环境Docker和WSL没什么区别，都是虚拟机，去Pull个Ubuntu的镜像然后一样配置就好了。网络上Docker教程也很多，就不赘述了。

## WSL2+VSCode

如果之前没装过WSL的话，可以用**管理员模式**打开PowerShell或者命令行（**如果可以代理请用`wsl --install -d Ubuntu-20.04`安装**），直接用`wsl --install`命令安装。默认安装Ubuntu22.04版本，可以在微软应用商店安装其他发行版。**强烈建议本次实验用20.04版本，部署环境能省去一大部分步骤，之前用22.04非常麻烦**。

安装完别忘记**重启**！重启之后去微软应用商店搜索**Ubuntu20.04**版本安装。

>必须运行 Windows 10 版本 2004 及更高版本（内部版本 19041 及更高版本）或 Windows 11 才能使用此命令。

其他情况可以访问[官方文档](https://learn.microsoft.com/zh-cn/windows/wsl/install#prerequisites)。

VSCode可以自行百度下载，下载后安装WSL插件：

![picture 11](.assets_IMG/MIT%206.S081%20Fall%202020%20Lab%201/IMG_20230925-084317.png)  

打开你的Ubuntu 20.04，输入`git clone git://g.csail.mit.edu/xv6-labs-2020`把代码克隆到本地。

`cd xv6-labs-2020` ,首先更新系统

```txt
sudo apt update
sudo apt upgrade
```

然后一步安装所有必备工具

`sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu`

切换到第一个实验util分支

`git checkout util`

编译xv6并开启模拟器

`make qemu`

成功会出现这些代码

```txt
xv6 kernel is booting

hart 1 starting
hart 2 starting
init: starting sh
$ 
```

按下ctrl+a松开后再按x退出qemu

## 如果你使用的是Ubuntu 22.04版本，请看这个

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
