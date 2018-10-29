//  KinectFirst.cpp : �������̨Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include "kinect.h"
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>  
using namespace cv;
using namespace std;


// ��ȫ�ͷ�ָ��
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease!=NULL)
	{
		pInterfaceToRelease->Release( );
		pInterfaceToRelease = NULL;
	}
}


//��ӡ��־�ķ���
inline void log(string str)
{
	cout<<"----------------------------"<<str<<endl;
}


//������
int _tmain(int argc , _TCHAR* argv[])
{
	//��1����ȡKinect�豸ָ�� 
	IKinectSensor* m_pKinectSensor;                //����һ������Ϊ  IKinectSensor��ָ�룬����ָ��һ��kinect�豸����
	HRESULT hr;
	hr = GetDefaultKinectSensor(&m_pKinectSensor);


	//�����ʼ��ʧ��
	if (FAILED(hr))
	{
		log("��ʼ��kinnect�豸ʧ��");
		return hr;
	}

	cout<<"the value of hr  "<<hr<<endl;

	log("��ʼ��kinnect�豸�ɹ�");



	//��2����ȡ������Դ����ȡ��
	IMultiSourceFrameReader* m_pMultiFrameReader = NULL;

	if (m_pKinectSensor)
	{
		//ʹ��ָ���ķ���ģʽ��Kinect��ʼ��ʽ��������
		hr = m_pKinectSensor->Open( );
		if (SUCCEEDED(hr))
		{
			// ��ȡ������Դ����ȡ��  m_pMultiFrameReader
			hr = m_pKinectSensor->OpenMultiSourceFrameReader(FrameSourceTypes::FrameSourceTypes_Color|FrameSourceTypes::FrameSourceTypes_Infrared|FrameSourceTypes::FrameSourceTypes_Depth , &m_pMultiFrameReader);
		}
	}

	if (!m_pKinectSensor||FAILED(hr))
	{
		return E_FAIL;
	}


	// ��������֡����
	IDepthFrameReference* m_pDepthFrameReference = NULL;
	IColorFrameReference* m_pColorFrameReference = NULL;
	IInfraredFrameReference* m_pInfraredFrameReference = NULL;

	//��������֡
	IInfraredFrame* m_pInfraredFrame = NULL;
	IDepthFrame* m_pDepthFrame = NULL;
	IColorFrame* m_pColorFrame = NULL;



	// ����ͼƬ��ʽ
	Mat i_rgb(1080 , 1920 , CV_8UC4);        //ע�⣺�������Ϊ4ͨ����ͼ��Kinect������ֻ����Bgra��ʽ����
	Mat i_depth(424 , 512 , CV_8UC1);
	Mat i_ir(424 , 512 , CV_16UC1);


	UINT16 *depthData = new UINT16[424*512];


	//system("pause");

	IMultiSourceFrame* m_pMultiFrame = nullptr;

	while (true)
	{

		// ��Ԫ����֡��ȡ-------------------------------------------------------------------------------------
		// ͨ����Ԫ����֡�Ķ�����ȡһ���µĶ�Դ����֡�������ַ�����ָ��m_pMultiFrame

		hr = m_pMultiFrameReader->AcquireLatestFrame(&m_pMultiFrame);

		if (FAILED(hr)||!m_pMultiFrame)
		{
			log("���ζ�Ԫ����֡��ȡʧ��  ------------------");
			continue;
		}
		else
		{
			log("��ȡ���µ�һ����Դ����֡++++++++++++++++++");
		}

		// ����֡��ȡ-------------------------------------------------------------------------------------
		//��1����ɫ����
		//1���Ӷ�Ԫ����֡��ò�ɫ����֡����
		if (SUCCEEDED(hr))
			hr = m_pMultiFrame->get_ColorFrameReference(&m_pColorFrameReference);
		//2:�Ӳ�ɫ����֡���û�ȡ��ɫ����֡
		if (SUCCEEDED(hr))
			hr = m_pColorFrameReference->AcquireFrame(&m_pColorFrame);

		//��2���������
		//1���Ӷ�Ԫ����֡������������֡����
		if (SUCCEEDED(hr))
			hr = m_pMultiFrame->get_DepthFrameReference(&m_pDepthFrameReference);
		//2:���������֡���ã���ȡ�������֡
		if (SUCCEEDED(hr))
			hr = m_pDepthFrameReference->AcquireFrame(&m_pDepthFrame);

		//��3���������
		//1���Ӷ�Ԫ����֡����ú�������֡����
		if (SUCCEEDED(hr))
			hr = m_pMultiFrame->get_InfraredFrameReference(&m_pInfraredFrameReference);
		//2:�Ӻ�������֡���ã���ȡ��������֡
		if (SUCCEEDED(hr))
			hr = m_pInfraredFrameReference->AcquireFrame(&m_pInfraredFrame);

		// ����֡����-------------------------------------------------------------------------------------
		// ����color������ͼƬ��
		UINT nColorBufferSize = 1920*1080*4;
		if (SUCCEEDED(hr))
			hr = m_pColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize , reinterpret_cast<BYTE*>(i_rgb.data) , ColorImageFormat::ColorImageFormat_Bgra);

		// ����depth������ͼƬ��
		if (SUCCEEDED(hr))
		{
			hr = m_pDepthFrame->CopyFrameDataToArray(424*512 , depthData);
			for (int i = 0; i<512*424; i++)
			{
				// 0-255���ͼ��Ϊ����ʾ���ԣ�ֻȡ������ݵĵ�8λ
				BYTE intensity = static_cast<BYTE>(depthData[i]%256);
				reinterpret_cast<BYTE*>(i_depth.data)[i] = intensity;
			}

			// ʵ����16λunsigned int����
			//hr = m_pDepthFrame->CopyFrameDataToArray(424 * 512, reinterpret_cast<UINT16*>(i_depth.data));
		}

		// ����infrared������ͼƬ��
		if (SUCCEEDED(hr))
		{
			hr = m_pInfraredFrame->CopyFrameDataToArray(424*512 , reinterpret_cast<UINT16*>(i_ir.data));
		}

		// ����֡��ʾ-------------------------------------------------------------------------------------------------
		imshow("rgb" , i_rgb);
		if (waitKey(1)==VK_ESCAPE)
			break;
		imshow("depth" , i_depth);
		if (waitKey(1)==VK_ESCAPE)
			break;
		imshow("ir" , i_ir);
		if (waitKey(1)==VK_ESCAPE)
			break;


		// �ͷ���Դ
		SafeRelease(m_pColorFrame);
		SafeRelease(m_pDepthFrame);
		SafeRelease(m_pInfraredFrame);
		SafeRelease(m_pColorFrameReference);
		SafeRelease(m_pDepthFrameReference);
		SafeRelease(m_pInfraredFrameReference);
		SafeRelease(m_pMultiFrame);

	}

	// �رմ��ڣ��豸
	cv::destroyAllWindows( );
	m_pKinectSensor->Close( );
	std::system("pause");

	return 0;
}