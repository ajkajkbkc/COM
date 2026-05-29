/**
  ******************************************************************************
  * @file    mb.c
  * @author  lixianyu
  * @version V0.0.1
  * @date    2019-03-19
  * @brief   Modbus slave 主程序
  ******************************************************************************
  */

#include "mb.h"
#include "verify_func.h"
#include "FreeRTOS.h"
#include "mb_ctrl.h"
#include "mb_download.h"
#include "mb_upload.h"
#include "plc_element.h"
#include "mb_maptable.h"
#include "plc_commonfunc.h"
#include "bsp_iwdg.h"
#include "bsp_dct.h"
#include "fsl_debug_console.h"
#include "kalyke_tool.h"

/*------------------------------------------------------------------------------
*   变量定义
*-----------------------------------------------------------------------------*/
mb_slave_diagnositic_info_st gtp_ModbusSlaveDiagInfo[MB_SENDER_MAX];

/*------------------------------------------------------------------------------
*   函数
*-----------------------------------------------------------------------------*/

/**
  * @brief  读取线圈状态
  * @param  None
  * @retval None
  */
void mb_slave_read_coils_status(md_slave_msg_pack *pMsg)
{
    unsigned short lsv_StartElement;
    unsigned short lsv_ElementCnt;
    unsigned char lcv_Ret;
    unsigned char lcv_ElementType;
    unsigned short lsv_ElementAddr;
    unsigned char *lcp_Value;
    unsigned short *lsp_BaseAddr;
    unsigned short lsv_ElementRang;
    unsigned short i;
    unsigned char lcv_RespByteNum;
    unsigned char lcv_BitsCnt;

    /*不支持广播消息*/
    if(pMsg->mcv_IsBroadcastInfo) {
        return;
    }

    if(pMsg->msv_ReceiveLen != 8) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    /*获取开始元件地址及读取元件数量*/
    lsv_StartElement =  GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[2]);
    lsv_ElementCnt = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[4]);
    if(lsv_ElementCnt > MB_MAX_R_BIT_NUM) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    lcv_Ret = mb_slave_convert_element_info(MB_BIT_ELEMENT, lsv_StartElement, &lcv_ElementType, &lsv_ElementAddr);
    if(lcv_Ret != pdPASS) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    switch(lcv_ElementType) {
        case MB_BIT_Y:
            lsp_BaseAddr = Y_ELEMENT;
            lsv_ElementRang = Y_RANG;
            break;
        case MB_BIT_X:
            lsp_BaseAddr = X_ELEMENT;
            lsv_ElementRang = X_RANG;
            break;
        case MB_BIT_M:
            lsp_BaseAddr = M_ELEMENT;
            lsv_ElementRang = M_RANG;
            break;
        case MB_BIT_SM:
            lsp_BaseAddr = SM_ELEMENT;
            lsv_ElementRang = SM_RANG;
            break;
        case MB_BIT_S:
            lsp_BaseAddr = S_ELEMENT;
            lsv_ElementRang = S_RANG;
            break;
        case MB_BIT_T:
            lsp_BaseAddr = T_ELEMENT;
            lsv_ElementRang = T_RANG;
            break;
        case MB_BIT_C:
            lsp_BaseAddr = C_ELEMENT;
            lsv_ElementRang = C_RANG;
            break;
        default:
            pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
            mb_slave_error_resp(pMsg);
            return;
    }

    /*范围判断*/
    if(lsv_ElementAddr + lsv_ElementCnt -1 > lsv_ElementRang) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    lcp_Value = &pMsg->mcp_RespBuff[3];
    *lcp_Value = 0;
    i=0;
    lcv_BitsCnt = 0;

#if 0
    if(lsv_ElementCnt >= 8) {
        lcv_RespByteNum = 0;
    } else {
        lcv_RespByteNum = 1;
    }
#else
    lcv_RespByteNum = 1;
#endif

    while(lsv_ElementCnt) {

        if(lcv_BitsCnt >= 8) {
            lcp_Value++;
            *lcp_Value = 0;
            lcv_BitsCnt = 0;
            lcv_RespByteNum ++;
        }
        *lcp_Value += (plc_get_bit_element_value(lsp_BaseAddr, lsv_ElementAddr+i) << lcv_BitsCnt);
        i++;
        lcv_BitsCnt++;
        lsv_ElementCnt --;
    }

    /*组响应帧*/
    for(i=0; i<2; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->mcp_RespBuff[2] = lcv_RespByteNum;

    pMsg->msv_RespLen = lcv_RespByteNum+3;
    mb_slave_verify_resp_msg(pMsg);

}

/**
  * @brief  读离散输入量状态
  * @param  None
  * @retval None
  */

void mb_slave_read_descrete_input_status(md_slave_msg_pack *pMsg)
{
    unsigned short lsv_StartElement;
    unsigned short lsv_ElementCnt;
    unsigned char lcv_Ret;
    unsigned char lcv_ElementType;
    unsigned short lsv_ElementAddr;
    unsigned char *lcp_Value;
    unsigned short i;
    unsigned char lcv_RespByteNum;
    unsigned char lcv_BitsCnt;

    /*不支持广播消息*/
    if(pMsg->mcv_IsBroadcastInfo) {
        return;
    }

    if(pMsg->msv_ReceiveLen != 8) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    /*获取开始元件地址及读取元件数量*/
    lsv_StartElement =  GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[2]);
    lsv_ElementCnt = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[4]);
    if(lsv_ElementCnt > MB_MAX_R_BIT_NUM) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    lcv_Ret = mb_slave_convert_element_info(MB_BIT_ELEMENT, lsv_StartElement, &lcv_ElementType, &lsv_ElementAddr);
    if(lcv_Ret != pdPASS) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    /*范围判断*/
    if(lsv_ElementAddr + lsv_ElementCnt -1 > X_RANG) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    lcp_Value = &pMsg->mcp_RespBuff[3];
    *lcp_Value = 0;
    i=0;
    lcv_BitsCnt = 0;

#if 0
    if(lsv_ElementCnt >= 8) {
        lcv_RespByteNum = 0;
    } else {
        lcv_RespByteNum = 1;
    }
#else
    lcv_RespByteNum = 1;
