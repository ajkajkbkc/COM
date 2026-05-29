/**
  ******************************************************************************
  * @file    plc_highspeedins.c
  * @author  lixianyu
  * @version V0.0.1
  * @date    2019-03-19
  * @brief   高速IO相关指令函数
  ******************************************************************************
  */
#include "plc_variable.h"
#include "plc_commonfunc.h"
#include "plc_element.h"
#include "plc_parseaddr.h"
#include "plc_highspeedins.h"
#include "plc_errormsg.h"
#include "plc_instruction.h"
#include "bsp_tim.h"
#include "fsl_debug_console.h"
#include "plc_spd.h"
#include "bsp_gpio.h"
#include "kalyke_opts.h"

hs_hcnt_t gHCNT;
hs_hcnt_t gHCNT2;
hs_dhscs_t gDHSCS;
hs_dhscr_t gDHSCR[MAX_INSTRUCTION_NUM];
hs_dhsz_t gDHSZ[MAX_INSTRUCTION_NUM];
hs_dhst_t gDHST[MAX_INSTRUCTION_NUM];
hs_dhsp_t gDHSP[MAX_INSTRUCTION_NUM];

hs_t gHS_DHSx;


extern unsigned char f_CI_HCNT(plc_run_power_flow_st *ltp_RunEnv);
extern unsigned char f_CI_DHSCS(plc_run_power_flow_st *ltp_RunEnv);
extern unsigned char f_CI_DHSCR(plc_run_power_flow_st *ltp_RunEnv);
extern unsigned char f_CI_DHSZ(plc_run_power_flow_st *ltp_RunEnv);
extern unsigned char f_CI_DHST(plc_run_power_flow_st *ltp_RunEnv);
extern unsigned char f_CI_DHSP(plc_run_power_flow_st *ltp_RunEnv);

static void init_DHSCS(void)
{
    memset(&gDHSCS, 0, sizeof(gDHSCS));
    uint8_t i;
    for (i = 0; i < (MAX_INSTRUCTION_NUM - 1); i++)
    {
        gDHSCS.dhscs[i].next = i + 1;
    }
    gDHSCS.dhscs[i].next = 0;
}
static void init_DHSCR(void)
{
    memset(gDHSCR, 0, sizeof(gDHSCR));
    LOGV("plc_highspeedins", "sizeof(gDHSCR) = %u", sizeof(gDHSCR));
}
static void init_DHSZ(void)
{
    memset(gDHSZ, 0, sizeof(gDHSZ));
}
static void init_DHST(void)
{
    memset(gDHST, 0, sizeof(gDHST));
}
static void init_DHSP(void)
{
}

void init_HCNT(void)
{
    memset(&gHCNT, 0, sizeof(gHCNT));
    memset(&gHCNT2, 0, sizeof(gHCNT2));
    uint8_t i;
    /* 初始化循环链表 */
    for (i = 0; i < (HCNT_MAX_NUMBER - 1); i++)
    {
        gHCNT2.hcnts[i].next = i + 1;
    }
    gHCNT2.hcnts[i].next = 0;
    gHCNT2.tail = 0;
    gHCNT2.head = 0;

    memset(&gHS_DHSx, 0, sizeof(gHS_DHSx));
    for (i = 0; i < MAX_INSTRUCTION_NUM; i++)/* 初始化循环链表 */
    {
        gHS_DHSx.hstate[i].next = i + 1;
    }
    gHS_DHSx.hstate[i].next = 0;
    gHS_DHSx.tail = 0;
    gHS_DHSx.head = 0;

    //init_DHSCS();
    //init_DHSCR();
    //init_DHSZ();
    //init_DHST();
    //init_DHSP();
}


// Kalyke supported Counter
static bool isSupportThisCounter(int counterNum)
{
    switch (counterNum)
    {
        case HCOUNTER236:
        case HCOUNTER237:
        case HCOUNTER238:
        case HCOUNTER239:
        case HCOUNTER242:
        case HCOUNTER243:
        case HCOUNTER244:
        case HCOUNTER245:
        case HCOUNTER246:
        case HCOUNTER247:
        case HCOUNTER249:
        case HCOUNTER251:
        case HCOUNTER252:
        case HCOUNTER254:
            return true;
        default:
            return false;
    }
}

