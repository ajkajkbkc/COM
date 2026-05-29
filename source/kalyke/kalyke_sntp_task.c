/**
  ******************************************************************************
  * @file    kalyke_sntp_task.c
  * @author  lixianyu
  * @version V0.0.1
  * @date    2019-05-11
  * @brief   SNTP
  ******************************************************************************
  */
#include <stdio.h>

#include "lwip/apps/sntp.h"
#include "fsl_debug_console.h"
#include "kalyke_event.h"
#include "plc_task.h"
#include "kalyke_internet_task.h"
#include "fsl_snvs_hp.h"
#include "kalyke_monitor_task.h"
#include "bsp.h"


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
TaskHandle_t gKalykeSNTPTaskHandle;

/*******************************************************************************
 * Code
 ******************************************************************************/

#if 0
void start_sntp(void)
{
    xTaskCreate((TaskFunction_t)kalyke_sntp_task,
                      (const char *)"kalyke_sntp_task",
                      2048,
                      (void *)NULL,
                      3,
                      (TaskHandle_t *)&gKalykeSNTPTaskHandle);
}
#endif
void kalyke_test_same_name_task(void *p_arg)
{
    uint32_t val = ((uint32_t)p_arg);
    while(1)
    {
        if (val == 1)
        {
            LOGV("sn_test", "val = %u.", val);
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
        else
        {
            LOGI("sn_test", "val = %u.", val);
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }
}

void kalyke_sntp_task(void *p_arg)
{
    LOGV("Kalyke_SNTP", "kalyke_sntp_task RUN. Free heap size is %d bytes", xPortGetFreeHeapSize());

    xEventGroupWaitBits(g_kalyke_event_group, KALYKE_EVENT_GOT_IP_SNTP, pdTRUE, pdFALSE, portMAX_DELAY);
    LOGD("Kalyke_SNTP", "Let us do SNTP because we had got the IP address.");
    vTaskDelay(5222);
#if 1
    snvs_hp_rtc_datetime_t rtcDate;
    SNVS_HP_RTC_GetDatetime(SNVS, &rtcDate);
    if (gBspIam1970 == true)
    {
        goto DO_GET;
    }
    if (rtcDate.year != 2020)
    {
        LOGW("Kalyke_SNTP", "rtcDate.year != 2020, so just return");
        vTaskDelete(NULL);
        return;
    }
#endif
DO_GET:
    while (1)
    {
        //if (link_is_up(&phyHandle1))
        if (gWanOK == true)
        {
            break;
        }
        else
        {
            vTaskDelay(5222);
        }
    }
    //vTaskDelay(3000);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //sntp_setservername(0, "cn.pool.ntp.org");
    sntp_setservername(0, "pool.ntp.org");
    //sntp_setservername(0, "148.251.69.45");
    //sntp_setservername(0, "185.209.85.222");
    
    //ipaddr_aton("193.228.143.12", &ip_addr);
    //ipaddr_aton("94.130.49.186", &ip_addr);
    //sntp_setserver(0, &ip_addr);
    //sntp_servermode_dhcp(0);
#if 0
#if LWIP_DHCP
    //sntp_setserver(0, &ip_addr);
    sntp_setservername(0, "cn.pool.ntp.org");

    sntp_servermode_dhcp(1); /* get SNTP server via DHCP */
    

#else /* LWIP_DHCP */
#if LWIP_IPV4
    //sntp_setserver(0, netif_ip_gw4(netif_default));
    //sntp_setserver(0, &ip_addr);
    
    sntp_setservername(0, "cn.pool.ntp.org");
#endif /* LWIP_IPV4 */
#endif /* LWIP_DHCP */
#endif
    LOGD("Kalyke_SNTP", "Before call sntp_init()");
    plc_re_run();
    sntp_init();
    plc_re_run();
    LOGD("Kalyke_SNTP", "After call sntp_init()");
    print_vTaskList();
    LOGW("Kalyke_SNTP", "Free heap size is %d bytes", xPortGetFreeHeapSize());
    vTaskDelay(30000);
#if 0
    for(;;)
    {
        vTaskDelay(100000);
        LOGV("Kalyke_SNTP", "Running");
    }
#else
    LOGW("Kalyke_SNTP", "Delete myself");
    vTaskDelete(NULL);
#endif
}

