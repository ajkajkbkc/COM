/**
  ******************************************************************************
  * @file    daisy_task.c
  * @author  lixianyu
  * @version V0.0.1
  * @date    2020-04-18
  * @brief
  ******************************************************************************
  */
#include <stdio.h>
#include "lwip/opt.h"

#include "lwip/api.h"
#include "lwip/apps/mqtt.h"
#include "lwip/dhcp.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/netifapi.h"
#include "lwip/prot/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"

//#include "ctype.h"

#include "board.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "fsl_pit.h"

#include "kalyke_opts.h"
#include "kalyke_event.h"

#include "daisy_task.h"
#include "fsl_debug_console.h"
#include "kalyke_version.h"
#include "kalyke_tool.h"
#include "kalyke_internet_task.h"
#include "kalyke_monitor_task.h"
#include "plc_sysblock.h"
#include "plc_errormsg.h"
#include "plc_variable.h"

#include "bsp.h"
#include "plc_element.h"
#include "bsp_dct.h"
#include "bsp_led.h"
#include "bsp_gpio.h"

TaskHandle_t gDaisyTaskHandle = NULL;

#if (DAISY_MASTER_FEATURE == 1)
#if (LOG_OPEN == 1)
/*
 * 0 = close
 * 1 = 以微秒为单位计算响应时间
 * 2 = 以毫秒为单位计算响应时间
 * 3 = 同时以毫秒、微秒计算响应时间
*/
#define DAISY_COMUNICATION_COUNT    3
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
typedef enum _DAISY_STATE
{
    DAISY_WAIT,
    DAISY_WORKING,
} daisy_state_e;

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PHY_ADDRESS_1 (0x02U) /* Phy address of enet port 0. */
#define PHY_ADDRESS_2 (0x01U)

#if (DAISY_CONFIG_WHEN_LOOP == 1)
#define LOOP_CONFIG_FLAG_MOD    19
#endif
#define RESP_TIME_OUT           20 //(ms)
/*******************************************************************************
 * Variables
 ******************************************************************************/
static const char *TAG = "DaisyMaster";

static TimerHandle_t gLoopTimer;
static TimerHandle_t gRespTimeOutTimer;
static TimerHandle_t gCloseErrLEDTimer;
static TimerHandle_t gPLCStartTimer;

static uint32_t gDaisyTick0;
static uint32_t gDaisyTickMS;

static struct netif gNetifWan;
static struct netif gNetifLan;
//static uint64_t gMcuID;

uint8_t gEthBroadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint8_t gEthOther[6] = {0x0C, 0x1C, 0x2B, 0x3F, 0x4D, 0x51};

/* 本地Mac   */
static uint8_t gDaisyMacWAN[6] = {0x8C, 0xEC, 0x4B, 0xAF, 0x8D, 0x01};
static uint8_t gDaisyMacLAN[6] = {0x8C, 0xEC, 0x4B, 0xAF, 0x8D, 0x33};

static uint8_t gDaisySendBuffer[1600];
static uint8_t *gpLoopData;
struct pbuf *gpbufLoop;

//static uint8_t gDaisyWANRecvBuffer[1500];
uint8_t gDaisyLANRecvBuffer[512];
static uint16_t gLanRecvLength;
//static TaskHandle_t gKalykeTCPIPHandle = NULL;
volatile daisy_fsm_e gDaisyFSM;
volatile uint16_t gConfigErrNum;
static volatile uint16_t gCurSubIdx = 0;
static uint8_t gWKC[MAX_SUB_STATION_ITEM];
uint16_t gSlaveIDUpgrade = 0; //给哪个从站升级

#if (DAISY_CONFIG_WHEN_LOOP == 1)
static uint32_t gLoopConfigflag = 0;
#endif

static uint32_t gDaisyScanBeginTime = 0; // ms
static uint32_t gDaisyRespTimeoutCounts = 0; // ms

#define DAISY_ERR_THRESHOLD    800
static volatile int gDaisyErrThresholdCounter = 0;



/*******************************************************************************
 * Code
 ******************************************************************************/
void kalyke_daisy_init(void)
{
    static bool justReboot = true;
    LOGV(TAG, "Enter %s()", __func__);
    if (g_plc_netcfg.lan.ioExp != 0)
    {
        LOGW(TAG, "g_plc_netcfg.lan.ioExp != 0, just return.");
        return;
    }
    if (justReboot == false)
    {
        xTimerStart(gPLCStartTimer, 0);
        return;
    }
    justReboot = false;
}

void kalyke_daisy_stop(void)
{
    LOGV(TAG, "Enter %s()", __func__);
    SET_SD_ELEMENT_VALUE(SD234, 101);
}

static void daisy_plc_start(TimerHandle_t ltv_TimeHandle)
{
    LOGV(TAG, "Enter %s(), ltv_TimeHandle = 0x%08X", __func__, ltv_TimeHandle);
#if (PLC_RUN_WAIT_DAISY == 1)
    xEventGroupSetBits(g_kalyke_event_group, KALYKE_EVENT_PLC_TASK_WAIT_DAISY);
#endif
}

static void log_pbuf(struct pbuf *p)
{
    LOGD(TAG, "Enter %s(), p = 0x%08X", __func__, p);
    LOGV(TAG, "p->next = 0x%08X", p->next);
    LOGD(TAG, "p->payload = 0x%08X", p->payload);
    LOGV(TAG, "p->tot_len = %u, p->len = %u", p->tot_len, p->len);
    hexdump(p->payload, p->len);
    LOGD(TAG, "p->type_internal = 0x%X", p->type_internal);
    LOGD(TAG, "p->flags = 0x%X", p->flags);
    LOGV(TAG, "p->ref = %u", p->ref);
    LOGD(TAG, "p->if_idx = %u", p->if_idx);
}

