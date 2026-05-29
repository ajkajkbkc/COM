/**
  ******************************************************************************
  * @file    bsp_tim.h
  * @author  lixianyu
  * @version V0.0.1
  * @date    2019-03-19
  * @brief   定时器相关驱动
  ******************************************************************************
  */

#ifndef __BSP_TIM_H
#define __BSP_TIM_H
void bsp_cycle_clock_enable(unsigned char lcv_En);
void bsp_init_cycle_clock_tim(void);

/*------------------------------------------------------------------------------
*  以下代码用以实现PLYS指令
*-----------------------------------------------------------------------------*/
#define MAX_PLSY_OUTPUT_CHANNEL_NUM     4
#define TIM3_COUNTER_CLOCK  42000000

/*plsy指令各通道信息结构体*/
typedef struct __BSP_PLSY_INS_CHANNEL_INFO_ST{
    /*通道当前信息，0: 关闭, 1: 正在输出方波*/
    unsigned char mcv_Status;
    /*当前输出高低电平*/
    unsigned char mcv_OutputLevel;
    /*CCR Value*/
    unsigned short msv_ccrValue;
    /*通道当前频率值*/
    unsigned long mlv_Freq;
    /*目标脉冲数量*/
    unsigned long mlv_DestPulseNum;
    /*已输出数量*/
    unsigned long mlv_OutPulseCnt;
}bsp_plsy_ins_channel_info_st;

extern bsp_plsy_ins_channel_info_st *gtp_BspPlsyChannelInfo;

void bsp_tim3_init(void);
void bsp_set_tim3_output_compare_mode(unsigned char lcv_Channel, unsigned short lsv_ccrValue);
void bsp_start_plsy_channel(unsigned char lcv_Channel, unsigned long llv_Freq, unsigned long llv_DestPulseNum);
#endif /*__BSP_TIM_H*/
