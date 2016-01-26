// test.cpp : �������̨Ӧ�ó������ڵ㡣
/*
1. mp4 --> h264 (with ffmpeg)
To extract a raw video from an MP4 or FLV container you should specify the -bsf:v h264_mp4toannexb 
or -vbfs h264_mp4toannexb option.

ffmpeg -i quicksort.mp4 -vcodec copy -an -bsf:v h264_mp4toannexb quicksort.h264
�������壺
    -an: disable audio stream
	-vn: disable video stream
	-sn: disable subtitle stream

The raw stream without H264 Annex B / NAL cannot be decode by player. With that option ffmpeg 
perform a simple "mux" of each h264 sample in a NAL unit.

2.mp4 --> aac (with ffmpeg)

�õ�.aac ==> ffmpeg -i input.mp4 -acodec copy output.aac
(�õ�.m4a ==> ffmpeg -i quicksort.mp4 -c:a copy -vn -sn quicksort.m4a)

ע��
	.aac is a raw MPEG-4 Part 3 bitstream with audio stream encoded
	.m4a is a container format

3. run test.exe to push video out
4. ��VLC media player������rtmp��Ƶ��/��Ƶ�������Է������ò��Ŷ˻��棨�����������죬���Ŷ˿��ܻ�ֱ�Ӷ���rtmp����

ע��
1. rtmp������������Ƿ�֧���̰߳�ȫ��������ò��ö��з���.
2. ����������´���һ��Ҫ���ʱ������⣺
	ERROR: WriteN, RTMP send error 32 (136 bytes)
	ERROR: WriteN, RTMP send error 32 (39 bytes)
	ERROR: WriteN, RTMP send error 9 (42 bytes)
   ����Ƶ��һ�������﷢��Ҳ�����ʱ������⣬����Ƶ��ʱ������ʱ�䲻Ҫ̫�󣬷���ᱨ���ϴ���
3. libfaac.dll�����ĵ����libfaac.lib�Ǵ�faac-1.28\libfaac��VS����ֱ�ӱ�������ģ��Լ���һ�Ѽ���
*/

#include "stdafx.h"

//#pragma comment(lib,"WS2_32.lib")
//#pragma comment(lib,"winmm.lib")
//#pragma comment(lib, "zlib.lib")

#include <stdio.h>
#include "RTMPStream.h"

#ifdef WIN32
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	//////////////////////////////////////////////////////////////////current path
	char curPath[256];
	GetCurrentDir(curPath, sizeof(curPath));
	fprintf(stdout, "the current working directory is %s.\n", curPath);

	//////////////////////////////////////////////////////////////////
	CRTMPStream rtmpSender;
	bool bRet = rtmpSender.Connect("rtmp://120.132.42.152/live/sam_sam_test");
	//rtmpSender.SendH264File("..\\quicksort.h264"); //push rtmp from h264
	rtmpSender.SendAACFile("..\\quicksort.aac"); //push rtmp from aac
	rtmpSender.Close();

	fprintf(stdout, "sent out.\n");

	return 0;
}


