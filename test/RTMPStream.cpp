/******************************************************************** 
filename:   RTMPStream.cpp
created:    2013-04-3
author:     firehood 
purpose:    发送H264视频到RTMP Server，使用libRtmp库
*********************************************************************/ 
#include "stdafx.h"
#include "RTMPStream.h"
#include "SpsDecode.h"
#ifdef WIN32  
#include <windows.h>
#endif

#ifdef WIN32
#pragma comment(lib,"WS2_32.lib")
#pragma comment(lib,"winmm.lib")
#endif

//TODO 优化掉这个定义???
#define RTMP_HEAD_SIZE (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

enum
{
	FLV_CODECID_H264 = 7,
};

int InitSockets()  
{  
#ifdef WIN32  
	WORD version;  
	WSADATA wsaData;  
	version = MAKEWORD(1, 1);  
	return (WSAStartup(version, &wsaData) == 0);  
#else  
	return TRUE;  
#endif  
}  

inline void CleanupSockets()  
{  
#ifdef WIN32  
	WSACleanup();  
#endif  
}  

char * put_byte( char *output, uint8_t nVal )  
{  
	output[0] = nVal;  
	return output+1;  
}  
char * put_be16(char *output, uint16_t nVal )  
{  
	output[1] = nVal & 0xff;  
	output[0] = nVal >> 8;  
	return output+2;  
}  
char * put_be24(char *output,uint32_t nVal )  
{  
	output[2] = nVal & 0xff;  
	output[1] = nVal >> 8;  
	output[0] = nVal >> 16;  
	return output+3;  
}  
char * put_be32(char *output, uint32_t nVal )  
{  
	output[3] = nVal & 0xff;  
	output[2] = nVal >> 8;  
	output[1] = nVal >> 16;  
	output[0] = nVal >> 24;  
	return output+4;  
}  
char *  put_be64( char *output, uint64_t nVal )  
{  
	output=put_be32( output, nVal >> 32 );  
	output=put_be32( output, nVal );  
	return output;  
}  
char * put_amf_string( char *c, const char *str )  
{  
	uint16_t len = strlen( str );  
	c=put_be16( c, len );  
	memcpy(c,str,len);  
	return c+len;  
}  
char * put_amf_double( char *c, double d )  
{  
	*c++ = AMF_NUMBER;  /* type: Number */  
	{  
		unsigned char *ci, *co;  
		ci = (unsigned char *)&d;  
		co = (unsigned char *)c;  
		co[0] = ci[7];  
		co[1] = ci[6];  
		co[2] = ci[5];  
		co[3] = ci[4];  
		co[4] = ci[3];  
		co[5] = ci[2];  
		co[6] = ci[1];  
		co[7] = ci[0];  
	}  
	return c+8;  
}

CRTMPStream::CRTMPStream(void):
m_pRtmp(NULL),
m_nFileBufSize(0),
m_nCurPos(0)
{
	m_pFileBuf = new unsigned char[FILEBUFSIZE];
	memset(m_pFileBuf,0,FILEBUFSIZE);
	InitSockets();
	m_pRtmp = RTMP_Alloc();  
	RTMP_Init(m_pRtmp);  
}

CRTMPStream::~CRTMPStream(void)
{
	Close();
	WSACleanup();  
	delete[] m_pFileBuf;
}

bool CRTMPStream::Connect(const char* url)
{
	//从Connect()到Close()之间的整个过程，都固定为这个url
	if(RTMP_SetupURL(m_pRtmp, (char*)url)<0)
	{
		return FALSE;
	}
	RTMP_EnableWrite(m_pRtmp);
	if(RTMP_Connect(m_pRtmp, NULL)<0)
	{
		return FALSE;
	}
	if(RTMP_ConnectStream(m_pRtmp,0)<0)
	{
		return FALSE;
	}
	return TRUE;
}

void CRTMPStream::Close()
{
	if(m_pRtmp)
	{
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
	}
}