static inline void handleInterruptOpen(uint16_t counterNum)
{
    switch (counterNum)
    {
        case HCOUNTER236:
            bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER237:
            bsp_kalyke_enable_X_interrupt(1, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER238:
            bsp_kalyke_enable_X_interrupt(2, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER239:
            bsp_kalyke_enable_X_interrupt(3, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER242:
            bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(2, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER243:
            bsp_kalyke_enable_X_interrupt(3, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(5, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER244:
            bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(2, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER245:
            bsp_kalyke_enable_X_interrupt(3, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(5, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER246:
            bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(1, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER247:
            bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(1, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(2, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER249:
            bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(1, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(2, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER251:
            //bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);//因为只有B相信号来时才计算是增计数还是减计数
            bsp_kalyke_enable_X_interrupt(1, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER252:
            //bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);//因为只有B相信号来时才计算是增计数还是减计数
            bsp_kalyke_enable_X_interrupt(1, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(2, kGPIO_IntRisingEdge);
            break;
        case HCOUNTER254:
            //bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);//因为只有B相信号来时才计算是增计数还是减计数
            bsp_kalyke_enable_X_interrupt(1, kGPIO_IntRisingEdge);
            bsp_kalyke_enable_X_interrupt(2, kGPIO_IntRisingEdge);
            break;

    }
}

static inline void addCounter(uint16_t counterNum, int32_t cNum, uint8_t *pc)
{
    uint8_t idx = counterNum - HCOUNTER236;
    if (gHCNT.hcnts[idx].counterNum == 0)
    {
        LOGV("addCounter", "Let us add counter %u", counterNum);
        gHCNT.hcnts[idx].compareNum = cNum;
        gHCNT.hcnts[idx].counterNum = counterNum;
        gHCNT.cntNumbers++;
        handleInterruptOpen(counterNum);
    }

    if (gHCNT2.cntNumbers >= (HCNT_MAX_NUMBER - 1))
    {
        return;
    }

    bool exist = false;
    idx = gHCNT2.head;
    while(true)
    {
        if (idx == gHCNT2.tail)
        {
            break;
        }
        if (gHCNT2.hcnts[idx].counterNum == counterNum)
        {
            exist = true;
            break;
        }
        idx = gHCNT2.hcnts[idx].next;
    }

    if (exist == false)//此时idx指向tail
    {
        gHCNT2.hcnts[idx].compareNum = cNum;
        gHCNT2.hcnts[idx].counterNum = counterNum;
        gHCNT2.cntNumbers++;
        gHCNT2.tail = gHCNT2.hcnts[idx].next;
    }
}
static inline void delCounter(uint16_t counterNum)
{
    uint8_t idx = counterNum - HCOUNTER236;
    if (gHCNT.hcnts[idx].counterNum != 0)
    {
        gHCNT.hcnts[idx].counterNum = 0;
        gHCNT.cntNumbers--;
    }

    uint8_t pre;
    idx = gHCNT2.head;
    while (true)
    {
        if (idx == gHCNT2.tail)
        {
            break;
        }
        if (gHCNT2.hcnts[idx].counterNum == counterNum)
        {
            if (idx == gHCNT2.head)
            {
                gHCNT2.head = gHCNT2.hcnts[idx].next;
            }
            else
            {
                gHCNT2.hcnts[pre].next = gHCNT2.hcnts[idx].next;
            }
            LOGV("delCounter", "Let us delete counter %u", counterNum);
            gHCNT2.cntNumbers--;
            break;
        }
        pre = idx;
        idx = gHCNT2.hcnts[idx].next;
    }
}


uint8_t getCurSerialNum(uint8_t idx)
{
    uint8_t sNum = 0;
    switch (gHS_DHSx.hstate[idx].type)
    {
        case DHSx_TYPE_DHSCS:
            sNum = gHS_DHSx.hstate[idx].dhsx.dhscs.sNum;
            break;
            
        case DHSx_TYPE_DHSCR:
            sNum = gHS_DHSx.hstate[idx].dhsx.dhscr.sNum;
            break;
        case DHSx_TYPE_DHSCI:
            sNum = gHS_DHSx.hstate[idx].dhsx.dhsci.sNum;
            break;
        case DHSx_TYPE_DHSZ:
            sNum = gHS_DHSx.hstate[idx].dhsx.dhsz.sNum;
            break;
            
        case DHSx_TYPE_DHST:
            sNum = gHS_DHSx.hstate[idx].dhsx.dhst.sNum;
            break;
            
        case DHSx_TYPE_DHSP:
            sNum = gHS_DHSx.hstate[idx].dhsx.dhsp.sNum;
            break;
    }
    
    return sNum;
}

static bool isDHSxRunning(uint8_t sNum)
{
    uint8_t idx = gHS_DHSx.head;
    while(true)
    {
        if (idx == gHS_DHSx.tail)
        {
            break;
        }
        if (getCurSerialNum(idx) == sNum)
        {
            return true;
        }
        idx = gHS_DHSx.hstate[idx].next;
    }
    return false;
}

static void stopDHSx(uint8_t sNum)
{
    uint8_t pre;
    uint8_t idx = gHS_DHSx.head;
    while (true)
    {
        if (idx == gHS_DHSx.tail)
        {
            break;
        }
        if (getCurSerialNum(idx) == sNum)
        {
            if (idx == gHS_DHSx.head)
            {
                gHS_DHSx.head = gHS_DHSx.hstate[idx].next;
            }
            else
            {
                gHS_DHSx.hstate[pre].next = gHS_DHSx.hstate[idx].next;
            }
            gHS_DHSx.dhsNumbers--;
            break;
        }
        pre = idx;
        idx = gHS_DHSx.hstate[idx].next;
    }
}

static void startDHSx(uint8_t sNum, uint16_t counterNum, uint8_t *pc, DHS_type dhsType)
{
    static const char *TAG = "startDHSx";
    static bool logFlag = true;
    if (logFlag) LOGI(TAG, "Enter %s(), sNum = %u, counterNum = %u, dhsType = %d, dhsNumbers = %u", __func__, sNum, counterNum, dhsType, gHS_DHSx.dhsNumbers);
    if (gHS_DHSx.dhsNumbers > MAX_INSTRUCTION_NUM)
    {
        return;
    }

    bool exist = false;
    uint8_t idx = gHS_DHSx.head;
    while(true)
    {
        if (idx == gHS_DHSx.tail)
        {
            break;
        }
        if (getCurSerialNum(idx) == sNum)
        {
            exist = true;
            break;
        }
        idx = gHS_DHSx.hstate[idx].next;
    }
    if (logFlag) LOGI(TAG, "exist = %u, idx = %u, head = %u, tail = %u", exist, idx, gHS_DHSx.head, gHS_DHSx.tail);
    if (exist == false)//此时idx指向tail
    {
        unsigned long cmpValue = 2020;
        gHS_DHSx.hstate[idx].type = dhsType;
        switch (dhsType)
        {
            case DHSx_TYPE_DHSCS:
                gHS_DHSx.hstate[idx].dhsx.dhscs.sNum = sNum;
                gHS_DHSx.hstate[idx].dhsx.dhscs.counterNum = counterNum;
                gHS_DHSx.hstate[idx].dhsx.dhscs.elemType = *(pc + 15);
                gHS_DHSx.hstate[idx].dhsx.dhscs.address = *((uint16_t *)(pc + 16));
                get_dword(pc + 2, &cmpValue, 0, 1);
                gHS_DHSx.hstate[idx].dhsx.dhscs.compareNum = cmpValue;
                if (logFlag) LOGW(TAG, "elemType = %u, address = %u, compareNum = %d", gHS_DHSx.hstate[idx].dhsx.dhscs.elemType, gHS_DHSx.hstate[idx].dhsx.dhscs.address, cmpValue);
                break;
                
            case DHSx_TYPE_DHSCR:
                gHS_DHSx.hstate[idx].dhsx.dhscr.sNum = sNum;
                gHS_DHSx.hstate[idx].dhsx.dhscr.counterNum = counterNum;
                gHS_DHSx.hstate[idx].dhsx.dhscr.elemType = *(pc + 15);
                gHS_DHSx.hstate[idx].dhsx.dhscr.address = *((uint16_t *)(pc + 16));
                get_dword(pc + 2, &cmpValue, 0, 1);
                gHS_DHSx.hstate[idx].dhsx.dhscr.compareNum = cmpValue;
                if (logFlag) LOGW(TAG, "elemType = %u, address = %u, compareNum = %d", gHS_DHSx.hstate[idx].dhsx.dhscr.elemType, gHS_DHSx.hstate[idx].dhsx.dhscr.address, cmpValue);
                break;
            case DHSx_TYPE_DHSCI:
                {
                gHS_DHSx.hstate[idx].dhsx.dhsci.sNum = sNum;
                gHS_DHSx.hstate[idx].dhsx.dhsci.counterNum = counterNum;
                uint16_t intNumber = 20;
                get_word(pc + 14, &intNumber, 0, 1);
                gHS_DHSx.hstate[idx].dhsx.dhsci.intNum = intNumber;
                get_dword(pc + 2, &cmpValue, 0, 1);
                gHS_DHSx.hstate[idx].dhsx.dhsci.compareNum = cmpValue;
                if (logFlag) LOGW(TAG, "intNum = %u, compareNum = %d", gHS_DHSx.hstate[idx].dhsx.dhsci.intNum, cmpValue);
                }   
                break;
            case DHSx_TYPE_DHSZ:
                gHS_DHSx.hstate[idx].dhsx.dhsz.sNum = sNum;
                gHS_DHSx.hstate[idx].dhsx.dhsz.counterNum = counterNum;
                gHS_DHSx.hstate[idx].dhsx.dhsz.elemType = *(pc + 21);
                gHS_DHSx.hstate[idx].dhsx.dhsz.address = *((uint16_t *)(pc + 22));
                get_dword(pc + 2, &cmpValue, 0, 1);
                gHS_DHSx.hstate[idx].dhsx.dhsz.compareNum1 = cmpValue;
                get_dword(pc + 8, &cmpValue, 0, 1);
                gHS_DHSx.hstate[idx].dhsx.dhsz.compareNum2 = cmpValue;
                if (logFlag) LOGW(TAG, "elemType = %u, address = %u", gHS_DHSx.hstate[idx].dhsx.dhsz.elemType, gHS_DHSx.hstate[idx].dhsx.dhsz.address);
                if (logFlag) LOGI(TAG, "compareNum1 = %d, compareNum2 = %d", gHS_DHSx.hstate[idx].dhsx.dhsz.compareNum1, gHS_DHSx.hstate[idx].dhsx.dhsz.compareNum2);
                break;
                
            case DHSx_TYPE_DHST:
            {
                gHS_DHSx.hstate[idx].dhsx.dhst.sNum = sNum;
                unsigned short recordNumbers = 0;
                get_word(pc + 8, &recordNumbers, 0, 1);
                gHS_DHSx.hstate[idx].dhsx.dhst.recordNum = recordNumbers;
                gHS_DHSx.hstate[idx].dhsx.dhst.counterNum = counterNum;
                gHS_DHSx.hstate[idx].dhsx.dhst.elemType = *(pc + 3);
                gHS_DHSx.hstate[idx].dhsx.dhst.address = *((uint32_t *)(pc + 4));
                gHS_DHSx.hstate[idx].dhsx.dhst.curRecord = 0;
                SET_SD_ELEMENT_VALUE(SD184, gHS_DHSx.hstate[idx].dhsx.dhst.curRecord + 1);
                /* SD182、SD183表示当前要比较的数据 */
                uint32_t *pSDVal = (uint32_t *)&gtv_PlcElement.msp_SDElement[SD182];
                if (gHS_DHSx.hstate[idx].dhsx.dhst.elemType == ADDR_D)
                {
                    uint32_t *pDVal = (uint32_t *)&gtv_PlcElement.msp_DElement[gHS_DHSx.hstate[idx].dhsx.dhst.address];
                    *pSDVal = *pDVal;
                }
                else if (gHS_DHSx.hstate[idx].dhsx.dhst.elemType == ADDR_R)
                {
                    uint32_t *pRVal = (uint32_t *)&gtv_PlcElement.msp_RElement[gHS_DHSx.hstate[idx].dhsx.dhst.address];
                    *pSDVal = *pRVal;
                }
                if (logFlag) LOGD(TAG, "elemType = %u, address = %u, recordNumbers = %d", gHS_DHSx.hstate[idx].dhsx.dhst.elemType, gHS_DHSx.hstate[idx].dhsx.dhst.address, recordNumbers);
            }
            break;
                
            case DHSx_TYPE_DHSP:
            {
                gHS_DHSx.hstate[idx].dhsx.dhsp.sNum = sNum;
                unsigned short recordNumbers = 0;
                get_word(pc + 8, &recordNumbers, 0, 1);
                gHS_DHSx.hstate[idx].dhsx.dhsp.counterNum = counterNum;
                gHS_DHSx.hstate[idx].dhsx.dhsp.elemType = *(pc + 3);
                gHS_DHSx.hstate[idx].dhsx.dhsp.address = *((uint32_t *)(pc + 4));
                gHS_DHSx.hstate[idx].dhsx.dhsp.recordNum = recordNumbers;
                gHS_DHSx.hstate[idx].dhsx.dhsp.curRecord = 0;
                SET_SD_ELEMENT_VALUE(SD184, gHS_DHSx.hstate[idx].dhsx.dhsp.curRecord + 1);
                /* SD182、SD183表示当前要比较的数据 */
                uint32_t *pSDVal = (uint32_t *)&gtv_PlcElement.msp_SDElement[SD182];
                if (gHS_DHSx.hstate[idx].dhsx.dhsp.elemType == ADDR_D)
                {
                    uint32_t *pDVal = (uint32_t *)&gtv_PlcElement.msp_DElement[gHS_DHSx.hstate[idx].dhsx.dhsp.address];
                    *pSDVal = *pDVal;
                }
                else if (gHS_DHSx.hstate[idx].dhsx.dhsp.elemType == ADDR_R)
                {
                    uint32_t *pRVal = (uint32_t *)&gtv_PlcElement.msp_RElement[gHS_DHSx.hstate[idx].dhsx.dhsp.address];
                    *pSDVal = *pRVal;
                }
                if (logFlag) LOGV(TAG, "elemType = %u, address = %u, recordNumbers = %d", gHS_DHSx.hstate[idx].dhsx.dhsp.elemType, gHS_DHSx.hstate[idx].dhsx.dhsp.address, recordNumbers);
            }
            break;
        }
        gHS_DHSx.dhsNumbers++;
        gHS_DHSx.tail = gHS_DHSx.hstate[idx].next;
    }
    else
    {
        if (logFlag) LOGV(TAG, "%u serial number already exist.", sNum);
    }
}

/************************************************************************
function: PLSY脉冲输出指令;
description:对Y端口输出操作;
input   : EXC_ENV结构的变量指针;
output  : no;
************************************************************************/
unsigned char run_ci_plsy_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    unsigned char lcv_Ret;
    unsigned long llv_Freq, llv_PulseNum;
    unsigned char lcv_OutputPoint;

    /*取脉冲输出点*/
    lcv_Ret = get_char(ltp_RunEnv->mcp_PC + 14, &lcv_OutputPoint, 0, 1);
    if(lcv_Ret != pdPASS)
    {
        return lcv_Ret;
    }

    if(lcv_OutputPoint > 3)
    {
        return ERR_OVER_ELEMENT_RANG;
    }

    if(GET_POWER_FLOW(ltp_RunEnv)
            && plc_get_bit_element_value(SM_ELEMENT, (80 + lcv_OutputPoint))
            && !plc_get_bit_element_value(SM_ELEMENT, (84 + lcv_OutputPoint)))
    {
        /*能流有效，且脉冲输出使能，当前未输出*/
        lcv_Ret = get_dword(ltp_RunEnv->mcp_PC + 2, &llv_Freq, 0, 1);
        if(lcv_Ret != pdPASS)
        {
            return lcv_Ret;
        }

        if((llv_Freq < 1) || (llv_Freq > 200000))
        {
            return ERR_OPERANDS;
        }

        /*取脉冲量*/
        lcv_Ret = get_dword(ltp_RunEnv->mcp_PC + 8, &llv_PulseNum, 0, 1);
        if(lcv_Ret != pdPASS)
        {
            return lcv_Ret;
        }

        if((llv_PulseNum < 1) || (llv_PulseNum > 2147483647))
        {
            return ERR_OPERANDS;
        }

        /*设置脉冲输出标志*/
        plc_set_bit_element_value(SM_ELEMENT, (84 + lcv_OutputPoint), 1);
        bsp_start_plsy_channel(lcv_OutputPoint, llv_Freq, llv_PulseNum);
    }
    else
    {
        /*清除脉冲输出标志*/
        plc_set_bit_element_value(SM_ELEMENT, (84 + lcv_OutputPoint), 0);
    }

    return pdPASS;
}

/************************************************************************
function: run_ci_hcnt_ins
description: config the user HCNT instruction;
input   : EXC_ENV结构的变量指针;
output  : HCNT instruction successful flag;
************************************************************************/
/* 
  E8 F0 
  00 14 ED 00 00 00 
  02 FF 0A 00 00 00
 */
unsigned char run_ci_hcnt_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    //LOGD("HS", "Enter %s(), ltp_RunEnv = 0x%08X", __func__, ltp_RunEnv);
#if (KALYKE_HIGH_SPEED_IO == 0)
    return f_CI_HCNT(ltp_RunEnv);
#else
    static const char *TAG = "HCNT";
    static bool logFlag = false;
#if 0
    static uint32_t mTick = 0;
    uint32_t curTick = xTaskGetTickCount();
    if (curTick - mTick > 2000)
    {
        logFlag = true;
        mTick = curTick;
    }
    else
    {
        logFlag = false;
    }
#endif

    //int *pCountNum = (int *)(ltp_RunEnv->mcp_PC + 4);
    int counterNum = *((int *)(ltp_RunEnv->mcp_PC + 4));
    if (logFlag) LOGV(TAG, "counterNum = %u", counterNum);
    if (isSupportThisCounter(counterNum) == false)
    {
        if (logFlag) LOGE(TAG, "ERR_OPERANDS: This product does not support this counter: C%u", counterNum);
        return ERR_OPERANDS;
    }
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (IS_C32_STARTED(counterNum))
        {
            SET_C32_STOP(counterNum);
            delCounter(counterNum);
        }
        return pdPASS;
    }

    if (!IS_C32_STARTED(counterNum))
    {
        unsigned long cmpValue;
        get_dword(ltp_RunEnv->mcp_PC + 8, &cmpValue, 0, 1);
        if (logFlag) LOGW(TAG, "cmpValue = %u", cmpValue);

        if (counterNum == HCOUNTER244 || counterNum == HCOUNTER249 ||
            counterNum == HCOUNTER254 || counterNum == HCOUNTER245)
        {
            if (counterNum == HCOUNTER245)
            {
                if (GPIO_PinReadPadStatus(X7_GPIO, X7_GPIO_PIN))
                {
                    SET_C32_STARTED(counterNum);
                    addCounter(counterNum, cmpValue, ltp_RunEnv->mcp_PC);
                }
            }
            else
            {
                if (GPIO_PinReadPadStatus(X6_GPIO, X6_GPIO_PIN))
                {
                    SET_C32_STARTED(counterNum);
                    addCounter(counterNum, cmpValue, ltp_RunEnv->mcp_PC);
                }
            }
            return pdPASS;
        }
        SET_C32_STARTED(counterNum);
        addCounter(counterNum, cmpValue, ltp_RunEnv->mcp_PC);
    }
    return pdPASS;
#endif
}

/************************************************************************
function: SPD pulse test instruction;
description: test the X000－X003 input pulse;
input   :
output  : no;
************************************************************************/
/* SPD X0 9000 D300
  E7 F0 
  00 00 00 00 
  00 FF 28 23 
  00 11 2C 01
*/
unsigned char run_ci_spd_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    unsigned char *usep = ltp_RunEnv->mcp_PC;
    if(*(usep + 3) != ADDR_X)// Must be X
    {
        return ERR_ELEMENT_TYPE;
    }
    uint8_t xNum = *(usep + 4);
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (gSPD[xNum].started == true)
        {
            gSPD[xNum].started = false;
            switch (xNum)
            {
                case 0:
                    spd_X0_stop();
                    break;
                    
                case 1:
                    spd_X1_stop();
                    break;
                    
                case 2:
                    spd_X2_stop();
                    break;
                    
                case 3:
                    spd_X3_stop();
                    break;
            }
        }
        return pdPASS;
    }

    //LOGI("SPD", "xNum = %u, spd_time_unit = %u", xNum, spd_time_unit);
    uint16_t *pAddress = (uint16_t *)(usep + 12);
    if (gSPD[xNum].started == false)
    {
        unsigned short spd_time_unit = 1000;
        unsigned char ret = get_word(usep + 6, (unsigned short *)&spd_time_unit, 0, 1);
        if(ret != pdPASS)
        {
            return ret;
        }
        gSPD[xNum].intCount = 0;
        gSPD[xNum].elemType = *(usep + 11);
        gSPD[xNum].address = *pAddress;
        gSPD[xNum].started = true;
        gSPD[xNum].timeBegin = xTaskGetTickCount();
        switch(xNum)
        {
            case 0:
                spd_X0_start(spd_time_unit);
                break;
            case 1:
                spd_X1_start(spd_time_unit);
                break;
            case 2:
                spd_X2_start(spd_time_unit);
                break;
            case 3:
                spd_X3_start(spd_time_unit);
                break;
            default:
                return ERR_OPERANDS;
        }
    }
    else
    {
        SET_D_ELEMENT_VALUE(*pAddress + 1, gSPD[xNum].intCount);
        SET_D_ELEMENT_VALUE(*pAddress + 2, xTaskGetTickCount() - gSPD[xNum].timeBegin);
    }
    return pdPASS;
}


/************************************************************************
function: DHSCS+high_counters instruction;  DHSCS
description: fit the high_counters's number to set user's element;
input   :  DHSCS (compare number)DINT+2  (counter)DINT+8 (set element)BOOL+14  TOTAL+18
output  : no;
************************************************************************/
    /* DF F0 
       02 FF 64 00 00 00 
       00 14 EC 00 00 00 
       00 01 00 00 
       00 00 -> cmdno
    */
unsigned char run_ci_dhscs_ins(plc_run_power_flow_st *ltp_RunEnv)
{
#if (KALYKE_HIGH_SPEED_IO == 0)
    return f_CI_DHSCS(ltp_RunEnv);
#else
    static const char *TAG = "DHSCS";
    static bool logFlag = false;
    #if 0
    static uint32_t mTick = 0;
    uint32_t curTick = xTaskGetTickCount();
    if (curTick - mTick > 2000)
    {
        logFlag = true;
        mTick = curTick;
    }
    else
    {
        logFlag = false;
    }
    #endif

    int counterNum = *((int *)(ltp_RunEnv->mcp_PC + 10));
    if (logFlag) LOGV(TAG, "counterNum = %u", counterNum);
    if (isSupportThisCounter(counterNum) == false)
    {
        if (logFlag) LOGE(TAG, "ERR_OPERANDS: This product does not support this counter: C%u", counterNum);
        return ERR_OPERANDS;
    }
    uint8_t sNum = *(ltp_RunEnv->mcp_PC + 18);
    if (logFlag) LOGV(TAG, "sNum = %u", sNum);
    if (sNum >= MAX_INSTRUCTION_NUM)
    {
        //return ERR_OVER_SERIAL_NUM;
    }
    #if 0
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (gDHSCS.dhscs[sNum].started)
        {
            gDHSCS.dhscs[sNum].started = false;
        }
        return pdPASS;
    }

    if (gDHSCS.dhscs[sNum].started == false)
    {
        if (logFlag) LOGD(TAG, "Let us start DHSCS");
        gDHSCS.dhscs[sNum].started = true;
        gDHSCS.dhscs[sNum].counterNum = counterNum;
        gDHSCS.dhscs[sNum].elemType = *(ltp_RunEnv->mcp_PC + 15);
        gDHSCS.dhscs[sNum].address = *((uint16_t *)(ltp_RunEnv->mcp_PC + 16));
        unsigned long cmpValue;
        get_dword(ltp_RunEnv->mcp_PC + 2, &cmpValue, 0, 1);
        gDHSCS.dhscs[sNum].compareNum = cmpValue;
        if (logFlag) LOGW(TAG, "elemType = %u, address = %u, compareNum = %d", gDHSCS.dhscs[sNum].elemType, gDHSCS.dhscs[sNum].address, cmpValue);
    }
    #else
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (isDHSxRunning(sNum))
        {
            stopDHSx(sNum);
        }
        return pdPASS;
    }

    if (isDHSxRunning(sNum) == false)
    {
        LOGD(TAG, "Let us start DHSCS, sNum = %u", sNum);
        startDHSx(sNum, counterNum, ltp_RunEnv->mcp_PC, DHSx_TYPE_DHSCS);
    }
    #endif
    return pdPASS;
#endif
}

/************************************************************************
function: DHSCR+high_counters instruction;
description: fit the high_counters's number to set user's element;
input   :
output  : no;
************************************************************************/
    /*
      E0 F0
      02 FF 64 00 00 00
      00 14 EC 00 00 00
      00 01 01 00
      01 00
     */
     /*
     E0 F0 
     02 FF BE 00 00 00 
     00 14 EC 00 00 00 
     00 02 EC 00 
     01 00 
     */
unsigned char run_ci_dhscr_ins(plc_run_power_flow_st *ltp_RunEnv)
{
#if (KALYKE_HIGH_SPEED_IO == 0)
    return f_CI_DHSCR(ltp_RunEnv);
#else
    static const char *TAG = "DHSCR";
    static bool logFlag = false;
#if 0
    static uint32_t mTick = 0;
    uint32_t curTick = xTaskGetTickCount();
    if (curTick - mTick > 2000)
    {
        logFlag = true;
        mTick = curTick;
    }
    else
    {
        logFlag = false;
    }
#endif

    int counterNum = *((int *)(ltp_RunEnv->mcp_PC + 10));
    if (logFlag) LOGV(TAG, "counterNum = %u", counterNum);
    if (isSupportThisCounter(counterNum) == false)
    {
        if (logFlag) LOGE(TAG, "ERR_OPERANDS: This product does not support this counter: C%u", counterNum);
        return ERR_OPERANDS;
    }
    uint8_t sNum = *(ltp_RunEnv->mcp_PC + 18);
    if (logFlag) LOGV(TAG, "sNum = %u", sNum);
    if (sNum >= MAX_INSTRUCTION_NUM)
    {
        //return ERR_OVER_SERIAL_NUM;
    }
#if 0
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (gDHSCR[sNum].started)
        {
            gDHSCR[sNum].started = false;
        }
        return pdPASS;
    }

    if (gDHSCR[sNum].started == false)
    {
        if (logFlag) LOGD(TAG, "Let us start DHSCR");
        gDHSCR[sNum].started = true;
        gDHSCR[sNum].counterNum = counterNum;
        gDHSCR[sNum].elemType = *(ltp_RunEnv->mcp_PC + 15);
        gDHSCR[sNum].address = *((uint16_t *)(ltp_RunEnv->mcp_PC + 16));
        unsigned long cmpValue;
        get_dword(ltp_RunEnv->mcp_PC + 2, &cmpValue, 0, 1);
        gDHSCR[sNum].compareNum = cmpValue;
        if (logFlag) LOGW(TAG, "elemType = %u, address = %u, compareNum = %d", gDHSCR[sNum].elemType, gDHSCR[sNum].address, cmpValue);
    }
#else
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (isDHSxRunning(sNum))
        {
            stopDHSx(sNum);
        }
        return pdPASS;
    }

    if (isDHSxRunning(sNum) == false)
    {
        LOGD(TAG, "Let us start DHSCR, sNum = %u", sNum);
        startDHSx(sNum, counterNum, ltp_RunEnv->mcp_PC, DHSx_TYPE_DHSCR);
    }
#endif
    return pdPASS;
#endif
}

/************************************************************************
function: DHSCS+high_counters instruction; DHSCI
description: fit the high_counters's number to set user's element;
input   :  DHSCS (compare number)DINT+2  (counter)DINT+8 (set element)BOOL+14  TOTAL+18
output  : no;
************************************************************************/
/*
 DHSCI 200 C236 20
 EB F0 
 02 FF C8 00 00 00 
 00 14 EC 00 00 00 
 00 FF 14 00 
 00 00 

 DHSCI 300 C237 21
 EB F0 
 02 FF 2C 01 00 00 
 00 14 ED 00 00 00 
 00 FF 15 00 
 01 00 
 */
unsigned char run_ci_dhsci_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    static const char *TAG = "DHSCI";
    static bool logFlag = false;
#if 0
    static uint32_t mTick = 0;
    uint32_t curTick = xTaskGetTickCount();
    if (curTick - mTick > 2000)
    {
        logFlag = true;
        mTick = curTick;
    }
    else
    {
        logFlag = false;
    }
#endif
    int counterNum = *((int *)(ltp_RunEnv->mcp_PC + 10));
    if (logFlag) LOGV(TAG, "counterNum = %u", counterNum);
    if (isSupportThisCounter(counterNum) == false)
    {
        if (logFlag) LOGE(TAG, "ERR_OPERANDS: This product does not support this counter: C%u", counterNum);
        return ERR_OPERANDS;
    }
    uint8_t sNum = *(ltp_RunEnv->mcp_PC + 18);
    if (logFlag) LOGV(TAG, "sNum = %u", sNum);
    if (sNum >= MAX_INSTRUCTION_NUM)
    {
        //return ERR_OVER_SERIAL_NUM;
    }

    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (isDHSxRunning(sNum))
        {
            stopDHSx(sNum);
        }
        return pdPASS;
    }

    if (isDHSxRunning(sNum) == false)
    {
        LOGD(TAG, "Let us start DHSCI, sNum = %u", sNum);
        startDHSx(sNum, counterNum, ltp_RunEnv->mcp_PC, DHSx_TYPE_DHSCI);
    }
    return pdPASS;
}

/************************************************************************
function: DHSZ+high_counters instruction;
description: fit the high_counters's number to set user's element;
input   :
output  : no;
************************************************************************/
    /*
     E1 F0 
     02 FF 64 00 00 00 
     02 FF C8 00 00 00 
     00 14 EC 00 00 00 
     00 01 04 00 
     00 00 
     */
unsigned char run_ci_dhsz_ins(plc_run_power_flow_st *ltp_RunEnv)
{
#if (KALYKE_HIGH_SPEED_IO == 0)
    return f_CI_DHSZ(ltp_RunEnv);
#else
    static const char *TAG = "DHSZ";
    static bool logFlag = false;
#if 0
    static uint32_t mTick = 0;
    uint32_t curTick = xTaskGetTickCount();
    if (curTick - mTick > 2000)
    {
        logFlag = true;
        mTick = curTick;
    }
    else
    {
        logFlag = false;
    }
#endif

    int counterNum = *((int *)(ltp_RunEnv->mcp_PC + 16));
    if (logFlag) LOGV(TAG, "counterNum = %u", counterNum);
    if (isSupportThisCounter(counterNum) == false)
    {
        if (logFlag) LOGE(TAG, "ERR_OPERANDS: This product does not support this counter: C%u", counterNum);
        return ERR_OPERANDS;
    }
    uint8_t sNum = *(ltp_RunEnv->mcp_PC + 24);
    if (logFlag) LOGV(TAG, "sNum = %u", sNum);
    if (sNum >= MAX_INSTRUCTION_NUM)
    {
        //return ERR_OVER_SERIAL_NUM;
    }
#if 0
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (gDHSZ[sNum].started)
        {
            gDHSZ[sNum].started = false;
        }
        return pdPASS;
    }

    if (gDHSZ[sNum].started == false)
    {
        if (logFlag) LOGD(TAG, "Let us start DHSZ");
        gDHSZ[sNum].started = true;
        gDHSZ[sNum].counterNum = counterNum;
        gDHSZ[sNum].elemType = *(ltp_RunEnv->mcp_PC + 21);
        gDHSZ[sNum].address = *((uint16_t *)(ltp_RunEnv->mcp_PC + 22));
        unsigned long cmpValue;
        get_dword(ltp_RunEnv->mcp_PC + 2, &cmpValue, 0, 1);
        gDHSZ[sNum].compareNum1 = cmpValue;
        get_dword(ltp_RunEnv->mcp_PC + 8, &cmpValue, 0, 1);
        gDHSZ[sNum].compareNum2 = cmpValue;
        if (logFlag) LOGW(TAG, "elemType = %u, address = %u, compareNum = %d", gDHSZ[sNum].elemType, gDHSZ[sNum].address, cmpValue);
    }
#else
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (isDHSxRunning(sNum))
        {
            stopDHSx(sNum);
        }
        return pdPASS;
    }

    if (isDHSxRunning(sNum) == false)
    {
        LOGD(TAG, "Let us start DHSZ, sNum = %u", sNum);
        startDHSx(sNum, counterNum, ltp_RunEnv->mcp_PC, DHSx_TYPE_DHSZ);
    }
#endif
    return pdPASS;
#endif
}


/************************************************************************
function: DHST+高速计数器的指令;
description:高速计数器的表格比较指令对Y端口输出操作;
input   : EXC_ENV结构的变量指针;
output  : no;
************************************************************************/
    /* DHST      D100 2 C236
     E2 F0 
     00 11 64 00 00 00 
     00 FF 02 00 
     00 14 EC 00 00 00 
     00 00 -> cmdno
     */
unsigned char run_ci_dhst_ins(plc_run_power_flow_st *ltp_RunEnv)
{
#if (KALYKE_HIGH_SPEED_IO == 0)
    return f_CI_DHST(ltp_RunEnv);
#else
    static const char *TAG = "DHST";
    static bool logFlag = false;
#if 0
    static uint32_t mTick = 0;
    uint32_t curTick = xTaskGetTickCount();
    if (curTick - mTick > 2000)
    {
        logFlag = true;
        mTick = curTick;
    }
    else
    {
        logFlag = false;
    }
#endif

    int counterNum = *((int *)(ltp_RunEnv->mcp_PC + 14));
    if (logFlag) LOGV(TAG, "counterNum = %u", counterNum);
    if (isSupportThisCounter(counterNum) == false)
    {
        if (logFlag) LOGE(TAG, "ERR_OPERANDS: This product does not support this counter: C%u", counterNum);
        return ERR_OPERANDS;
    }
    uint8_t sNum = *(ltp_RunEnv->mcp_PC + 18);
    if (logFlag) LOGV(TAG, "sNum = %u", sNum);
    if (sNum >= MAX_INSTRUCTION_NUM)
    {
        //return ERR_OVER_SERIAL_NUM;
    }
#if 0
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (gDHST[sNum].started)
        {
            gDHST[sNum].started = false;
        }
        return pdPASS;
    }
    if (gDHST[sNum].started == false)
    {
        if (logFlag) LOGD(TAG, "Let us start DHST");
        unsigned short recordNumbers = 0;
        get_word(ltp_RunEnv->mcp_PC + 8, &recordNumbers, 0, 1);
        if(recordNumbers == 0 || recordNumbers > 128)//判断记录数大于0并且小于129
        {
            return ERR_OPERANDS;
        }
        gDHST[sNum].started = true;
        gDHST[sNum].counterNum = counterNum;
        gDHST[sNum].elemType = *(ltp_RunEnv->mcp_PC + 3);
        gDHST[sNum].address = *((uint32_t *)(ltp_RunEnv->mcp_PC + 4));
        gDHST[sNum].recordNum = recordNumbers;
        gDHST[sNum].curRecord = 0;
        SET_SD_ELEMENT_VALUE(SD184, gDHST[sNum].curRecord + 1);
        /* SD182、SD183表示当前要比较的数据 */
        uint32_t *pSDVal = (uint32_t *)&gtv_PlcElement.msp_SDElement[SD182];
        if (gDHST[sNum].elemType == ADDR_D)
        {
            uint32_t *pDVal = (uint32_t *)&gtv_PlcElement.msp_DElement[gDHST[sNum].address];
            *pSDVal = *pDVal;
        }
        else if (gDHST[sNum].elemType == ADDR_R)
        {
            uint32_t *pRVal = (uint32_t *)&gtv_PlcElement.msp_RElement[gDHST[sNum].address];
            *pSDVal = *pRVal;
        }
        if (logFlag) LOGW(TAG, "elemType = %u, address = %u, recordNumbers = %d", gDHST[sNum].elemType, gDHST[sNum].address, recordNumbers);
    }
#else
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (isDHSxRunning(sNum))
        {
            stopDHSx(sNum);
        }
        return pdPASS;
    }

    if (isDHSxRunning(sNum) == false)
    {
        LOGD(TAG, "Let us start DHST, sNum = %u", sNum);
        startDHSx(sNum, counterNum, ltp_RunEnv->mcp_PC, DHSx_TYPE_DHST);
    }
#endif
    return pdPASS;
#endif
}


/************************************************************************
function: DHSP+high_counters instruction;
description: fit the high_counters's number to set user's element;
input   :
output  : no;
************************************************************************/
    /*
     E3 F0 
     00 11 C8 00 00 00 
     00 FF 03 00 
     00 14 EC 00 00 00 
     00 00
     */
unsigned char run_ci_dhsp_ins(plc_run_power_flow_st *ltp_RunEnv)
{
#if (KALYKE_HIGH_SPEED_IO == 0)
    return f_CI_DHSP(ltp_RunEnv);
#else
    static const char *TAG = "DHSP";
    static bool logFlag = false;
#if 0
    static uint32_t mTick = 0;
    uint32_t curTick = xTaskGetTickCount();
    if (curTick - mTick > 2000)
    {
        logFlag = true;
        mTick = curTick;
    }
    else
    {
        logFlag = false;
    }
#endif

    int counterNum = *((int *)(ltp_RunEnv->mcp_PC + 14));
    if (logFlag) LOGV(TAG, "counterNum = %u", counterNum);
    if (isSupportThisCounter(counterNum) == false)
    {
        if (logFlag) LOGE(TAG, "ERR_OPERANDS: This product does not support this counter: C%u", counterNum);
        return ERR_OPERANDS;
    }
    uint8_t sNum = *(ltp_RunEnv->mcp_PC + 18);
    if (logFlag) LOGV(TAG, "sNum = %u", sNum);
    if (sNum >= MAX_INSTRUCTION_NUM)
    {
        //return ERR_OVER_SERIAL_NUM;
    }
#if 0
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (gDHSP[sNum].started)
        {
            gDHSP[sNum].started = false;
        }
        return pdPASS;
    }
    if (gDHSP[sNum].started == false)
    {
        if (logFlag) LOGD(TAG, "Let us start DHSP");
        unsigned short recordNumbers = 0;
        get_word(ltp_RunEnv->mcp_PC + 8, &recordNumbers, 0, 1);
        if(recordNumbers == 0 || recordNumbers > 128)//判断记录数大于0并且小于129
        {
            return ERR_OPERANDS;
        }
        gDHSP[sNum].started = true;
        gDHSP[sNum].counterNum = counterNum;
        gDHSP[sNum].elemType = *(ltp_RunEnv->mcp_PC + 3);
        gDHSP[sNum].address = *((uint32_t *)(ltp_RunEnv->mcp_PC + 4));
        gDHSP[sNum].recordNum = recordNumbers;
        gDHSP[sNum].curRecord = 0;
        SET_SD_ELEMENT_VALUE(SD184, gDHSP[sNum].curRecord + 1);
        /* SD182、SD183表示当前要比较的数据 */
        uint32_t *pSDVal = (uint32_t *)&gtv_PlcElement.msp_SDElement[SD182];
        if (gDHSP[sNum].elemType == ADDR_D)
        {
            uint32_t *pDVal = (uint32_t *)&gtv_PlcElement.msp_DElement[gDHSP[sNum].address];
            *pSDVal = *pDVal;
        }
        else if (gDHSP[sNum].elemType == ADDR_R)
        {
            uint32_t *pRVal = (uint32_t *)&gtv_PlcElement.msp_RElement[gDHSP[sNum].address];
            *pSDVal = *pRVal;
        }
        if (logFlag) LOGW(TAG, "elemType = %u, address = %u, recordNumbers = %d", gDHSP[sNum].elemType, gDHSP[sNum].address, recordNumbers);
    }
#else
    if(!GET_POWER_FLOW(ltp_RunEnv))
    {
        if (isDHSxRunning(sNum))
        {
            stopDHSx(sNum);
        }
        return pdPASS;
    }

    if (isDHSxRunning(sNum) == false)
    {
        LOGD(TAG, "Let us start DHSP, sNum = %u", sNum);
        startDHSx(sNum, counterNum, ltp_RunEnv->mcp_PC, DHSx_TYPE_DHSP);
    }
#endif
    return pdPASS;
#endif

}