#endif

    while(lsv_ElementCnt) {

        if(lcv_BitsCnt >= 8) {
            lcp_Value++;
            *lcp_Value = 0;
            lcv_BitsCnt = 0;
            lcv_RespByteNum ++;
        }
        *lcp_Value += (plc_get_bit_element_value(X_ELEMENT, lsv_ElementAddr+i) << lcv_BitsCnt);
        *lcp_Value <<= 1;
        i++;
        lsv_ElementCnt --;
        lcv_BitsCnt ++;
    }

    /*组响应帧*/
    for(i=0; i<2; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->mcp_RespBuff[2] = lcv_RespByteNum;

    pMsg->msv_RespLen = lcv_RespByteNum+3;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  读寄存器，支持D SD Z T C R
  * @param  None
  * @retval None
  */
void mb_slave_read_holding_register(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr;
    unsigned short lsv_ElementAddr;
    unsigned char lcv_ElementType;
    unsigned short lsv_ElementCnt;
    unsigned char i;
    unsigned short lsv_ElementValue;
    unsigned long llv_C32Value;
    unsigned short lsv_DataLen;

    /*不支持广播消息*/
    if(pMsg->mcv_IsBroadcastInfo) {
        return;
    }

    if(pMsg->msv_ReceiveLen != 8) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[2]);
    lsv_ElementCnt = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[4]);

    if(lsv_ElementCnt > MB_MAX_R_WORD_NUM) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    lcv_Ret = mb_slave_convert_element_info(MB_WORD_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
    if(lcv_Ret != pdPASS) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    lsv_DataLen = 0;

    switch(lcv_ElementType) {
        case MB_WORD_D:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > D_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                lsv_ElementValue = GET_D_ELEMENT_VALUE(lsv_ElementAddr+i);
                pMsg->mcp_RespBuff[3+i*2] = (unsigned char)(lsv_ElementValue >> 8);
                pMsg->mcp_RespBuff[3+i*2+1] = (unsigned char)(lsv_ElementValue);
                lsv_DataLen += 2;
            }
            break;

        case MB_WORD_SD:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > SD_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                lsv_ElementValue = GET_SD_ELEMENT_VALUE(lsv_ElementAddr+i);
                pMsg->mcp_RespBuff[3+i*2] = (unsigned char)(lsv_ElementValue >> 8);
                pMsg->mcp_RespBuff[3+i*2+1] = (unsigned char)(lsv_ElementValue);
                lsv_DataLen += 2;
            }
            break;

        case MB_WORD_Z:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > Z_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                lsv_ElementValue = GET_Z_ELEMENT_VALUE(lsv_ElementAddr+i);
                pMsg->mcp_RespBuff[3+i*2] = (unsigned char)(lsv_ElementValue >> 8);
                pMsg->mcp_RespBuff[3+i*2+1] = (unsigned char)(lsv_ElementValue);
                lsv_DataLen += 2;
            }
            break;

        case MB_WORD_C:
            if(lsv_ElementAddr < C16_RANG) {
                /*16bit 计数器*/
                if(lsv_ElementAddr + lsv_ElementCnt -1 > C16_RANG) {
                    pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                    mb_slave_error_resp(pMsg);
                    return;
                }

                for(i=0; i<lsv_ElementCnt; i++) {
                    lsv_ElementValue = GET_C16_CURRENT_VALUE(lsv_ElementAddr+i);
                    pMsg->mcp_RespBuff[3+i*2] = (unsigned char)(lsv_ElementValue >> 8);
                    pMsg->mcp_RespBuff[3+i*2+1] = (unsigned char)(lsv_ElementValue);
                    lsv_DataLen += 2;
                }
            } else {
                /*32Bit 计数器*/
                if(lsv_ElementAddr + lsv_ElementCnt -1 > gtp_PlcElementInfo->msv_CElement.msv_ElementCnt) {
                    pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                    mb_slave_error_resp(pMsg);
                    return;
                }

                for(i=0; i<lsv_ElementCnt; i++) {
                    llv_C32Value = GET_C32_CURRENT_VALUE(lsv_ElementAddr+i);
                    pMsg->mcp_RespBuff[3+i*4] = (unsigned char)(llv_C32Value >> 24);
                    pMsg->mcp_RespBuff[3+i*4+1] = (unsigned char)(llv_C32Value >> 16);
                    pMsg->mcp_RespBuff[3+i*4+2] = (unsigned char)(llv_C32Value >> 8);
                    pMsg->mcp_RespBuff[3+i*4+3] = (unsigned char)(llv_C32Value);
                    lsv_DataLen += 4;
                }

            }
            break;

        case MB_WORD_T:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > T_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                lsv_ElementValue = GET_T_CURRENT_VALUE(lsv_ElementAddr+i);
                pMsg->mcp_RespBuff[3+i*2] = (unsigned char)(lsv_ElementValue >> 8);
                pMsg->mcp_RespBuff[3+i*2+1] = (unsigned char)(lsv_ElementValue);
                lsv_DataLen += 2;
            }
            break;

        case MB_WORD_R:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > R_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                lsv_ElementValue = GET_R_ELEMENT_VALUE(lsv_ElementAddr+i);
                pMsg->mcp_RespBuff[3+i*2] = (unsigned char)(lsv_ElementValue >> 8);
                pMsg->mcp_RespBuff[3+i*2+1] = (unsigned char)(lsv_ElementValue);
                lsv_DataLen += 2;
            }
            break;

    }

    /*组响应帧*/
    for(i=0; i<2; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->mcp_RespBuff[2] = lsv_DataLen;

    pMsg->msv_RespLen = lsv_DataLen + 3;
    mb_slave_verify_resp_msg(pMsg);

}

/**
  * @brief  写单线圈 Y M SM S T C
  * @param  None
  * @retval None
  */