int CRTMPStream::SendVideoPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp)
{
	if(m_pRtmp == NULL)
	{
		return FALSE;
	}

	RTMPPacket packet;
	RTMPPacket_Reset(&packet);
	RTMPPacket_Alloc(&packet,size);

	packet.m_packetType = nPacketType;
	packet.m_nChannel = STREAM_CHANNEL_VIDEO;  
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;  
	packet.m_nTimeStamp = nTimestamp;  
	packet.m_nInfoField2 = m_pRtmp->m_stream_id;
	packet.m_nBodySize = size;
	memcpy(packet.m_body,data,size);

	int nRet = RTMP_SendPacket(m_pRtmp,&packet,0);

	RTMPPacket_Free(&packet);

	return nRet;
}

bool CRTMPStream::SendMetadata(LPRTMPMetadata lpMetaData)
{
	if(lpMetaData == NULL)
	{
		return false;
	}
	char body[1024] = {0};;

	char * p = (char *)body;  
	p = put_byte(p, AMF_STRING );
	p = put_amf_string(p , "@setDataFrame" );

	p = put_byte( p, AMF_STRING );
	p = put_amf_string( p, "onMetaData" );

	p = put_byte(p, AMF_OBJECT );  
	p = put_amf_string( p, "copyright" );  
	p = put_byte(p, AMF_STRING );  
	p = put_amf_string( p, "firehood" );  

	p =put_amf_string( p, "width");
	p =put_amf_double( p, lpMetaData->nWidth);

	p =put_amf_string( p, "height");
	p =put_amf_double( p, lpMetaData->nHeight);

	p =put_amf_string( p, "framerate" );
	p =put_amf_double( p, lpMetaData->nFrameRate); 

	p =put_amf_string( p, "videocodecid" );
	p =put_amf_double( p, FLV_CODECID_H264 );

	p =put_amf_string( p, "" );
	p =put_byte( p, AMF_OBJECT_END  );

	int index = p-body;

	SendVideoPacket(RTMP_PACKET_TYPE_INFO,(unsigned char*)body,p-body,0);

	int i = 0;
	body[i++] = 0x17; // 1:keyframe  7:AVC
	body[i++] = 0x00; // AVC sequence header

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00; // fill in 0;

	// AVCDecoderConfigurationRecord.
	body[i++] = 0x01; // configurationVersion
	body[i++] = lpMetaData->Sps[1]; // AVCProfileIndication
	body[i++] = lpMetaData->Sps[2]; // profile_compatibility
	body[i++] = lpMetaData->Sps[3]; // AVCLevelIndication 
	body[i++] = 0xff; // lengthSizeMinusOne  

	// sps nums
	body[i++] = 0xE1; //&0x1f
	// sps data length
	body[i++] = lpMetaData->nSpsLen>>8;
	body[i++] = lpMetaData->nSpsLen&0xff;
	// sps data
	memcpy(&body[i],lpMetaData->Sps,lpMetaData->nSpsLen);
	i= i+lpMetaData->nSpsLen;

	// pps nums
	body[i++] = 0x01; //&0x1f
	// pps data length 
	body[i++] = lpMetaData->nPpsLen>>8;
	body[i++] = lpMetaData->nPpsLen&0xff;
	// sps data
	memcpy(&body[i],lpMetaData->Pps,lpMetaData->nPpsLen);
	i= i+lpMetaData->nPpsLen;

	return SendVideoPacket(RTMP_PACKET_TYPE_VIDEO,(unsigned char*)body,i,0);

}

bool CRTMPStream::SendAudioPacket(unsigned char *buf, int len, unsigned int nTimeStamp)	//入参已经是去掉7字节AAC frame header的数据了
{

	if(len>0){
		RTMPPacket *packet;
		unsigned char *body;

		packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + len + 2);
		memset(packet, 0, RTMP_HEAD_SIZE);

		packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
		body = (unsigned char *)packet->m_body;

		/* AF 01 + AAC raw data */
		body[0] = 0xAF;
		body[1] = 0x01;
		memcpy(&body[2], buf, len);

		packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
		packet->m_nBodySize = len+2;
		packet->m_nChannel = STREAM_CHANNEL_AUDIO;
		packet->m_nTimeStamp = nTimeStamp;
		packet->m_hasAbsTimestamp = 0;
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
		packet->m_nInfoField2 = m_pRtmp->m_stream_id;

		/* 调用发送接口 */
		RTMP_SendPacket(m_pRtmp, packet, TRUE);
		free(packet);
	}
	return 0;
}

