# MIT 6.S081 Fall 2020 Lab 2

****System calls****

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

