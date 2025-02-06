/*******************************************************************************
* Function Name  : WriteDataFlash(UINT16 Addr,Punsigned char buf,unsigned char len)
* Description    : DataFlashĐ´
* Input          : UINT16 AddrŁŹPUINT16 buf,unsigned char len 
* Output         : None 
* Return         : 
*******************************************************************************/
unsigned char WriteDataFlash(unsigned char Addr,unsigned char *buf,unsigned char len);

/*******************************************************************************
* Function Name  : ReadDataFlash(unsigned char Addr,unsigned char len,Punsigned char buf)
* Description    : śÁDataFlash
* Input          : UINT16 Addr Punsigned char buf
* Output         : None
* Return         : unsigned char l
*******************************************************************************/
unsigned char ReadDataFlash(unsigned char Addr,unsigned char len,unsigned char *buf);