// Hello.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include<string>
#include<thread>



// 线程A 方法
void FunA( )
{
	// do something...
	for (int i = 1;i<=9;i++)
	{
		printf("AAAA  %d  \n" , i);
	}
}


// 线程B方法
void FunB( )
{
	// do something...
	for (int i = 1;i<=9;i++)
	{
		printf("BBBB  %d  \n" , i);
	}
}


int main( )
{


	std::thread t1(FunA);
	t1.join( );
	return 0;
}