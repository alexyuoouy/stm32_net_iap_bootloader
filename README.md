# stm32_net_iap
A stm32 bootloader  download the bin file to upgrade the firmware via the Internet
基于stm32的网络升级bootloader

flash分区：
1.bootloader        初始化硬件环境，从web文件服务器下载bin文件、烧录
2.application       应用区
3.parameter         参数区，用来存储必要数据

移植方法：
1.底层需要实现socket的POSIX接口，填入socket.h中