static void notify_daisy_task(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(gDaisyTaskHandle, &higherPriorityTaskWoken);
    if (higherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR (higherPriorityTaskWoken);
    }
}

static inline void daisy_set_bit_element_value(unsigned short * BaseAddr, unsigned short Element, unsigned char Value)
{
    unsigned short * lsp_BaseAddr = BaseAddr;

    lsp_BaseAddr += (Element >> 4);

    if(Value) {
        *lsp_BaseAddr |= (0x01 << (Element & 0x0F));
    } else {
        *lsp_BaseAddr &= (~(0x01 << (Element & 0x0F)));
    }
}

static inline void daisy_set_8bit_element_value(unsigned short *baseAddr, unsigned short bitElement, unsigned char value)
{
    baseAddr += (bitElement >> 4);
    if ((bitElement & 0x0F) == 0)
    {
        *baseAddr &= 0xFF00;
        *baseAddr |= value;
    }
    else
    {
        *baseAddr &= 0x00FF;
        *baseAddr |= (value << 8);
    }
}
static bool judgeTheFirstLostSlave(uint16_t idx, uint8_t *pData)
{
    gWKC[gBusConfig.pPDO1A[idx].eAddr] = pData[gBusConfig.pPDO1A[idx].offset];
    //LOGV(TAG, "gWKC[%u] = %u", gBusConfig.pPDO1A[i].eAddr, gWKC[gBusConfig.pPDO1A[i].eAddr]);
    if (gWKC[gBusConfig.pPDO1A[idx].eAddr] == 0) //如果WKC为0，则说明从站失联了
    {
        SET_SD_ELEMENT_VALUE(SD233, gBusConfig.pPDO1A[idx].eAddr);
        plc_refresh_error_msg(ERR_SLAVE_OFFLINE);
        guv_NonStopError.bit.extend_bus_err = 1;
    #if (DAISY_CONFIG_WHEN_LOOP == 1)
        gLoopConfigflag = 0;
        gDaisyFSM = DAISY_FSM_LOOP_CONFIG;
    #endif
        LOGW(TAG, "We lost slave : %u", GET_SD_ELEMENT_VALUE(SD233));
        return true;
    }
    else
    {
        if (GET_SD_ELEMENT_VALUE(SD233) != 0)
        {
            BaseType_t higherPriorityTaskWoken = pdFALSE;
            xTimerStartFromISR(gCloseErrLEDTimer, &higherPriorityTaskWoken);
            SET_SD_ELEMENT_VALUE(SD233, 0);
            guv_NonStopError.bit.extend_bus_err = 0;
        }
    }
    return false;
}

//处理从从站收到的数据
static void daisy_handle_loop_data(uint8_t *pData)
{
    static bool bFindFirstLostSlave = false;

    bFindFirstLostSlave = false;
    for (uint16_t i = 0; i < gBusConfig.pdo1ACount; i++)
    {
        //LOGW(TAG, "gBusConfig.pPDO1A[%u].eType = %u", i, gBusConfig.pPDO1A[i].eType);
        switch (gBusConfig.pPDO1A[i].eType)
        {
            case DAISY_E_TYPE_WKC:
                if (bFindFirstLostSlave == false)
                {
                    bFindFirstLostSlave = judgeTheFirstLostSlave(i, pData);
                }
                break;

            case DAISY_E_TYPE_X:
                daisy_set_8bit_element_value(X_ELEMENT, gBusConfig.pPDO1A[i].eAddr, pData[gBusConfig.pPDO1A[i].offset]);
                break;

            case DAISY_E_TYPE_D:
                gtv_PlcElement.msp_DElement[gBusConfig.pPDO1A[i].eAddr] = (pData[gBusConfig.pPDO1A[i].offset + 1] << 8) | (pData[gBusConfig.pPDO1A[i].offset]);
                break;

            case DAISY_E_TYPE_SD:
                gtv_PlcElement.msp_SDElement[gBusConfig.pPDO1A[i].eAddr] = (pData[gBusConfig.pPDO1A[i].offset + 1] << 8) | (pData[gBusConfig.pPDO1A[i].offset]);
                break;

            case DAISY_E_TYPE_R:
                gtv_PlcElement.msp_RElement[gBusConfig.pPDO1A[i].eAddr] = (pData[gBusConfig.pPDO1A[i].offset + 1] << 8) | (pData[gBusConfig.pPDO1A[i].offset]);
                break;

            default:
                LOGE(TAG, "%s(): element type error(%u)", __func__, gBusConfig.pPDO1A[i].eType);
                break;
        }
    }
}

#if (DAISY_CONFIG_WHEN_LOOP == 1)
static inline void daisy_handle_loop_config_data(uint8_t *pData)
{
    for (uint16_t i = 0; i < gBusConfig.pdo1ACount; i++)
    {
        //LOGW(TAG, "gBusConfig.pPDO1A[%u].eType = %u", i, gBusConfig.pPDO1A[i].eType);
        switch (gBusConfig.pPDO1A[i].eType)
        {
            case DAISY_E_TYPE_X:
                daisy_set_8bit_element_value(X_ELEMENT, gBusConfig.pPDO1A[i].eAddr, pData[gBusConfig.pPDO1A[i].offset]);
                break;

            case DAISY_E_TYPE_D:
                gtv_PlcElement.msp_DElement[gBusConfig.pPDO1A[i].eAddr] = (pData[gBusConfig.pPDO1A[i].offset + 1] << 8) | (pData[gBusConfig.pPDO1A[i].offset]);
                break;

            case DAISY_E_TYPE_SD:
                gtv_PlcElement.msp_SDElement[gBusConfig.pPDO1A[i].eAddr] = (pData[gBusConfig.pPDO1A[i].offset + 1] << 8) | (pData[gBusConfig.pPDO1A[i].offset]);
                break;

            case DAISY_E_TYPE_R:
                gtv_PlcElement.msp_RElement[gBusConfig.pPDO1A[i].eAddr] = (pData[gBusConfig.pPDO1A[i].offset + 1] << 8) | (pData[gBusConfig.pPDO1A[i].offset]);
                break;

            default:
                LOGE(TAG, "%s(): element type error(%u)", __func__, gBusConfig.pPDO1A[i].eType);
                break;
        }
    }
}
#endif

