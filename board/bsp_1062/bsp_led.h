/**
  ******************************************************************************
  * @file    bsp_led.h
  * @author  lixianyu
  * @version V0.0.1
  * @date    2019-03-19
  * @brief   LED Çý¶¯
  ******************************************************************************
  */
#ifndef __BSP_LED_H
#define __BSP_LED_H
#include "FreeRTOS.h"
#include "board.h"
#include "fsl_gpio.h"
#include "kalyke_opts.h"

#if defined (PROJECT_FORLINX)
#define LED_1 GPIO1 // MCU_LED_RUN
#define LED_1_PIN (9U)
#define LED_1_PIN_MASK (0x00000200)
    
#define LED_2 GPIO2 // MCU_LED_NET
#define LED_2_PIN (30U)
#define LED_2_PIN_MASK (0x40000000)
    
#define LED_3 GPIO2 // MCU_LED_ERR
#define LED_3_PIN (2U)
#define LED_3_PIN_MASK (0x00000004)

#elif defined (PROJECT_KALYKE)
#define LED_1 GPIO3 // MCU_LED_RUN
#define LED_1_PIN (25U)
#define LED_1_PIN_MASK (0x02000000)
#define LED_RUN  LED_1
#define LED_RUN_PIN_MASK LED_1_PIN_MASK


#define LED_2 GPIO1 // MCU_LED_NET
#define LED_2_PIN (5U)
#define LED_2_PIN_MASK (0x00000020)
#define LED_NET  LED_2
#define LED_NET_PIN_MASK LED_2_PIN_MASK

#define LED_3 GPIO1 // MCU_LED_ERR
#define LED_3_PIN (4U)
#define LED_3_PIN_MASK (0x00000010)
#define LED_ERR  LED_3
#define LED_ERR_PIN_MASK LED_3_PIN_MASK

#define LED_4 GPIO1 // LED on the FeiLing.
#define LED_4_PIN (9U)
#define LED_4_PIN_MASK (0x00000200)
#endif


#if 0
#define SYS_RED_LED_ON          GPIO_ResetBits(GPIOI, GPIO_Pin_13)
#define SYS_RED_LED_OFF         GPIO_SetBits(GPIOI, GPIO_Pin_13)

#define SYS_GREEN_LED_ON        GPIO_ResetBits(GPIOI, GPIO_Pin_14)
#define SYS_GREEN_LED_OFF       GPIO_SetBits(GPIOI, GPIO_Pin_14)

#define SYS_BLUE_LED_ON         GPIO_ResetBits(GPIOI, GPIO_Pin_15)
#define SYS_BLUE_LED_OFF        GPIO_SetBits(GPIOI, GPIO_Pin_15)
#endif
#define UART0_A_LED_VALUE   0x00000001
#define UART0_A_LED_MASK    0x00000001

#define UART0_B_LED_VALUE   0x00000002
#define UART0_B_LED_MASK    0x00000002

#define UART1_A_LED_VALUE   0x00000008
#define UART1_A_LED_MASK    0x00000008

#define UART1_B_LED_VALUE   0x00000010
#define UART1_B_LED_MASK    0x00000010

static inline void bsp_toggle_led_RUN(void)
{
    GPIO_PortToggle(LED_1, LED_1_PIN_MASK);
}
static inline void bsp_toggle_led_2(void)
{
    GPIO_PortToggle(LED_2, LED_2_PIN_MASK);
}

static inline void bsp_toggle_led_ERR(void)
{
    GPIO_PortToggle(LED_3, LED_3_PIN_MASK);
}
static inline void bsp_open_led_2(void)
{
    LED_2->DR_CLEAR = LED_2_PIN_MASK;
}
static inline void bsp_close_led_2(void)
{
    LED_2->DR_SET = LED_2_PIN_MASK;
}
static inline void bsp_close_LED_RUN_ERR(void)
{
    LED_RUN->DR_SET = LED_1_PIN_MASK;
    LED_ERR->DR_SET = LED_3_PIN_MASK;
}

static inline void bsp_open_run_led(void)
{
    LED_RUN->DR_CLEAR = LED_RUN_PIN_MASK;
}
static inline void bsp_close_run_led(void)
{
    LED_RUN->DR_SET = LED_RUN_PIN_MASK;
}

static inline void bsp_open_net_led(void)
{
    LED_NET->DR_CLEAR = LED_NET_PIN_MASK;
}
static inline void bsp_close_net_led(void)
{
    LED_NET->DR_SET = LED_NET_PIN_MASK;
}

static inline void bsp_open_err_led(void)
{
    LED_ERR->DR_CLEAR = LED_ERR_PIN_MASK;
}
static inline void bsp_close_err_led(void)
{
    LED_ERR->DR_SET = LED_ERR_PIN_MASK;
}

static inline void bsp_open_led_4(void)
{
    LED_4->DR_CLEAR = LED_4_PIN_MASK;
}
static inline void bsp_close_led_4(void)
{
    LED_4->DR_SET = LED_4_PIN_MASK;
}


extern void bsp_led_init(void);
extern void bsp_refresh_io_port_led(unsigned short lsv_InputValue, unsigned short lsv_OutputValue);
extern void bsp_refresh_communication_led(unsigned long llv_Vaule, unsigned long llv_mask);
extern void bsp_open_all_led(void);
extern void bsp_toggle_all_led(void);
extern void bsp_close_all_led(void);
extern void bsp_close_err_led(void);
#endif /*__BSP_LED_H*/

