/******************************************************************** 
filename:   RTMPStream.h
created:    2013-04-3
author:     firehood 
purpose:    发送H264视频到RTMP Server，使用libRtmp库
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

// NALU单元
typedef struct _NaluUnit
{
	int type;
	int size;
	unsigned char *data;
}NaluUnit;

//AAC Frame（多个AAC Frame直接构成.aac，其中每个AAC Frame由7个字节AAC Frame header开头）
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
	// 连接到RTMP Server
	bool Connect(const char* url);
	// 断开连接
	void Close();
	// 发送MetaData
	bool SendMetadata(LPRTMPMetadata lpMetaData);
	// 发送H264数据帧
	bool SendH264Packet(unsigned char *data,unsigned int size,bool bIsKeyFrame,unsigned int nTimeStamp);
	// 发送H264文件
	bool SendH264File(const char *pFileName);
	// 发送AAC文件
	bool SendAACFile(const char* pFileName);
private:
	// 送缓存中读取一个NALU包
	bool ReadOneNaluFromBuf(NaluUnit &nalu);
	// 发送数据
	int SendVideoPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp);

	//发送AAC audio data
	bool ReadAACFrameFromBuf(AACFrame &aacf);
	//发送AAC spec
	bool SendAACSpec(unsigned char *spec_buf, int spec_len);
	//发送AAC encoded data
	bool SendAudioPacket(unsigned char *buf, int len, unsigned int nTimeStamp);
private:
	RTMP* m_pRtmp;
	unsigned char* m_pFileBuf;
	unsigned int  m_nFileBufSize;
	unsigned int  m_nCurPos;
};
