/******************************************************************** 
filename:   RTMPStream.h
created:    2013-04-3
author:     firehood 
purpose:    ����H264��Ƶ��RTMP Server��ʹ��libRtmp��
*********************************************************************/ 
#pragma once
#include "rtmp.h"
#include "rtmp_sys.h"
#include "faac.h"
#include "amf.h"
#include <stdio.h>

#define SPS_MAXSIZE 1024
#define PPS_MAXSIZE 1024
#define FILEBUFSIZE (1024 * 1024 * 10)       //  10M

#define STREAM_CHANNEL_METADATA  0x03
#define STREAM_CHANNEL_VIDEO     0x04
#define STREAM_CHANNEL_AUDIO     0x05

// NALU��Ԫ
typedef struct _NaluUnit
{
	int type;
	int size;
	unsigned char *data;
}NaluUnit;

//AAC Frame�����AAC Frameֱ�ӹ���.aac������ÿ��AAC Frame��7���ֽ�AAC Frame header��ͷ��
typedef struct _AACFrame
{
	int size;
	unsigned char *data;
}AACFrame;

typedef struct _RTMPMetadata
{
	// video, must be h264 type
	unsigned int	nWidth;
	unsigned int	nHeight;
	unsigned int	nFrameRate;		// fps
	unsigned int	nVideoDataRate;	// bps
	unsigned int	nSpsLen;
	unsigned char	Sps[SPS_MAXSIZE];
	unsigned int	nPpsLen;
	unsigned char	Pps[PPS_MAXSIZE];

	// audio, must be aac type
	bool	        bHasAudio;
	unsigned int	nAudioSampleRate;
	unsigned int	nAudioSampleSize;
	unsigned int	nAudioChannels;
	char		    pAudioSpecCfg;
	unsigned int	nAudioSpecCfgLen;

} RTMPMetadata,*LPRTMPMetadata;


class CRTMPStream
{
public:
	CRTMPStream(void);
	~CRTMPStream(void);
public:
	// ���ӵ�RTMP Server
	bool Connect(const char* url);
	// �Ͽ�����
	void Close();
	// ����MetaData
	bool SendMetadata(LPRTMPMetadata lpMetaData);
	// ����H264����֡
	bool SendH264Packet(unsigned char *data,unsigned int size,bool bIsKeyFrame,unsigned int nTimeStamp);
	// ����H264�ļ�
	bool SendH264File(const char *pFileName);
	// ����AAC�ļ�
	bool SendAACFile(const char* pFileName);
private:
	// �ͻ����ж�ȡһ��NALU��
	bool ReadOneNaluFromBuf(NaluUnit &nalu);
	// ��������
	int SendVideoPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp);

	//����AAC audio data
	bool ReadAACFrameFromBuf(AACFrame &aacf);
	//����AAC spec
	bool SendAACSpec(unsigned char *spec_buf, int spec_len);
	//����AAC encoded data
	bool SendAudioPacket(unsigned char *buf, int len, unsigned int nTimeStamp);
private:
	RTMP* m_pRtmp;
	unsigned char* m_pFileBuf;
	unsigned int  m_nFileBufSize;
	unsigned int  m_nCurPos;
};
