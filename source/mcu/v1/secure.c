/*
 *All Rights Reserved 2014, XingHuo Info Inc,.
*/
//
//安全模块数据接收发送程序,采用 I2C 接口模式//
//本程序仅为示例程序,对有些信号的判断未加超时判断
//

#include "STC15F2K08S2.h"
#include <intrins.h>
#include "secure.h"
#include "common.h"

/*
#define TX_FRAME   P2_6
#define SDAT	   P2_5
#define SCLK 	   P2_4
#define RX_FRAME   P2_7
*/
//new porting on idr board
#define TX_FRAME   P26
#define SDAT	   P25
#define SCLK 	   P24
#define RX_FRAME   P27

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

void init_i2c()
{

  RX_FRAME=0;
  SCLK =1;
  SDAT =1;
}
unsigned char  read_sec(unsigned char idata* dat)
{
	unsigned char idata temp,len;
	unsigned char idata c1,c2;
	len =0;
	c1=c2=0;
	EA = 0;
	// 等待接收数据 because some commands may handled by SAM itself,
	//we just wait a litte time out here
	while(TX_FRAME==0){
		Delay1us();
		if(++c1>=255) {c2++;c1=0;}
		if(c2>=20) goto retn;
	}
	//once TX_FRAME is high,we should disable interrutp to simulate i2c xfer

    while(SCLK);

	// 开始接收数据,因接收数据较快,所以采用每个bit 顺序接收,未采用循环的方法
	while (TX_FRAME)
   {
	   	temp =0x00;			
		{ 			
			while(!SCLK); 			
			if ( SDAT)
				temp |= 0x80;
			while(SCLK){ if (!TX_FRAME) goto retn;}
		}
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp|= 0x40;
			while(SCLK);
		}
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp |= 0x20;
			while(SCLK);
		}
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp |= 0x10;
			while(SCLK);
		}	
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp |= 0x08;
			while(SCLK);
		}
		{
			while(!SCLK); 			
			if ( SDAT)
				temp |= 0x04;
			while(SCLK);
		}			
	    {    
			while(!SCLK); 			
			if ( SDAT)
				temp |= 0x02;
			while(SCLK);
		}	
		{    
			while(!SCLK); 			
			if ( SDAT)
				temp |= 0x01;
			while(SCLK);
		}
		{
			//ninth clock
			SDAT =0;
			while (!SCLK);
			*dat++ =temp;
			while (SCLK);
			SDAT =1; 	
			len++;
		}
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
	 // 开始发送数据
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
