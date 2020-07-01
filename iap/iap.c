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
#include "md5.h"

static struct HTTPClient * httpclient_t;
static struct iap_parameter iap_para;
/* 标记已下载区块, 一个bit标记一个块,512k的flash，1024的块大小最多需要64Bytes记录,flash可能不是（DOWNLOAD_BLOCK_SIZE * 8）对齐，向上取整 */
static unsigned char down_zone_flag[(FLASH_BANK1_END - FLASH_BASE ) / (DOWNLOAD_BLOCK_SIZE * 8) + 1];
static struct parameter_item para_item[IAP_PARA_ITEM_NUM] = {
    {"Url:", "%s", &iap_para.url},
    {"Size:", "%ld", &iap_para.size},
    {"Md5:", "%s", &iap_para.MD5},
    {"App-Flash-Base:", "0x%08x", &iap_para.app_flash_base},
    {"App-Flash-Size:", "0x%08x", &iap_para.app_flash_size},
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
    if (address % 2048 != 0 || url == NULL)
    {
        DBG_LOG("download address is not alined in 2k! or url is null!\n");
        return -1;
    }

    if (flash_open((unsigned short *)address, size, Write) != 0)
    {
        DBG_LOG("flash open error!\n");
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
		httpclient_t->req_header.break_point_resume = 0;
        http_make_reqheader(httpclient_t);
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

        DBG_LOG("file size : %d KB\n", (int)((float)httpclient_t->resp_header.content_length / 1024));
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
/**
 * download remote file to flash use break point
 *
 * @param url               the input server URL address
 *        address           flash address
 *        size              no less than file size
 *
 * @return  -1 :    error
 *          >=0:    downloaded file size
 */
static int download_to_flash_bpr( char *url, uint32_t address, int size)
{
     if (address % 2048 != 0 || url == NULL)
    {
        DBG_LOG("download address is not alined in 2k! or url is null!\n");
        return -1;
    }
    while (flash_open((unsigned short *)address, size, Write) != 0);
    httpclient_init();
    httpclient_t->req_header.break_point_resume = 1;
    int i;
    for(i = 0; i < (FLASH_BANK1_END - FLASH_BASE ) / (DOWNLOAD_BLOCK_SIZE * 8) + 1; i++)
    {
        down_zone_flag[i] = 0;
    }

    int receive_length = 0;
    int err_count = 0;
    int zone_count = 0;

__retry:
    while( http_connect(url) != 0 );

    DBG_LOG("divided into %d zones\n", (size - 1) / DOWNLOAD_BLOCK_SIZE + 1);
    for(i = 0; i < (size - 1 ) / (DOWNLOAD_BLOCK_SIZE * 8) + 1; i++)
    {
        int num;
        if (i == (size - 1 ) / (DOWNLOAD_BLOCK_SIZE * 8))
        {
            num = ((size - 1) / DOWNLOAD_BLOCK_SIZE + 1) % 8;
        }
        else
        {
            num = 8;
        }
        int j;
        for(j = 0; j < num; j++)
        {
            if((down_zone_flag[i] & ((unsigned char)1 << j)) == 0)
            {
                int start = 8 * DOWNLOAD_BLOCK_SIZE * i + DOWNLOAD_BLOCK_SIZE * j;
                int end;
                if (j == num -1)
                {
                    end = start + size % DOWNLOAD_BLOCK_SIZE - 1;
                }
                else
                {
                    end = start + DOWNLOAD_BLOCK_SIZE - 1;
                }

                sprintf(httpclient_t->req_header.content_range, "%d-%d", start , end);
                http_make_reqheader(httpclient_t);

                __resendheader:
                if ( http_send_reqheader() < 0 )
                {
                    err_count++;
                    DBG_LOG("send header failed!\n");
                    if (err_count > 3)
                    {
                        err_count = 0;
                        http_close();
                        goto __retry;
                    }
                }
                if ( http_recv_respheader() < 0 )
                {
                    DBG_LOG("revice response header failed!\n");
                    goto __resendheader;
                }
                http_print_resp_header();

                if ( http_respheader_parse() < 0 )
                {
                    DBG_LOG("response header parse failed!\n");
                    goto __resendheader;
                }

                DBG_LOG("current download size : %d bytes, totle size : %d KB\n", httpclient_t->resp_header.content_length , (int)((float)httpclient_t->resp_header.content_overall_length / 1024));
                DBG_LOG("current download zone %d\n", 8 * i + j);
                int recive = 0;
                int length = 0;
                int write_len = 0;
                while (recive < httpclient_t->resp_header.content_length)
                {
                    length = http_read(httpclient_t->data_buffer, httpclient_t->resp_header.content_length);
                    if (length < httpclient_t->resp_header.content_length)
                    {
                        DBG_LOG("read data error!\n");
                        goto __resendheader;
                    }
                    DBG_LOG("length is :%d\n", length);
                    recive += length;
                    __rewrite:
                    /*flash lseek to undownload area*/
                    flash_lseek( 8 * DOWNLOAD_BLOCK_SIZE * i + DOWNLOAD_BLOCK_SIZE * j, SEEK_SET);
                    write_len = flash_write(httpclient_t->data_buffer, DOWNLOAD_BLOCK_SIZE);
                    if (write_len < DOWNLOAD_BLOCK_SIZE)
                    {
                        DBG_LOG("file write error!\n");
                        goto __rewrite;
                    }
                    DBG_LOG("flash wtited %d bytes!\n",write_len);

                    DBG_LOG("zone %d download %d\%...\n\n", 8 * i + j, (int)((float)recive / (float )(httpclient_t->resp_header.content_length) * 100));
                }
                down_zone_flag[i] |= ((unsigned char)1 << j);
                zone_count++;
                receive_length += recive;
                DBG_LOG("zone %d download successed!\n", 8 * i + j);
                DBG_LOG("download zones/total zones: %d/%d\n\n\n\n", zone_count, (size - 1) / DOWNLOAD_BLOCK_SIZE + 1);
                rt_thread_delay(RT_TICK_PER_SECOND);
            }/* if not download */
        }/* for j 1k */
    }/* for i 8k */
    if (zone_count < (size - 1) / DOWNLOAD_BLOCK_SIZE + 1)
    {
        goto __retry;
    }
    flash_close();
    http_close();
    return receive_length;
}

static __ASM void MSR_MSP(unsigned int addr)
{
    MSR     MSP, r0         /* set Main Stack value */
    BX      LR				/* return  */
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
	__redownload:
    /* download application bin file */
    if (download_to_flash_bpr(iap_para.url, iap_para.app_flash_base, iap_para.size) == -1)
    {
        DBG_LOG("download error!\n");
        return -1;
    }

    /*md5 verify*/
    char s[33];
    generate_bin_md5(iap_para.app_flash_base, &iap_para, s);
    s[32] = '\0';
    DBG_LOG("md5: %s\n", s);
    if (strcmp(s, iap_para.MD5) != 0)
    {
        DBG_LOG("md5 verify failed!\n", s);
        goto __redownload;
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
    if (para_addr % 2048 != 0 )
    {
        DBG_LOG("download address must be 2K alined! And the iap para cannot be empty!\n");
        return -1;
    }
    if (flash_open( (volatile unsigned short *)para_addr, PARAMETER_FLASH_SIZE , Read ) != 0)
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
    unsigned char ch;
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
                   *ptr = '\0';
                   break;
               }
               else if (ch == '\r') flag = 1;
               else flag = 0;
           }
        }
    }
	DBG_LOG("parameter length is: %d\n", strlen(temp));
    ptr = temp;
    char format[11] = "%*s ";
    for (i = 0; i < IAP_PARA_ITEM_NUM; i++)
    {
        ptr = strstr(temp, para_item[i].name);
        if (ptr)
        {
            strcat(format, para_item[i].type);
            sscanf(ptr, format, para_item[i].struct_item_addr);
            format[4] = '\0';
        }
        else
        {
            DBG_LOG("response parameter error!\n");
            flash_close();
            free(temp);
            return -1;
        }
    }
    if ( i == IAP_PARA_ITEM_NUM )
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

static int generate_bin_md5(uint32_t addr, struct iap_parameter *iap_para_t, char *md5_out)
{
    if (addr % 2048 != 0 )
    {
        DBG_LOG("address must be 2K alined!\n");
        return -1;
    }
    if (flash_open( (volatile unsigned short *)addr, iap_para_t->size , Read ) != 0)
    {
        DBG_LOG("flash open failed!\n");
        return -1;
    }
    int length = 0;
    int count_length = 0;
    unsigned char buf[512];
    MD5_CTX md5;
	MD5Init(&md5);
    unsigned char temp[16];
    while (count_length < iap_para_t->size)
    {
        length = flash_read(buf, 512);
        MD5Update(&md5, buf, length);
        count_length += length;
    }
    if (count_length == iap_para_t->size)
    {
        DBG_LOG("read success%d\n", count_length);
        MD5Final(&md5, temp);
        int i;
        for (i = 0; i < 16; i++)
        {
            sprintf(md5_out, "%02x", temp[i]);
            md5_out += 2;
        }
        flash_close();
        return 0;
    }
    else
    {
        DBG_LOG("flash read num error!\n");
        flash_close();
        return -1;
    }
}
