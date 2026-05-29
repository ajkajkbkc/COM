/**
  ******************************************************************************
  * @file    plc_spd.c
  * @author  lixianyu
  * @version V0.0.1
  * @date    2020-01-11
  * @brief   
  ******************************************************************************
  */
#include "fsl_debug_console.h"
#include "plc_spd.h"
#include "plc_variable.h"
#include "plc_parseaddr.h"

#include "fsl_gpt.h"
#include "fsl_qtmr.h"
#include "bsp_gpio.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/


/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Variables
 ******************************************************************************/
static const char *TAG = "SPD";
hs_spd_t gSPD[SPD_X_NUMBER];


/*******************************************************************************
 * Code
 ******************************************************************************/
static inline void handleIrq(uint8_t xNum)
{
    switch (gSPD[xNum].elemType)
    {
        case ADDR_D:
            //LOGW(TAG, "gSPD[0].intCount = %u", gSPD[0].intCount);
            SET_D_ELEMENT_VALUE(gSPD[xNum].address, gSPD[xNum].intCount);
            break;
    }
    gSPD[xNum].intCount = 0;
    gSPD[xNum].timeBegin = xTaskGetTickCount();
}

/********************************* X0 ******************************/
void GPT1_IRQHandler(void)
{
    /* Clear interrupt flag.*/
    GPT_ClearStatusFlags(GPT1, kGPT_OutputCompare1Flag);

    handleIrq(0);
    
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

void spd_X0_start(uint16_t ms)
{
    LOGV(TAG, "Enter %s()", __func__);
    gpt_config_t gptConfig;
    GPT_GetDefaultConfig(&gptConfig);
    /* Initialize GPT module */
    GPT_Init(GPT1, &gptConfig);
    //CLOCK_GetFreq(kCLOCK_PerClk); //375000 Hz
    /* 因为GPT的时钟频率为375000Hz，所以1ms的counter值为375 */
    GPT_SetOutputCompareValue(GPT1, kGPT_OutputCompare_Channel1, 375U * ms);
    /* Enable GPT Output Compare1 interrupt */
    GPT_EnableInterrupts(GPT1, kGPT_OutputCompare1InterruptEnable);
    /* Enable at the Interrupt */
    EnableIRQ(GPT1_IRQn);

    GPT_StartTimer(GPT1);
    bsp_kalyke_enable_X_interrupt(0, kGPIO_IntRisingEdge);
    LOGV(TAG, "Leave %s()", __func__);
}
void spd_X0_stop(void)
{
    LOGV(TAG, "Enter %s()", __func__);
    if (gSPD[0].started == false)
    {
        return;
    }
    bsp_kalyke_disable_X_interrupt(0);
    GPT_DisableInterrupts(GPT1, kGPT_OutputCompare1InterruptEnable);
    //GPT_StopTimer(GPT1);
    GPT_Deinit(GPT1);
    DisableIRQ(GPT1_IRQn);
    
    LOGV(TAG, "Leave %s()", __func__);
}

/********************************* X1 ******************************/
void GPT2_IRQHandler(void)
{
    /* Clear interrupt flag.*/
    GPT_ClearStatusFlags(GPT2, kGPT_OutputCompare1Flag);

    handleIrq(1);
    
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

void spd_X1_start(uint16_t ms)
{
    LOGV(TAG, "Enter %s()", __func__);
    gpt_config_t gptConfig;
    GPT_GetDefaultConfig(&gptConfig);
    /* Initialize GPT module */
    GPT_Init(GPT2, &gptConfig);
    //CLOCK_GetFreq(kCLOCK_PerClk); //375000 Hz
    /* 因为GPT的时钟频率为375000Hz，所以1ms的counter值为375 */
    GPT_SetOutputCompareValue(GPT2, kGPT_OutputCompare_Channel1, 375U * ms);
    /* Enable GPT Output Compare1 interrupt */
    GPT_EnableInterrupts(GPT2, kGPT_OutputCompare1InterruptEnable);
    /* Enable at the Interrupt */
    EnableIRQ(GPT2_IRQn);

    GPT_StartTimer(GPT2);
    bsp_kalyke_enable_X_interrupt(1, kGPIO_IntRisingEdge);
    LOGV(TAG, "Leave %s()", __func__);
}
void spd_X1_stop(void)
{
    LOGV(TAG, "Enter %s()", __func__);
    if (gSPD[1].started == false)
    {
        return;
    }
    bsp_kalyke_disable_X_interrupt(1);
    GPT_DisableInterrupts(GPT2, kGPT_OutputCompare1InterruptEnable);
    //GPT_StopTimer(GPT2);
    GPT_Deinit(GPT2);
    DisableIRQ(GPT2_IRQn);
    LOGV(TAG, "Leave %s()", __func__);
}

/********************************* X2 ******************************/
#define X2_QTMR_BASEADDR     TMR3
/* Get source clock for QTMR driver */
#define QTMR_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_IpgClk) //150000000 Hz

void TMR3_IRQHandler(void)
{
    /* Clear interrupt flag.*/
    QTMR_ClearStatusFlags(X2_QTMR_BASEADDR, kQTMR_Channel_1, kQTMR_CompareFlag);

    handleIrq(2);

/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

void spd_X2_start(uint16_t ms)
{
    LOGV(TAG, "Enter %s()", __func__);
    qtmr_config_t qtmrConfig;
    QTMR_GetDefaultConfig(&qtmrConfig);
    /* Init the first channel to use the IP Bus clock div by 128 */
    qtmrConfig.primarySource = kQTMR_ClockDivide_128; // 150000000 / 128 = 1171875 Hz
    QTMR_Init(X2_QTMR_BASEADDR, kQTMR_Channel_0, &qtmrConfig);

    /* Init the second channel to use output of the first channel as we are chaining the first channel and the second
     * channel */
    qtmrConfig.primarySource = kQTMR_ClockCounter0Output;
    QTMR_Init(X2_QTMR_BASEADDR, kQTMR_Channel_1, &qtmrConfig);

    /* Set the first channel period to be 1 millisecond */
    uint16_t ticks = MSEC_TO_COUNT(1U, (QTMR_SOURCE_CLOCK / 128));
    LOGW(TAG, "ticks of 1ms = %u", ticks);
    QTMR_SetTimerPeriod(X2_QTMR_BASEADDR, kQTMR_Channel_0, ticks);

    /* Set the second channel count which increases every millisecond, set compare event for ms millisecond. */
    QTMR_SetTimerPeriod(X2_QTMR_BASEADDR, kQTMR_Channel_1, ms);

    /* Enable at the NVIC */
    EnableIRQ(TMR3_IRQn);

    /* Enable the second channel compare interrupt */
    QTMR_EnableInterrupts(X2_QTMR_BASEADDR, kQTMR_Channel_1, kQTMR_CompareInterruptEnable);

    /* Start the second channel in cascase mode, chained to the first channel as set earlier via the primary source
     * selection */
    QTMR_StartTimer(X2_QTMR_BASEADDR, kQTMR_Channel_1, kQTMR_CascadeCount);

    /* Start the first channel to count on rising edge of the primary source clock */
    LOGV(TAG, "Before QTMR_StartTimer(X2_QTMR_BASEADDR, kQTMR_Channel_0, kQTMR_PriSrcRiseEdge)");
    QTMR_StartTimer(X2_QTMR_BASEADDR, kQTMR_Channel_0, kQTMR_PriSrcRiseEdge);

    bsp_kalyke_enable_X_interrupt(2, kGPIO_IntRisingEdge);
    LOGV(TAG, "Leave %s()", __func__);
}
void spd_X2_stop(void)
{
    LOGV(TAG, "Enter %s()", __func__);
    if (gSPD[2].started == false)
    {
        return;
    }
    bsp_kalyke_disable_X_interrupt(2);
    //QTMR_StopTimer(X2_QTMR_BASEADDR, kQTMR_Channel_0);
    //QTMR_StopTimer(X2_QTMR_BASEADDR, kQTMR_Channel_1);
    QTMR_Deinit(X2_QTMR_BASEADDR, kQTMR_Channel_0);
    QTMR_Deinit(X2_QTMR_BASEADDR, kQTMR_Channel_1);
    DisableIRQ(TMR3_IRQn);
    
    LOGV(TAG, "Leave %s()", __func__);
}

/********************************* X3 ******************************/
#define X3_QTMR_BASEADDR     TMR4

void TMR4_IRQHandler(void)
{
    /* Clear interrupt flag.*/
    QTMR_ClearStatusFlags(X3_QTMR_BASEADDR, kQTMR_Channel_1, kQTMR_CompareFlag);

    handleIrq(3);

/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

void spd_X3_start(uint16_t ms)
{
    LOGV(TAG, "Enter %s()", __func__);
    qtmr_config_t qtmrConfig;
    QTMR_GetDefaultConfig(&qtmrConfig);
    /* Init the first channel to use the IP Bus clock div by 128 */
    qtmrConfig.primarySource = kQTMR_ClockDivide_128; // 150000000 / 128 = 1171875 Hz
    QTMR_Init(X3_QTMR_BASEADDR, kQTMR_Channel_0, &qtmrConfig);

    /* Init the second channel to use output of the first channel as we are chaining the first channel and the second
     * channel */
    qtmrConfig.primarySource = kQTMR_ClockCounter0Output;
    QTMR_Init(X3_QTMR_BASEADDR, kQTMR_Channel_1, &qtmrConfig);

    /* Set the first channel period to be 1 millisecond */
    uint16_t ticks = MSEC_TO_COUNT(1U, (QTMR_SOURCE_CLOCK / 128));
    LOGW(TAG, "ticks of 1ms = %u", ticks);
    QTMR_SetTimerPeriod(X3_QTMR_BASEADDR, kQTMR_Channel_0, ticks);

    /* Set the second channel count which increases every millisecond, set compare event for ms millisecond. */
    QTMR_SetTimerPeriod(X3_QTMR_BASEADDR, kQTMR_Channel_1, ms);

    /* Enable at the NVIC */
    EnableIRQ(TMR4_IRQn);

    /* Enable the second channel compare interrupt */
    QTMR_EnableInterrupts(X3_QTMR_BASEADDR, kQTMR_Channel_1, kQTMR_CompareInterruptEnable);

    /* Start the second channel in cascase mode, chained to the first channel as set earlier via the primary source
     * selection */
    QTMR_StartTimer(X3_QTMR_BASEADDR, kQTMR_Channel_1, kQTMR_CascadeCount);

    /* Start the first channel to count on rising edge of the primary source clock */
    LOGV(TAG, "Before QTMR_StartTimer(X3_QTMR_BASEADDR, kQTMR_Channel_0, kQTMR_PriSrcRiseEdge)");
    QTMR_StartTimer(X3_QTMR_BASEADDR, kQTMR_Channel_0, kQTMR_PriSrcRiseEdge);

    bsp_kalyke_enable_X_interrupt(3, kGPIO_IntRisingEdge);
    LOGV(TAG, "Leave %s()", __func__);
}

void spd_X3_stop(void)
{
    LOGV(TAG, "Enter %s()", __func__);
    if (gSPD[3].started == false)
    {
        return;
    }
    bsp_kalyke_disable_X_interrupt(3);
    //QTMR_StopTimer(X2_QTMR_BASEADDR, kQTMR_Channel_0);
    //QTMR_StopTimer(X2_QTMR_BASEADDR, kQTMR_Channel_1);
    QTMR_Deinit(X3_QTMR_BASEADDR, kQTMR_Channel_0);
    QTMR_Deinit(X3_QTMR_BASEADDR, kQTMR_Channel_1);
    DisableIRQ(TMR4_IRQn);
    
    LOGV(TAG, "Leave %s()", __func__);
}

void spd_init(void)
{
    LOGV(TAG, "Enter %s(), sizeof(gSPD) = %u", __func__, sizeof(gSPD));
    memset(gSPD, 0, sizeof(gSPD));
}

void spd_deinit(void)
{
    LOGD(TAG, "Enter %s()", __func__);
    for (int i = 0; i < SPD_X_NUMBER; i++)
    {
        switch (i)
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
}
