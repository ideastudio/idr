/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- ʹ������MCU��STC15ϵ�е�Ƭ�����д���ISP���ؾ��� ----------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-755-82905966 -------------------------------------------*/
/* --- Tel: 86-755-82948412 -------------------------------------------*/
/* --- Web: www.STCMCU.com --------------------------------------------*/
/* ���Ҫ�ڳ�����ʹ�ô˴���,���ڳ�����ע��ʹ���˺꾧�Ƽ������ϼ�����   */
/* ���Ҫ��������Ӧ�ô˴���,����������ע��ʹ���˺꾧�Ƽ������ϼ�����   */
/*---------------------------------------------------------------------*/

//��ʾ����Keil������������ѡ��Intel��8058оƬ�ͺŽ��б���
//�ٶ�����оƬ�Ĺ���Ƶ��Ϊ11.0592MHz

//ע��:ʹ�ñ������STC15ϵ�еĵ�Ƭ����������ʱ,����Ҫִ����Download����֮��,
//���ܸ�Ŀ��оƬ�ϵ�,����Ŀ��оƬ���޷���ȷ����
#include "platform.h"
#include "packet.h"
#include "transfer.h"
#include "download.h"


#ifdef LOBYTE
#undef LOBYTE
#endif
#ifdef HIBYTE
#undef HIBYTE
#endif
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

//�ꡢ��������
#define FALSE               0
#define TRUE                1
#define LOBYTE(w)           ((BYTE)(WORD)(w))
#define HIBYTE(w)           ((BYTE)((WORD)(w) >> 8))

#define MINBAUD             2400L
#define MAXBAUD             115200L

#define FOSC                11059200L                   //����оƬ����Ƶ��
#define BR(n)               (65536 - FOSC/4/(n))        //����оƬ���ڲ����ʼ��㹫ʽ
#define T1MS                (65536 - FOSC/1000)         //����оƬ1ms��ʱ��ֵ

#define FUSER               24000000L                   //15ϵ��Ŀ��оƬ����Ƶ��
#define RL(n)               (65536 - FUSER/4/(n))       //15ϵ��Ŀ��оƬ���ڲ����ʼ��㹫ʽ


//��������
static BOOL f1ms;                                              //1ms��־λ
static BOOL UartReceived;                                      //�������ݽ�����ɱ�־λ
static BYTE UartRecvStep;                                      //�������ݽ��տ���
static BYTE TimeOut;                                           //����ͨѶ��ʱ������
static BYTE TxBuffer[256];                               //�������ݷ��ͻ�����
static BYTE RxBuffer[256];                               //�������ݽ��ջ�����

static int handle_download_port;
static char* firmware_buffer;
static long firmware_size;
static void* threadhandle_download;
static void* threadhandle_timer;
//��������
static void DelayXms(WORD x);
static BYTE UartSend(BYTE dat);
static void CommInit(void);
static void CommSend(BYTE size);
static BOOL Firmware_Download(BYTE *pdat, long size);



