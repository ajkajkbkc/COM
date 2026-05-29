/**
  ******************************************************************************
  * @file    daisy_task.h
  * @author  lixianyu
  * @version V0.0.1
  * @date    2020-04-18
  * @brief   
  ******************************************************************************
  */
#ifndef _DAISY_TASK_H
#define _DAISY_TASK_H
#include "FreeRTOS.h"
#include "task.h"
#include "mb.h"

typedef enum _DAISY_FSM
{
    DAISY_FSM_IDLE     = 0, // Ready to send DAISY_CMD_0101 to slave.
    DAISY_FSM_CONFIG   = 1, // Ready to send DAISY_CMD_1C1C to slave.
    DAISY_FSM_PRE_LOOP = 2, // Pre loop
    DAISY_FSM_PRE_LOOP2= 3, // Pre loop 2
    DAISY_FSM_LOOP     = 4, // Loop
#if (DAISY_CONFIG_WHEN_LOOP == 1)
    DAISY_FSM_LOOP_CONFIG = 5, // Config when looping
#endif
}daisy_fsm_e;

/* Should be same as slave. */
typedef enum _DAISY_ERR
{
    DAISY_ERR_NO_ERR = 0,
    DAISY_ERR_MODULE_NUM = 1,
    DAISY_ERR_BFM_IDX_SDO = 2,
    DAISY_ERR_BFM_IDX_PDO = 3,
}daisy_err_e;

/* 当从站收到主站发来的数据时，根据第一个字节判定相应的操作 */
#define DAISY_CMD_0101    0x0101 // 获取从站个数
#define DAISY_CMD_1111    0x1111 // 获取从站ID（型号）
#define DAISY_CMD_1C1C    0x1C1C // 逐个配置从站属性
#define DAISY_CMD_2C2C    0x2C2C // 配置指定的从站属性
#define DAISY_CMD_2222    0x2222 // 收到该帧后，从站变为数据交换工作模式
#define DAISY_CMD_3333    0x3333 // 主循环
#define DAISY_CMD_8888    0x8888 // 升级从站


#define BFM_IDX_0x1000    0x1000 //存放模块识别码
#define BFM_IDX_0x1001    0x1001 //存放版本号
#define BFM_IDX_0x1002    0x1002 //存放错误码
#define BFM_IDX_0x1003    0x1003 //本站数据在帧中的偏移
#define BFM_IDX_0x1004    0x1004 //本站数据的长度（字节）
#define BFM_IDX_0x1005    0x1005 //网口后面挂接的模块数
#define BFM_IDX_0x1006    0x1006 //SPI后面挂接的模块数

#define BFM_IDX_0x1050    0x1050 //X0 ~ X7
#define BFM_IDX_0x1051    0x1051 //X10 ~ X17

#define BFM_IDX_0x1060    0x1060 //Y0 ~ Y7
#define BFM_IDX_0x1061    0x1061 //Y10 ~ Y17



extern void start_daisy_task(void);
extern void daisy_get_info(md_slave_msg_pack *pMsg);
extern void kalyke_daisy_init(void);
extern void kalyke_daisy_stop(void);
extern void daisy_LAN_send_bin(uint8_t *pBuf, uint16_t len);

extern volatile daisy_fsm_e gDaisyFSM;
extern TaskHandle_t gDaisyTaskHandle;
extern uint16_t gSlaveIDUpgrade;
extern uint8_t gDaisyLANRecvBuffer[512];
#endif /* _DAISY_TASK_H */