void mb_slave_write_single_coil(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr, lsv_ElementAddr, lsv_ElementValue;
    unsigned char lcv_ElementType;
    unsigned char lcv_BitValue;
    unsigned char i;

    // 02 05 07 D0 FF 00 8C 84
    lsv_ElementValue = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[4]);
    if(lsv_ElementValue != 0xFF00 && lsv_ElementValue != 0x0000) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    if(lsv_ElementValue > 0)
        lcv_BitValue = 1;
    else
        lcv_BitValue = 0;

    lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[2]);

    lcv_Ret = mb_slave_convert_element_info(MB_BIT_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
    if(lcv_Ret != pdPASS) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }
    PRINTF("lsv_ElementAddr = %u\r\n", lsv_ElementAddr);
    switch(lcv_ElementType) {
        case MB_BIT_Y:
            plc_set_bit_element_value(Y_ELEMENT, lsv_ElementAddr, lcv_BitValue);
            break;

        case MB_BIT_M:
            plc_set_bit_element_value(M_ELEMENT, lsv_ElementAddr, lcv_BitValue);
            break;

        case MB_BIT_SM:
            /*20170811：需要增加写权限验证...*/
            plc_set_bit_element_value(SM_ELEMENT, lsv_ElementAddr, lcv_BitValue);
            break;

        case MB_BIT_S:
            plc_set_bit_element_value(S_ELEMENT, lsv_ElementAddr, lcv_BitValue);
            break;
            
        case MB_BIT_T:
            plc_set_bit_element_value(T_ELEMENT, lsv_ElementAddr, lcv_BitValue);
            break;

        case MB_BIT_C:
            plc_set_bit_element_value(C_ELEMENT, lsv_ElementAddr, lcv_BitValue);
            break;
        default:
            pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
            mb_slave_error_resp(pMsg);
            return;
    }

    /*组响应帧,返回帧为请求帧的复制*/
    for(i=0; i<6; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->msv_RespLen = 6;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  写单寄存器 D SD Z T C R
  * @param  None
  * @retval None
  */
void mb_slave_write_register(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr, lsv_ElementAddr, lsv_ElementValue;
    unsigned char lcv_ElementType;
    unsigned char i;

    lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[2]);
    lsv_ElementValue = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[4]);

    lcv_Ret = mb_slave_convert_element_info(MB_WORD_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
    if(lcv_Ret != pdPASS) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    switch(lcv_ElementType) {
        case MB_WORD_D:
            SET_D_ELEMENT_VALUE(lsv_ElementAddr, lsv_ElementValue);
            break;
        case MB_WORD_SD:
            /*20170811: 需要增加写入权限校验...*/
            SET_SD_ELEMENT_VALUE(lsv_ElementAddr, lsv_ElementValue);
            break;
        case MB_WORD_Z:
            SET_Z_ELEMENT_VALUE(lsv_ElementAddr, lsv_ElementValue);
            break;
        case MB_WORD_T:
            SET_T_CURRENT_VALUE(lsv_ElementAddr, lsv_ElementValue);
            break;
        case MB_WORD_R:
            SET_R_ELEMENT_VALUE(lsv_ElementAddr, lsv_ElementValue);
            break;
        case MB_WORD_C:
            if(lsv_ElementAddr < C16_RANG)
                SET_C16_CURRENT_VALUE(lsv_ElementAddr, lsv_ElementValue);
            else
                SET_C32_CURRENT_VALUE(lsv_ElementAddr, lsv_ElementValue);
            break;
    }

    /*组响应帧,返回帧为请求帧的复制*/
    for(i=0; i<6; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->msv_RespLen = 6;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  写多线圈
  * @param  None
  * @retval None
  */

void mb_slave_write_multiple_coils(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr, lsv_ElementCnt, lsv_ElementAddr;
    unsigned char lcv_ElementType, lcv_ValueByteNum;
    unsigned short i;
    unsigned char *lcp_Value, j;

    lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[2]);
    lsv_ElementCnt = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[4]);
    lcv_ValueByteNum = pMsg->mcp_ReceiveBuff[6];

    if(((lsv_ElementCnt>>3) != lcv_ValueByteNum)
       &&  ((lsv_ElementCnt>>3) != (lcv_ValueByteNum-1))) {
        /*字节数与要写入的位元件数量不匹配*/
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    if (!(lsv_ElementCnt%8) && ((lsv_ElementCnt>>3) != lcv_ValueByteNum)) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    if ((lsv_ElementCnt%8) && ((lsv_ElementCnt>>3) != (lcv_ValueByteNum -1))) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    if(lsv_ElementCnt > MB_MAX_W_BIT_NUM || lsv_ElementCnt < 1) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    lcv_Ret = mb_slave_convert_element_info(MB_BIT_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
    if(lcv_Ret != pdPASS) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    lcp_Value = &pMsg->mcp_ReceiveBuff[7];

    switch(lcv_ElementType) {
        case MB_BIT_Y:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > Y_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lcv_ValueByteNum; i++) {
                for(j=0; j<8; j++) {
                    plc_set_bit_element_value(Y_ELEMENT, lsv_ElementAddr+i*8+j, (lcp_Value[i]>>j & 0x01));
                    lsv_ElementCnt --;
                    if(!lsv_ElementCnt) {
                        break;
                    }
                }
            }
            break;

        case MB_BIT_M:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > M_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lcv_ValueByteNum; i++) {
                for(j=0; j<8; j++) {
                    plc_set_bit_element_value(M_ELEMENT, lsv_ElementAddr+i*8+j, (lcp_Value[i]>>j & 0x01));
                    lsv_ElementCnt --;
                    if(!lsv_ElementCnt) {
                        break;
                    }
                }
            }
            break;

        case MB_BIT_SM:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > SM_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lcv_ValueByteNum; i++) {
                for(j=0; j<8; j++) {
                    /*20170811: 需要增加SM元件写入权限校验...*/
                    plc_set_bit_element_value(SM_ELEMENT, lsv_ElementAddr+i*8+j, (lcp_Value[i]>>j & 0x01));
                    lsv_ElementCnt --;
                    if(!lsv_ElementCnt) {
                        break;
                    }
                }
            }
            break;

        case MB_BIT_S:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > S_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lcv_ValueByteNum; i++) {
                for(j=0; j<8; j++) {
                    plc_set_bit_element_value(S_ELEMENT, lsv_ElementAddr+i*8+j, (lcp_Value[i]>>j & 0x01));
                    lsv_ElementCnt --;
                    if(!lsv_ElementCnt) {
                        break;
                    }
                }
            }
            break;

        case MB_BIT_T:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > T_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lcv_ValueByteNum; i++) {
                for(j=0; j<8; j++) {
                    plc_set_bit_element_value(T_ELEMENT, lsv_ElementAddr+i*8+j, (lcp_Value[i]>>j & 0x01));
                    lsv_ElementCnt --;
                    if(!lsv_ElementCnt) {
                        break;
                    }
                }
            }
            break;

        case MB_BIT_C:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > C_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lcv_ValueByteNum; i++) {
                for(j=0; j<8; j++) {
                    plc_set_bit_element_value(C_ELEMENT, lsv_ElementAddr+i*8+j, (lcp_Value[i]>>j & 0x01));
                    lsv_ElementCnt --;
                    if(!lsv_ElementCnt) {
                        break;
                    }
                }
            }
            break;

        default:
            pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
            mb_slave_error_resp(pMsg);
            return;
    }

    /*组响应帧*/
    for(i=0; i<6; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->msv_RespLen = 6;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  写多寄存器
  * @param  None
  * @retval None
  */
void mb_slave_write_multiple_registers(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr, lsv_ElementCnt, lsv_ElementAddr;
    unsigned char lcv_ElementType, lcv_ValueByteNum;
    unsigned short i;
    unsigned short *lsp_Value;
    unsigned long llv_C32Value;

    lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[2]);
    lsv_ElementCnt = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[4]);
    lcv_ValueByteNum = pMsg->mcp_ReceiveBuff[6];

    LOGD("mb", "Enter %s(), lsv_ModbusAddr = %u, lsv_ElementCnt = %u, lcv_ValueByteNum = %u", __func__, lsv_ModbusAddr, lsv_ElementCnt, lcv_ValueByteNum);
    if((lsv_ElementCnt > MB_MAX_W_WORD_NUM) || (lsv_ElementCnt < 1)) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    if(lsv_ElementCnt != lcv_ValueByteNum>>1) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    lcv_Ret = mb_slave_convert_element_info(MB_WORD_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
    if(lcv_Ret != pdPASS) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
        mb_slave_error_resp(pMsg);
        return;
    }

    lsp_Value = (unsigned short *)&pMsg->mcp_ReceiveBuff[7];
    LOGD("mb", "lcv_ElementType = %u, lsv_ElementAddr = %u", lcv_ElementType, lsv_ElementAddr);
    switch(lcv_ElementType) {
        case MB_WORD_D:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > D_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                SET_D_ELEMENT_VALUE(lsv_ElementAddr+i, GET_BIGPU16_DATA((uint8_t*)&lsp_Value[i]));
            }
            break;

        case MB_WORD_SD:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > SD_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                /*20170811:需要增加SD元件写入权限校验...*/
                SET_SD_ELEMENT_VALUE(lsv_ElementAddr+i, GET_BIGPU16_DATA((uint8_t*)&lsp_Value[i]));
            }
            break;

        case MB_WORD_Z:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > Z_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                SET_Z_ELEMENT_VALUE(lsv_ElementAddr+i, GET_BIGPU16_DATA((uint8_t*)&lsp_Value[i]));
            }
            break;

        case MB_WORD_T:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > T_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                SET_T_CURRENT_VALUE(lsv_ElementAddr+i, GET_BIGPU16_DATA((uint8_t*)&lsp_Value[i]));
            }
            break;

        case MB_WORD_C:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > C_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            if((lsv_ElementAddr) < C16_RANG) {

                for(i=0; i<lsv_ElementCnt; i++) {
                    SET_C16_CURRENT_VALUE(lsv_ElementAddr+i, GET_BIGPU16_DATA((uint8_t*)&lsp_Value[i]));
                }
            } else {
                lsv_ElementCnt <<= 1;

                for(i=0; i<lsv_ElementCnt; i+=2) {
                    //llv_C32Value = (unsigned long)(lsp_Value[i]<<16) + lsp_Value[i+1];
                    llv_C32Value = GET_BIGPU32_DATA((uint8_t*)&lsp_Value[i]);
                    SET_C32_CURRENT_VALUE(lsv_ElementAddr+i/2, llv_C32Value);
                }
            }
            break;

        case MB_WORD_R:
            if(lsv_ElementAddr + lsv_ElementCnt -1 > R_RANG) {
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
            }

            for(i=0; i<lsv_ElementCnt; i++) {
                SET_R_ELEMENT_VALUE(lsv_ElementAddr+i, GET_BIGPU16_DATA((uint8_t*)&lsp_Value[i]));
            }
            break;

        default:
            pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
            mb_slave_error_resp(pMsg);
            return;
    }

    /*组响应帧*/
    for(i=0; i<6; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->msv_RespLen = 6;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  批量读位元件
  * @param  None
  * @retval None
  */
void mb_slave_read_multiple_bit_element(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr, lsv_ElementAddr;
    unsigned char lcv_ElementCnt, lcv_ElementType;
    unsigned short i;

    /*不支持广播消息*/
    if(pMsg->mcv_IsBroadcastInfo) {
        return;
    }

    lcv_ElementCnt = pMsg->mcp_ReceiveBuff[2];
    if(lcv_ElementCnt > 0x50) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    for(i=0; i<lcv_ElementCnt; i++) {
        pMsg->mcp_RespBuff[3 + 3*i] = pMsg->mcp_ReceiveBuff[3+2*i];
        pMsg->mcp_RespBuff[4 + 3*i] = pMsg->mcp_ReceiveBuff[4+2*i];

        lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[3 + 2*i]);

        lcv_Ret = mb_slave_convert_element_info(MB_BIT_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
        if(lcv_Ret != pdPASS) {
            pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
            mb_slave_error_resp(pMsg);
            return;
        }

        switch(lcv_ElementType) {
            case MB_BIT_Y:
                pMsg->mcp_RespBuff[5 + 3*i] = plc_get_bit_element_value(Y_ELEMENT, lsv_ElementAddr);
                break;
            case MB_BIT_X:
                pMsg->mcp_RespBuff[5 + 3*i] = plc_get_bit_element_value(X_ELEMENT, lsv_ElementAddr);
                break;
            case MB_BIT_M:
                pMsg->mcp_RespBuff[5 + 3*i] = plc_get_bit_element_value(M_ELEMENT, lsv_ElementAddr);
                break;
            case MB_BIT_SM:
                pMsg->mcp_RespBuff[5 + 3*i] = plc_get_bit_element_value(SM_ELEMENT, lsv_ElementAddr);
                break;
            case MB_BIT_S:
                pMsg->mcp_RespBuff[5 + 3*i] = plc_get_bit_element_value(S_ELEMENT, lsv_ElementAddr);
                break;
            case MB_BIT_T:
                pMsg->mcp_RespBuff[5 + 3*i] = plc_get_bit_element_value(T_ELEMENT, lsv_ElementAddr);
                break;
            case MB_BIT_C:
                pMsg->mcp_RespBuff[5 + 3*i] = plc_get_bit_element_value(C_ELEMENT, lsv_ElementAddr);
                break;
            default:
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
        }
    }

    /*组响应帧*/
    for(i=0; i<3; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->msv_RespLen = (lcv_ElementCnt + 1)*3;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  批量读字元件
  * @param  None
  * @retval None
  */
void mb_slave_read_multiple_word_element(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr, lsv_ElementAddr;
    unsigned char lcv_ElementCnt, lcv_ElementType;
    unsigned char i;
    unsigned long llv_C32Value;

    /*不支持广播消息*/
    if(pMsg->mcv_IsBroadcastInfo) {
        return;
    }

    lcv_ElementCnt = pMsg->mcp_ReceiveBuff[2];
    if(lcv_ElementCnt > 0x3C) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    for(i=0; i<lcv_ElementCnt; i++) {
        pMsg->mcp_RespBuff[3 + 4*i] = pMsg->mcp_ReceiveBuff[3 + 2*i];
        pMsg->mcp_RespBuff[4 + 4*i] = pMsg->mcp_ReceiveBuff[4 + 2*i];

        lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[3 + 2*i]);

        lcv_Ret = mb_slave_convert_element_info(MB_WORD_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
        if(lcv_Ret != pdPASS) {
            pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
            mb_slave_error_resp(pMsg);
            return;
        }

        switch(lcv_ElementType) {
            case MB_WORD_D:
                *(unsigned short *)&pMsg->mcp_RespBuff[5 + 4*i] = GET_D_ELEMENT_VALUE(lsv_ElementAddr);
                break;
            case MB_WORD_SD:
                *(unsigned short *)&pMsg->mcp_RespBuff[5 + 4*i] = GET_SD_ELEMENT_VALUE(lsv_ElementAddr);
                break;
            case MB_WORD_Z:
                *(unsigned short *)&pMsg->mcp_RespBuff[5 + 4*i] = GET_Z_ELEMENT_VALUE(lsv_ElementAddr);
                break;
            case MB_WORD_T:
                *(unsigned short *)&pMsg->mcp_RespBuff[5 + 4*i] = GET_T_CURRENT_VALUE(lsv_ElementAddr);
                break;
            case MB_WORD_C:
                if(lsv_ElementAddr < C16_RANG) {
                    *(unsigned short *)&pMsg->mcp_RespBuff[5 + 4*i] = GET_C16_CURRENT_VALUE(lsv_ElementAddr);
                } else {
                    llv_C32Value = GET_C32_CURRENT_VALUE((lsv_ElementAddr - C16_RANG)/2 + C16_RANG);
                    *(unsigned short *)&pMsg->mcp_RespBuff[5 + 4*i] = (unsigned short)(llv_C32Value >> 16);

                    i++;
                    pMsg->mcp_RespBuff[3 + 4*i] = pMsg->mcp_ReceiveBuff[3 + 2*i];
                    pMsg->mcp_RespBuff[4 + 4*i] = pMsg->mcp_ReceiveBuff[4 + 2*i];
                    *(unsigned short *)&pMsg->mcp_RespBuff[5 + 4*i] = (unsigned short)(llv_C32Value);
                }
                break;
            case MB_WORD_R:
                *(unsigned short *)&pMsg->mcp_RespBuff[5 + 4*i] = GET_R_ELEMENT_VALUE(lsv_ElementAddr);
                break;
            default:
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
        }
    }

    /*组响应帧*/
    for(i=0; i<3; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->msv_RespLen = 3 + lcv_ElementCnt*4;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  批量写位元件
  * @param  None
  * @retval None
  */
void mb_slave_write_multiple_bit_element(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr, lsv_ElementAddr;
    unsigned char lcv_ElementCnt, lcv_ElementType;
    unsigned short i;

    /*不支持广播消息*/
    if(pMsg->mcv_IsBroadcastInfo) {
        return;
    }

    lcv_ElementCnt = pMsg->mcp_ReceiveBuff[2];
    if(lcv_ElementCnt > 0x50) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    for(i=0; i<lcv_ElementCnt; i++) {

        lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[3 + 3*i]);

        lcv_Ret = mb_slave_convert_element_info(MB_BIT_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
        if(lcv_Ret != pdPASS) {
            pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
            mb_slave_error_resp(pMsg);
            return;
        }

        switch(lcv_ElementType) {
            /*
            case MB_BIT_X:
                plc_set_bit_element_value(X_ELEMENT, lsv_ElementAddr, pMsg->mcp_ReceiveBuff[5 + 3*i]);
                break;
                */
            case MB_BIT_Y:
                plc_set_bit_element_value(Y_ELEMENT, lsv_ElementAddr, pMsg->mcp_ReceiveBuff[5 + 3*i]);
                break;
            case MB_BIT_M:
                plc_set_bit_element_value(M_ELEMENT, lsv_ElementAddr, pMsg->mcp_ReceiveBuff[5 + 3*i]);
                break;
            case MB_BIT_SM:
                /*20170811: 需要增加SM元件写入权限校验...*/
                plc_set_bit_element_value(SM_ELEMENT, lsv_ElementAddr, pMsg->mcp_ReceiveBuff[5 + 3*i]);
                break;
            case MB_BIT_S:
                plc_set_bit_element_value(S_ELEMENT, lsv_ElementAddr, pMsg->mcp_ReceiveBuff[5 + 3*i]);
                break;
            case MB_BIT_T:
                plc_set_bit_element_value(T_ELEMENT, lsv_ElementAddr, pMsg->mcp_ReceiveBuff[5 + 3*i]);
                break;
            case MB_BIT_C:
                plc_set_bit_element_value(C_ELEMENT, lsv_ElementAddr, pMsg->mcp_ReceiveBuff[5 + 3*i]);
                break;
            default:
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
        }
    }

    /*组响应帧*/
    for(i=0; i<2; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->msv_RespLen = 2;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  错误帧组帧发送
  * @param  None
  * @retval None
  */
void mb_slave_write_multiple_word_element(md_slave_msg_pack *pMsg)
{
    unsigned char lcv_Ret;
    unsigned short lsv_ModbusAddr, lsv_ElementAddr;
    unsigned char lcv_ElementCnt, lcv_ElementType;
    unsigned char i;
    unsigned long llv_C32Value;

    /*不支持广播消息*/
    if(pMsg->mcv_IsBroadcastInfo) {
        return;
    }

    lcv_ElementCnt = pMsg->mcp_ReceiveBuff[2];
    if(lcv_ElementCnt > 0x3C) {
        pMsg->mcv_ErrorCode = MB_ILIEGAL_DATA;
        mb_slave_error_resp(pMsg);
        return;
    }

    for(i=0; i<lcv_ElementCnt; i++) {

        lsv_ModbusAddr = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[3 + 4*i]);

        lcv_Ret = mb_slave_convert_element_info(MB_WORD_ELEMENT, lsv_ModbusAddr, &lcv_ElementType, &lsv_ElementAddr);
        if(lcv_Ret != pdPASS) {
            pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
            mb_slave_error_resp(pMsg);
            return;
        }

        switch(lcv_ElementType) {
            case MB_WORD_D:
                SET_D_ELEMENT_VALUE(lsv_ElementAddr, GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[5 + 4*i]));
                break;
            case MB_WORD_SD:
                /*20170811: 需要增加SD元件写入权限校验...*/
                SET_SD_ELEMENT_VALUE(lsv_ElementAddr, GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[5 + 4*i]));
                break;
            case MB_WORD_Z:
                SET_Z_ELEMENT_VALUE(lsv_ElementAddr, GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[5 + 4*i]));
                break;
            case MB_WORD_T:
                SET_T_CURRENT_VALUE(lsv_ElementAddr, GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[5 + 4*i]));
                break;
            case MB_WORD_C:
                if(lsv_ElementAddr < C16_RANG) {
                    SET_C16_CURRENT_VALUE(lsv_ElementAddr, GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[5 + 4*i]));
                } else {
                    llv_C32Value = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[5 + 4*i]);
                    i++;

                    llv_C32Value <<= 16;
                    llv_C32Value += GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[5 + 4*i]);

                    SET_C32_CURRENT_VALUE(((lsv_ElementAddr - C16_RANG)/2 + C16_RANG), llv_C32Value);
                }
                break;
            case MB_WORD_R:
                SET_R_ELEMENT_VALUE(lsv_ElementAddr, GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[5 + 4*i]));
                break;
            default:
                pMsg->mcv_ErrorCode = MB_ILIEGAL_ADDR;
                mb_slave_error_resp(pMsg);
                return;
        }
    }

    /*组响应帧*/
    for(i=0; i<2; i++)
        pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];

    pMsg->msv_RespLen = 2;
    mb_slave_verify_resp_msg(pMsg);
}