static void daisy_handle_lan_received_data(uint8_t *pData, uint16_t length)
{
    gDaisyRespTimeoutCounts = 0;
    uint16_t cmd = *(pData + 1) << 8 | *pData;
    if(gDaisyFSM != DAISY_FSM_LOOP)
    {
        LOGV(TAG, "Enter %s(), gDaisyFSM = %u, cmd = 0x%04X", __func__, gDaisyFSM, cmd);
    }
    if (cmd == DAISY_CMD_8888)
    {
        LOGV(TAG, "Enter %s(), length = %u, gDaisyFSM = %u, cmd = 0x%04X", __func__, length, gDaisyFSM, cmd);
        //hexdump(pData, 16);
        //88 88 01 68 69 03 EE 55 00 00 00 00 00 00 00 00
        memcpy(gDaisyLANRecvBuffer, pData + 2, 14);
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xEventGroupSetBitsFromISR(g_kalyke_event_group, KALYKE_EVENT_UPGRADE_SLAVE, &higherPriorityTaskWoken);
        return;
    }
    if (cmd == DAISY_CMD_1111)
    {
        LOGD(TAG, "cmd == DAISY_CMD_1111");
        memset(gDaisyLANRecvBuffer, 0, sizeof(gDaisyLANRecvBuffer));
        memcpy(gDaisyLANRecvBuffer, pData, length);
        gLanRecvLength = length;

        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xEventGroupSetBitsFromISR(g_kalyke_event_group, KALYKE_EVENT_DAISY_WAIT_ID, &higherPriorityTaskWoken);
        return;
    }
    switch (gDaisyFSM)
    {
        case DAISY_FSM_IDLE:
        {
            uint16_t subNum = *(pData + 3) << 8 | *(pData + 2);
            LOGD(TAG, "subNum = %u, gBusConfig.nSubStationCount = %u", subNum, gBusConfig.nSubStationCount);
            if (subNum == gBusConfig.nSubStationCount)
            {
                gDaisyFSM = DAISY_FSM_CONFIG;
                gCurSubIdx = 0;
                if (guv_NonStopError.bit.extend_io_num_err == 1)
                {
                    guv_NonStopError.bit.extend_io_num_err = 0;
                    BaseType_t higherPriorityTaskWoken = pdFALSE;
                    xTimerStartFromISR(gCloseErrLEDTimer, &higherPriorityTaskWoken);
                }
            }
            else
            {
                LOGE(TAG, "Slave number ERROR");
                if (guv_NonStopError.bit.extend_io_num_err == 0)
                {
                    plc_refresh_error_msg(ERR_SLAVE_NUM_ERR);
                    guv_NonStopError.bit.extend_io_num_err = 1;
                }
            }
            notify_daisy_task();
        }
        break;

        case DAISY_FSM_CONFIG:
        {
            gConfigErrNum = *(pData + 3) << 8 | *(pData + 2);
            LOGV(TAG, "gConfigErrNum = %u", gConfigErrNum);
            if (gConfigErrNum == DAISY_ERR_NO_ERR)
            {
                gCurSubIdx++;
                if (gCurSubIdx >= gBusConfig.nSubStationCount)
                {
                    gDaisyFSM = DAISY_FSM_PRE_LOOP;
                }
            }
            else
            {
                LOGE(TAG, "Slave config ERROR");
                if (guv_NonStopError.bit.extend_cfg_err == 0)
                {
                    plc_refresh_error_msg(ERR_SLAVE_CFG_ERR);
                    guv_NonStopError.bit.extend_cfg_err = 1;
                }
            }
            notify_daisy_task();
        }
        break;

        case DAISY_FSM_PRE_LOOP:
        {
            uint16_t subNum = *(pData + 3) << 8 | *(pData + 2);
            LOGW(TAG, "subNum = %u, gBusConfig.nSubStationCount = %u", subNum, gBusConfig.nSubStationCount);
            if (subNum == gBusConfig.nSubStationCount)
            {
                gDaisyFSM = DAISY_FSM_PRE_LOOP2;
            }
            notify_daisy_task();
        }
        break;

        case DAISY_FSM_LOOP:
            daisy_handle_loop_data(pData + 2);
            notify_daisy_task();
            break;

    #if (DAISY_CONFIG_WHEN_LOOP == 1)
        case DAISY_FSM_LOOP_CONFIG:
            if ((gLoopConfigflag ) % LOOP_CONFIG_FLAG_MOD == 0 || (GET_SD_ELEMENT_VALUE(SD233) == 1))
            {
                gConfigErrNum = *(pData + 3) << 8 | *(pData + 2);
                LOGV(TAG, "gConfigErrNum = %u", gConfigErrNum);
                if (gConfigErrNum == DAISY_ERR_NO_ERR)
                {
                    gDaisyFSM = DAISY_FSM_LOOP;
                }
            }
            else
            {
                daisy_handle_loop_config_data(pData + 2);
            }
            notify_daisy_task();
            break;
    #endif

        default:
            LOGE(TAG, "ERROR: gDaisyFSM = %u", gDaisyFSM);
            break;
    }

    //LOGD(TAG, "Leave %s()", __func__);
}

