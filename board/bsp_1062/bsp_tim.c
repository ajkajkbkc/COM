/**
  ******************************************************************************
  * @file    bsp_tim.c
  * @author  lixianyu
  * @version V0.0.1
  * @date    2019-03-19
  * @brief   定时器相关驱动
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "bsp_tim.h"
#include "FreeRTOS.h"
#include "task.h"
#include "plc_element.h"
#include "bsp_led.h"
#include "fsl_pit.h"
#include "fsl_debug_console.h"
#include "plc_internalmanage.h"


#define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_PerClk)
#define PIT_IRQ_ID PIT_IRQn
#define PIT_KALYKE_HANDLER PIT_IRQHandler

/*------------------------------------------------------------------------------
*  外部函数定义
*-----------------------------------------------------------------------------*/
extern void plc_refresh_cycle_clock(void);

/**
  * @brief  时钟振荡时钟源使能
  * @param  None
  * @retval None
  */
void bsp_cycle_clock_enable(unsigned char lcv_En)
{
    if (lcv_En)
    {
        PIT_StartTimer(PIT, kPIT_Chnl_0);
        PIT_StartTimer(PIT, kPIT_Chnl_1);
        PIT_StartTimer(PIT, kPIT_Chnl_2);
        PIT_StartTimer(PIT, kPIT_Chnl_3);
    }
    else
    {
        PIT_StopTimer(PIT, kPIT_Chnl_0);
        PIT_StopTimer(PIT, kPIT_Chnl_1);
        PIT_StopTimer(PIT, kPIT_Chnl_2);
        PIT_StopTimer(PIT, kPIT_Chnl_3);
    }
}

/**
  * @brief  TIM7 中断函数
  * @param  None
  * @retval None
  */
