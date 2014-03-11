#include <stdio.h>
#include "STC15F2K08S2.h"
#include "spi.h"
#include "secure.h"
#include "common.h"
#include "packet.h"

#define MAX_BUFSIZE 160

void SAM_packet_handler(uint8* buf,int size);
void PRIVATE_packet_handler(uint8* buf,int size);
void Delay1ms(void)		//@13.56MHz
{
	unsigned char i, j;

	i = 14;
	j = 45;
	do
	{
		while (--j);
	} while (--i);
}
void DelayMs(int ms){
	while(ms-->0) Delay1ms();
}
/*
 * idr work flow
 *			  HOST(PC-like uart terminal)
 *             /|\              
 *              |  	
 * THM3060<--->MCU<----->SAM
 *
 * THM3060 is RF non contact ISO/IEC14443 A/B ,ISO/IEC15693 card reader
 * SAM is ID2 card security module
 *
 * Interface between THM3060 and MCU is SPI
 * Interface between SAM and MCU are I2C and UART
 * Interface between HOST and MCU is UART
 *
 * For MCU STC15F2K08S2 based application:
 *     SPI and I2C is gpio simulated.
 *     UART1 is for HOST&MCU
 *     UART2 is for SAM&MCU
 * 
*/


//main function  

void main()
{
	unsigned char idata buf[160];
	unsigned char len; 	
	unsigned char prot;
	GenPacket p;
	init_i2c();
	uart1_init();
	uart2_init();
	AUXR = 0x00;	
    reset_prf();				//SPI ģʽоƬ��λ
    write_reg(TMRL,0x08);    	//���������Ӧʱ��,��λ302us

	while(1)
 	{   	
	  //step 1:read uart1 
	  len = uart1_read(buf,MAX_BUFSIZE);


	  //step 2:check it's valid command
	  if(len<=0){
	  	 DelayMs(100);
		 continue;
	  }
	  p.pbuf = buf;
	  p.plen = len;
	  prot = packet_protocol(&p);
	  //step 3:command dispatch
	  //secure card command 
	  //read prf command
	  //mcu command
	  if(PPROT_SAM==prot){
	  	//just write to secure
		SAM_packet_handler(buf,len);
	  }else if(PPROT_PRIV==prot){
	  	PRIVATE_packet_handler(buf,len);
	  }else {
	  	//just drop ???
	  }

  	}  	 
}


void SAM_packet_handler(uint8* buf,int size){
	unsigned char len;
	unsigned char i;
	uart2_write(buf,size);
	//reuse buf;
	len= read_sec(buf); 	 			//������ģ��
	if (len!=0)
	{
		if (buf[0]==0x05)
		{
			close_prf();				 //�ر���Ƶ
			for (i=0;i<255;i++);			    
			open_prf();					 //����Ƶ
			for (i=0;i<255;i++);
        }

	  	EA =0;
		write_prf( buf,len);			 //��������
		EA =1;
		len = read_prf(buf);			 //�������֤��������Ϣ
            buf[len]=0x00; 				 //�����ݺ�һλ0����ΪCRCУ����Ϣ
	  		len =len+1;
			    
	        write_sec(buf,len);			 //д����ģ��
	}

	//read from uart2 (for SAM return)
	//and write it to uart1
	do {
		DelayMs(10);//fine tune?
    	len = uart2_read(buf,MAX_BUFSIZE);
		if(len>0)
			uart1_write(buf,len);
	}while(len>0);

}

void PRIVATE_packet_handler(uint8* buf,int size){
	if(buf){
	}
	if(size){
	}
}

