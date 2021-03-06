//  KinectFirst.cpp : 定义控制台应用程序的入口点。
#include "stdafx.h"
#include "kinect.h"
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>  
#include "KinectTest.h"
using namespace cv;
using namespace std;



// 安全释放指针的模板函数
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease!=NULL)
	{
		pInterfaceToRelease->Release( );
		pInterfaceToRelease = NULL;
	}
}

//打印日志的方法
inline void log(string str)
{
	cout<<"----------------------------"<<str<<endl;
}

HRESULT hr;
//主方法
int _tmain(int argc , _TCHAR* argv[])
{
	//与数据源相关的指针
	IKinectSensor* m_pKinectSensor;                   //声明一个类型为  IKinectSensor的指针，用于指向一个kinect设备对象
	IDepthFrameSource* m_pDepthFrameSource = NULL;    //深度帧数据源
	IDepthFrameReader* m_pDepthFrameReader = NULL;    //深度帧数据源读取器
	IDepthFrame* pDepthFrame = NULL;                  //深度帧数据

	UINT16 *depthData = new UINT16[424*512];          //用于装下深度帧的数据

   //用于存放帧数据的图片对象
	Mat i_rgb(1080 , 1920 , CV_8UC4);  //rgb     //注意：这里必须为4通道的图，Kinect的数据只能以Bgra格式传出
	Mat i_depth(424 , 512 , CV_8UC1);  //深度
	Mat i_ir(424 , 512 , CV_16UC1);    //红外

	//【1】获取Kinect设备  m_pKinectSensor
	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	//如果初始化失败
	if (FAILED(hr))
	{
		log("初始化kinnect设备失败");
		return hr;
	}
	else
	{
		log("初始化kinnect设备成功！");
	}

	//【2】打开kinect设备
	hr = m_pKinectSensor->Open( );
	if (FAILED(hr))
	{
		log("kinnect设备打开失败");
		return hr;
	}
	else
	{
		log("kinnect设备打开成功！");
	}

	//【3】根据设备获取帧数据源
	hr = m_pKinectSensor->get_DepthFrameSource(&m_pDepthFrameSource);
	if (FAILED(hr))
	{
		log("m_pDepthFrameSource获取失败");
		return hr;
	}

	//【4】使用帧数据源打开帧数据阅读器
	hr = m_pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
	if (FAILED(hr))
	{
		log("m_pDepthFrameReader获取失败");
		return hr;
	}

	while (true)
	{
		//【5】使用帧阅读器来获取帧数据，可以对帧数据进行不间断的获取和显示来作为视频
		hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
		if (FAILED(hr)||!pDepthFrame)
		{
			log("深度数据帧获取失败     ------------------");
			continue;
		}
		else
		{
			log("获取到新的一个多源数据帧++++++++++++++++++");
		}

		// 【】depth拷贝到图片中
		if (SUCCEEDED(hr))
		{
			//hr = pDepthFrame->CopyFrameDataToArray(424*512 , depthData);
			hr = pDepthFrame->CopyFrameDataToArray(424*512 , reinterpret_cast<UINT16*>(i_depth.data));
			for (int i = 0; i<512*424; i++)
			{

				BYTE intensity = static_cast<BYTE>(depthData[i]%256);   // 0-255深度图，为了显示明显，只取深度数据的低8位
				reinterpret_cast<BYTE*>(i_depth.data)[i] = intensity;
			}

			// 实际是16位unsigned int数据
			//hr = m_pDepthFrame->CopyFrameDataToArray(424 * 512, reinterpret_cast<UINT16*>(i_depth.data));


		// 数据帧显示-------------------------------------------------------------------------------------------------
			imshow("depth" , i_depth);                               //openCV显示图片
			if (waitKey(1)==VK_ESCAPE)
				break;
			imwrite("C:/Users/1234/Desktop/i_depth.jpg" , i_depth);  //openCV保存图片

			cout<<"sizeof(depthData)  =  "<<sizeof(*depthData)<<endl;
			for (size_t i = 0; i<512; i++)
			{
				cout<<depthData[i]<<endl;
			}

			// 释放资源
			SafeRelease(pDepthFrame);
			break;
		}

		// 关闭窗口，设备
		cv::destroyAllWindows( );
		m_pKinectSensor->Close( );
		std::system("pause");

		return 0;
	}