/************************************************************************
function: DHSCS+high_counters instruction;
description: fit the high_counters's number to set user's element;
input   :  DHSPI (compare number)DINT+2  (sd)DINT+8 (set element)BOOL+14  TOTAL+18
output  : no;
************************************************************************/

unsigned char run_ci_dhspi_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}


/*************************************************************************************************************************
function: run_ci_dszr_ins
description: DSZR DOG +2, ZERO +6,Y PULSE +10, Y ASPECT +14
(回归速度)DINT+2  (爬行速度)DINT+8 (近点信号)BOOL+14   （脉冲输出地址）BOOL+18   TOTAL+22
input   : EXC_ENV结构的变量指针;
output  : no;
*************************************************************************************************************************/
unsigned char run_ci_dszr_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}


/*************************************************************************************************************************
function: f_CI_ABS
description:ABS (输入信号)BOOL+2  (控制信号)BOOL+6 (ABS数据)DINT+10   TOTAL+16
input   : EXC_ENV结构的变量指针;
output  : no;
*************************************************************************************************************************/
unsigned char run_ci_abs_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}


/*************************************************************************************************************************
function: f_CI_DRVA
description:  DRVA (位置)DINT+2  (频率)DINT+8 (Y0/Y1)BOOL+14  (方向Y)BOOL+18 TOTAL+22
input   : EXC_ENV结构的变量指针;
output  : no;
*************************************************************************************************************************/
unsigned char run_ci_drva_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}



