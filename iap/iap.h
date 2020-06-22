/*
 * HTTP Download file
 *
 * Change Logs:
 * Date	        Author      Notes
 * 2020.5.24	yuoouy      the first version
 *
 *
 *
*/
#ifndef __HTTPDOWNLOAD__
#define __HTTPDOWNLOAD__

extern struct HTTPClient httpclient;


typedef void (* App_Reset)( void );

int download_to_flash( char *url, uint32_t address, int size);
void MSR_MSP(unsigned int addr);
#endif
