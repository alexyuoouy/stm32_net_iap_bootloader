/*
 * HTTP Download file
 * 
 * Change Logs:
 * Date			Author		Notes
 * 2020.5.24	yuoouy		the first version
 *
 *
 *
*/
#ifndef __HTTPDOWNLOAD__
#define __HTTPDOWNLOAD__

extern struct HTTPClient httpclient;
#define APP_FLASH_BASE      0x08040000U             /* application start address */

typedef void (* App_Reset)( void );

int download_to_flash( char *url, char *file_and_name);
void MSR_MSP(unsigned int addr);
#endif
