# xv6-2023 - find Lab

## Overview

Write a simple version of the UNIX find program for xv6: find all the files in a directory tree with a specific name. Your solution should be in the file user/find.c.

Some hints:

Look at user/ls.c to see how to read directories.
Use recursion to allow find to descend into sub-directories.
Don't recurse into "." and "..".
Changes to the file system persist across runs of qemu; to get a clean file system run make clean and then make qemu.
You'll need to use C strings. Have a look at K&R (the C book), for example Section 5.5.
Note that == does not compare strings like in Python. Use strcmp() instead.
Add the program to UPROGS in Makefile.

## solve it

根据上文的提示，我要参照`user/ls.c`中的实现，来编写`find()`。

我们知道，`ls`命令是列出选定目录的所有文件，而我们的`find`命令是查找选定目录下的文件，这也就代表着，`ls`和`find`命令是及其相近的，不过`find`多了一个递归**子目录**的功能，和对比文件名称的功能。

## user/ls.c 函数解析

通过查看`user/ls.c`中的定义，`void ls(char *path)`，我们来看`user/ls.c`主函数的代码：

```c
// void ls(char *path)

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit(0);
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0);
}
```

我们可以看到，`ls`函数的参数是`char *path`，我们在仿写`find`函数的时候，可以按照他`ls`命令来仿写。

```c
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
```

在`user/ls.c`中，`buf`是存储目录信息的缓冲区，`p`是目录信息缓冲区的指针，`fd`是目录文件描述符，`de`是目录项结构体，`st`是文件状态结构体。

关于`dirent`结构体和`stat`结构体之间的关系，可以查看以下图示：

```c
[目录 inode]   (type = T_DIR)
     │
     │   包含数据块指针
     ▼
+-------------------------------+
|   目录数据块 (存放 dirent[])  |
+-------------------------------+
|  dirent:                      |
|   inum = 5   name = "."       |
|  dirent:                      |
|   inum = 1   name = ".."      |
|  dirent:                      |
|   inum = 12  name = "foo.txt" |───┐
|  dirent:                      |   │
|   inum = 13  name = "bar"     |───┼──► [inode #13] (type = T_DIR)
|  ...                          |   │
+-------------------------------+   │
                                    │
                                    ▼
                              [inode #12] (type = T_FILE)
                               │
                               │ inode 里有元数据
                               ▼
                         struct stat {
                            dev=1,
                            ino=12,
                            type=T_FILE,
                            nlink=1,
                            size=1024
                         }
```

接下来分析：

```c
  if((fd = open(path, O_RDONLY)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }
```
函数`open()`打开文件，并将文件描述符进行配置，设置文件描述符的只读模式。我们可以具体去`kernel/sysfile.c`中查看`sys_open()`函数的实现：

### kernel/sysfile.c sys_open() 函数解析

```c
uint64
sys_open(void)
{
  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  argint(1, &omode);
  if((n = argstr(0, path, MAXPATH)) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }

  if(ip->type == T_DEVICE){
    f->type = FD_DEVICE;
    f->major = ip->major;
  } else {
    f->type = FD_INODE;
    f->off = 0;
  }
  f->ip = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if((omode & O_TRUNC) && ip->type == T_FILE){
    itrunc(ip);
  }

  iunlock(ip);
  end_op();

  return fd;
}
```

我们不再递归，将其他函数全部解析，只告诉函数的作用。


```c
  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  argint(1, &omode);
  if((n = argstr(0, path, MAXPATH)) < 0)
    return -1;
```

这段代码我们应该很熟悉了，将寄存器中保存的参数取出，也就是`open(path, O_RDONLY)`中的`path`和`O_RDONLY`。

```c
  begin_op();
  end_op();
```
`begin_op()`和`end_op()`函数是别用于开始和结束一个磁盘操作(保证磁盘操作的**原子性**操作)。