/**
  * @brief  Modbus 诊断校验,清除控制计数器及诊断寄存器
  * @param  None
  * @retval None
  */
void mb_slave_diag_clear_ctrl_diag_reg(void)
{
    unsigned char i;

    for(i=0; i<MB_SENDER_MAX; i++) {
        gtp_ModbusSlaveDiagInfo[i].mcv_ListenOnlyMode = 0;
        gtp_ModbusSlaveDiagInfo[i].msv_BusPackageCnt = 0;
        gtp_ModbusSlaveDiagInfo[i].msv_BusCrcErrCnt = 0;
        gtp_ModbusSlaveDiagInfo[i].msv_SlaveErrCnt = 0;
        gtp_ModbusSlaveDiagInfo[i].msv_SlavePackageCnt = 0;
        gtp_ModbusSlaveDiagInfo[i].msv_SlaveNoRespCnt = 0;
        gtp_ModbusSlaveDiagInfo[i].msv_BusCharOverrunCnt = 0;
    }
}

/**
  * @brief  Modbus 诊断校验
  * @param  None
  * @retval None
  */
void mb_slave_diagnostic_func(md_slave_msg_pack *pMsg)
{
    unsigned short i;
    unsigned short lsv_Data;
    unsigned short lsv_Code;

    lsv_Code = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[2]);
    lsv_Data = GET_BIGPU16_DATA(&pMsg->mcp_ReceiveBuff[4]);

    /*广播消息，非支持功能码处理*/
    if(pMsg->mcv_IsBroadcastInfo) {
        if((lsv_Code != MB_DIAG_RESTART_COMMUNICATION)
           && (lsv_Code != MB_DIAG_FORCE_LISTEN_ONLY_MODE)
           && (lsv_Code != MB_DIAG_CLEAR_CTRL_DIAG_REG)
           && (lsv_Code != MB_DIAG_BROAD_RETURN_QUERY_DATA)) {
            gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_SlaveErrCnt ++;
            return;
        }
    }

    switch(pMsg->mcp_ReceiveBuff[3]) {
        /*广播请求帧返回*/
        case MB_DIAG_BROAD_RETURN_QUERY_DATA:
            for(i=1; i<pMsg->msv_ReceiveLen-2; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }
            pMsg->mcp_RespBuff[0] = pMsg->slaveID;
            pMsg->msv_RespLen = pMsg->msv_ReceiveLen-2;
            mb_slave_verify_resp_msg(pMsg);
            break;

        /*请求帧返回*/
        case MB_DIAG_RETURN_QUERY_DATA:
            for(i=0; i<pMsg->msv_ReceiveLen-2; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }

            pMsg->msv_RespLen = pMsg->msv_ReceiveLen-2;
            mb_slave_verify_resp_msg(pMsg);
            break;

        /*重启通讯选项*/
        case MB_DIAG_RESTART_COMMUNICATION:
            if((lsv_Data == 0x0000) || (lsv_Data == 0xFF00)) {
                gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].mcv_ListenOnlyMode = 0;

                for(i=0; i<pMsg->msv_ReceiveLen-2; i++) {
                    pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
                }

                pMsg->msv_RespLen = pMsg->msv_ReceiveLen-2;
                mb_slave_verify_resp_msg(pMsg);

            } else if(gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].mcv_ListenOnlyMode) {
                /*Nothing*/
                ;
            } else {
                gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_SlaveErrCnt ++;
                pMsg->mcv_ErrorCode = MB_ILIEGAL_CODE;
                mb_slave_error_resp(pMsg);
            }
            break;

        /*进入只听模式*/
        case MB_DIAG_FORCE_LISTEN_ONLY_MODE:
            gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].mcv_ListenOnlyMode = 1;
            break;

        /*清除计数器及诊断寄存器*/
        case MB_DIAG_CLEAR_CTRL_DIAG_REG:
            mb_slave_diag_clear_ctrl_diag_reg();

            for(i=0; i<pMsg->msv_ReceiveLen-2; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }

            pMsg->msv_RespLen = pMsg->msv_ReceiveLen-2;
            mb_slave_verify_resp_msg(pMsg);
            break;

        /*返回总线报文计数*/
        case MB_DIAG_RETURN_BUS_PACKAGE_CNT:
            if(gtp_ModbusSlaveDiagInfo[0].mcv_SlaveId == gtp_ModbusSlaveDiagInfo[1].mcv_SlaveId) {
                lsv_Data = gtp_ModbusSlaveDiagInfo[0].msv_BusPackageCnt + gtp_ModbusSlaveDiagInfo[1].msv_BusPackageCnt -
                           gtp_ModbusSlaveDiagInfo[0].msv_BusCrcErrCnt - gtp_ModbusSlaveDiagInfo[1].msv_BusCrcErrCnt;
            } else {
                lsv_Data = gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_BusPackageCnt - gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_BusCrcErrCnt;
            }

            for(i=0; i<4; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }

            pMsg->mcp_RespBuff[i++] = lsv_Data >> 8;
            pMsg->mcp_ReceiveBuff[i++] = (lsv_Data & 0xFF);

            pMsg->msv_RespLen = i;
            mb_slave_verify_resp_msg(pMsg);
            break;

        /*返回总线CRC报文计数*/
        case MB_DIAG_RETURN_CRC_ERR_CNT:
            if(gtp_ModbusSlaveDiagInfo[0].mcv_SlaveId == gtp_ModbusSlaveDiagInfo[1].mcv_SlaveId) {
                lsv_Data = gtp_ModbusSlaveDiagInfo[0].msv_BusCrcErrCnt + gtp_ModbusSlaveDiagInfo[1].msv_BusCrcErrCnt;
            } else {
                lsv_Data = gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_BusCrcErrCnt;
            }

            for(i=0; i<4; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }

            pMsg->mcp_RespBuff[i++] = lsv_Data >> 8;
            pMsg->mcp_ReceiveBuff[i++] = (lsv_Data & 0xFF);

            pMsg->msv_RespLen = i;
            mb_slave_verify_resp_msg(pMsg);
            break;

        /*返回从站异常差错计数*/
        case MB_DIAG_SLAVE_ERR_CNT:
            if(gtp_ModbusSlaveDiagInfo[0].mcv_SlaveId == gtp_ModbusSlaveDiagInfo[1].mcv_SlaveId) {
                lsv_Data = gtp_ModbusSlaveDiagInfo[0].msv_SlaveErrCnt + gtp_ModbusSlaveDiagInfo[1].msv_SlaveErrCnt;
            } else {
                lsv_Data = gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_SlaveErrCnt;
            }

            for(i=0; i<4; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }

            pMsg->mcp_RespBuff[i++] = lsv_Data >> 8;
            pMsg->mcp_ReceiveBuff[i++] = (lsv_Data & 0xFF);

            pMsg->msv_RespLen = i;
            mb_slave_verify_resp_msg(pMsg);
            break;

        /*返回从站报文计数*/
        case MB_DIAG_SLAVE_BUS_MSG_CNT:
            if(gtp_ModbusSlaveDiagInfo[0].mcv_SlaveId == gtp_ModbusSlaveDiagInfo[1].mcv_SlaveId) {
                lsv_Data = gtp_ModbusSlaveDiagInfo[0].msv_SlavePackageCnt + gtp_ModbusSlaveDiagInfo[1].msv_SlavePackageCnt;
            } else {
                lsv_Data = gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_SlavePackageCnt;
            }

            for(i=0; i<4; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }

            pMsg->mcp_RespBuff[i++] = lsv_Data >> 8;
            pMsg->mcp_ReceiveBuff[i++] = (lsv_Data & 0xFF);

            pMsg->msv_RespLen = i;
            mb_slave_verify_resp_msg(pMsg);
            break;

        /*返回从站无响应计数*/
        case MB_DIAG_SLAVE_NO_RESP_CNT:
            if(gtp_ModbusSlaveDiagInfo[0].mcv_SlaveId == gtp_ModbusSlaveDiagInfo[1].mcv_SlaveId) {
                lsv_Data = gtp_ModbusSlaveDiagInfo[0].msv_SlaveNoRespCnt + gtp_ModbusSlaveDiagInfo[1].msv_SlaveNoRespCnt;
            } else {
                lsv_Data = gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_SlaveNoRespCnt;
            }

            for(i=0; i<4; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }

            pMsg->mcp_RespBuff[i++] = lsv_Data >> 8;
            pMsg->mcp_ReceiveBuff[i++] = (lsv_Data & 0xFF);

            pMsg->msv_RespLen = i;
            mb_slave_verify_resp_msg(pMsg);
            break;

        /*返回总线字符超限计数*/
        case MB_DIAG_BUS_CHAR_OVERRUN_CNT:
            if(gtp_ModbusSlaveDiagInfo[0].mcv_SlaveId == gtp_ModbusSlaveDiagInfo[1].mcv_SlaveId) {
                lsv_Data = gtp_ModbusSlaveDiagInfo[0].msv_BusCharOverrunCnt + gtp_ModbusSlaveDiagInfo[1].msv_BusCharOverrunCnt;
            } else {
                lsv_Data = gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_BusCharOverrunCnt;
            }

            for(i=0; i<4; i++) {
                pMsg->mcp_RespBuff[i] = pMsg->mcp_ReceiveBuff[i];
            }

            pMsg->mcp_RespBuff[i++] = lsv_Data >> 8;
            pMsg->mcp_ReceiveBuff[i++] = (lsv_Data & 0xFF);

            pMsg->msv_RespLen = i;
            mb_slave_verify_resp_msg(pMsg);
            break;

        default:
            gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_SlaveErrCnt ++;
            pMsg->mcv_ErrorCode = MB_ILIEGAL_CODE;
            mb_slave_error_resp(pMsg);
    }
}

