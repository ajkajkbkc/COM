/**
  ******************************************************************************
  * @file    kalyke_sntp_task.h
  * @author  lixianyu
  * @version V0.0.1
  * @date    2019-05-11
  * @brief   SNTP
  ******************************************************************************
  */
#ifndef __KALYKE_SNTP_TASK_H
#define __KALYKE_SNTP_TASK_H
#include "FreeRTOS.h"
#include "task.h"

extern void kalyke_sntp_task(void *p_arg);

extern TaskHandle_t gKalykeSNTPTaskHandle;

#endif /* __KALYKE_SNTP_TASK_H */

