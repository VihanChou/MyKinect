// ConsoleTest.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include"VihanClass.h"
#include <stdio.h>
#include <stdlib.h>
#include<iostream>
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
	VihanClass myclass;

	std::cout<<"内容"<<myclass.testVar<<std::endl;

	std::thread t1(FunA);
	std::thread t2(FunB);
	system("pause");

	return 0;
}