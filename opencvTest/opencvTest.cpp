// opencvTest.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <iostream>


using namespace cv;
using namespace std;

//Author :Vihan 
//Emainl :vihanmy@gmail.com
//Time   :
//Note   :用于OpenCV的学习，测试

int main( )
{
	//打开一张图片并显示
	//Mat image = imread("C:/Users/1234/Desktop/C++/WorkSpace/C++ProLearningSolution/img.jpg");  //存放自己图像的路径 
	//imshow("显示图像" , image);
	//cvWaitKey(0);

	Mat img(10 , 10 , CV_8UC3 , Scalar(33 , 66 , 99)); // Scalar()设置初始化值，依次是b,g,r

	cout<<"像素数据 "<<img<<endl;
	cout<<"矩阵维度 "<<img.dims<<endl;
	cout<<"行数     "<<img.rows<<endl;
	cout<<"列数     "<<img.cols<<endl;
	cout<<"矩阵大小 "<<img.size( )<<endl;
	cout<<"通道数   "<<img.channels( )<<endl;


	//查看某个点的像素
	//10x10  的图像从【0，0】到【9，9】
	Vec3b intensity = img.at<Vec3b>(9 , 9);
	uchar blue = intensity.val[0];
	uchar green = intensity.val[1];
	uchar red = intensity.val[2];


	cout<<"内容"<<intensity<<endl;


	imshow("test" , img);
	int temp = waitKey(0);


	cout<<"	 waitKey(0);"<<temp<<endl;

	cout<<"内容"<<"waitKey已经被执行"<<endl;

	//system("pause");
	return 0;
}

