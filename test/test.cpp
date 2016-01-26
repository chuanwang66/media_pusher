// test.cpp : 定义控制台应用程序的入口点。
/*
1. mp4 --> h264 (with ffmpeg)
To extract a raw video from an MP4 or FLV container you should specify the -bsf:v h264_mp4toannexb 
or -vbfs h264_mp4toannexb option.

ffmpeg -i quicksort.mp4 -vcodec copy -an -bsf:v h264_mp4toannexb quicksort.h264
参数含义：
    -an: disable audio stream
	-vn: disable video stream
	-sn: disable subtitle stream

The raw stream without H264 Annex B / NAL cannot be decode by player. With that option ffmpeg 
perform a simple "mux" of each h264 sample in a NAL unit.

2.mp4 --> aac (with ffmpeg)

得到.aac ==> ffmpeg -i input.mp4 -acodec copy output.aac
(得到.m4a ==> ffmpeg -i quicksort.mp4 -c:a copy -vn -sn quicksort.m4a)

注：
	.aac is a raw MPEG-4 Part 3 bitstream with audio stream encoded
	.m4a is a container format

3. run test.exe to push video out
4. 用VLC media player来播放rtmp视频流/音频流，可以方便设置播放端缓存（否则推流过快，播放端可能会直接丢弃rtmp包）

注：
1. rtmp服务器不清楚是否支持线程安全，所以最好采用队列发送.
2. 如果出现以下错误，一定要检查时间戳问题：
	ERROR: WriteN, RTMP send error 32 (136 bytes)
	ERROR: WriteN, RTMP send error 32 (39 bytes)
	ERROR: WriteN, RTMP send error 9 (42 bytes)
   音视频在一个队列里发送也会出现时间戳问题，音视频的时间戳相差时间不要太大，否则会报以上错误
3. libfaac.dll和它的导入库libfaac.lib是从faac-1.28\libfaac的VS工程直接编译出来的，自己编一把即可
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


