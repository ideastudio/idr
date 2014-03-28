#include "platform.h"
#include "hex2bin.h"

#define MAX_SIZE_HEX_FILE  2048*1024  //�ֽ�
#define ABSOLUTE_ADDR_OFFSET   16 //���ݵ�ַƫ�� 16���ֽ������Զ���ͷ

static unsigned char BCD2Byte(char h, char l)
{
	unsigned char byte;

	if(l>='0'&&l<='9')
	{
		l=l-'0';
	}
	else if(l>='a' && l<='f')
	{
		l=l-'a'+0xa;
	}
	else if(l>='A' && l<='F')
	{
		l=l-'A'+0xa;
	}
	else
	{
		l = 0x00;
	}

	if(h>='0'&&h<='9')
	{
		h=h-'0';
	}
	else if(h>='a' && h<='f')
	{
		h=h-'a'+0xa;
	}
	else if(h>='A' &&h <='F')
	{
		h=h-'A'+0xa;
	}
	else
	{
		h = 0x00;
	}

	byte=(h*16+l);
	return byte;
}

//��� ��¼��β��У�����Ƿ���ȷ
static bool RecordChecksum(unsigned char *p,unsigned int RecordLen)
{
	unsigned int sum =0;
	unsigned char checksum = 0;
	unsigned int i;
	for ( i = 0;i<=RecordLen-2;i++ )
	{
		sum += p[i];
	}
	checksum =(unsigned char)( (~sum)+0x01);

	if (checksum == p[RecordLen-1])
	{
		return true;
	}
	else
	{
		return false;
	}

}