bool CRTMPStream::SendH264Packet(unsigned char *data,unsigned int size,bool bIsKeyFrame,unsigned int nTimeStamp)
{
	if(data == NULL && size<11)
	{
		return false;
	}

	unsigned char *body = new unsigned char[size+9];

	int i = 0;
	if(bIsKeyFrame)
	{
		body[i++] = 0x17;// 1:Iframe  7:AVC
	}
	else
	{
		body[i++] = 0x27;// 2:Pframe  7:AVC
	}
	body[i++] = 0x01;// AVC NALU
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	// NALU size
	body[i++] = size>>24;
	body[i++] = size>>16;
	body[i++] = size>>8;
	body[i++] = size&0xff;;

	// NALU data
	memcpy(&body[i],data,size);

	bool bRet = SendVideoPacket(RTMP_PACKET_TYPE_VIDEO,body,i+size,nTimeStamp);

	delete[] body;

	return bRet;
}

bool CRTMPStream::SendAACSpec(unsigned char *spec_buf, int spec_len){
	RTMPPacket * packet;
	unsigned char * body;
	int len;

	len = spec_len;  /*spec data长度,一般是2*/

	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+len+2);
	memset(packet,0,RTMP_HEAD_SIZE);

	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;

	/*AF 00 + AAC RAW data*/
	body[0] = 0xAF;
	body[1] = 0x00;
	memcpy(&body[2],spec_buf,len); /*spec_buf是AAC sequence header数据*/

	packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
	packet->m_nBodySize = len+2;
	packet->m_nChannel = STREAM_CHANNEL_AUDIO;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;

	/*调用发送接口*/
	RTMP_SendPacket(m_pRtmp,packet,TRUE);

	return TRUE;
}

