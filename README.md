# stm32_net_iap
A stm32 bootloader  download the bin file to upgrade the firmware via the Internet
基于stm32的网络升级bootloader

移植方法：
1.底层需要实现socket的POSIX接口，填入socket.h中
