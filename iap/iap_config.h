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


#ifdef DEBUG
    #define DBG_LOG(...) printf(__VA_ARGS__)                                                   /*need to be implemented*/
#else
    #define DBG_LOG(...)
#endif

#endif