bool CRTMPStream::SendAACFile(const char *pFileName)
{
	if(pFileName == NULL)
	{
		return FALSE;
	}
	FILE *fp = fopen(pFileName, "rb");
	if(!fp)
	{
		printf("ERROR:open AAC file %s failed!", pFileName);
	}
	fseek(fp, 0, SEEK_SET);
	m_nFileBufSize = fread(m_pFileBuf, sizeof(unsigned char), FILEBUFSIZE, fp);
	if(m_nFileBufSize >= FILEBUFSIZE)
	{
		printf("warning : File size is larger than BUFSIZE\n");
	}
	fclose(fp); 

	//读取AAC sequence header（在这里我们从aac文件中读取，因此是我们自己计算的）
	//参见libfaac\frame.c中int FAACAPI faacEncGetDecoderSpecificInfo(faacEncHandle hEncoder,unsigned char** ppBuffer,unsigned long* pSizeOfDecoderSpecificInfo)实现
/**
参考文档：
ADTS格式：(AAC的两种格式之一，用于音频流)
ADTS vs. ADIF http://blog.csdn.net/wlsfling/article/details/5876016
ADTS帧头部格式： http://wiki.multimedia.cx/index.php?title=ADTS

=============================================================================
.aac文件从头到尾已经是ADTS帧构成的了，我们的测试文件quicksort.aac的ADTS帧为： FF F1 4C 40 0B BF FC (十六进制) ...

二进制比特对应关系：
AAAA AAAA AAAA BCCD EEFF FFGH HHIJ KLMM MMMM MMMM MMMO OOOO OOOO OOPP (QQQQQQQQ QQQQQQQQ)
1111 1111 1111 0001 0100 1100 0100 0000 0000 1011 1011 1111 1111 1100

二进制：
A = 1111 1111 1111	==> AAC ADTS帧头同步比特
B = 0	==> 0: MPEG-4 / 1: MPEG-2
C = 00
D = 1	==> protection absent ==> 1: no CRC / 0: CRC
E = 01	==> the MPEG-4 Audio Object Type minus 1 ==> aacObjectType=2: AAC LC (Low Complexity) 参见：http://wiki.multimedia.cx/index.php?title=MPEG-4_Audio#Audio_Object_Types
F = 0011	==> MPEG-4 Sampling Frequency Index ==> 3: 48000 Hz / 4: 44100Hz	参见：http://wiki.multimedia.cx/index.php?title=MPEG-4_Audio#Sampling_Frequencies
G = 0
H = 001		==> MPEG-4 Channel Configuration  ==> 1: 1 channel: front-center / 2: 2 channels: front-left, front-right 参见：http://wiki.multimedia.cx/index.php?title=MPEG-4_Audio#Channel_Configurations
I = 0		==> 0: encoding / 1: decoding
J = 0		==> 0: encoding / 1: decoding
K = 0		==> 0: encoding
L = 0		==> 0: encoding
M = 00 0000 1011 101	==> frame length, this value must include 7 or 9 bytes of header length: FrameLength = (ProtectionAbsent == 1 ? 7 : 9) + size(AACFrame) ==> 93(dec) = 7 + size(AACFrame)
O = 1 1111 1111 11	==> buffer fullness
P = 00		==> number of AAC frames in ADTS frame minus 1 ==> 1 AAC frame
**/

	unsigned char aacObjectType = 2; //AAC LC (Low Complexity)
	unsigned char sampleRateIdx = 3; //48000Hz
	unsigned char channelConfig = 1; //1 channel: front-center

	unsigned int audio_decoder_spec_info = 0x0000; //只用低2字节
	
	audio_decoder_spec_info |= ((aacObjectType<<11) & 0xF800); //5bit aacObjectTye
	audio_decoder_spec_info |= ((sampleRateIdx<<7) & 0x0780); //4bit sampleRateIdx
	audio_decoder_spec_info |= ((channelConfig<<3) & 0x0078); //4bit channel config
	//last 3 bit left blank, that is, three '0' at the end.

	int i=0;
	int body_len = 2;
	unsigned char *body = new unsigned char[body_len];
	body[i++] = audio_decoder_spec_info>>8;
	body[i] = audio_decoder_spec_info&0xFF;

	/* 发送AAC spec */
	SendAACSpec(body, body_len);
	fprintf(stdout, "AAC spec sent.\n");

	/* 读取AAC frame --去掉首部7个字节后得到AAC raw data, 拼装成RTMP packet--> 然后推流出去 */
	unsigned int tick = 0;
	AACFrame aacf;
	while(ReadAACFrameFromBuf(aacf))
	{
		// 发送AAC frame
		SendAudioPacket(aacf.data, aacf.size, tick);
		fprintf(stdout, "audio packet sent(len=%d).\n", aacf.size);

		//音频时间戳也可以从0开始计算，48K采样每帧递增21；44.1K采样每帧递增23
		//msleep(21);
		tick +=21;
	}

	return TRUE;
}

