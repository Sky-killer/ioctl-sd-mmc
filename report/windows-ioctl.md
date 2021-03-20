[toc]

# 前期准备

## 环境配置

> 下载相应的sdk，安装后需要，切换库，然后在属性->vc++目录里包含km

## GUID码

> 在设备管理器->磁盘驱动器->详细信息->物理设备对象名称中找到设备码

# 开始实验

## 问题

1. `GetLastError()`返回5，使用权限不够，需要使用管理员权限

2. 在本机上运行会出现栈溢出错误，主要是由于电脑没有`sd`卡，而小电脑上是code: 5，权限不够；

   * **本机上栈溢出**，**原因**是没有申请到足够的内存给`SFFDISK_DEVICE_COMMAND_DATA`，即使解决这个问题仍然存在拒绝访问

     > 参考：[变长内存空间](https://blog.csdn.net/qq_40036519/article/details/107191726)，[变长数组](https://stackoverflow.com/questions/19853309/what-is-the-difference-between-int-var-and-int-var0)

   * 权限不够，待定。。。