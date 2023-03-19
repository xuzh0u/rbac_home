# RBAC访问控制-实验报告

## 背景

RBAC (Role-Based Access Control) 是一种用于访问控制的安全模型。它的主要特点是不直接给用户赋予权限，而是将访问控制策略建立在角色上，再为用户分配角色。这种方式的好处在于使目标系统在实现较强的访问控制的同时也具有较高的易用性。

在操作系统中，RBAC 可以用于管理系统资源访问权限，以保证系统资源的安全和数据的隔离。

## 设计

本实验采用 LSM 的方式实现了一个简易的RBAC模块 `myrbac_lsm`，该模块设计与文件有关的四个操作的访问控制。

### 模块策略设计

本模块的策略仅考虑粗粒度的文件操作权限管理，策略存放在固定的配置文件中。

#### 用户 (Users)

可以为一个用户设置至多一个角色。用户的角色信息保存在 `/etc/myrbac_lsm/users` 文件夹下：

- 文件名是用户的 `uid` ，可以通过 `whoami` 命令来确认；
- 文件内容是用户的角色名称。

用户态程序 `my_gov` 可以管理用户的角色设置：

```sh
my_gov -u         # 列出所有用户-角色的对应信息
my_gov -u 1001 r1 # 对于 uid 为 1001 的用户，将其角色设置/更新为 r1
```

#### 角色 (Roles)

可以为一个角色设置具备多个权限。角色的权限信息保存在 `/etc/myrbac_lsm/users` 文件夹下：

- 文件名是角色的名称，长度不超过 `MAX_ROLENAME_LEN`；
- 文件内容是 4 位二进制串，每一位分别代表一个权限，
    - 1 代表具备该权限，
    - 0 代表不具备该权限。

用户态程序 `my_gov` 可以管理角色的权限设置：

```sh
my_gov -r         # 列出所有角色-权限的对应信息
my_gov -r r1 1100 # 对于角色 r1，将其权限设置/更新为 inode_create + inode_rename
```

#### 权限 (Permissions)

该模块涉及的操作有：

- `inode_crate` 文件创建；
- `inode_rename` 文件重命名；
- `inode_mkdir` 目录创建；
- `inode_rmdir` 目录删除。

在本设计中，权限被硬编码在模块内部，这也符合系统安全管理的客观需要：

```c
// myrbac.h
enum ops
{
    OP_INODE_CREATE = 0,
    OP_INODE_RENAME = 1,
    OP_INODE_MKDIR = 2,
    OP_INODE_RMDIR = 3,
};
```

这些操作对应着 RBAC 模型中需要管理的权限。

## 实现

### myrbac_lsm（LSM）

该模块的实现主要有两方面内容：

1. 学习 `yama_lsm` 的写法，注册必要的钩子函数
2. 依据策略管理的设计，实现相关功能

主要实现了以下函数：

```c
// myrbac_lsm.c

// 检查模块当前是否启用
int is_enabled(void);

// 根据当前用户 uid 和操作情况，确定是否拦截操作，并打印相关提示
int alert_exec_result(int uid, int op);

// 检查给定角色是否具备给定操作的权限
int is_role_permitted(const char *role, const int op);

// 检查给定用户是否具备给定操作的权限
int is_user_permitted(int uid, int op);
```

### my_gov（用户态角色维护程序）

该程序主要实现以下函数：

```c
// my_gov.c

// 获取模块当前状态
int state_get();

// 设置模块当前状态
int state_set(int state);

// 设置/更新角色的权限
int role_set_perm(const char *role, const char *perm);

// 删除角色
int role_delete(const char *role);

// 列表展示所有角色的信息
int role_show();

// 设置/更新用户的角色
int user_set_role(const char *uid, const char *role);

// 列表展示所有用户的信息
int user_show();

// 打印程序用法提示
void alert_usage();
```

用户可以通过命令行对策略进行维护，具体展示见 “评估” 一节。

## 评估

### 环境搭建

#### myrbac_lsm（LSM）

宿主机信息：

```shell
vboxuser@u16c:~$ uname -a
Linux u16c 4.4.0-210-generic #242-Ubuntu SMP Fri Apr 16 09:57:56 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux

vboxuser@u16c:~$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 16.04.7 LTS
Release:        16.04
Codename:       xenial
```

为方便起见，也为了规避相关工具链向前/向后兼容的问题，选择 Linux 内核 4.4.10 版本。解压后进入文件夹，生成配置：

```sh
make allnoconfig
make defconfig
make menuconfig # 在这一步里选择自己的 lsm，否则无法将自定义模块编入内核
```

接着进行编译

```sh
make -j8
```

之后使用编译得到的内核。

#### my_gov（用户态角色维护程序）