/**
  * @brief  错误帧组帧发送
  * @param  None
  * @retval None
  */
void mb_slave_error_resp(md_slave_msg_pack *pMsg)
{
    unsigned short lsv_Crc;

    if(pMsg->mcv_IsBroadcastInfo) {
        return;
    }

    pMsg->mcp_RespBuff[0] = pMsg->mcp_ReceiveBuff[0];
    pMsg->mcp_RespBuff[1] = pMsg->mcp_ReceiveBuff[1] + 0x80;
    pMsg->mcp_RespBuff[2] = pMsg->mcv_ErrorCode;

    lsv_Crc = calc_crc16(pMsg->mcp_RespBuff, 3);

    /*小端序,低字节在前*/
    pMsg->mcp_RespBuff[3] = (unsigned char)lsv_Crc;
    pMsg->mcp_RespBuff[4] = (unsigned char)(lsv_Crc >> 8);
    pMsg->msv_RespLen = 5;

    if (pMsg->mcv_Sender == MB_SENDER_USB)
    {
        pMsg->resp_func(pMsg->mcp_RespBuff, pMsg->msv_RespLen);
        return;
    }
    if(pMsg->resp_func != NULL)
    {
        pMsg->resp_func(pMsg->mcp_RespBuff, pMsg->msv_RespLen);
    }
    if (pMsg->isTcpClient)
    {
        if (pMsg->tcp_resp_func != NULL)
        {
            pMsg->tcp_resp_func(pMsg->mcp_RespBuff, pMsg->msv_RespLen, pMsg->clientID);
        }
    }
}

