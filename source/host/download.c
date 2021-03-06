/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- 使用其它MCU对STC15系列单片机进行串口ISP下载举例 ----------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-755-82905966 -------------------------------------------*/
/* --- Tel: 86-755-82948412 -------------------------------------------*/
/* --- Web: www.STCMCU.com --------------------------------------------*/
/* 如果要在程序中使用此代码,请在程序中注明使用了宏晶科技的资料及程序   */
/* 如果要在文章中应用此代码,请在文章中注明使用了宏晶科技的资料及程序   */
/*---------------------------------------------------------------------*/

//本示例在Keil开发环境下请选择Intel的8058芯片型号进行编译
//假定测试芯片的工作频率为11.0592MHz

//注意:使用本代码对STC15系列的单片机进行下载时,必须要执行了Download代码之后,
//才能给目标芯片上电,否则目标芯片将无法正确下载
#include "platform.h"
#include "packet.h"
#include "transfer.h"
#include "download.h"
#include "hex2bin.h"

#undef LOBYTE
#undef HIBYTE
#undef FALSE 
#undef TRUE

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

//宏、常量定义
#define FALSE               0
#define TRUE                1
#define LOBYTE(w)           ((BYTE)(WORD)(w))
#define HIBYTE(w)           ((BYTE)((WORD)(w) >> 8))

#define MINBAUD             2400L
#define MAXBAUD             115200L

#define FOSC                11059200L                   //主控芯片工作频率
#define BR(n)               (65536 - FOSC/4/(n))        //主控芯片串口波特率计算公式
#define T1MS                (65536 - FOSC/1000)         //主控芯片1ms定时初值

#define FUSER               27000000L                   //15系列目标芯片工作频率
#define RL(n)               (65536 - FUSER/4/(n))       //15系列目标芯片串口波特率计算公式


//变量定义
static BOOL UartReceived;                                      //串口数据接收完成标志位
static BYTE UartRecvStep;                                      //串口数据接收控制
static int  TimeOut;                                           //串口通讯超时计数器
static BYTE TxBuffer[256];                               //串口数据发送缓冲区
static BYTE RxBuffer[256];                               //串口数据接收缓冲区

static int handle_download_port;
static char* firmware_buffer;
static long firmware_size;
static void* threadhandle_download;
static void* threadhandle_timer;
//函数声明
static void CommInit(void);
static void CommSend(BYTE size);
static BOOL Firmware_Download(BYTE *pdat, long size);



//1ms定时器中断服务程序
void* download_timer_thread(void* thread_arg){
	int exitthread=0;
	while(!exitthread){
		platform_usleep(1000*100);
		if (TimeOut>0) TimeOut--;		
	}
	return NULL;
}

void* download_recv_thread(void* thread_arg){
    static WORD RecvSum;
    static BYTE RecvIndex;
    static BYTE RecvCount;
	int exitthread=0;
	int ret;
	int readlength,consumed;
    BYTE dat;
	while(!exitthread){
//		platform_usleep(1000);
		readlength = serialport_read(handle_download_port,&dat,1,0);
		if(readlength>0)
	    {
	    	//printf("got uart bytes dat=0x%x UartRecvStep=0x%x\n",dat,UartRecvStep);
	        switch (UartRecvStep)
	        {
	        case 1:
	            if (dat != 0xb9) goto L_CheckFirst;
	            UartRecvStep++;
	            break;
	        case 2:
	            if (dat != 0x68) goto L_CheckFirst;
	            UartRecvStep++;
	            break;
	        case 3:
	            if (dat != 0x00) goto L_CheckFirst;
	            UartRecvStep++;
	            break;
	        case 4:
	            RecvSum = 0x68 + dat;
	            RecvCount = dat - 6;
	            RecvIndex = 0;
	            UartRecvStep++;
	            break;
	        case 5:
	            RecvSum += dat;
	            RxBuffer[RecvIndex++] = dat;
	            if (RecvIndex == RecvCount) UartRecvStep++;
	            break;
	        case 6:
	            if (dat != HIBYTE(RecvSum)) goto L_CheckFirst;
	            UartRecvStep++;
	            break;
	        case 7:
	            if (dat != LOBYTE(RecvSum)) goto L_CheckFirst;
	            UartRecvStep++;
	            break;
	        case 8:
	            if (dat != 0x16) goto L_CheckFirst;
	            UartReceived = TRUE;
	            UartRecvStep++;
	            break;
	L_CheckFirst:
	        case 0:
	        default:
	            CommInit();
	            UartRecvStep = (dat == 0x46 ? 1 : 0);
	            break;
	        }
	    }	
	}
	return NULL;
}


