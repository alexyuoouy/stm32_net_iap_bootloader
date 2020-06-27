/*
 * iap config
 *
 * Change Logs:
 * Date	        Author      Notes
 * 2020.6.21	yuoouy      the first version
 *
 *
 *
*/
#ifndef __IAP_CONFIG__
#define __IAP_CONFIG__



/*
 *  如果使用stm32大容量产品，下列地址必须2K字节（flash单页大小）对齐，其余产品参照产品手册
 *  num    name              size        start address
 *  1      bootloader        256KB       0x08000000
 *  2      application       248KB       0x08040000
 *  3      parameter         2KB         0x0807E000
 *
 */

#define     DEBUG

//#define     APP_FLASH_SIZE              248 * 1024
//#define     APP_FLASH_BASE              0x08040000U             /* application start address */
#define     PARAMETER_FLASH_SIZE        2 * 1024
#define     PARAMETER_FLASH_BASE        0x0807F800U             /* parameter start address */

//#define     APP_DOWNLOAD_URL            "http://.../app.bin"
#define     PARA_DOWNLOAD_URL           "http://.../parameter.txt"




/*parameter example

size: 23432
MD5: 9b4765e4b5f337e425721a1a845d1ef7


*/




#ifdef DEBUG
    #define DBG_LOG(...) printf(__VA_ARGS__)                                                   /*need to be implemented*/
#else
    #define DBG_LOG(...)
#endif










#endif