/**
  * @brief  响应帧组帧发送
  * @param  None
  * @retval None
  */
void mb_slave_verify_resp_msg(md_slave_msg_pack *pMsg)
{
    //PRINTF("Enter %s\r\n", __func__);
    unsigned short lsv_Crc;

    lsv_Crc = calc_crc16(pMsg->mcp_RespBuff, pMsg->msv_RespLen);
    pMsg->mcp_RespBuff[pMsg->msv_RespLen] = (unsigned char)(lsv_Crc&0xFF);
    pMsg->mcp_RespBuff[pMsg->msv_RespLen+1] = (unsigned char)(lsv_Crc>>0x08);
    pMsg->msv_RespLen += 2;

    if (pMsg->mcv_Sender == MB_SENDER_USB)
    {
        pMsg->resp_func(pMsg->mcp_RespBuff, pMsg->msv_RespLen);
        return;
    }

    if(pMsg->resp_func != NULL)
    {
        pMsg->resp_func(pMsg->mcp_RespBuff, pMsg->msv_RespLen);
    }
    if (pMsg->isTcpClient)
    {
        if (pMsg->tcp_resp_func != NULL)
        {
            pMsg->tcp_resp_func(pMsg->mcp_RespBuff, pMsg->msv_RespLen, pMsg->clientID);
        }
    }
}

