# rbac_home

使用时请假设该仓库为当前用户的 `~` 目录 (home).

配置文件仅供参考, 若需直接替换使用请保证所使用的开源软件版本一致.

## 仓库结构

- busybox-1.26.0/             策略维护程序 my_gov (用户态)
  - my_gov.c                  核心程序
  - ...                       必要的配置文件
- linux-4.4.10/               RBAC模块 myrbac_lsm/ (内核模块)
  - my_rbac_lsm.c             核心程序
  - ...                       必要的配置文件
- report/                     实验报告
  - README.md                 markdown 源码
- make_busy_tool.sh           用户态程序编译打包脚本
- rbac_home.h                 核心程序使用的共享头文件
- README.md                   仓库结构说明