static inline void calculate_resp_time(struct netif *netif)
{
#if (DAISY_COMUNICATION_COUNT == 2)
    uint32_t tick1 = xTaskGetTickCount();
    uint32_t interval = tick1 - gDaisyTick0;
    LOGV(TAG, "Enter %s(), netif = 0x%08X, response interval = %u(ms)", __func__, netif, interval);
#elif (DAISY_COMUNICATION_COUNT == 1)
    uint32_t tickInterval;
    uint32_t tick1 = SysTick->VAL;
    if (gDaisyTick0 >= tick1)
    {
        tickInterval = gDaisyTick0 - tick1;
    }
    else
    {
        tickInterval = 600000 - tick1 + gDaisyTick0;
    }
    double us = tickInterval / 600.0;
    if(gDaisyFSM != DAISY_FSM_LOOP)
    {
        LOGI(TAG, "Enter %s(), netif = 0x%08X, response interval = %.1f(us), cpu ticks: %u", __func__, netif, us, tickInterval);
    }
    else
    {
        if (gtv_PlcElement.msp_SDElement[SD234] == 1999)
        {
            LOGI(TAG, "Enter %s(), netif = 0x%08X, response interval = %.1f(us), cpu ticks: %u", __func__, netif, us, tickInterval);
        }
    }
#elif (DAISY_COMUNICATION_COUNT == 3)
    uint32_t tick1 = SysTick->VAL;
    uint32_t tickMS = xTaskGetTickCount();
    uint32_t intervalMS = tickMS - gDaisyTickMS;
    uint32_t tickInterval;
    if (gDaisyTick0 >= tick1)
    {
        tickInterval = gDaisyTick0 - tick1;
    }
    else
    {
        tickInterval = 600000 - tick1 + gDaisyTick0;
    }
    double us = tickInterval / 600.0;
    if(gDaisyFSM != DAISY_FSM_LOOP)
    {
        LOGI(TAG, "Enter %s(), netif = 0x%08X, response interval = %.1f(us) | %u(ms), cpu ticks: %u", __func__, netif, us, intervalMS, tickInterval);
    }
    else
    {
        if (gtv_PlcElement.msp_SDElement[SD234] == 1999)
        {
            LOGI(TAG, "Enter %s(), netif = 0x%08X, response interval = %.1f(us) | %u(ms), cpu ticks: %u", __func__, netif, us, intervalMS, tickInterval);
        }
    }
#endif
}

//This funciton called in interrupt, so we can't call FreeRTOS API.
err_t daisy_ethernet_input(struct pbuf *p, struct netif *netif)
{
#if (LOG_OPEN == 1)
    calculate_resp_time(netif);
#endif

    /* points to packet payload, which starts with an Ethernet header */
    struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
    if (ETHTYPE_ETHER_KALYKE != lwip_htons(ethhdr->type))
    {
        LOGE(TAG, "lwip_htons(ethhdr->type) = 0x%04X, ethhdr->type = 0x%04X", lwip_htons(ethhdr->type), ethhdr->type);
        pbuf_free(p);
        return ERR_RTE;
    }

    if (gtv_PlcElement.msp_SDElement[SD234] == 1999 || (gDaisyFSM != DAISY_FSM_LOOP))
    {
        #if 0
        log_pbuf(p);

        LOGW(TAG, "ethernet_input: dest:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", src:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", type:%"X16_F"\n",
                   (unsigned char)ethhdr->dest.addr[0], (unsigned char)ethhdr->dest.addr[1], (unsigned char)ethhdr->dest.addr[2],
                   (unsigned char)ethhdr->dest.addr[3], (unsigned char)ethhdr->dest.addr[4], (unsigned char)ethhdr->dest.addr[5],
                   (unsigned char)ethhdr->src.addr[0],  (unsigned char)ethhdr->src.addr[1],  (unsigned char)ethhdr->src.addr[2],
                   (unsigned char)ethhdr->src.addr[3],  (unsigned char)ethhdr->src.addr[4],  (unsigned char)ethhdr->src.addr[5],
                   lwip_htons(ethhdr->type));
        #endif
    }

    if (netif == (&fsl_netif1))
    {
        daisy_handle_lan_received_data((uint8_t *)p->payload + SIZEOF_ETH_HDR, p->len - SIZEOF_ETH_HDR);
    }

    pbuf_free(p);

    return ERR_OK;
}

/* Just for testing */
static void daisy_send(uint8_t *pBuf, uint16_t len, struct netif* src_netif, struct netif* dst_netif)
{
    LOGV(TAG, "Enter %s(), src_netif = 0x%08X, dst_netif = 0x%08X", __func__, src_netif, dst_netif);
    //hexdump(pBuf, len);

    struct pbuf *p = NULL;
    p = pbuf_alloc(PBUF_LINK, len, PBUF_RAM);
    //LOGD(TAG, "p = 0x%08X, p->payload = 0x%08X, p->len = %u", p, p->payload, p->len);
    if (p && p->payload)
    {
        memcpy(p->payload, pBuf, len);
    }
    log_pbuf(p);

    err_t ret = ethernet_output(src_netif, p, (struct eth_addr *)(src_netif->hwaddr), (struct eth_addr *)(dst_netif->hwaddr), ETHTYPE_ETHER_KALYKE);
    pbuf_free(p);

    LOGD(TAG, "Leave %s(), ret = %d", __func__, ret);
}

