
//
//��ȫģ�����ݽ��շ��ͳ���,���� I2C �ӿ�ģʽ//
//�������Ϊʾ������,����Щ�źŵ��ж�δ�ӳ�ʱ�ж�
//

#include "../at89x52.h"
#include <intrins.h>
#include "secure.h"

#define TX_FRAME   P2_6
#define SDAT	   P2_5
#define SCLK 	   P2_4
#define RX_FRAME   P2_7

void delay_nop()
{
 	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
 	_nop_();
	_nop_();




}

#if 0
// Receive  Data to Secure Module
unsigned char  read_sec(unsigned char data * dat)
{
	unsigned char temp,i,len;
	len =0;
	while (TX_FRAME)
   {
		
		*dat =0; temp =0x01;	  			
		for (i=0;i<8;i++)
		{    
			while(!SCLK); 			
			if ( SDAT)
			(*dat)|= temp;								   			
  			temp <<=1;
			while(SCLK);
		}		
		SDAT =0;
		while (!SCLK);
		while (SCLK);
		SDAT =1; 	
		dat++;
		len++;
   }
   SDAT =1;
   SCLK =1;
   return len;
}
#endif

void init_i2c()
{

  RX_FRAME=0;
  SCLK =1;
  SDAT =1;
}
unsigned char  read_sec(unsigned char idata * dat)
{
	unsigned char temp,len;
	len =0;
	EA = 0;
	// �ȴ���������
	while(TX_FRAME==0);	   
    while (SCLK);
	// ��ʼ��������,��������ݽϿ�,���Բ���ÿ��bit ˳�����,δ����ѭ���ķ���
	while (TX_FRAME)
   {
	   	temp =0x00;			
		{ 			
			while(!SCLK); 			
			if ( SDAT)
				temp = temp | 0x80;
			while(SCLK)
			{
				if (!TX_FRAME) goto retn;
			}			

		}
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp = temp | 0x40;
			while(SCLK)
			{
				if (!TX_FRAME) goto retn;
			}
		}
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp = temp | 0x20;
			while(SCLK)
			{
				if (!TX_FRAME) goto retn;
			}
		}
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp = temp | 0x10;
			while(SCLK)
			{
				if (!TX_FRAME) goto retn;
			}
		}	
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp = temp | 0x08;
			while(SCLK)
			{
				if (!TX_FRAME) goto retn;
			}
		}	
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp = temp | 0x04;
			while(SCLK)
			{
				if (!TX_FRAME) goto retn;
			}
		}			
	    {    
			while(!SCLK); 			
			if ( SDAT)
				temp = temp | 0x02;
			while(SCLK)
			{
				if (!TX_FRAME) goto retn;
			}
		}	
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp = temp | 0x01;
			while(SCLK)
			{
				if (!TX_FRAME) goto retn;
			}
		}	
		SDAT =0;
		while (!SCLK);
		*dat =temp;
		while (SCLK)
		{
				if (!TX_FRAME) goto retn;
		}
		SDAT =1; 	
		dat++;
		len++;
   }
retn:
   SDAT =1;
   SCLK =1;
   EA =1;
   return len;
}



// 	Send data To Secure Module
void  write_sec(unsigned char * dat,unsigned short len)
{
	 unsigned char temp,i,bak;
	 EA =0;
	 // ��ʼ��������
	 RX_FRAME =1;

	 delay_nop();
	 SDAT =0;
	 delay_nop();
	 SCLK =0;
	 while (len--)
	 {
	     bak =*dat;
	 	 temp =0x80;	
		 for (i=0;i<8;i++)
		{    
			
			
			if (bak & temp)
			{
			 	SDAT =1;
			}else{
				SDAT =0;
			}
			temp >>= 1;		
			SCLK =1;
			delay_nop();
			SCLK =0;
			delay_nop();				
			
		}
		
		SDAT =1;
		delay_nop();
		SCLK= 1;
		delay_nop();
		SCLK =0;
		dat++;	
	}
	SCLK =1;
	SDAT =1;
	RX_FRAME = 0;
	EA =1;
}