//1ms��ʱ���жϷ������
void* download_timer_thread(void* thread_arg){
    static BYTE Counter100=0;
	int exitthread=0;
	while(!exitthread){
		platform_usleep(1000);
		f1ms = TRUE;
		if (Counter100-- == 0)
		{
			Counter100 = 100;
			if (TimeOut) TimeOut--;
		}
		
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
		readlength = serialport_read(handle_download_port,&dat,1,5000);
		if(readlength>0)
	    {
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


//Xms��ʱ����
void DelayXms(WORD x)
{
    platform_usleep(x*1000);
}

//�������ݷ��ͳ���
BYTE UartSend(BYTE dat)
{
	serialport_write(handle_download_port,&dat,1);
	return dat;
}

//����ͨѶ��ʼ��
void CommInit(void)
{
    UartRecvStep = 0;
    TimeOut = 20;
    UartReceived = FALSE;
}

//���ʹ���ͨѶ���ݰ�
void CommSend(BYTE size)
{
	WORD sum;
    BYTE i;
    
    UartSend(0x46);
    UartSend(0xb9);
    UartSend(0x6a);
    UartSend(0x00);
    sum = size + 6 + 0x6a;
    UartSend(size + 6);
    for (i=0; i<size; i++)
    {
        sum += UartSend(TxBuffer[i]);
    }
    UartSend(HIBYTE(sum));
    UartSend(LOBYTE(sum));
    UartSend(0x16);

    CommInit();
}

void Initial(void)
{
	//��������ģʽ����Ϊ8λ����+1λż����
	serialport_config(handle_download_port,MINBAUD,8,1,'e');
}

//��STC15ϵ�е�оƬ�����������س���
int Firmware_Download(BYTE *pdat, long size)
{
    BYTE arg;
    BYTE cnt;
    WORD addr;
	
    Initial();
	
    //����
    CommInit();
	TimeOut=100;//100uint,100*100=10s
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

    //���ò���
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

    //׼��
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
    
    //����
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

    //д�û�����
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

	#if 0
    //дӲ��ѡ��(�������Ҫ�޸�Ӳ��ѡ��,�˲����ֱ������)
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

    //�������
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

int download_firmware(const char* download_port,const char* firmware_filename){
	int err=eDownloadErrCodeSuccess;
	handle_download_port = firmware_size = -1;
	handle_download_port = serialport_open(download_port);
	if(handle_download_port<0){
		err = eDownloadErrCodePortError;goto failed;		
	}
	//now open file to buffer
	firmware_size = platform_readfile2buffer(firmware_filename,&firmware_buffer);
	if(firmware_size<0){
		err = eDownloadErrCodeFileError;goto failed;
	}				
	//now firmware buffer is ready;
	printf("firmware[%s] size[%ld] prepareokay!\n",firmware_filename,firmware_size);

        
	//create received thread
	threadhandle_download = platform_createthread(download_recv_thread,(void*)handle_download_port);
	if(!threadhandle_download){
		printf("create firmware download thread failed\n");
		return eDownloadErrCodeUnknown;
	}
	threadhandle_timer = platform_createthread(download_timer_thread,(void*)NULL);
	if(!threadhandle_timer){
		printf("create firmware timer thread failed\n");
		return eDownloadErrCodeUnknown;
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
    
	return err;
failed:
	if(firmware_size>0)
		platform_releasebuffer(firmware_buffer);
	if(handle_download_port>0)
		serialport_close(handle_download_port);	
	if(threadhandle_timer)
		platform_terminatethread(threadhandle_timer);
	if(threadhandle_download)
		platform_terminatethread(threadhandle_download);
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
	mcu_xfer xfer;

	payload_len=setup_vendor_payload(payload_buffer,16,CMD_CLASS_MCU,MCU_SUB_CMD_RESET,2/*params num*/,MCU_RESET_TYPE_ISP,1);
	
	packet_len=setup_vendor_packet(io_buf,64,payload_buffer,payload_len);

	xfer.xfer_type = XFER_TYPE_OUT_IN;
	xfer.xfer_to = DEF_XFER_TIMEOUT_MS;
	xfer.req = io_buf;xfer.reqsize = packet_len;
	xfer.resp = io_buf,xfer.respsize = 64;
	xfer.xfer_impl = NULL;
	ret = submit_xfer(download_port,&xfer);
	if(!ret){
		if((io_buf[0]==0x0)&&(io_buf[1]==0x0)){
			printf("mcu reset in %dms\n"
				"Enter download firmware process now...\n"
				,2000);
			return download_firmware(download_port,firmware_filename);
		}else {
			printf("mcu reset failed ,err=[%2x%2x]\n",io_buf[0],io_buf[1]);
			return eDownloadErrCodeUnknown;
		}
	}

	printf("xfer packet error[%x],please check your port\n",ret);
	return eDownloadErrCodePortError;
	
}