#if 0
void TIM7_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM7, TIM_IT_Update) == SET)
    {
        plc_refresh_cycle_clock();
    }

    TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
}
#else
void PIT_KALYKE_HANDLER(void)
{
    if (PIT_GetStatusFlags(PIT, kPIT_Chnl_0) & kPIT_TimerFlag) // 5ms timer
    {
        /*10ms反转*/
        if(gtv_ClockCycleRecord.mcv_Status & 0x01)
        {
            //LOGI("bsp_tim", "10ms timer happen......");
            plc_set_bit_element_value(SM_ELEMENT, 10, 0);
            gtv_ClockCycleRecord.mcv_Status &= ~0x01;
        }
        else
        {
            plc_set_bit_element_value(SM_ELEMENT, 10, 1);
            gtv_ClockCycleRecord.mcv_Status |= 0x01;
        }
        /* Clear interrupt flag.*/
        PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
    }
    else if (PIT_GetStatusFlags(PIT, kPIT_Chnl_1) & kPIT_TimerFlag)//50ms timer
    {
        /*100ms反转*/
        if(gtv_ClockCycleRecord.mcv_Status & 0x02)
        {
            //LOGI("bsp_tim", "100ms timer happen......");
            plc_set_bit_element_value(SM_ELEMENT, 11, 0);
            gtv_ClockCycleRecord.mcv_Status &= ~0x02;
        }
        else
        {
            plc_set_bit_element_value(SM_ELEMENT, 11, 1);
            gtv_ClockCycleRecord.mcv_Status |= 0x02;
        }
        /* Clear interrupt flag.*/
        PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
    }
    else if (PIT_GetStatusFlags(PIT, kPIT_Chnl_2) & kPIT_TimerFlag) // 500ms timer
    {
        /*1秒反转*/
        if(gtv_ClockCycleRecord.mcv_Status & 0x04)
        {
            //LOGI("bsp_tim", "1s timer happen......");
            plc_set_bit_element_value(SM_ELEMENT, 12, 0);
            gtv_ClockCycleRecord.mcv_Status &= ~0x04;
        }
        else
        {
            plc_set_bit_element_value(SM_ELEMENT, 12, 1);
            gtv_ClockCycleRecord.mcv_Status |= 0x04;
        }
        /* Clear interrupt flag.*/
        PIT_ClearStatusFlags(PIT, kPIT_Chnl_2, kPIT_TimerFlag);
    }
    else if (PIT_GetStatusFlags(PIT, kPIT_Chnl_3) & kPIT_TimerFlag) // 30s timer
    {
        gtv_ClockCycleRecord.mlv_Hour += 30000;

        /*1分钟反转*/
        if(gtv_ClockCycleRecord.mcv_Status & 0x08)
        {
            plc_set_bit_element_value(SM_ELEMENT, 13, 0);
            gtv_ClockCycleRecord.mcv_Status &= ~0x08;
        }
        else
        {
            plc_set_bit_element_value(SM_ELEMENT, 13, 1);
            gtv_ClockCycleRecord.mcv_Status |= 0x08;
        }

        /*1小时反转*/
        if(gtv_ClockCycleRecord.mlv_Hour >= 30 * 60 * 1000)
        {
            if(gtv_ClockCycleRecord.mcv_Status & 0x10)
            {
                plc_set_bit_element_value(SM_ELEMENT, 14, 0);
                gtv_ClockCycleRecord.mcv_Status &= ~0x10;
            }
            else
            {
                plc_set_bit_element_value(SM_ELEMENT, 14, 1);
                gtv_ClockCycleRecord.mcv_Status |= 0x10;
            }
            gtv_ClockCycleRecord.mlv_Hour = 0;
        }
        /* Clear interrupt flag.*/
        PIT_ClearStatusFlags(PIT, kPIT_Chnl_3, kPIT_TimerFlag);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
       exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

#endif

/**
  * @brief  初始化振荡时钟中断源,
            kPIT_Chnl_0: 5ms触发一次中断
            kPIT_Chnl_1: 50ms触发一次中断
            kPIT_Chnl_2: 500ms触发一次中断
            kPIT_Chnl_3: 30s 触发一次中断
  * @param  None
  * @retval None
  */
void bsp_init_cycle_clock_tim(void)
{
    //CLOCK_EnableClock(kCLOCK_Gpio1);
#if 1
    CLOCK_SetMux(kCLOCK_PerclkMux, 1U);
    CLOCK_SetDiv(kCLOCK_PerclkDiv, 63U);
#endif
    pit_config_t pitConfig;
    /*
     * pitConfig.enableRunInDebug = false;
     */
    PIT_GetDefaultConfig(&pitConfig);

    /* Init pit module */
    PIT_Init(PIT, &pitConfig);
    PRINTF("CHANNEL[1].TCTRL = 0x%08X\r\n", PIT->CHANNEL[1].TCTRL);

    /* Set timer period for channel 0 */
    PRINTF("PIT_SOURCE_CLOCK = %u\r\n", PIT_SOURCE_CLOCK); // 75MHz
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, MSEC_TO_COUNT(5U, PIT_SOURCE_CLOCK)); // 5ms
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, MSEC_TO_COUNT(50U, PIT_SOURCE_CLOCK));// 50ms
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_2, MSEC_TO_COUNT(500U, PIT_SOURCE_CLOCK));// 500ms
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_3, MSEC_TO_COUNT(30000U, PIT_SOURCE_CLOCK));// 30s

    PIT_SetTimerChainMode(PIT, kPIT_Chnl_0, false);
    PIT_SetTimerChainMode(PIT, kPIT_Chnl_1, false);
    PIT_SetTimerChainMode(PIT, kPIT_Chnl_2, false);
    PIT_SetTimerChainMode(PIT, kPIT_Chnl_3, false);

    /* Enable timer interrupts */
    PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
    PIT_EnableInterrupts(PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable);
    PIT_EnableInterrupts(PIT, kPIT_Chnl_2, kPIT_TimerInterruptEnable);
    PIT_EnableInterrupts(PIT, kPIT_Chnl_3, kPIT_TimerInterruptEnable);

    PRINTF("CHANNEL[1].TCTRL = 0x%08X\r\n", PIT->CHANNEL[1].TCTRL);
    /* Enable at the NVIC */
    EnableIRQ(PIT_IRQ_ID);

}



