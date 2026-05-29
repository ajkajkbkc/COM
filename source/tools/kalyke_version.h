/**
  ******************************************************************************
  * @file    kalyke_version.h
  * @author  lixianyu
  * @version V0.0.1
  * @date    2019-04-13
  * @brief   Versions
  ******************************************************************************
  */
#ifndef __KALYKE_VERSION_H
#define __KALYKE_VERSION_H

/*实际上飞凌核心板GPIO_AD_B0_09是作为LED使用的，
 *Kalyke的P1板将不再使用该脚作为X0
 */
//#define X0_AS_LED

#define SW_VERSION  "1.1.0606.01"

#define PROGRAM_CAPACITY    64
/**
 * MiStudio收到该值后，会除以1000得到PLC版本号
 * 例如：1002 -> 1.002
 *          1 -> 0.001
 *
 * 每次提供新的OTA升级固件时，该值必须增加（每次加一即可）。
 * 每个具体客户的版本号可达10000个，从0.000至9.999
 */
#define FIRMWARE_IMAGE_ID    55U

#endif /* __KALYKE_VERSION_H */

