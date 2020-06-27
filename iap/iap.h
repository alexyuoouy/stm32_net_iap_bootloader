/*
 * Internet IAP
 *
 * Change Logs:
 * Date	        Author      Notes
 * 2020.5.24	yuoouy      the first version
 *
 *
 *
*/
#ifndef __IAP__
#define __IAP__

struct iap_parameter
{
    char            *url;                   /* bin file url */
    int             size;                   /* bin file size */
    char            *MD5;                   /* MD5 verify */
    uint32_t        app_flash_base;         /* application flash start address */
    int             app_flash_size;         /* application flash size */
};

enum
{
    URL             =  0,
    SIZE                ,
    MD5                 ,
    APP_FLASH_BASE      ,
    APP_SIZE            ,

    NUM                     /*这是一个计数项，用来表示enum有多少个*/
};

struct parameter_item
{
    const char *name;
    const char *type;
    void *struct_item_addr;
};


typedef void (* App_Reset)( void );

static int download_to_flash( char *url, uint32_t address, int size);
static void MSR_MSP(unsigned int addr);
static void interrupt_disable(void);
int upgrade(void);
#endif
