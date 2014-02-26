
#include "../at89x52.h"
#include "../spi/spi.h"
#include "../secure/secure.h"
#include <stdio.h>

sfr AUXR = 0x8e ;
#define __enable_T1()		TR1 =  1
#define __disable_T1()  	TR1 =  0

#define __enable_UART_interrupt()		ES = 1; __enable_T1()
#define __disable_UART_interrup()	    ES = 0; __disable_T1()


unsigned short FWT = 2000;

//initial seril interrupt 
void initial_serial()
{
    SCON  = 0x50;                   	/* SCON: mode 1, 8-bit UART, enable rcvr    */
    PCON  = 0X80;                       /* SMOD =0, K=1;SMOD =1, K=2;               */
    PS=1;                               /* COMM HIGH PRIORITY                       */
    TMOD  = 0x21;                   	/* TMOD: timer 1, mode 2, 8-bit reload      */
    TH1   =0x8b;//ff;// 0xff;;                    	/* TH1:  reload value for 115200 baud @22.1184MHz */
    TL1   =0x8b;//ff;//0xff;                    	/* Tl1:  reload value for 115200 baud @22.1184MHz */ 
    __enable_T1();                  	/* TR1:  timer 1 run */
	TI = 1;

}

//main function  
void main()
{
	unsigned char idata buf[160];
	unsigned char len; 	
	unsigned char i;
	init_i2c();
	AUXR = 0x00;	
	initial_serial();	  
    reset_prf();				//SPI ģʽоƬ��λ
    write_reg(TMRL,0x08);    	//���������Ӧʱ��,��λ302us
	P1_4=0;

	while(1)
 	{   	
	  len= read_sec(buf); 	 			//������ģ��
	  if (len!=0)
	  {

	
#if 1
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
#endif
#if 1
		len = read_prf(buf);			 //�������֤��������Ϣ
#endif
#if 1		
            buf[len]=0x00; 				 //�����ݺ�һλ0����ΪCRCУ����Ϣ
	  		len =len+1;
			    
	        write_sec(buf,len);			 //д����ģ��
        
#endif


#if 0
		 printf("R:length =: %bd\n",len);
	     for (i=0;i<len;i++)
		 {
		  	printf("%bx,",buf[i]);

		 }
		 printf("\n");
#endif

	  }

  	}  	 
}


