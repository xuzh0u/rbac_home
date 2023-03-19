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