使用 busybox 作为一个简易的操作系统模拟，将角色维护程序 `my_gov` 打包为 busybox 的一个 applet，以便测试。

下载 busybox-1.26.0，解压后进入文件夹，生成默认设置：

```sh
make defconfig
```

接着使用菜单设置：

```sh
make menuconfig
```

在菜单里选择静态编译，这样我们的程序才能够跑在内核上：

```
Busybox Settings ---> Build BusyBox as a static binary (no shared libs) ---> yes
```

保存设置后退出，然后运行脚本 `make_busy_tool.h` ：

```sh
#!/bin/bash

# 打开命令回显
set -x 

# 进入 busybox 目录

cd ~/busybox*

# 清除之前的文件
make clean
rm initramfs.cpio

# 编译 & 安装
make -j8
make install

# 进入 _install 目录
cd _install

# 为用户态创建和设置必要的初始化文件
mkdir -p proc sys dev etc lib tmp home \
         etc/init.d etc/myrbac_LSM \
         etc/myrbac_LSM/users etc/myrbac_LSM/roles
touch etc/myrbac_LSM/stat etc/passwd etc/group

ln -sf linuxrc init
cat > etc/init.d/rcS <<EOF
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
/sbin/mdev -s
ifconfig lo up
EOF
chmod +x etc/init.d/rcS

cat > etc/inittab <<EOF
# /etc/inittab
::sysinit:/etc/init.d/rcS
::askfirst:-/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
EOF

cat > etc/passwd <<EOF
# /etc/passwd
root:x:1000:1000:root:/root:/bin/sh  
root:x:0:0:Linux User,,,:/home/root:/bin/sh
EOF

# 为 root 用户设置默认策略
echo "0" > etc/myrbac_LSM/users/1000
echo "1111" > etc/myrbac_LSM/roles/0

#生成根文件系统
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz

#回到上级目录，解压
cd ..
gunzip initramfs.cpio.gz -f

```

### 运行展示

在qemu上的linux kernel上运行busybox

```sh
qemu-system-x86_64 \
-kernel ~/linux-4.4.10/arch/x86_64/boot/bzImage \
-initrd ~/busybox-1.26.0/initramfs.cpio \
-nographic -append "console=ttyS0"
```

####  模块状态检查/更新

```
/ # my_gov -s                  # 模块未开启
Module is not installed
/ # my_gov -s enable           # 开启模块
Module enabled successfully
/ # my_gov -s                  # 状态更新成功
Module is enabled
/ # my_gov -s disable          # 关闭模块
Module disabled successfully
/ # my_gov -s                  # 状态更新成功
Module is disabled
```

#### 更新策略

```
/ # my_gov -u 1001 r1     # 设置用户 1001 角色
[  307.808749] [[myrbac_LSM]] <inode_create> exec by user[uid: 0]: success
The role of user added successfully
/ # my_gov -u 1002 r2     # 设置用户 1002 角色
[  316.632154] [[myrbac_LSM]] <inode_create> exec by user[uid: 0]: success
The role of user added successfully
/ # my_gov -r r1 1110     # 设置角色 r1 权限
Role added successfully
/ # my_gov -r r2 1001     # 设置角色 r2 权限
Role added successfully
```

用命令行验证设置结果：（第一行均为 root 用户的预设保留信息，且在程序中已硬编码规定其不可更改）

```
/ # my_gov -u             # 用户信息展示
user role
1000 0
1001 r1
1002 r2
/ # my_gov -r             # 角色信息展示
role    permission
0       1111
r1      1110
r2      1001
```

可以看到，上面的更新都生效了

#### 验证权限管理

```
/ # my_gov -s enable               # 确认模块已生效
Module enabled successfully
```

##### u1

用户 u1 具备删除目录之外的所有权限，下面是它的测试过程：

```
/ # su u1                          # 切换到用户 u1
~ $ whoami                         # 确认用户信息-名称
u1
~ $ id u1                          # 确认用户信息-uid
uid=1001(u1) gid=1001(u1) groups=1001(u1)
/ $ cd
[  781.227419] [[myrbac_LSM]] <inode_create> exec by role[r1] is permitted
[  781.240143] user[uid: 1001]: success
~ $ touch u1text                   # 创建文件：成功
[  794.070221] [[myrbac_LSM]] <inode_create> exec by role[r1] is permitted
[  794.089177] user[uid: 1001]: success
~ $ mkdir u1folder                 # 创建目录：成功
[  801.573352] [[myrbac_LSM]] <inode_mkdir> exec by role[r1] is permitted
[  801.574470] user[uid: 1001]: success
~ $ mv u1text u1tetttt             # 重命名文件：成功
[  816.692466] [[myrbac_LSM]] <inode_rename> exec by role[r1] is permitted
[  816.703470] user[uid: 1001]: success
~ $ rm u1folder/ -r                # 删除目录：失败
[  826.743394] [[myrbac_LSM]] <inode_rmdir> exec by role[r1] is NOT permitted
[  826.743394] user[uid: 1001]: fail
rm: can't remove 'u1folder': Operation not permitted
```