/*------------------------------------------------------------------------------
*  以下代码用以实现PLYS指令
*-----------------------------------------------------------------------------*/
bsp_plsy_ins_channel_info_st *gtp_BspPlsyChannelInfo = NULL;

/**
  * @brief  TIM3 初始化，用作PLSY指令周期产生
  * @param  None
  * @retval None
  */
#if 0 // 先不弄这块，因为 CI_PLSY是运动控制方面的
void bsp_tim3_init()
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseSt;
    NVIC_InitTypeDef NVIC_InitSt;
    unsigned short lsv_PrescalerValue;
    unsigned char i;

    TIM_DeInit(TIM3);

    RCC_APB1PeriphClockCmd(RCC_APB1ENR_TIM3EN, ENABLE);

    TIM_TimeBaseStructInit(&TIM_TimeBaseSt);
    TIM_TimeBaseSt.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseSt.TIM_Period = 65535;
    TIM_TimeBaseSt.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseSt.TIM_Prescaler = 0;

    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseSt);

    /*设置TIM3的计数频率为 10MHz*/
    lsv_PrescalerValue = (unsigned short)((SystemCoreClock / 2) / TIM3_COUNTER_CLOCK) - 1;
    TIM_PrescalerConfig(TIM3, lsv_PrescalerValue, TIM_PSCReloadMode_Immediate);

    /*TIM3 中断配置*/
    NVIC_InitSt.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitSt.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitSt.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitSt.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitSt);

    /*分配内存*/
    if(gtp_BspPlsyChannelInfo == NULL)
    {
        gtp_BspPlsyChannelInfo = (bsp_plsy_ins_channel_info_st *)pvPortMalloc(sizeof(bsp_plsy_ins_channel_info_st) * MAX_PLSY_OUTPUT_CHANNEL_NUM);
        configASSERT(gtp_BspPlsyChannelInfo != NULL);
    }

    if(gtp_BspPlsyChannelInfo)
    {
        for(i = 0; i < MAX_PLSY_OUTPUT_CHANNEL_NUM; i++)
        {
            gtp_BspPlsyChannelInfo[i].mcv_Status = 0;
            gtp_BspPlsyChannelInfo[i].mcv_OutputLevel = 0;
            gtp_BspPlsyChannelInfo[i].msv_ccrValue = 0;
            gtp_BspPlsyChannelInfo[i].mlv_Freq = 0;
            gtp_BspPlsyChannelInfo[i].mlv_DestPulseNum = 0;
            gtp_BspPlsyChannelInfo[i].mlv_OutPulseCnt = 0;
        }
    }

}
#else
void bsp_tim3_init(void)
{
}
#endif

/**
  * @brief  TIM3四个通道输出比较模式配置
  * @param  None
  * @retval None
  */
#if 0 // TODO:
void bsp_set_tim3_output_compare_mode(unsigned char lcv_Channel, unsigned short lsv_ccrValue)
{
    TIM_OCInitTypeDef TIM_OCInitSt;

    TIM_OCInitSt.TIM_OCMode = TIM_OCMode_Timing;
    TIM_OCInitSt.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitSt.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitSt.TIM_Pulse = lsv_ccrValue;

    switch(lcv_Channel)
    {
    case 0:
        TIM_OC1Init(TIM3, &TIM_OCInitSt);
        TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
        break;

    case 1:
        TIM_OC2Init(TIM3, &TIM_OCInitSt);
        TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Disable);
        break;

    case 2:
        TIM_OC3Init(TIM3, &TIM_OCInitSt);
        TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Disable);
        break;

    case 3:
        TIM_OC4Init(TIM3, &TIM_OCInitSt);
        TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Disable);
        break;
    }
}
#else
void bsp_set_tim3_output_compare_mode(unsigned char lcv_Channel, unsigned short lsv_ccrValue)
{
}
#endif

/**
  * @brief  TIM3四个通道输出使能
  * @param  None
  * @retval None
  */
