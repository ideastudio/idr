/*
 *All Rights Reserved 2014, XingHuo Info Inc,.
*/
#include "common.h"


#define INC_PTR(e,max)	\
	if((++e) >= max) e = 0;		  


#define RCV_BUF_SIZE 256

#define S2_S0 0x01              //P_SW2.0

static unsigned char xdata  uart1_read_buf[RCV_BUF_SIZE];
static unsigned char xdata uart1_read_in,uart1_read_out;

void uart1_init(void)	  //115200bps@27MHz
{
	SCON = 0x50;		//8λ����,�ɱ䲨����
	AUXR |= 0x40;		//��ʱ��1ʱ��ΪFosc,��1T
	AUXR &= 0xFE;		//����1ѡ��ʱ��1Ϊ�����ʷ�����
	TMOD &= 0x0F;		//�趨��ʱ��1Ϊ16λ�Զ���װ��ʽ
	ET1 = 0;		//��ֹ��ʱ��1�ж�
	TR1 = 1;		//������ʱ��1
	TL1 = 0xC5;		//�趨��ʱ��ֵ
	TH1 = 0xFF;		//�趨��ʱ��ֵ
	ES = 1;
	EA = 1;
	uart1_read_in  = uart1_read_out = 0;
}



static void serial_interrupt(void) interrupt 4  using 1
{   		   	    
	if(RI)  /* Receive mode */
	{
		RI = 0;
		uart1_read_buf[uart1_read_in] = SBUF;
		INC_PTR(uart1_read_in,RCV_BUF_SIZE);	
	}  
}



void uart1_write(const char* buf,int size){
	uint8 i=0; 	
	while(i<size){
		SBUF = buf[i];
		while(0==(TI));	TI=0;
		i++;
	}
}

//return read char count
int uart1_read(char* buf,int max_size){
	int ret=0;
	if(uart1_read_in != uart1_read_out) {
		do {
			buf[ret++]=uart1_read_buf[uart1_read_out];
			INC_PTR(uart1_read_out,RCV_BUF_SIZE);			
		}while((ret<max_size)&&uart1_read_in != uart1_read_out);
	}
	return ret;
}



static unsigned char xdata  uart2_read_buf[RCV_BUF_SIZE];
static unsigned char xdata uart2_read_in,uart2_read_out;

#define S2RI 0x01
#define S2TI 0x02
#define S2RB8 0x04
#define S2TB8 0x08
void uart2_init()	  //115200bps@27MHz
{
    P_SW2 &= ~S2_S0;            //S2_S0=0 (P1.0/RxD2, P1.1/TxD2)
//  P_SW2 |= S2_S0;             //S2_S0=1 (P4.6/RxD2_2, P4.7/TxD2_2)
	S2CON = 0x50;		//8λ����,�ɱ䲨����
	AUXR |= 0x14;		//������ʱ��2
	T2L = 0xC5;//(65536 - (FOSC/4/BAUDRATE));   //���ò�������װֵ  0xC5;		//�趨��ʱ��ֵ
	T2H = 0xff;//(65536 - (FOSC/4/BAUDRATE))>>8;		//�趨��ʱ��ֵ
	IE2 |= 0x1;
	EA = 1;
	uart2_read_in  = uart2_read_out = 0;

}


static void serial2_interrupt(void) interrupt 8  using 1
{   						    
	if(S2CON&S2RI)  /* Receive mode */
	{
		S2CON &=~S2RI;        
		uart2_read_buf[uart2_read_in] = S2BUF;
		INC_PTR(uart2_read_in,RCV_BUF_SIZE);	
	}
}


void uart2_write(const char* buf,int size){
	uint8 i=0; 	
	while(i<size){
		S2BUF = buf[i];
		while(0==(S2TI&S2CON)) ;S2CON&=~S2TI;
		i++;
	}
}

int uart2_read(char* buf,int max_size){
	int ret=0;
	if(uart2_read_in != uart2_read_out) {
		do {
			buf[ret++]=uart2_read_buf[uart2_read_out];
			INC_PTR(uart2_read_out,RCV_BUF_SIZE);			
		}while((ret<max_size)&&uart2_read_in != uart2_read_out);

	}
	return ret;
}


