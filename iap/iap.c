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

#include <httpclient.h>
#include <iap.h>
#include <stdlib.h>
#include <posix.h>
#include "flash.h"
#include "stm32f1xx.h"
#include "string.h"



/**
 * 
 *
 * @param url 				the input server URL address
 * 		  app_flash_base	application start address
 *
 * @return   
 */
int http_download( char *url, char *file_and_name)
{
    if (url == NULL)
    {
        DBG_LOG("please input a url!\n");
        return -1;
    }
    else
    {
        httpclient_init();
        if( http_connect(url) < 0 )
        {
            DBG_LOG("download failed, connect failed!\n");
            http_close();
            return -1;
        }
        if ( http_send_reqheader() < 0 )
        {
            DBG_LOG("download failed, send header failed!\n");
            http_close();
            return -1;
        }
        if ( http_recv_respheader() < 0 )
        {
            DBG_LOG("download failed, revice response header failed!\n");
            http_close();
            return -1;
        }
        http_print_resp_header();
        if ( http_respheader_parse() < 0 )
        {
            DBG_LOG("download failed, response header parse failed!\n");
            http_close();
            return -1;
        }
        http_print_resp_header2();
        
        if (flash_open((unsigned short *)APP_FLASH_BASE, httpclient.resp_header.content_length, Write) != 0)
        {
            DBG_LOG("flash open error!\n");
            return -1;
        }
        
        DBG_LOG("file size : %d MB\n", (int)((float)httpclient.resp_header.content_length / 1048576));
        long recive = 0;
        int length = 0;
        int count = 0;
        int write_len = 0;
        while (recive < httpclient.resp_header.content_length)
        {
            length = http_read(httpclient.data_buffer, HTTPCLIENT_RECV_BUFFER_SIZE);
            if (length <= 0)
            {
                DBG_LOG("read data error!\n");
                rt_thread_delay( RT_TICK_PER_SECOND );
                count += 1;
                if (count >= 3)
                {
                    DBG_LOG("read data out of time!\n");
                    //close(fd);
                    flash_close();
                    http_close();
                    return -1;
                }
            }
            else
            {
                DBG_LOG("length is :%d\n", length);
                count = 0;
                recive += length;
                write_len = flash_write(httpclient.data_buffer, length);
                //write_len = write(fd, httpclient.data_buffer, length);
                if (length != write_len)
                {
                    DBG_LOG("file write error!\n");
                    //close(fd);
                    flash_close();
                    http_close();
                    return -1;
                }
                DBG_LOG("falsh wtited %d bytes!\n",write_len);
            }
            DBG_LOG("\nrecive length is :%d\n", recive);
            DBG_LOG("total length is :%d\n", httpclient.resp_header.content_length);
            DBG_LOG("download %d\%...\n\n", (int)((float)recive / (float )(httpclient.resp_header.content_length) * 100));
        }
        
		DBG_LOG("download successed!\n");
        //close(fd);
        flash_close();
        http_close();
        return recive;
    }
}


__ASM void MSR_MSP(unsigned int addr) 
{
    MSR MSP, r0 			//set Main Stack value
    BX r14
}