//Xms延时程序
static inline void DelayXms(WORD x)
{
    platform_usleep(x*1000);
}

//串口数据发送程序
static inline BYTE UartSend(BYTE dat)
{
	serialport_write(handle_download_port,&dat,1);
	return dat;
}

//串口通讯初始化
static inline void CommInit(void)
{
    UartRecvStep = 0;
    TimeOut = 20;
    UartReceived = FALSE;
}

//发送串口通讯数据包
static inline void CommSend(BYTE size)
{
	WORD sum;
    BYTE i;
    
    UartSend(0x46);
    UartSend(0xb9);
    UartSend(0x6a);
    UartSend(0x00);
    sum = size + 6 + 0x6a;
    UartSend(size + 6);
	serialport_write(handle_download_port,TxBuffer,size);
    for (i=0; i<size; i++)
    {
        sum += TxBuffer[i];
    }
    UartSend(HIBYTE(sum));
    UartSend(LOBYTE(sum));
    UartSend(0x16);

    CommInit();
}

static void Initial(void)
{
	//串口数据模式必须为8位数据+1位偶检验
	serialport_config(handle_download_port,MINBAUD,8,1,'e');
}

//对STC15系列的芯片进行数据下载程序
static int Firmware_Download(BYTE *pdat, long size)
{
    BYTE arg;
    BYTE cnt;
    WORD addr;
	
    Initial();
	
    //握手
    CommInit();
    while (1)
    {
        if (UartRecvStep == 0)
        {
            UartSend(0x7f);
            DelayXms(10);
        }
        if (UartReceived)
        {
            arg = RxBuffer[4];
            if (RxBuffer[0] == 0x50) 
				break;
            return eDownloadErrCodeHandshake;
        }		
        if (TimeOut == 0) {
			printf("waiting for mcu isp timeout\n");
			return eDownloadErrCodeTimeout;
        }
    }

	printf("set comm param\n");

    //设置参数
    TxBuffer[0] = 0x01;
    TxBuffer[1] = arg;
    TxBuffer[2] = 0x40;
	TxBuffer[3] = HIBYTE(RL(MAXBAUD));
	TxBuffer[4] = LOBYTE(RL(MAXBAUD));
	TxBuffer[5] = 0x00;
	TxBuffer[6] = 0x00;
	TxBuffer[7] = 0xc3;
    CommSend(8);
	while (1)
	{
        if (TimeOut == 0) return eDownloadErrCodeTimeout;
        if (UartReceived)
        {
            if (RxBuffer[0] == 0x01) break;
            return eDownloadErrCodeSetCommParam;
        }
	}

	printf("download prepare \n");

    //准备
    serialport_config(handle_download_port,MAXBAUD,8,1,'e');
    DelayXms(10);
	TxBuffer[0] = 0x05;
	CommSend(1);
	while (1)
	{
        if (TimeOut == 0) return eDownloadErrCodeTimeout;
        if (UartReceived)
        {
            if (RxBuffer[0] == 0x05) break;
            return eDownloadErrCodeSetCommParam;
        }
	}
    
	printf("erase flash ...\n");
    //擦除
    DelayXms(10);
	TxBuffer[0] = 0x03;
	TxBuffer[1] = 0x00;
	CommSend(2);
    TimeOut = 100;
    while (1)
	{
        if (TimeOut == 0) return eDownloadErrCodeTimeout;
        if (UartReceived)
        {
            if (RxBuffer[0] == 0x03) break;
            return eDownloadErrCodeEraseFlash;
        }
	}

	printf("program flash size[0x%lx]...\n",size);

    //写用户代码
    DelayXms(10);
    addr = 0;
	TxBuffer[0] = 0x22;
	while (addr < size)
	{
        TxBuffer[1] = HIBYTE(addr);
        TxBuffer[2] = LOBYTE(addr);
        cnt = 0;
        while (addr < size)
        {
            TxBuffer[cnt+3] = pdat[addr];
            addr++;
            cnt++;
            if (cnt >= 128) break;
        }
        CommSend(cnt + 3);
		while (1)
		{
            if (TimeOut == 0) return eDownloadErrCodeTimeout;
            if (UartReceived)
            {
                if ((RxBuffer[0] == 0x02) && (RxBuffer[1] == 'T')) break;
                return eDownloadErrCodeProgramFlash;
            }
		}
		TxBuffer[0] = 0x02;
	}

	printf("program hardware option ... skipped\n");

	#if 0
    //写硬件选项(如果不需要修改硬件选项,此步骤可直接跳过)
    DelayXms(10);
    for (cnt=0; cnt<128; cnt++)
    {
        TxBuffer[cnt] = 0xff;
	}
	//Do we know details of hardware options???
    TxBuffer[0] = 0x04;
	TxBuffer[1] = 0x00;
	TxBuffer[2] = 0x00;
	TxBuffer[34] = 0xfd;
	TxBuffer[62] = arg;
	TxBuffer[63] = 0x7f;
	TxBuffer[64] = 0xf7;
	TxBuffer[65] = 0x7b;
	TxBuffer[66] = 0x1f;
	CommSend(67);
	while (1)
	{
        if (TimeOut == 0) return eDownloadErrCodeTimeout;
        if (UartReceived)
        {
            if ((RxBuffer[0] == 0x04) && (RxBuffer[1] == 'T')) break;
            return eDownloadErrCodeProgramOption;
        }
	}
	#endif

    //下载完成
    return eDownloadErrCodeSuccess;
}


