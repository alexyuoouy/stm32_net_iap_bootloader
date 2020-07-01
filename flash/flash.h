/*
 * flash read and program
 *
 * Use HAL library
 *
 * Change Logs:
 * Date			Author		Notes
 * 2020.6.14	yuoouy		the first version
 *
 *
 *
*/

#ifndef __FLASH__
#define __FLASH__
#include "stm32f1xx.h"

typedef enum
{
    On_Chip_Flash = 0U,
    External_flash = 1U
} Flash_Type;

typedef enum
{
    Read         = 1U,
    Write        = 2U,
    ReadAndWrite = 3U	 
} Open_Flash_Type;

struct Flash_fd
{
    /*后续可以加线程安全、多种flash设备共存的一些属性，不过一个bootloader貌似不需要这些*/
    //int num;                                    /*flash magic number*/
    //Flash_Type type;                            /*flash type*/
    //int lock;									/*flash lock*/

	int RW_flag;                                /* read, write or read and write */
	volatile unsigned short *start_addr;		/*start address*/
    int size;                                   /* read or write size */
	uint32_t cur_ptr;		                    /*current read or write pointer*/
	
																   
    unsigned short temp;                        /*temporary storage*/
    int temp_flag;                              /*flag is record temp status*/
};


int flash_open( volatile unsigned short * start_addr,int size, Open_Flash_Type flag );
int flash_read( unsigned char *buf, int length );
int flash_write( unsigned char *buf, int length );
int flash_lseek(int offset, int whence);										
int flash_close(void );


#endif
