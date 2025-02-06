/********************************** (C) COPYRIGHT *******************************
* File Name          : DataFlash.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        : CH554 DataFlash字节读写函数定义   
*******************************************************************************/

#include "ch554.h"                                                         
#include "dataflash.h"

/*******************************************************************************
* Function Name  : WriteDataFlash(unsigned char Addr,Punsigned char buf,unsigned char len)
* Description    : DataFlash写
* Input          : unsigned char Addr，PUINT16 buf,unsigned char len
* Output         : None
* Return         : unsigned char i 返回写入长度
*******************************************************************************/
unsigned char WriteDataFlash(unsigned char Addr,unsigned char* buf,unsigned char len)
{
    unsigned char i;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;                                                           //进入安全模式
    GLOBAL_CFG |= bDATA_WE;                                                    //使能DataFlash写
    SAFE_MOD = 0;                                                              //退出安全模式	
		ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    Addr <<= 1;
    for(i=0;i<len;i++)
	  {
        ROM_ADDR_L = Addr + i*2;
        ROM_DATA_L = *(buf+i);			
        if ( ROM_STATUS & bROM_ADDR_OK ) {                                     // 操作地址有效
           ROM_CTRL = ROM_CMD_WRITE;                                           // 写入
        }
        if((ROM_STATUS ^ bROM_ADDR_OK) > 0) return i;                          // 返回状态,0x00=success,  0x02=unknown command(bROM_CMD_ERR)
	  }
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;                                                           //进入安全模式
    GLOBAL_CFG &= ~bDATA_WE;                                                   //开启DataFlash写保护
    SAFE_MOD = 0;                                                              //退出安全模式	
    return i;		
}

/*******************************************************************************
* Function Name  : ReadDataFlash(unsigned char Addr,unsigned char len,Punsigned char buf)
* Description    : 读DataFlash
* Input          : unsigned char Addr unsigned char len Punsigned char buf
* Output         : None
* Return         : unsigned char i 返回写入长度
*******************************************************************************/
unsigned char ReadDataFlash(unsigned char Addr,unsigned char len,unsigned char *buf)
{
    unsigned char i;
    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    Addr <<= 1;
    for(i=0;i<len;i++){
	  ROM_ADDR_L = Addr + i*2;                                          
	  ROM_CTRL = ROM_CMD_READ;
	  *(buf+i) = ROM_DATA_L;
		}
    return i;
}