const char* download_error_code2string(int code){
	static const char* errorstring[]={
		"success",
		"download port error",
		"firmware file error",
		"timeout",
		"hand shake failed",
		"set comm params",
		"erase flash",
		"program flash",
		"program option",
		"unknown error",
	};
	if(code<0)
		code=-code;
	if(code>eDownloadErrCodeMax)
		code=eDownloadErrCodeMax;
	return errorstring[code];
};
/*
static char DEMO[256] = 
	{
		0x02,0x00,0x5E,0x12,0x00,0x4B,0x75,0xB0,
		0xEF,0x12,0x00,0x2C,0x75,0xB0,0xDF,0x12,
		0x00,0x2C,0x75,0xB0,0xFE,0x12,0x00,0x2C,
		0x75,0xB0,0xFD,0x12,0x00,0x2C,0x75,0xB0,
		0xFB,0x12,0x00,0x2C,0x75,0xB0,0xF7,0x12,
		0x00,0x2C,0x80,0xDA,0xE4,0xFF,0xFE,0xE4,
		0xFD,0xFC,0x0D,0xBD,0x00,0x01,0x0C,0xBC,
		0x01,0xF8,0xBD,0xF4,0xF5,0x0F,0xBF,0x00,
		0x01,0x0E,0xBE,0x03,0xEA,0xBF,0xE8,0xE7,
		0x02,0x00,0x4B,0x75,0x80,0xFF,0x75,0x90,
		0xFF,0x75,0xA0,0xFF,0x75,0xB0,0xFF,0x75,
		0xC0,0xFF,0x75,0xC8,0xFF,0x22,0x78,0x7F,
		0xE4,0xF6,0xD8,0xFD,0x75,0x81,0x07,0x02,
		0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	};
*/	
	