/* 因为我是主站，我总是发往下一级 */
static void daisy_LAN_send(uint8_t *pBuf, uint16_t len)
{
    //LOGV(TAG, "Enter %s()", __func__);
    struct pbuf *p = pbuf_alloc(PBUF_LINK, len, PBUF_RAM);
    if (p && p->payload)
    {
        memcpy(p->payload, pBuf, len);
    }
    else
    {
        pbuf_free(p);
        LOGE(TAG, "%s: ERROR, p = 0x%08X, p->payload = 0x%08X", __func__, p, p->payload);
        return;
    }
    //log_pbuf(p);
#if (DAISY_COMUNICATION_COUNT == 2)
    gDaisyTick0 = xTaskGetTickCount();
#elif (DAISY_COMUNICATION_COUNT == 1)
    gDaisyTick0 = SysTick->VAL;
#elif (DAISY_COMUNICATION_COUNT == 3)
    gDaisyTick0 = SysTick->VAL;
    gDaisyTickMS = xTaskGetTickCount();
#endif
    err_t ret = ethernet_output(&fsl_netif1, p, (struct eth_addr *)(fsl_netif1.hwaddr), (struct eth_addr *)(gEthBroadcast), ETHTYPE_ETHER_KALYKE);
    pbuf_free(p);

    //LOGD(TAG, "Leave %s(), ret = %d", __func__, ret);
}

void daisy_LAN_send_bin(uint8_t *pBuf, uint16_t len)
{
    //LOGV(TAG, "Enter %s()", __func__);
    struct pbuf *p = pbuf_alloc(PBUF_LINK, len + 4, PBUF_RAM);
    if (p && p->payload)
    {
        uint16_t *pData = (uint16_t *)p->payload;
        *pData++ = DAISY_CMD_8888;
        *pData++ = gSlaveIDUpgrade;
        memcpy(pData, pBuf, len + 4);
    }
    else
    {
        pbuf_free(p);
        LOGE(TAG, "%s: ERROR, p = 0x%08X, p->payload = 0x%08X", __func__, p, p->payload);
        return;
    }
    //log_pbuf(p);
#if (DAISY_COMUNICATION_COUNT == 2)
    gDaisyTick0 = xTaskGetTickCount();
#elif (DAISY_COMUNICATION_COUNT == 1)
    gDaisyTick0 = SysTick->VAL;
#elif (DAISY_COMUNICATION_COUNT == 3)
    gDaisyTick0 = SysTick->VAL;
    gDaisyTickMS = xTaskGetTickCount();
#endif
    err_t ret = ethernet_output(&fsl_netif1, p, (struct eth_addr *)(fsl_netif1.hwaddr), (struct eth_addr *)(gEthBroadcast), ETHTYPE_ETHER_KALYKE);
    pbuf_free(p);

    //LOGD(TAG, "Leave %s(), ret = %d", __func__, ret);
}

static void test_speed(uint16_t length)
{
    LOGD(TAG, "Enter %s(), length = %u", __func__, length);
    memset(gDaisySendBuffer, 0, sizeof(gDaisySendBuffer));
    uint16_t cnt;
    if (length % 100 == 0)
    {
        cnt = length / 100;
    }
    else
    {
        cnt = length / 100;
        cnt++;
    }
    for (uint16_t i = 0; i < cnt; i++)
    {
        strcpy((char *)gDaisySendBuffer + i * 100, "1234567890_Nice to meet you!!!!0123456789.&&&&((((1234567890_Nice to meet you!!!!0123456789.&&&&(((("); //100 bytes
    }
    daisy_LAN_send(gDaisySendBuffer, length);
    //sprintf((char *)gDaisySendBuffer, "1234567890_Nice to meet you!!!!0123456789.0x%08X", gKalykeSecondTickCurrent);
    //sprintf((char *)gDaisySendBuffer, "1234567890_Nice to meet you!!!!0123456789.&&&&"); //46 bytes
    //daisy_LAN_send(gDaisySendBuffer, strlen((char *)gDaisySendBuffer));
}

static inline uint8_t daisy_get_bit_element_value(unsigned short * BaseAddr, unsigned short Element)
{
    unsigned char lcv_ElementValue;

    lcv_ElementValue = (*(BaseAddr + (Element >> 4)) >> (Element & 0x0F)) & 0x01;

    return lcv_ElementValue;
}

static inline uint8_t daisy_get_8bit_element_value(uint16_t *baseAddr, uint16_t element)
{
    uint8_t lcv_ElementValue;

    lcv_ElementValue = (*(baseAddr + (element >> 4)) >> (element & 0x0F));

    return lcv_ElementValue;
}