#if 0 // TODO:
void bsp_start_plsy_channel(unsigned char lcv_Channel, unsigned long llv_Freq, unsigned long llv_DestPulseNum)
{
    if(gtp_BspPlsyChannelInfo[lcv_Channel].mlv_Freq != llv_Freq)
    {
        gtp_BspPlsyChannelInfo[lcv_Channel].mlv_Freq = llv_Freq;
        gtp_BspPlsyChannelInfo[lcv_Channel].msv_ccrValue = (unsigned short)(TIM3_COUNTER_CLOCK / (llv_Freq * 2));

        bsp_set_tim3_output_compare_mode(lcv_Channel, gtp_BspPlsyChannelInfo[lcv_Channel].msv_ccrValue);
    }

    gtp_BspPlsyChannelInfo[lcv_Channel].mlv_DestPulseNum = llv_DestPulseNum;
    gtp_BspPlsyChannelInfo[lcv_Channel].mlv_OutPulseCnt = 0;

    gtp_BspPlsyChannelInfo[lcv_Channel].mcv_Status = 1;
    gtp_BspPlsyChannelInfo[lcv_Channel].mcv_OutputLevel = 0;

    switch(lcv_Channel)
    {
    case 0:
        TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);
        break;
    case 1:
        TIM_ITConfig(TIM3, TIM_IT_CC2, ENABLE);
        break;
    case 2:
        TIM_ITConfig(TIM3, TIM_IT_CC3, ENABLE);
        break;
    case 3:
        TIM_ITConfig(TIM3, TIM_IT_CC4, ENABLE);
        break;
    }

    TIM_Cmd(TIM3, ENABLE);
}
#else
void bsp_start_plsy_channel(unsigned char lcv_Channel, unsigned long llv_Freq, unsigned long llv_DestPulseNum)
{
}
#endif

/**
  * @brief  TIM3中断服务函数
  * @param  None
  * @retval None
  */