static int m_bAddHeaderInfo = 0;
static int m_bUnusefulDataType=0;
static unsigned int m_uiCustomNo=1;
static unsigned int m_uiSoftNo = 1;
int hex2bin(unsigned char* pInBuffer,unsigned int inlen,unsigned char* pOutBuffer,unsigned int *outlen)
{
	int errcode = eHex2BinSuccess;
	unsigned long fileLen,ByteIndex;
	unsigned int OffsetAddr,StartAddr;

	unsigned long AbsoluteAddr ;
	unsigned long BinDataLen;
	unsigned long index ;

	unsigned long RecordIndex ;

	unsigned char DataNum;
	unsigned char DataType;
	unsigned int PacketNum;
	unsigned int TotalChecksum;

//	char *pInBuffer = new char[MAX_SIZE_HEX_FILE];// �ݲ�֧�ֳ���2M��hex�ļ�  ��̬���� �ǵ��ͷ�
	unsigned char  *pRecord = (unsigned char*)malloc(512);//hex ������¼���ᳬ�� 521���ֽ�  ��̬���� �ǵ��ͷ�
//	unsigned char  *pOutBuffer = new unsigned char [MAX_SIZE_HEX_FILE/2];//hex�ļ�����ܳ���2M ��̬���� �ǵ��ͷ�
	//�ֲ�������ʼ��
	fileLen = 0;
	ByteIndex = 0;	OffsetAddr = 0;
	StartAddr = 0;
	AbsoluteAddr = 0;
	RecordIndex = 0;
	index = 0;
	DataNum = 0;
	BinDataLen = 0;
	DataType = 0x01;
	TotalChecksum = 0;
	PacketNum = 0;
	if(*outlen<(inlen/2)){
		errcode = eHex2BinErrOutOfBuffer;
		goto convert_error;
	}
		
	//#####################################ѡ��###############################################################
	if (m_bUnusefulDataType)//���0x00
	{
		DataNum = 0x00;
	}
	else
	{
		DataNum = 0xff;
	}
	for (index = 0;index<inlen/2;index++)//Ϊ�˴���ĳЩhex��ַ������������ �ȳ�ʼ�����������ַΪ0xff
	{
		pOutBuffer[index] = DataNum;
	}

	//####################################################################################################

	fileLen = inlen;
	//####################################################################################################
	//ת������
	ByteIndex = 0;

	while(ByteIndex<fileLen)
	{
		if (pInBuffer[ByteIndex] == ':')//һ���µļ�¼ :��ʼ �س����н���
		{
			RecordIndex++;
			unsigned char i = 0;
			//ByteIndex++;
			while (pInBuffer[ByteIndex +1] !=0x0d && pInBuffer[ByteIndex+2] !=0x0A) //�ǻس����з�
			{
				//�ַ�ת��ΪBYTE
				pRecord[i]= BCD2Byte(pInBuffer[ByteIndex+1],pInBuffer[ByteIndex+2]);//�ַ�ת��16����
				i++;
				ByteIndex+=2;
			}
			//����һ����¼ ����У�� ��ʱi���� pRecord �ĳ��� �˳��Ȳ��ܳ���255
			if(FALSE == RecordChecksum(pRecord,i) )//У��λ�Ƿ����
			{
				errcode = eHex2BinErrRecordChecksum; 
				goto convert_error;
			}
			else//У������ 
			{
				DataNum = pRecord[0];
				DataType = pRecord[3];

				switch(DataType)
				{
				case 0x00://�����ֶ�
					if (RecordIndex ==1)//�׸���¼�εĵ�ַ��Ϊ ����ַ
					{
						StartAddr = pRecord[1]*256+pRecord[2];

					}
					OffsetAddr = pRecord[1]*256+pRecord[2];

					index = AbsoluteAddr + OffsetAddr-StartAddr+16;


					//������д��bin����
					for (i=0;i<DataNum;i++)
					{
						pOutBuffer[index + i] = pRecord[i+4];
					}

					if (BinDataLen <AbsoluteAddr + OffsetAddr + DataNum -StartAddr )//ʼ�ռ�¼���ľ��Ե�ַ
					{
						BinDataLen = AbsoluteAddr + OffsetAddr + DataNum -StartAddr ;//bin ����ߵ�ַ
					}
					break;
				case 0x01://�����ֶ�
					//BinDataLen = AbsoluteAddr + OffsetAddr+ DataNum+16;//bin ����ߵ�ַ

					break;
				case 0x02://��չ�ε�ַ
					if( RecordIndex ==1)
					{
						StartAddr = pRecord[4]*256+pRecord[5];
						StartAddr <<=4;
					}
					AbsoluteAddr =  pRecord[4]*256 + pRecord[5];
					AbsoluteAddr <<=4;
					break;
				case 0x04://��չ���Ե�ַ

					if( RecordIndex ==1)
					{
						StartAddr = pRecord[4]*256+pRecord[5];
						StartAddr <<=16;
					}
					AbsoluteAddr =  pRecord[4]*256 + pRecord[5];
					AbsoluteAddr <<=16;
					break;

				case 0x03://�ε�ַ��ʼ
				case 0x05://���Ե�ַ��ʼ
				default: {
					errcode = eHex2BinErrRecordUnsupported;
					goto convert_error;
				}

				}

			}
		}
		else 
		{
			ByteIndex++;
		}
	}
	//####################################################################################################

	if (BinDataLen%128 !=0)
	{
		PacketNum = BinDataLen/128 +1;
	}
	else
	{
		PacketNum = BinDataLen/128;
	}
	if (m_bAddHeaderInfo)
	{
		BinDataLen = 128*PacketNum +16;
	} 
	TotalChecksum = 0;
	//����bin�ļ���У��������ݰ�����
	for (index = 16;index <BinDataLen;index++)
	{
		TotalChecksum += pOutBuffer[index];
	}

	TotalChecksum = ((TotalChecksum) )&0x0000ffff;



	//����ļ�ͷ��Ϣ 16���ֽ� 0x00
	for (index = 0;index <16;index++)
	{
		pOutBuffer [index]= 0x00;
	}
	pOutBuffer[0] = (unsigned char) (m_uiCustomNo);
	pOutBuffer[1] = (unsigned char) (m_uiSoftNo);
	pOutBuffer[2] = TotalChecksum/256;
	pOutBuffer[3] = TotalChecksum%256;

	pOutBuffer[4] = PacketNum/256;
	pOutBuffer[5] = PacketNum%256;


	
	if (m_bAddHeaderInfo)//����Զ���Bin�ļ�ͷ��Ϣ
	{
		*outlen=BinDataLen;
		//f.Write(pOutBuffer,BinDataLen);
	}
	else//û�����Bin�ļ�ͷ��Ϣ ����ͨ��ת��
	{
		*outlen=BinDataLen;
		memmove(pOutBuffer,pOutBuffer+16,BinDataLen);
		//f.Write(&pOutBuffer[16],BinDataLen);
	}
	printf("\n\nFirmware Information\n");
	printf("start address:0x%.8x   end address:0x%.8x \n",StartAddr,m_bAddHeaderInfo?(BinDataLen-16):BinDataLen);
	printf("size:%d KB \n",BinDataLen/1024);
	printf("custom no:%d firmware no:%d\n",m_uiCustomNo,m_uiSoftNo);
	printf("checksum:0x%.4x \n",TotalChecksum);
	printf("packet num:%d \n\n",PacketNum);
	errcode = eHex2BinSuccess;

convert_error:
	if(pRecord)
		free(pRecord);
	return errcode;
}

