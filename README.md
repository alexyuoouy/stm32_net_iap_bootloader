# stm32_net_iap_bootloader

 基于stm32的网络升级bootloader / A stm32 bootloader  download the bin file to upgrade the firmware via the Internet

* ## 当前代码flash分区：

|flash分区|内容|
|:---------:|:------------------------------------|
|bootloader|bootloader|        
|application|应用代码|
|iap-parameter|bootloader参数，用来存储必要数据|

* ## 主要流程：

1. 首先从http文件服务器下载配置文件，放入iap-parameter中。
2. 从iap-parameter中读取参数，比如下载地址和大小等参数。（后续再做成消息拉取）
3. 利用iap-parameter的参数从http文件服务器分块下载（断点续传）application 的bin文件到指定地址。
4. 将已经烧录进flash的bin文件读出，计算MD5，同iap-parameter中的MD5比对，相同则校验成功，不成功重新下载。
5. 跳转至APP。

* ## iap-parameter文件格式：

```
Url: http://.../app.bin
Size: 64940
Md5: 03d3babeec45a70b08845c13db82eb15
App-Flash-Base: 0x08040000
App-Flash-Size: 0x0003f800
Version: V0.0.1

```

> 注意：冒号前的关键字不可更改，冒号后有空格，每行以\r\n结束，文件末尾使用\r\n\r\n结束。



* ## 移植方法：
1. 需要实现socket.h中的POSIX接口。
2. 需将iap_config.h中的DBG_LOG重定向到串口打印。
* ## 代码引用
* MD5的计算部分需感谢<https://blog.csdn.net/xhhjin/article/details/8450686>
