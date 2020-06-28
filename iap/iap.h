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

#include "stm32f1xx.h"

#define IAP_PARA_ITEM_NUM  6

struct iap_parameter
{
    char            url[64];                     /* bin file url */
    int             size;                        /* bin file size */
    char            MD5[33];                     /* MD5 verify */
    uint32_t        app_flash_base;              /* application flash start address */
    uint32_t        app_flash_size;              /* application flash size */
    char            version[10];                 /* application version */
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
static int parameter_parser(uint32_t para_addr);
int generate_bin_md5(uint32_t addr, struct iap_parameter *iap_para_t, char *md5_out);
int upgrade(void);
#endif
