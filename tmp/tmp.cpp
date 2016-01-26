// tmp.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	unsigned char *p = new unsigned char[10];
	p[0] = 0xFF;
	p[1] = 0xF1;

	int i=0;
	if(p[i++] == 0xFF && (p[i++]&0xF0)){
		printf("a\n");
	}
	printf("b\n");
	return 0;
}

