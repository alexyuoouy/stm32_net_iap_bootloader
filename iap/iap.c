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
#include "iap.h"
#include <stdlib.h>
#include "flash.h"
#include <stm32f1xx.h>
#include <string.h>
#include "iap_config.h"
#include "httpclient.h"

static struct HTTPClient * httpclient_t;
static struct iap_parameter iap_para;

static struct parameter_item para_item[IAP_PARA_ITEM_NUM] = {
    {"Url:", "%s", &iap_para.url},
    {"Size:", "%ld", &iap_para.size},
    {"Md5:", "%s", &iap_para.MD5},
    {"App-Flash-Base:", "%d", &iap_para.app_flash_base},
    {"App-Flash-Size:", "%d", &iap_para.app_flash_size},
    {"Version:", "%s", &iap_para.version}
};

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
static int download_to_flash( char *url, uint32_t address, int size)
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
        flash_close();
        return -1;
    }
    else
    {
        httpclient_init();
        if( http_connect(url) < 0 )
        {
            DBG_LOG("download failed, connect failed!\n");
            flash_close();
            http_close();
            return -1;
        }
        if ( http_send_reqheader() < 0 )
        {
            DBG_LOG("download failed, send header failed!\n");
            flash_close();
            http_close();
            return -1;
        }
        if ( http_recv_respheader() < 0 )
        {
            DBG_LOG("download failed, revice response header failed!\n");
            flash_close();
            http_close();
            return -1;
        }
        http_print_resp_header();
        if ( http_respheader_parse() < 0 )
        {
            DBG_LOG("download failed, response header parse failed!\n");
            flash_close();
            http_close();
            return -1;
        }

        DBG_LOG("file size : %d MB\n", (int)((float)httpclient_t->resp_header.content_length / 1048576));
        long recive = 0;
        int length = 0;
        int count = 0;
        int write_len = 0;
        while (recive < httpclient_t->resp_header.content_length)
        {
            length = http_read(httpclient_t->data_buffer, HTTPCLIENT_RECV_BUFFER_SIZE);
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
                write_len = flash_write(httpclient_t->data_buffer, length);
                //write_len = write(fd, httpclient_t->data_buffer, length);
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
            DBG_LOG("total length is :%d\n", httpclient_t->resp_header.content_length);
            DBG_LOG("download %d\%...\n\n", (int)((float)recive / (float )(httpclient_t->resp_header.content_length) * 100));
        }

        DBG_LOG("download successed!\n");
        //close(fd);
        flash_close();
        http_close();
        return recive;
    }
}


static __ASM void MSR_MSP(unsigned int addr)
{
    MSR     MSP, r0         /* set Main Stack value */
    BX      r14
}

static __ASM void interrupt_disable(void)
{
    CPSID   I
    BX      LR
}

int upgrade(void)
{
    httpclient_t = get_httpclient();


    /* download parameter file*/
    if (download_to_flash(PARA_DOWNLOAD_URL, PARAMETER_FLASH_BASE, PARAMETER_FLASH_SIZE) == -1)
    {
        DBG_LOG("download error!\n");
        return -1;
    }

    /*parameter parser*/
    if (parameter_parser(PARAMETER_FLASH_BASE) != 0)
    {
        DBG_LOG("parameter parser error!\n");
        return -1;
    }

    /* download application bin file */
    if (download_to_flash(iap_para.url, iap_para.app_flash_base, iap_para.size) == -1)
    {
        DBG_LOG("download error!\n");
        return -1;
    }

    /* run application */
    if(((*(volatile unsigned int *)iap_para.app_flash_base)&0x2FFE0000) == 0x20000000)
    {
        App_Reset app_reset;
        app_reset=(App_Reset)*(volatile uint32_t *)(iap_para.app_flash_base + 4);        /* get application RESET handler */
        MSR_MSP(*(volatile unsigned int*)iap_para.app_flash_base);
        DBG_LOG("jump!\n");
        interrupt_disable();
        app_reset();
    }
}

/**
 * parameter parser
 *
 * @param para_addr parameter start address
 *
 * @return  -1 :    error
 *           0 :    successed
 */
static int parameter_parser(uint32_t para_addr)
{
    if (para_addr % 2048 != 0 || iap_para_t == NULL)
    {
        DBG_LOG("download address must be 2K alined! And the iap para cannot be empty!\n");
        return -1;
    }
    if (flash_open( (volatile unsigned short)para_addr, PARAMETER_FLASH_SIZE , Read ) != 0)
    {
        DBG_LOG("flash open failed!\n");
        return -1;
    }
    char * temp = malloc(PARAMETER_FLASH_SIZE);
    if (temp == NULL)
    {
        DBG_LOG("parameter_parser malloc failed!\n");

        flash_close();
        return -1;
    }

    int i, flag = 0;
    char ch;
    char *ptr = temp;
    for (i = 0; i < PARAMETER_FLASH_SIZE; i++)
    {
        if (flash_read(&ch, 1) <= 0)
        {
            DBG_LOG("parameter_parser read failed!\n");

            flash_close();
            free(temp);
            return -1;
        }
        else
        {
            *ptr++ = ch;
            if ( flag == 0 )	{ if ( ch == '\r' ) flag = 1;}
            else if ( flag == 1 )
           {
               if (ch == '\n') flag = 2;
               else if (ch == '\r') flag = 1;
               else flag = 0;
           }
           else if (flag == 2)
           {
               if (ch == '\r') flag = 3;
               else flag = 0;
           }
           else if (flag == 3)
           {
               if (ch == '\n')
               {
                   break;
               }
               else if (ch == '\r') flag = 1;
               else flag = 0;
           }
        }
    }

    ptr = temp;
    char format[8] = "%*s ";
    for (i = 0; i < IAP_PARA_ITEM_NUM; i++)
    {
        ptr = strstr(temp, para_item[i].name);
        if (ptr)
        {
            strcat(format, para_item[i].type);
            if (sscanf(ptr, format, para_item[i].struct_item_addr);rt_kprintf(format2, i, para_item[i].struct_item_addr) == -1)
            {
                DBG_LOG("sscanf parameter error!\n");
                flash_close();
                free(temp);
                return -1;
            }
            format[4] = '\0';
        }
        else
        {
            DBG_LOG("strstr parameter error!\n");
            flash_close();
            free(temp);
            return -1;
        }
    }
    if ( i == NUM )
    {
        flash_close();
        free(temp);
        return 0;
    }
    else
    {
        flash_close();
        free(temp);
        return -1;
    }
}
