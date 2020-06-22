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
#include <iap.h>
#include <stdlib.h>
#include <posix.h>
#include "flash.h"
#include "stm32f1xx.h"
#include "string.h"
#include "iap_config.h"
#include <httpclient.h>


/**
 * download remote file to flash
 *
 * @param url               the input server URL address
 *        address           flash address
 *        size              no less than file size
 *
 * @return  -1 :    error
 *          >=0:    downloaded file size
 */
int download_to_flash( char *url, uint32_t address, int size)
{
    if (address % 2048 != 0)
    {
        DBG_LOG("download address must be 2K alined!\n");
        return -1;
    }

    if (flash_open((unsigned short *)address, size, Write) != 0)
    {
        DBG_LOG("flash open error!\n");
        return -1;
    }

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
    MSR MSP, r0         /* set Main Stack value */
    BX r14
}

int upgrade(void)
{
    /* download parameter file*/
    if (download_to_flash(PARA_DOWNLOAD_URL, PARAMETER_FLASH_BASE, PARAMETER_FLASH_SIZE) == -1)
    {
        DBG_LOG("download error!\n");
        return -1;
    }
    int app_bin_size = 0;

    /*add json parser function*/

    /* download application bin file */
    if (download_to_flash(APP_DOWNLOAD_URL, APP_FLASH_BASE, app_bin_size) == -1)
    {
        DBG_LOG("download error!\n");
        return -1;
    }

    /* run application */
    if(((*(volatile unsigned int *)APP_FLASH_BASE)&0x2FFE0000)==0x20000000)
    {
        App_Reset app_reset;
        app_reset=(App_Reset)*(volatile uint32_t *)(APP_FLASH_BASE + 4);        /* get application RESET handler */
        MSR_MSP(*(volatile unsigned int*)APP_FLASH_BASE);
        DBG_LOG("jump!\n");
        app_reset();
    }
}