// 发送集数帧
static inline void loop_send(void)
{
    //LOGV(TAG, "Enter %s()", __func__);
    //uint8_t *pData = gDaisySendBuffer + 2;
    uint8_t *pData = gpLoopData;
    //hexdump(pData, gBusConfig.nSynBuffLen);
    for (int i = 0; i < gBusConfig.pdo1BCount; i++)
    {
        switch (gBusConfig.pPDO1B[i].eType)
        {
            case DAISY_E_TYPE_Y:
                pData[gBusConfig.pPDO1B[i].offset] = daisy_get_8bit_element_value(Y_ELEMENT, gBusConfig.pPDO1B[i].eAddr);
                break;

            case DAISY_E_TYPE_D:
                pData[gBusConfig.pPDO1B[i].offset] = gtv_PlcElement.msp_DElement[gBusConfig.pPDO1B[i].eAddr] & 0x00FF;
                pData[gBusConfig.pPDO1B[i].offset + 1] = (gtv_PlcElement.msp_DElement[gBusConfig.pPDO1B[i].eAddr] & 0xFF00) >> 8;
                break;

            case DAISY_E_TYPE_SD:
                pData[gBusConfig.pPDO1B[i].offset] = gtv_PlcElement.msp_SDElement[gBusConfig.pPDO1B[i].eAddr] & 0x00FF;
                pData[gBusConfig.pPDO1B[i].offset + 1] = (gtv_PlcElement.msp_SDElement[gBusConfig.pPDO1B[i].eAddr] & 0xFF00) >> 8;
                break;

            case DAISY_E_TYPE_R:
                pData[gBusConfig.pPDO1B[i].offset] = gtv_PlcElement.msp_RElement[gBusConfig.pPDO1B[i].eAddr] & 0x00FF;
                pData[gBusConfig.pPDO1B[i].offset + 1] = (gtv_PlcElement.msp_RElement[gBusConfig.pPDO1B[i].eAddr] & 0xFF00) >> 8;
                break;

            default:
                LOGE(TAG, "ERROR: eType = %u", gBusConfig.pPDO1B[i].eType);
                break;
        }
    }
    //daisy_LAN_send(gDaisySendBuffer, 2 + gBusConfig.nSynBuffLen);
    //err_t ret = ethernet_output(&fsl_netif1, gpbufLoop, (struct eth_addr *)(fsl_netif1.hwaddr), (struct eth_addr *)(gEthBroadcast), ETHTYPE_ETHER_KALYKE);
#if (DAISY_COMUNICATION_COUNT == 2)
    gDaisyTick0 = xTaskGetTickCount();
#elif (DAISY_COMUNICATION_COUNT == 1)
    gDaisyTick0 = SysTick->VAL;
#elif (DAISY_COMUNICATION_COUNT == 3)
    gDaisyTick0 = SysTick->VAL;
    gDaisyTickMS = xTaskGetTickCount();
#endif
    err_t ret = ethernet_output_daisy(&fsl_netif1, gpbufLoop);
    //LOGV(TAG, "Leave %s(), ret = %d", __func__, ret);
}

static void close_err_led(TimerHandle_t ltv_TimeHandle)
{
    bsp_close_err_led();
}

// 集数帧发出后，没有收到响应的处理
static void resp_timeout(TimerHandle_t ltv_TimeHandle)
{
    gDaisyRespTimeoutCounts++;
    LOGV(TAG, "Enter %s(), gDaisyFSM = %u, gDaisyRespTimeoutCounts = %u", __func__, gDaisyFSM, gDaisyRespTimeoutCounts);
    if (gDaisyRespTimeoutCounts < 5)
    {
        notify_daisy_task();
        return;
    }
    //bsp_reboot_system();



#if (DAISY_CONFIG_WHEN_LOOP == 1)
    if (GET_SD_ELEMENT_VALUE(SD233) == 1)
    {
        gDaisyFSM = DAISY_FSM_LOOP_CONFIG;
    }
#endif
    switch (gDaisyFSM)
    {
        case DAISY_FSM_IDLE:
        case DAISY_FSM_CONFIG:
        case DAISY_FSM_PRE_LOOP:
        case DAISY_FSM_PRE_LOOP2:
        case DAISY_FSM_LOOP:
    #if (DAISY_CONFIG_WHEN_LOOP == 1)
        case DAISY_FSM_LOOP_CONFIG:
    #endif
            plc_refresh_error_msg(ERR_SLAVE_OFFLINE);
            SET_SD_ELEMENT_VALUE(SD233, 1);
            guv_NonStopError.bit.extend_bus_err = 1;
            notify_daisy_task();
            break;
        default:
            LOGE(TAG, "ERROR: gDaisyFSM = %u", gDaisyFSM);
            break;
    }
    LOGD(TAG, "Leave %s()", __func__);
}

static void begin_loop(TimerHandle_t ltv_TimeHandle)
{
    LOGV(TAG, "Enter %s()", __func__);
    notify_daisy_task();
}

void daisy_get_info(md_slave_msg_pack *pMsg)
{
    vTaskSuspend(gDaisyTaskHandle);
    LOGV(TAG, "Enter %s(), pMsg = 0x%08X", __func__, pMsg);
    int i;
    for(i = 0; i < 3; i++)
    {
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
    }

    memset(gDaisySendBuffer, 0, 404);
    uint16_t *pu16Data = (uint16_t *)gDaisySendBuffer;
    *pu16Data = DAISY_CMD_1111;
    // 一个从站ID是2个字节，200从站是400个字节
    daisy_LAN_send(gDaisySendBuffer, 404);
    xEventGroupWaitBits(g_kalyke_event_group, KALYKE_EVENT_DAISY_WAIT_ID, pdTRUE, pdFALSE, 1000);

    pu16Data = (uint16_t *)(gDaisyLANRecvBuffer + 2);
    uint16_t nb = *pu16Data; //从站个数
    uint16_t idBytesLen = nb * 2;

    nb += 1; //从站个数 + 1
    LOGI(TAG, "idBytesLen = %u, nb = %u", idBytesLen, nb);
    pMsg->mcp_RespBuff[3] = (uint8_t)(nb & 0xFF);
    pMsg->mcp_RespBuff[4] = (uint8_t)(nb >> 8);

    LOGD(TAG, "mlv_DeviceTypeId = 0x%04X", gtv_DeviceConfigTable.mtv_DevInfo.mlv_DeviceTypeId);
    pMsg->mcp_RespBuff[5] = (uint8_t)(gtv_DeviceConfigTable.mtv_DevInfo.mlv_DeviceTypeId & 0xFF);
    pMsg->mcp_RespBuff[6] = (uint8_t)(gtv_DeviceConfigTable.mtv_DevInfo.mlv_DeviceTypeId >> 8);

    memcpy(pMsg->mcp_RespBuff + 7, (uint8_t *)(pu16Data + 1), idBytesLen);
    pMsg->msv_RespLen = idBytesLen + 7;
    mb_slave_verify_resp_msg(pMsg);
    hexdump(pMsg->mcp_RespBuff, 16);
    LOGD(TAG, "Leave %s()", __func__);
    vTaskResume(gDaisyTaskHandle);
}