int download_firmware(const char* download_port,const char* firmware_filename){
	int err=eDownloadErrCodeSuccess;
	unsigned char* inbuf,*outbuf;
	unsigned int inlen,outlen;
	handle_download_port = firmware_size = -1;
	threadhandle_timer = threadhandle_download = NULL;
	inbuf = outbuf = firmware_buffer = NULL;
	handle_download_port = serialport_open(download_port);
	if(handle_download_port<0){
		err = eDownloadErrCodePortError;goto failed;		
	}
	
	//now open file to buffer
	inlen = platform_readfile2buffer(firmware_filename,0,(char**)&inbuf);
	if(inlen<0){
		err = eDownloadErrCodeFileError;goto failed;
	}

	//allocate out buffer ,max size as half of inlen
	outlen = inlen;
	outbuf = (unsigned char* )malloc(outlen);
	if(!outbuf){
		err = eDownloadErrCodeUnknown;
		goto failed;
	}
	//convert to bin format
	err = mcu_hex2bin(inbuf,inlen,outbuf,&outlen);

	firmware_buffer = outbuf;	
	firmware_size = outlen;
	//only for demo
//	firmware_buffer = DEMO;
//	firmware_size=256;
	//now firmware buffer is ready;
	printf("firmware[%s] hex size[0x%x] bin size[0x%x] prepareokay!\n",firmware_filename,
		inlen,outlen);

        
	//create received thread
	threadhandle_download = platform_createthread(download_recv_thread,(void*)handle_download_port);
	if(!threadhandle_download){
		printf("create firmware download thread failed\n");
		err = eDownloadErrCodeUnknown;goto failed;
		
	}
	threadhandle_timer = platform_createthread(download_timer_thread,(void*)NULL);
	if(!threadhandle_timer){
		printf("create firmware timer thread failed\n");
		err = eDownloadErrCodeUnknown;goto failed;
	}
	
    err = Firmware_Download(firmware_buffer, firmware_size);
    if (eDownloadErrCodeSuccess==err)
    {
        printf("\n\n\n"
		"----Congratulations!Firmware download successful!----\n\n\n");
    }
    else
    {
        printf("\n\n\n"
		"----FAIL FAIL FAIL![E%d,%s]----\n\n\n",err,download_error_code2string(err));
    }
    
failed:
	if(threadhandle_timer)
		platform_terminatethread(threadhandle_timer);
	if(threadhandle_download)
		platform_terminatethread(threadhandle_download);
	if(inbuf)
		platform_releasebuffer(inbuf);
	if(outbuf)
		platform_releasebuffer(outbuf);	
	if(handle_download_port>0)
		serialport_close(handle_download_port);	
	return err;	
}

/*
	We assume target mcu is running now,and send command to MCU for resetting to ISP mode 
	work flow:
	sending reset cmd->mcu->wait response
	if(response okay)
		run download firmware process
*/
int download_firmware_all_in_one(const char* download_port,const char* firmware_filename){
	uint8 io_buf[64];
	uint8 payload_buffer[16];
	int payload_len,packet_len;
	int ret;
	uint8 reset_delay=2;
	mcu_xfer xfer;

	payload_len=setup_vendor_payload(payload_buffer,16,CMD_CLASS_MCU,MCU_SUB_CMD_RESET,2/*params num*/,MCU_RESET_TYPE_ISP,reset_delay);
	
	packet_len=setup_vendor_packet(io_buf,64,payload_buffer,payload_len);

	xfer.xfer_type = XFER_TYPE_OUT_IN;
	xfer.xfer_to = DEF_XFER_TIMEOUT_MS;
	xfer.req = io_buf;xfer.reqsize = packet_len;
	xfer.resp = io_buf,xfer.respsize = 64;
	xfer.xfer_impl = NULL;
	ret = submit_xfer(download_port,NULL,&xfer);
	if(!ret){
		if(STATUS_CODE(xfer.resp)==STATUS_CODE_SUCCESS){
			printf("mcu reset in %dms\n"
				"Enter download firmware process now...\n"
				,reset_delay*1000);
			return download_firmware(download_port,firmware_filename);
		}else {
			printf("mcu reset failed ,err=[%2x%2x]\n",io_buf[0],io_buf[1]);
			return eDownloadErrCodeUnknown;
		}
	}

	printf("xfer packet error[%x],please check your port\n",ret);
	return eDownloadErrCodePortError;
	
}