bool CRTMPStream::SendH264File(const char *pFileName)
{
	if(pFileName == NULL)
	{
		return FALSE;
	}
	FILE *fp = fopen(pFileName, "rb");  
	if(!fp)  
	{  
		printf("ERROR:open H264 file %s failed!",pFileName);
	}  
	fseek(fp, 0, SEEK_SET);
	m_nFileBufSize = fread(m_pFileBuf, sizeof(unsigned char), FILEBUFSIZE, fp);
	if(m_nFileBufSize >= FILEBUFSIZE)
	{
		printf("warning : File size is larger than BUFSIZE\n");
	}
	fclose(fp);  

	RTMPMetadata metaData;
	memset(&metaData,0,sizeof(RTMPMetadata));

	NaluUnit naluUnit;

	ReadOneNaluFromBuf(naluUnit);
	if(naluUnit.type == 6){ //如果开头是1个SEI帧，则跳过SEI帧
		ReadOneNaluFromBuf(naluUnit);
	}

	// 读取SPS帧
	metaData.nSpsLen = naluUnit.size;
	if(metaData.nSpsLen > SPS_MAXSIZE){
		fprintf(stdout, "invalid sps in h264 file whose length is %d (%d at most).\n", metaData.nSpsLen, SPS_MAXSIZE);
		return FALSE;
	}
	memcpy(metaData.Sps,naluUnit.data,naluUnit.size);

	// 读取PPS帧
	ReadOneNaluFromBuf(naluUnit);
	metaData.nPpsLen = naluUnit.size;
	if(metaData.nPpsLen > PPS_MAXSIZE){
		fprintf(stdout, "invalid pps in h264 file whose length is %d (%d at most).\n", metaData.nPpsLen, PPS_MAXSIZE);
		return FALSE;
	}
	memcpy(metaData.Pps,naluUnit.data,naluUnit.size);

	// 解码SPS,获取视频图像宽、高信息
	int width = 0,height = 0;
	h264_decode_sps(metaData.Sps,metaData.nSpsLen,width,height);
	metaData.nWidth = width;
	metaData.nHeight = height;
	metaData.nFrameRate = 25;
	fprintf(stdout, "width=%d, height=%d, frame rate=%d.\n", metaData.nWidth, metaData.nHeight, metaData.nFrameRate);

	// 发送MetaData
	SendMetadata(&metaData);
	fprintf(stdout, "metaData sent.\n");

	unsigned int tick = 0;
	while(ReadOneNaluFromBuf(naluUnit))
	{
		bool bKeyframe  = (naluUnit.type == 0x05) ? TRUE : FALSE;
		// 发送H264数据帧
		SendH264Packet(naluUnit.data,naluUnit.size,bKeyframe,tick);
		fprintf(stdout, "h264 video packet sent.\n");

		//视频时间戳可以从0开始计算，每帧时间戳 + 1000/fps (25fps每帧递增25；30fps递增33)
		//msleep(40);
		tick +=40;
	}

	return TRUE;
}

bool CRTMPStream::ReadAACFrameFromBuf(AACFrame &aacf){
	int i = m_nCurPos;

	while(i<m_nFileBufSize)
	{
		/************************************************************************/
		/*       AAC frame头部7个字节:FF F1 4C 40 0B BF FC                      */
		/************************************************************************/
		if(m_pFileBuf[i++] == 0xFF && (m_pFileBuf[i++]&0xF0) == 0xF0)
		{
			i+=5; //跳过头部7个字节, i在FC后面数据开始的位置
			int pos = i;
			while (pos<m_nFileBufSize)
			{
				if(m_pFileBuf[pos++] == 0xFF && (m_pFileBuf[pos++]&0xF0) == 0xF0)
				{
					break;
				}
			}
			//pos在下一个AAC frame的4C位置
			if(pos == m_nFileBufSize)
			{
				aacf.size = pos-i;
			}
			else
			{
				aacf.size = (pos-2)-i;
			}
			aacf.data = &m_pFileBuf[i];
			//printf("2=====size=%d, data=%02X%02X%02X\n", aacf.size, aacf.data[0], aacf.data[1], aacf.data[2]);

			m_nCurPos = pos-2;
			return TRUE;
		}
	}
	return FALSE;
}

bool CRTMPStream::ReadOneNaluFromBuf(NaluUnit &nalu)
{
	int i = m_nCurPos;
	while(i<m_nFileBufSize)
	{
		if(m_pFileBuf[i++] == 0x00 &&
			m_pFileBuf[i++] == 0x00 &&
			m_pFileBuf[i++] == 0x00 &&
			m_pFileBuf[i++] == 0x01
			)
		{
			int pos = i;
			while (pos<m_nFileBufSize)
			{
				if(m_pFileBuf[pos++] == 0x00 &&
					m_pFileBuf[pos++] == 0x00 &&
					m_pFileBuf[pos++] == 0x00 &&
					m_pFileBuf[pos++] == 0x01
					)
				{
					break;
				}
			}
			if(pos == m_nFileBufSize)
			{
				nalu.size = pos-i;	
			}
			else
			{
				nalu.size = (pos-4)-i;
			}
			nalu.type = m_pFileBuf[i]&0x1f;
			nalu.data = &m_pFileBuf[i];

			m_nCurPos = pos-4;
			return TRUE;
		}
	}
	return FALSE;
}