static void daisy_task_handle_idle(void)
{
    uint16_t *pu16Data;
    LOGW(TAG, "Enter %s, gDaisyFSM = %u", __func__, gDaisyFSM);
    xTimerStop(gRespTimeOutTimer, 0);
    vTaskDelay(2000);
    pu16Data = (uint16_t *)gDaisySendBuffer;
    *pu16Data++ = DAISY_CMD_0101;
    *pu16Data = 0;
    xTimerStart(gRespTimeOutTimer, 0);
    daisy_LAN_send(gDaisySendBuffer, 4);
}

static void daisy_task_handle_config(void)
{
    uint16_t *pu16Data;
    uint8_t  *pu8Data;
    LOGW(TAG, "Enter %s, gDaisyFSM = %u", __func__, gDaisyFSM);
    xTimerStop(gRespTimeOutTimer, 0);
    if (gConfigErrNum == DAISY_ERR_NO_ERR)
    {
        vTaskDelay(10);
    }
    else
    {
        vTaskDelay(2000);
    }
    pu16Data = (uint16_t *)gDaisySendBuffer;
    *pu16Data++ = DAISY_CMD_1C1C;

    pu8Data = (uint8_t *)pu16Data;
    memcpy(pu8Data, gBusConfig.item[gCurSubIdx].pSubStationbuf, gBusConfig.item[gCurSubIdx].len);
    xTimerStart(gRespTimeOutTimer, 0);
    daisy_LAN_send(gDaisySendBuffer, 2 + gBusConfig.item[gCurSubIdx].len);
}

static void daisy_task_handle_pre_loop(void)
{
    uint16_t *pu16Data;
    LOGW(TAG, "Enter %s, gDaisyFSM = %u", __func__, gDaisyFSM);
    xTimerStop(gRespTimeOutTimer, 0);
    vTaskDelay(1000);
    pu16Data = (uint16_t *)gDaisySendBuffer;
    *pu16Data++ = DAISY_CMD_2222;
    *pu16Data = 0;
    xTimerStart(gRespTimeOutTimer, 0);
    daisy_LAN_send(gDaisySendBuffer, 2);
}

static void daisy_task_handle_pre_loop_2(void)
{
    LOGW(TAG, "Enter %s, gDaisyFSM = %u", __func__, gDaisyFSM);
    xTimerStop(gRespTimeOutTimer, 0);
    vTaskDelay(500);
    gpbufLoop = pbuf_alloc(PBUF_LINK, 2 + gBusConfig.nSynBuffLen, PBUF_RAM);
    LOGV(TAG, "gpbufLoop = 0x%08X", gpbufLoop);
    memset(gpbufLoop->payload, 0, 2 + gBusConfig.nSynBuffLen);
    gpLoopData = (uint8_t *)gpbufLoop->payload;
    *gpLoopData++ = DAISY_CMD_3333 & 0xFF;
    *gpLoopData++ = DAISY_CMD_3333 >> 8;
    struct eth_hdr *ethhdr;
    u16_t eth_type_be = lwip_htons(ETHTYPE_ETHER_KALYKE);
    LOGD(TAG, "eth_type_be = 0x%04X", eth_type_be);
    pbuf_add_header(gpbufLoop, SIZEOF_ETH_HDR);
    ethhdr = (struct eth_hdr *)gpbufLoop->payload;
    ethhdr->type = eth_type_be;
    SMEMCPY(&ethhdr->dest, (struct eth_addr *)(gEthBroadcast), ETH_HWADDR_LEN);
    SMEMCPY(&ethhdr->src,  (struct eth_addr *)(fsl_netif1.hwaddr), ETH_HWADDR_LEN);
    gDaisyFSM = DAISY_FSM_LOOP;

    xTimerStart(gLoopTimer, 0);
#if (PLC_RUN_WAIT_DAISY == 1)
    xEventGroupSetBits(g_kalyke_event_group, KALYKE_EVENT_PLC_TASK_WAIT_DAISY);
#endif
}

static void daisy_scan_delay(void)
{
    int32_t interval;
    if (gDaisyScanBeginTime == 0)
    {
        interval = gtv_PlcElement.msp_SDElement[SD234];
    }
    else
    {
        interval = xTaskGetTickCount() - gDaisyScanBeginTime;
    }
    /* 计时器溢出的处理 */
    if(interval < 0)
    {
        interval = xTaskGetTickCount() + (0xFFFFFFFFUL - gDaisyScanBeginTime);
    }


    if (interval > gtv_PlcElement.msp_SDElement[SD234])
    {
        gDaisyErrThresholdCounter += 8;
        if (gDaisyErrThresholdCounter > DAISY_ERR_THRESHOLD)
        {
            plc_refresh_error_msg(ERR_SLAVE_SCAN_OVER_TIME);
        }
        gtv_PlcElement.msp_SDElement[SD235] = interval;
        LOGE(TAG, "OMG, daisy scan time interval so big. %d",interval);
    }
    else
    {
        if (gDaisyErrThresholdCounter > 0)
        {
            gDaisyErrThresholdCounter--;
        }


        if ( gtv_PlcElement.msp_SDElement[SD234] == 0)
        {
            gtv_PlcElement.msp_SDElement[SD235] = interval;
        }
        else
        {
            vTaskDelay(gtv_PlcElement.msp_SDElement[SD234] - interval);
            gtv_PlcElement.msp_SDElement[SD235] = gtv_PlcElement.msp_SDElement[SD234];
        }
    }


    if(gtv_PlcElement.msp_SDElement[SD235] > gtv_PlcElement.msp_SDElement[SD236]) 
    {
       gtv_PlcElement.msp_SDElement[SD236] = gtv_PlcElement.msp_SDElement[SD235];
    }
}

