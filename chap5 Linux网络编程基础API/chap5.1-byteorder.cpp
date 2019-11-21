/***********************
《Linux高性能服务器编程》游双  著



代码清单5-1：判断机器字节序
*******************************/

#include <stdio.h>
void byteorder()
{
	union MyUnion
	{
		short value;
		char union_bytes[sizeof(short)];
	} test;

	test.value = 0x0102;
	if ((test.union_bytes[0] == 1) && (test.union_bytes[1] == 2))
	{
		printf("Big  endian. \r\n");//网络字节序，大端对齐（高位在前面）
	}
	else if ((test.union_bytes[1] == 1) && (test.union_bytes[0] == 2))
	{
		printf("little  endian. \r\n");//主机字节序，小端对齐
	}
	else
	{
		printf("unkonwn...\r\n");
	}

}

void main()
{
	byteorder();

	getchar();
}