/**
  * @brief  mb_slave_msg_handler
  * @param  None
  * @retval None
  */
void mb_slave_msg_handler(md_slave_msg_pack *pMsg)
{
    //PRINTF("Enter %s\r\n", __func__);
    if (pMsg->msv_ReceiveLen < 5)
    {
        LOGE("mb", "msv_ReceiveLen(%u) < 5, so just RETURN!\r\n", pMsg->msv_ReceiveLen);
        if (pMsg->msv_ReceiveLen != 0)
        {
            hexdump(pMsg->mcp_ReceiveBuff, pMsg->msv_ReceiveLen);
        }
        return;
    }
    
    unsigned short lsv_Crc, lsv_CurCrc;

    /*喂狗*/
    bsp_feed_watch_dog();

    /*帧数据CRC校验*/
    lsv_CurCrc = *(unsigned short *)(pMsg->mcp_ReceiveBuff + pMsg->msv_ReceiveLen-2);
    lsv_Crc = calc_crc16(pMsg->mcp_ReceiveBuff, pMsg->msv_ReceiveLen-2);
    //LOGW("mb", "CRC recv: 0x%X, CRC calc: 0x%X", lsv_CurCrc, lsv_Crc);
    if(lsv_CurCrc != lsv_Crc) {
        gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_BusCrcErrCnt++;
        pMsg->mcv_ErrorCode = MB_ERROR_FRAME;
        mb_slave_error_resp(pMsg);
        return;
    }

    /*只听模式判断*/
    if(gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].mcv_ListenOnlyMode) {
        if((pMsg->mcp_ReceiveBuff[1]!=MB_DIAG_DIAGNOSTIC)
           && (pMsg->mcp_ReceiveBuff[3]!=MB_DIAG_RESTART_COMMUNICATION)) {
            return;
        }
    }

    switch(pMsg->mcp_ReceiveBuff[1]) {

        /*读线圈状态*/
        case MB_READ_COILS_STATUS:
            mb_slave_read_coils_status(pMsg);
            break;
        /*读离散输入量状态*/
        case MB_READ_DESCRETE_INPUT_STATUS:
            mb_slave_read_descrete_input_status(pMsg);
            break;
        /*读寄存器值*/
        case MB_READ_HOLDING_REGISTER:
            mb_slave_read_holding_register(pMsg);
            break;
        /*写单线圈*/
        case MB_WRITE_SINGLE_COIL:
            mb_slave_write_single_coil(pMsg);
            break;
        /*写寄存器*/
        case MB_WRITE_REGISTER:
            mb_slave_write_register(pMsg);
            break;
        /*回送诊断校验*/
        case MB_DIAG_DIAGNOSTIC:
            mb_slave_diagnostic_func(pMsg);
            break;
        /*写多个线圈*/
        case MB_WRITE_MULTIPLE_COILS:
            mb_slave_write_multiple_coils(pMsg);
            break;
        /*写多个寄存器*/
        case MB_WRITE_MULTIPLE_REGISTERS:
            mb_slave_write_multiple_registers(pMsg);
            break;
        /*批量读位元件*/
        case MB_READ_MULTIPLE_BIT_ELEMENT:
            mb_slave_read_multiple_bit_element(pMsg);
            break;
        /*批量读字元件*/
        case MB_READ_MULTIPLE_WORD_ELEMENT:
            mb_slave_read_multiple_word_element(pMsg);
            break;
        /*批量写位元件*/
        case MB_WRITE_MULTIPLE_BIT_ELEMENT:
            mb_slave_write_multiple_bit_element(pMsg);
            break;
        /*批量写字元件*/
        case MB_WRITE_MULTIPLE_WORD_ELEMENT:
            mb_slave_write_multiple_word_element(pMsg);
            break;
        /*下载*/
        case MB_DOWNLOWD_FUNC:
            mb_slave_download_manage(pMsg);
            break;
        /*上载*/
        case MB_UPLOAD_FUNC:
            mb_slave_upload_manage(pMsg);
            //resume_task_after_download_ucode();
            break;
        /*控制*/
        case MB_CTRL_FUNC:
            mb_slave_ctrl_manage(pMsg);
            break;

        default:
            gtp_ModbusSlaveDiagInfo[pMsg->mcv_Sender].msv_SlaveErrCnt ++;
            pMsg->mcv_ErrorCode = MB_ILIEGAL_CODE;
            mb_slave_error_resp(pMsg);
    }
}

