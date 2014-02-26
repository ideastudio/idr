/////////////////////////////////////////////////////////////////////////////////
//
//           
//        T H M 3 0 6 0      S P I   I N T E R F A C E   P R O G R A M  
//
// Project:           THM3060 DEMO
//
//
// resource usage:
//
// history:
//                    Created by DingYM 2009.08.11
// 
// note:
//          
// 			
//   (C)TONGFANG  Electronics  2009.08   All rights are reserved. 
//
/////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <intrins.h>
#include "..\at89x52.h"
#include "spi.h"


// �˴��û����Ը����Լ�Ӧ�ý����޸�
#define SS_N       P1_0
#define SPI_CLK    P1_1	   
#define MOSI	   P1_2
#define MISO       P1_3
#define POR        P1_5	   



// THM3060 �Ĵ�����ַ
#define DATA       0
#define PSEL	   1
#define FCONB	   2
#define EGT		   3
#define CRCSEL	   4
#define RSTAT	   5
#define SCON_60	   6
#define INTCON	   7
#define RSCH       8
#define RSCL	   9
#define CRCH	   0X0A
#define CRCL	   0X0B
#define TMRH	   0X0C
#define TMRL	   0X0D
#define BPOS	   0X0E
#define SMOD	   0X10
#define PWTH	   0X11




//SPI ģʽоƬ��λ
void reset_prf()
{

	SPI_CLK =0;	     //���� SPI_CLK ��̬Ϊ��
	SS_N   = 1;		 //SPI �ӻ�Ƭѡ������Ч
	POR = 0;		 //��λ
	POR = 1;  

}
	

//����һ���ֽ�

static void send_byte(unsigned char dat)
{
   unsigned char i;
   for (i =0;i<8;i++)
   {
		SPI_CLK = 0;
		 _nop_();
		 _nop_();
		 _nop_();
		 _nop_();
		if (dat & 0x80)
			MOSI = 1;
		 else
		 	MOSI = 0;
		 dat = dat << 1;
	     SPI_CLK =1;
		 _nop_();
		 _nop_();
		 _nop_();
		 _nop_();

	   }
   SPI_CLK =0;
}

// ����һ���ֽ�
static unsigned char recv_byte()
{
    unsigned char i,dat,temp;
	SPI_CLK =0;
	dat =0; temp =0x80;
	for (i=0;i<8;i++)
	{
		SPI_CLK =1;
	   	 _nop_();
		 _nop_();
		 _nop_();
		 _nop_();
		if ( MISO)
			dat|= temp;					    
		SPI_CLK =0;
		temp >>= 1;		
		 _nop_();
		 _nop_();
		 _nop_();
		 _nop_();
	}
	return dat;
}

// ͨ��SPI ������ THM3060 д�� num ���ȸ��ֽ�
static void send_buff(unsigned char *buf,unsigned int num)
{
 	if ((buf== NULL)||(num ==0)) return;
	while( num--)
	{
	 	send_byte(*buf++);
	}  
}	   

// ͨ��SPI ������ THM3060 ���� num ���ȸ��ֽ�
static void receive_buff(unsigned char *buf,unsigned int num)
{
	if ((buf== NULL)||(num ==0)) return;
	while(num--)
	{
		*buf++ = recv_byte();	
	 }
}


// ͨ�� SPI ���߶� address �Ĵ�����ֵ
extern unsigned char read_reg(unsigned char address)
{
	unsigned char temp_buff[1];
	SS_N = 0;
	temp_buff[0] = address & 0x7F;	
	send_buff(temp_buff,1);
	receive_buff(temp_buff,1);
	SS_N = 1;
	return(temp_buff[0]);
}

// ͨ�� SPI ����д address �Ĵ�����ֵ
extern void write_reg(unsigned char address,unsigned char content)
{
	unsigned char temp_buff[2];
	temp_buff[0] = address | 0x80;
	temp_buff[1] = content;
	SS_N = 0;
	send_buff(temp_buff,2);
	SS_N = 1;
}

/////////////////////////////////////////////////////////////////////////////
//
//   ��������û���Ҫ����
//
/////////////////////////////////////////////////////////////////////////////


// SPI ģʽ������������,���ݴ��ڻ����� buffer �У�����ֵΪ�յ�����

extern unsigned int read_prf(unsigned char *buffer)
{
	unsigned char temp;
	unsigned int num =0;
	
	temp = read_reg(RSTAT);
	    
	while( (temp &0x80) != 0x80 )
	{  	  	
	temp = read_reg(RSTAT);
	}
	
	if(temp&0x01)
	{
		    
			 	//���㳤��	
        	num = (unsigned int)read_reg(RSCL)+ ((unsigned int)(read_reg(RSCH))<<8);
			if(num==0)return (num); 
			temp = 0x00;              //��ȡ DAT_REG�Ĵ�������
			SS_N = 0;	
			send_buff(&temp,1);       //���Ͷ�ȡ����
			receive_buff(buffer,num); //��ȡ����
			SS_N = 1;
		
	}
	else
	{
		num =0;	
			
	}
	return num;

}

// ���������ӳ���		 
extern void write_prf(unsigned char *buffer,unsigned int num)
{
	unsigned char temp;	
	write_reg(SCON_60,0x5);	 //PTRCLR =1,CARRYON =1
	write_reg(SCON_60,0x01);    //PTRCLR=0;
	temp = 0x80;			 //д���ݼĴ������� 
	SS_N = 0;
	send_buff(&temp,1);
	send_buff(buffer,num);	 //д������
	SS_N = 1;

	write_reg(SCON_60,0x03);    // STARTSEND =1 ����ʼ����	



}


//����Ƶ
extern void open_prf()
{
    unsigned short i;  	
	write_reg(SCON_60,0x01);
	for (i=0;i<10000;i++);
	
}

	
//�ر���Ƶ
extern void close_prf()
{
    unsigned short i;  	
	write_reg(SCON_60,0x00);
	for (i=0;i<10000;i++);
		
}
