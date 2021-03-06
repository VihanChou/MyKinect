// C++Socket.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

//【】Socket通信
#include <WINSOCK2.H>   

//【】线程
#include<thread>

//【】Socket
#define PORT           5150 //服务器端口
#define MSGSIZE        1024 //数据大小
#pragma comment(lib, "ws2_32.lib")

void Function4Thread( );
int main( )
{
	std::thread t1(Function4Thread);
	t1.join( );

	return 0;
}

void Function4Thread( )
{
	while (true)
	{
		if (MSGSIZE!=0)
		{
			WSADATA wsaData;
			SOCKET sListen;
			SOCKET sClient;

			SOCKADDR_IN local;
			SOCKADDR_IN client;

			char szMessage[MSGSIZE];
			int ret;
			int iaddrSize = sizeof(SOCKADDR_IN);

			WSAStartup(0x0202 , &wsaData);

			sListen = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);

			local.sin_family = AF_INET;
			local.sin_port = htons(PORT);
			local.sin_addr.s_addr = htonl(INADDR_ANY);

			bind(sListen , (struct sockaddr *) &local , sizeof(SOCKADDR_IN));
			listen(sListen , 1);

			printf("============================================\n");
			printf("远程控制服务已开启,等待控制设备接入\n");
			sClient = accept(sListen , (struct sockaddr *) &client , &iaddrSize);
			printf("控制设备已连接  %s:%d\n" , inet_ntoa(client.sin_addr) , ntohs(client.sin_port));


			while (TRUE)
			{
				ret = recv(sClient , szMessage , MSGSIZE , 0);
				szMessage[ret] = '\0';
				if (ret==0)
				{
					printf("控制设备已断开\n\n");
					WSACleanup( );
					break;
				}
				printf("Received [%d bytes]: '%s'\n" , ret , szMessage);
				if (strcmp(szMessage , "up") == 0)
				{
					printf("__________\n");
				}

			}
		}
	}
}