确认结果：

```
~ $ ls -l
total 0
drwxr-sr-x    2 u1       u1              40 Mar 19 12:24 u1folder
-rw-r--r--    1 u1       u1               0 Mar 19 12:24 u1tetttt
```

可以看到，所有操作都符合预期。

##### u2

用户 u2 只具备创建文件和删除文件夹的权限，下面是它的测试过程：

```
/ # mkdir home/u2/wait_u2          # root 预先创建一个文件夹
[ 1325.097453] [[myrbac_LSM]] <inode_mkdir> exec by user[uid: 0]: success
/ # su u2                          # 切换到用户 u2
/ $ cd
~ $ ls
wait_u2
~ $ whoami                         # 确认用户信息-名称
u2
~ $ id u2                          # 确认用户信息-uid
uid=1002(u2) gid=1002(u2) groups=1002(u2)
~ $ touch u2file                   # 创建文件：成功
[ 1355.172254] [[myrbac_LSM]] <inode_create> exec by role[r2] is permitted
[ 1355.172254] user[uid: 1002]: success
~ $ mv u2file u2fileAAA            # 重命名文件：失败
[ 1364.241543] [[myrbac_LSM]] <inode_rename> exec by role[r2] is NOT permitted
[ 1364.244605] user[uid: 1002]: fail
mv: can't rename 'u2file': Operation not permitted
~ $ mkdir u2folder                 # 创建目录：失败
[ 1370.745387] [[myrbac_LSM]] <inode_mkdir> exec by role[r2] is NOT permitted
[ 1370.761575] user[uid: 1002]: fail
mkdir: can't create directory 'u2folder': Operation not permitted
~ $ ls                             # 确认当前文件情况（创建目录确实失败了）
u2file   wait_u2
~ $ rm wait_u2/ -r                 # 删除目录：成功
rm: descend into directory 'wait_u2'? yes
[ 1383.256195] [[myrbac_LSM]] <inode_rmdir> exec by role[r2] is permitted
[ 1383.271331] user[uid: 1002]: success
~ $ ls -l                          # 确认当前文件情况（删除目录成功，创建文件成功）
total 0
-rw-r--r--    1 u2       u2               0 Mar 19 12:33 u2file
```

由于 u2 只具备删除目录的能力，而不具备删除文件的能力，故进一步尝试让 u2 删除内部有文件的目录：

```
/ # su u2                          # 切换到用户 u2
/ # cd
~ $ ls -l                          # 展示目录信息
total 0
drwxr-sr-x    2 root     u2              80 Mar 19 12:38 a_folder_with_sth
-rw-r--r--    1 u2       u2               0 Mar 19 12:33 u2file
~ $ ls -l a_folder_with_sth/       # 展示待测试删除目录下的文件
total 0
-rw-r--r--    1 root     u2               0 Mar 19 12:38 aaaa
-rw-r--r--    1 root     u2               0 Mar 19 12:38 aaaf
~ $ rm a_folder_with_sth/ -r        # 尝试删除目录，但是因为不具备删除文件的权限而最终失败
rm: descend into directory 'a_folder_with_sth'? yes
rm: remove 'a_folder_with_sth/aaaf'? yes
rm: can't remove 'a_folder_with_sth/aaaf': Permission denied
rm: remove 'a_folder_with_sth/aaaa'? yes
rm: can't remove 'a_folder_with_sth/aaaa': Permission denied
[ 1657.714501] [[myrbac_LSM]] <inode_rmdir> exec by role[r2] is permitted
[ 1657.714501] user[uid: 1002]: success
rm: can't remove 'a_folder_with_sth': Directory not empty
~ $ ls -l                           # 目录删除失败：确认
total 0
drwxr-sr-x    2 root     u2              80 Mar 19 12:38 a_folder_with_sth
-rw-r--r--    1 u2       u2               0 Mar 19 12:33 u2file
```

可以看到，所有操作都符合预期。

## 总结

通过本实验，我成功实现了一个简易的 RBAC 访问控制模块，并通过测试验证了该模块的功能。代码和使用的脚本均已提交。

## 参考资料

1. [LSM内核模块 Demo 保姆级教程_linux lsm 完整示例_CNG Steve·Curcy的博客-CSDN博客](https://blog.csdn.net/weixin_40788897/article/details/123374309)
2. [BusyBox 添加 自定义命令\小程序 (applet) - schips - 博客园](https://www.cnblogs.com/schips/p/12125736.html)

