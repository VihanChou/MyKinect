//  KinectFirst.cpp : 定义控制台应用程序的入口点。
#include "stdafx.h"
#include "kinect.h"
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>  
using namespace cv;
using namespace std;


// 安全释放指针
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


//主方法
int _tmain(int argc , _TCHAR* argv[])
{
	//【1】获取Kinect设备指针 
	IKinectSensor* m_pKinectSensor;                //声明一个类型为  IKinectSensor的指针，用于指向一个kinect设备对象
	HRESULT hr;
	hr = GetDefaultKinectSensor(&m_pKinectSensor);


	//如果初始化失败
	if (FAILED(hr))
	{
		log("初始化kinnect设备失败");
		return hr;
	}

	cout<<"the value of hr  "<<hr<<endl;

	log("初始化kinnect设备成功");



	//【2】获取多数据源到读取器
	IMultiSourceFrameReader* m_pMultiFrameReader = NULL;

	if (m_pKinectSensor)
	{
		//使用指定的访问模式从Kinect开始流式传输数据
		hr = m_pKinectSensor->Open( );
		if (SUCCEEDED(hr))
		{
			// 获取多数据源到读取器  m_pMultiFrameReader
			hr = m_pKinectSensor->OpenMultiSourceFrameReader(FrameSourceTypes::FrameSourceTypes_Color|FrameSourceTypes::FrameSourceTypes_Infrared|FrameSourceTypes::FrameSourceTypes_Depth , &m_pMultiFrameReader);
		}
	}

	if (!m_pKinectSensor||FAILED(hr))
	{
		return E_FAIL;
	}


	// 三个数据帧引用
	IDepthFrameReference* m_pDepthFrameReference = NULL;
	IColorFrameReference* m_pColorFrameReference = NULL;
	IInfraredFrameReference* m_pInfraredFrameReference = NULL;

	//三个数据帧
	IInfraredFrame* m_pInfraredFrame = NULL;
	IDepthFrame* m_pDepthFrame = NULL;
	IColorFrame* m_pColorFrame = NULL;



	// 三个图片格式
	Mat i_rgb(1080 , 1920 , CV_8UC4);        //注意：这里必须为4通道的图，Kinect的数据只能以Bgra格式传出
	Mat i_depth(424 , 512 , CV_8UC1);
	Mat i_ir(424 , 512 , CV_16UC1);


	UINT16 *depthData = new UINT16[424*512];


	//system("pause");

	IMultiSourceFrame* m_pMultiFrame = nullptr;

	while (true)
	{

		// 多元数据帧获取-------------------------------------------------------------------------------------
		// 通过多元数据帧阅读器获取一个新的多源数据帧并将其地址存放于指针m_pMultiFrame

		hr = m_pMultiFrameReader->AcquireLatestFrame(&m_pMultiFrame);

		if (FAILED(hr)||!m_pMultiFrame)
		{
			log("本次多元数据帧获取失败  ------------------");
			continue;
		}
		else
		{
			log("获取到新的一个多源数据帧++++++++++++++++++");
		}

		// 数据帧获取-------------------------------------------------------------------------------------
		//【1】彩色数据
		//1：从多元数据帧获得彩色数据帧引用
		if (SUCCEEDED(hr))
			hr = m_pMultiFrame->get_ColorFrameReference(&m_pColorFrameReference);
		//2:从彩色数据帧引用获取彩色数据帧
		if (SUCCEEDED(hr))
			hr = m_pColorFrameReference->AcquireFrame(&m_pColorFrame);

		//【2】深度数据
		//1：从多元数据帧，获得深度数据帧引用
		if (SUCCEEDED(hr))
			hr = m_pMultiFrame->get_DepthFrameReference(&m_pDepthFrameReference);
		//2:从深度数据帧引用，获取深度数据帧
		if (SUCCEEDED(hr))
			hr = m_pDepthFrameReference->AcquireFrame(&m_pDepthFrame);

		//【3】深度数据
		//1：从多元数据帧，获得红外数据帧引用
		if (SUCCEEDED(hr))
			hr = m_pMultiFrame->get_InfraredFrameReference(&m_pInfraredFrameReference);
		//2:从红外数据帧引用，获取红外数据帧
		if (SUCCEEDED(hr))
			hr = m_pInfraredFrameReference->AcquireFrame(&m_pInfraredFrame);

		// 数据帧拷贝-------------------------------------------------------------------------------------
		// 【】color拷贝到图片中
		UINT nColorBufferSize = 1920*1080*4;
		if (SUCCEEDED(hr))
			hr = m_pColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize , reinterpret_cast<BYTE*>(i_rgb.data) , ColorImageFormat::ColorImageFormat_Bgra);

		// 【】depth拷贝到图片中
		if (SUCCEEDED(hr))
		{
			hr = m_pDepthFrame->CopyFrameDataToArray(424*512 , depthData);
			for (int i = 0; i<512*424; i++)
			{
				// 0-255深度图，为了显示明显，只取深度数据的低8位
				BYTE intensity = static_cast<BYTE>(depthData[i]%256);
				reinterpret_cast<BYTE*>(i_depth.data)[i] = intensity;
			}

			// 实际是16位unsigned int数据
			//hr = m_pDepthFrame->CopyFrameDataToArray(424 * 512, reinterpret_cast<UINT16*>(i_depth.data));
		}

		// 【】infrared拷贝到图片中
		if (SUCCEEDED(hr))
		{
			hr = m_pInfraredFrame->CopyFrameDataToArray(424*512 , reinterpret_cast<UINT16*>(i_ir.data));
		}

		// 数据帧显示-------------------------------------------------------------------------------------------------
		imshow("rgb" , i_rgb);
		if (waitKey(1)==VK_ESCAPE)
			break;
		imshow("depth" , i_depth);
		if (waitKey(1)==VK_ESCAPE)
			break;
		imshow("ir" , i_ir);
		if (waitKey(1)==VK_ESCAPE)
			break;


		// 释放资源
		SafeRelease(m_pColorFrame);
		SafeRelease(m_pDepthFrame);
		SafeRelease(m_pInfraredFrame);
		SafeRelease(m_pColorFrameReference);
		SafeRelease(m_pDepthFrameReference);
		SafeRelease(m_pInfraredFrameReference);
		SafeRelease(m_pMultiFrame);

	}

	// 关闭窗口，设备
	cv::destroyAllWindows( );
	m_pKinectSensor->Close( );
	std::system("pause");

	return 0;
}