static void daisy_task_process_loop(void)
{
    loop_send();
}

#if (DAISY_CONFIG_WHEN_LOOP == 1)
static void daisy_task_process_loop_config(void)
{
    uint16_t slaveID = GET_SD_ELEMENT_VALUE(SD233);
    if (slaveID == 0) // 从站已重新插入
    {
        gDaisyFSM = DAISY_FSM_LOOP;
        xTimerStart(gLoopTimer, 0);
        gLoopConfigflag = 0;
        return;
    }

    gLoopConfigflag++;

    uint16_t curSubIdx = slaveID - 1;
    if (gLoopConfigflag % LOOP_CONFIG_FLAG_MOD == 0 || slaveID == 1)
    {
        memset(gDaisySendBuffer, 0x00, sizeof(gDaisySendBuffer));
        uint16_t *pu16Data = (uint16_t *)gDaisySendBuffer;
        *pu16Data++ = DAISY_CMD_2C2C;

        uint8_t *pu8Data = (uint8_t *)pu16Data;
        memcpy(pu8Data, gBusConfig.item[curSubIdx].pSubStationbuf, gBusConfig.item[curSubIdx].len);
        daisy_LAN_send(gDaisySendBuffer, 2 + gBusConfig.item[curSubIdx].len);
    }
    else
    {
        loop_send();
    }
}
#endif

static void daisy_task(void *p_arg)
{
    LOGV(TAG, "daisy_task RUN. Free heap size is %d bytes", xPortGetFreeHeapSize());
    xEventGroupWaitBits(g_kalyke_event_group, KALYKE_EVENT_ENET_INIT_DONE_PLC, pdTRUE, pdFALSE, portMAX_DELAY);
    vTaskDelay(2000);

    gDaisyFSM = DAISY_FSM_IDLE;

    for(;;)
    {
        //LOGV(TAG, "Task running..., gDaisyFSM = %u", gDaisyFSM);
    #if 0// Test speed.
        vTaskDelay(10000);
        test_speed(1482);
    #else
        switch (gDaisyFSM)
        {
            case DAISY_FSM_IDLE:
                daisy_task_handle_idle();
                break;

            case DAISY_FSM_CONFIG:
                daisy_task_handle_config();
                break;

            case DAISY_FSM_PRE_LOOP:
                daisy_task_handle_pre_loop();
                break;

            case DAISY_FSM_PRE_LOOP2:
                daisy_task_handle_pre_loop_2();
                break;

            case DAISY_FSM_LOOP:
                xTimerStop(gRespTimeOutTimer, 0);
                daisy_scan_delay();
                //xTimerStart(gRespTimeOutTimer, 0);
                xTimerChangePeriod(gRespTimeOutTimer, RESP_TIME_OUT, 0);
                gDaisyScanBeginTime = xTaskGetTickCount();
                daisy_task_process_loop();
                break;

        #if (DAISY_CONFIG_WHEN_LOOP == 1)
            case DAISY_FSM_LOOP_CONFIG:
                xTimerStop(gRespTimeOutTimer, 0);
                daisy_scan_delay();
                xTimerStart(gRespTimeOutTimer, 0);
                gDaisyScanBeginTime = xTaskGetTickCount();
                daisy_task_process_loop_config();
                break;
        #endif

            default:
                LOGE(TAG, "ERROR: gDaisyFSM = %u", gDaisyFSM);
                break;
        }

        /* Sleep until something is received on LAN */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        //LOGI(TAG, "daisy_task wakeup...");
    #endif
    }
}

void start_daisy_task(void)
{
    LOGV(TAG, "Enter %s()", __func__);
    BaseType_t ret = xTaskCreate((TaskFunction_t)daisy_task,
                      (const char *)"daisy_task",
                      DAISY_TASK_STACK_SIZE,
                      (void *)NULL,
                      DAISY_TASK_PRIO,
                      (TaskHandle_t *)&gDaisyTaskHandle);
    if (ret != pdPASS)
    {
        LOGE(TAG, "Create kalyke_monitor_task error!\r\n");
    }
    gLoopTimer = xTimerCreate((const char *)"loop",
                               (TickType_t  )10 / portTICK_PERIOD_MS,
                               (UBaseType_t )pdFALSE,
                               (void *      )0,
                               (TimerCallbackFunction_t)begin_loop);
    gRespTimeOutTimer = xTimerCreate((const char *)"respTimer",
                               (TickType_t  )2000 / portTICK_PERIOD_MS,
                               (UBaseType_t )pdFALSE,
                               (void *      )0,
                               (TimerCallbackFunction_t)resp_timeout);
    gCloseErrLEDTimer = xTimerCreate((const char *)"closeErrLED",
                               (TickType_t  )999 / portTICK_PERIOD_MS,
                               (UBaseType_t )pdFALSE,
                               (void *      )0,
                               (TimerCallbackFunction_t)close_err_led);
    gPLCStartTimer = xTimerCreate((const char *)"plcStart",
                               (TickType_t  )10 / portTICK_PERIOD_MS,
                               (UBaseType_t )pdFALSE,
                               (void *      )0,
                               (TimerCallbackFunction_t)daisy_plc_start);
    LOGD(TAG, "Leave %s(), gLoopTimer = 0x%08X, gRespTimeOutTimer = 0x%08X", __func__, gLoopTimer, gRespTimeOutTimer);
}
#else
void start_daisy_task(void)
{
}
void kalyke_daisy_init(void){}
void kalyke_daisy_stop(void){}
void daisy_get_info(md_slave_msg_pack *pMsg){}
void daisy_LAN_send_bin(uint8_t *pBuf, uint16_t len){}
#endif