/*************************************************************************************************************************
function: run_ci_zrn_ins
description: ZRN (回归速度)DINT+2  (爬行速度)DINT+8 (近点信号)BOOL+14   （脉冲输出地址）BOOL+18   TOTAL+22
input   : ltp_RunEnv结构的变量指针;
output  : no;
*************************************************************************************************************************/
unsigned char run_ci_zrn_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}


unsigned char run_ci_drvc_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}


/*************************************************************************************************************************
function: f_CI_DRVI
description:DRVI (脉冲数)DINT+2  (频率)DINT+8 (Y0/Y1)BOOL+14  (方向Y)BOOL+18 TOTAL+22
input   : EXC_ENV结构的变量指针;2145 2970
output  : no;
*************************************************************************************************************************/
unsigned char run_ci_drvi_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}


/*****************************************************************************************************************
function: f_CI_PLSV
description: PLSV (频率)DINT+2  (Y0/Y1)BOOL+8  (方向Y)BOOL+12 TOTAL+16
input   : EXC_ENV结构的变量指针;
output  : no;
*****************************************************************************************************************/
unsigned char run_ci_plsv_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}

/*************************************************************************************************************************
function: f_CI_PLS
description:PLS (起始D元件)word+2  (输出段数)word+6 (输出端口)BOOL+10 TOTAL+14
input   : EXC_ENV结构的变量指针;
output  : no;
*************************************************************************************************************************/
unsigned char run_ci_pls_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}