#if 0
void TIM3_IRQHandler()
{
    unsigned short msv_Capture;

    if (TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);

        if(gtp_BspPlsyChannelInfo[0].mcv_OutputLevel)
        {
            GPIOJ->BSRRL = GPIO_Pin_8;
            gtp_BspPlsyChannelInfo[0].mcv_OutputLevel = 0;
        }
        else
        {
            GPIOJ->BSRRH = GPIO_Pin_8;
            gtp_BspPlsyChannelInfo[0].mcv_OutputLevel = 1;
        }

        gtp_BspPlsyChannelInfo[0].mlv_OutPulseCnt ++;
        SET_SD_ELEMENT_VALUE(80, (gtp_BspPlsyChannelInfo[1].mlv_OutPulseCnt / 2));

        if((gtp_BspPlsyChannelInfo[0].mlv_OutPulseCnt > gtp_BspPlsyChannelInfo[0].mlv_DestPulseNum * 2) ||
                !plc_get_bit_element_value(SM_ELEMENT, 80))
        {
            TIM_ITConfig(TIM3, TIM_IT_CC1, DISABLE);
            gtp_BspPlsyChannelInfo[0].mcv_Status = 0;
            /*清除脉冲输出标志*/
            plc_set_bit_element_value(SM_ELEMENT, 84, 0);
        }
        else
        {
            msv_Capture = TIM_GetCapture1(TIM3);
            TIM_SetCompare1(TIM3, msv_Capture + gtp_BspPlsyChannelInfo[0].msv_ccrValue);
        }
    }
    else if (TIM_GetITStatus(TIM3, TIM_IT_CC2) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC2);

        if(gtp_BspPlsyChannelInfo[1].mcv_OutputLevel)
        {
            GPIOJ->BSRRL = GPIO_Pin_9;
            gtp_BspPlsyChannelInfo[1].mcv_OutputLevel = 0;
        }
        else
        {
            GPIOJ->BSRRH = GPIO_Pin_9;
            gtp_BspPlsyChannelInfo[1].mcv_OutputLevel = 1;
        }

        gtp_BspPlsyChannelInfo[1].mlv_OutPulseCnt ++;
        SET_SD_ELEMENT_VALUE(81, (gtp_BspPlsyChannelInfo[1].mlv_OutPulseCnt / 2));

        if((gtp_BspPlsyChannelInfo[1].mlv_OutPulseCnt >= gtp_BspPlsyChannelInfo[1].mlv_DestPulseNum * 2) ||
                !plc_get_bit_element_value(SM_ELEMENT, 81))
        {
            TIM_ITConfig(TIM3, TIM_IT_CC2, DISABLE);
            gtp_BspPlsyChannelInfo[1].mcv_Status = 0;
            /*清除脉冲输出标志*/
            plc_set_bit_element_value(SM_ELEMENT, 85, 0);
        }
        else
        {
            msv_Capture = TIM_GetCapture2(TIM3);
            TIM_SetCompare2(TIM3, msv_Capture + gtp_BspPlsyChannelInfo[1].msv_ccrValue);
        }
    }
    else if (TIM_GetITStatus(TIM3, TIM_IT_CC3) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC3);

        if(gtp_BspPlsyChannelInfo[2].mcv_OutputLevel)
        {
            GPIOJ->BSRRL = GPIO_Pin_10;
            gtp_BspPlsyChannelInfo[2].mcv_OutputLevel = 0;
        }
        else
        {
            GPIOJ->BSRRH = GPIO_Pin_10;
            gtp_BspPlsyChannelInfo[2].mcv_OutputLevel = 1;
        }

        gtp_BspPlsyChannelInfo[2].mlv_OutPulseCnt ++;
        SET_SD_ELEMENT_VALUE(82, (gtp_BspPlsyChannelInfo[1].mlv_OutPulseCnt / 2));

        if((gtp_BspPlsyChannelInfo[2].mlv_OutPulseCnt >= gtp_BspPlsyChannelInfo[2].mlv_DestPulseNum * 2) ||
                !plc_get_bit_element_value(SM_ELEMENT, 82))
        {
            TIM_ITConfig(TIM3, TIM_IT_CC3, DISABLE);
            gtp_BspPlsyChannelInfo[2].mcv_Status = 0;
            /*清除脉冲输出标志*/
            plc_set_bit_element_value(SM_ELEMENT, 86, 0);
        }
        else
        {
            msv_Capture = TIM_GetCapture3(TIM3);
            TIM_SetCompare3(TIM3, msv_Capture + gtp_BspPlsyChannelInfo[2].msv_ccrValue);
        }
    }
    else
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC4);

        if(gtp_BspPlsyChannelInfo[3].mcv_OutputLevel)
        {
            GPIOJ->BSRRL = GPIO_Pin_11;
            gtp_BspPlsyChannelInfo[3].mcv_OutputLevel = 0;
        }
        else
        {
            GPIOJ->BSRRH = GPIO_Pin_11;
            gtp_BspPlsyChannelInfo[3].mcv_OutputLevel = 1;
        }

        gtp_BspPlsyChannelInfo[3].mlv_OutPulseCnt ++;
        SET_SD_ELEMENT_VALUE(83, (gtp_BspPlsyChannelInfo[1].mlv_OutPulseCnt / 2));

        if((gtp_BspPlsyChannelInfo[3].mlv_OutPulseCnt >= gtp_BspPlsyChannelInfo[3].mlv_DestPulseNum * 2) ||
                !plc_get_bit_element_value(SM_ELEMENT, 83))
        {
            TIM_ITConfig(TIM3, TIM_IT_CC4, DISABLE);
            gtp_BspPlsyChannelInfo[3].mcv_Status = 0;
            /*清除脉冲输出标志*/
            plc_set_bit_element_value(SM_ELEMENT, 87, 0);
        }
        else
        {
            msv_Capture = TIM_GetCapture4(TIM3);
            TIM_SetCompare4(TIM3, msv_Capture + gtp_BspPlsyChannelInfo[3].msv_ccrValue);
        }
    }

}
#endif