/*************************************************************************************************************************
function: f_CI_DVIT
description:DVIT (中断后脉冲数)DINT+2 (频率)DINT+8 （Y0/Y1）BOOL+14  (方向Y)BOOL+18 TOTAL+22
input   : EXC_ENV结构的变量指针;2145 2970
output  : no;
*************************************************************************************************************************/
unsigned char run_ci_dvit_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}


unsigned char run_ci_stopdv_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}

/************************************************************************
function: PLSR加减脉冲输出指令;
description:对Y端口输出操作;
input   : EXC_ENV结构的变量指针;
output  : no;
************************************************************************/
unsigned char run_ci_plsr_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}

/************************************************************************
function: PLSB带基底速度加减脉冲输出指令;
description:对Y端口输出操作; PLSB 基底速度＋2 最高速度＋6 总脉冲数＋10 加减速时间＋16 输出端口 ＋20 total＋24
input   : EXC_ENV结构的变量指针;
output  : no;
************************************************************************/
unsigned char run_ci_plsb_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}


/************************************************************************
function: PWM脉冲输出指令;
description:对Y端口输出操作;
input   : EXC_ENV结构的变量指针;
output  : no;
************************************************************************/
unsigned char run_ci_pwm_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}

unsigned char run_ci_camtable_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}

// 电子齿轮指令增加方向信号
//（通过SD490~SD497配置，SD490~SD497系统初始值为：0xFF,有效值为0x0~0xf,分别对应Y0~Y17）


unsigned char run_ci_cambox_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}



unsigned char run_ci_gearbox_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}



unsigned char run_ci_movelink_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}



unsigned char run_ci_lin_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}



unsigned char run_ci_cw_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}



unsigned char run_ci_ccw_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}



unsigned char run_ci_drv_ins(plc_run_power_flow_st *ltp_RunEnv)
{
    return pdPASS;
}

