﻿// System includes  
#include "stdafx.h"
#include <string>
#include <strsafe.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <new>
#include<iostream>

#include "resource.h"
#include "KinectFusionBasics.h"

//██Added  by Vihan
#include "KinectFusionParams.h"
#include "KinectFusionHelper.h"

//【】Socket通信
#include <WINSOCK2.H>   

//【】线程
#include<thread>

//【】Socket 服务器控制相关
#define PORT           5150 //服务器端口
#define MSGSIZE        1024 //数据大小
#pragma comment(lib, "ws2_32.lib")

#define MSG_CONNECT 8899
#define MSG_DISCONNECT 8898


HWND m_hWnd;
int mainThreID;
CKinectFusionBasics myapplication;
//应用Style 声明
#pragma comment(linker,"\"/manifestdependency:type='win32' "\
						"name='Microsoft.Windows.Common-Controls' "\
						"version='6.0.0.0' processorArchitecture='*' "\
						"publicKeyToken='6595b64144ccf1df' language='*'\"")
HRESULT SaveMeshFile(INuiFusionColorMesh* pMesh , KinectFusionMeshTypes saveMeshType);
void  ThreadFun4Contrllor( );

//██程序入口
int APIENTRY wWinMain(_In_ HINSTANCE hInstance , _In_opt_ HINSTANCE , _In_ LPWSTR , _In_ int nCmdShow)
{
	CKinectFusionBasics application;
	//初始化控制台窗口
	//AllocConsole( );
	//freopen("CONOUT$" , "w+t" , stdout);
	//freopen("CONIN$" , "r+t" , stdin);
	//std::cout<<"程序启动调试,日志窗口成功打开"<<std::endl;
	std::thread t1(ThreadFun4Contrllor);
	myapplication = application;
	t1.detach( );
	application.Run(hInstance , nCmdShow);
}

//██设置矩阵  在Helper中已经被定义过,在这里不进行再次定义
/// Set Identity in a Matrix4
/// <param name="mat">The matrix to set to identity
//void SetIdentityMatrix(Matrix4 &mat)
//{
//	mat.M11 = 1; mat.M12 = 0; mat.M13 = 0; mat.M14 = 0;
//	mat.M21 = 0; mat.M22 = 1; mat.M23 = 0; mat.M24 = 0;
//	mat.M31 = 0; mat.M32 = 0; mat.M33 = 1; mat.M34 = 0;
//	mat.M41 = 0; mat.M42 = 0; mat.M43 = 0; mat.M44 = 1;
//}

//██构造器，初始化四十个变量
CKinectFusionBasics::CKinectFusionBasics( ) :
	m_pD2DFactory(nullptr) ,
	m_pDrawDepth(nullptr) ,
	m_pVolume(nullptr) ,
	m_pNuiSensor(nullptr) ,
	m_cDepthImagePixels(0) ,
	m_bMirrorDepthFrame(false) ,
	m_bTranslateResetPoseByMinDepthThreshold(true) ,
	m_bAutoResetReconstructionWhenLost(false) ,
	m_bAutoResetReconstructionOnTimeout(true) ,
	m_lastFrameTimeStamp(0) ,
	m_bResetReconstruction(false) ,
	m_cLostFrameCounter(0) ,
	m_bTrackingFailed(false) ,
	m_cFrameCounter(0) ,
	m_fStartTime(0) ,
	m_pMapper(nullptr) ,
	m_pDepthImagePixelBuffer(nullptr) ,
	m_pDepthDistortionMap(nullptr) ,
	m_pDepthDistortionLT(nullptr) ,
	m_pDepthFloatImage(nullptr) ,
	m_pPointCloud(nullptr) ,
	m_pShadedSurface(nullptr) ,
	m_bInitializeError(false) ,
	m_pDepthFrameReader(NULL) ,
	m_coordinateMappingChangedEvent(NULL) ,
	m_bHaveValidCameraParameters(false)
{
	// 从枚举 NUI_IMAGE_RESOLUTION 获得深渡帧的大小
	// You can use NUI_IMAGE_RESOLUTION_640x480 or NUI_IMAGE_RESOLUTION_320x240 in this sample
	// 较小的分辨率有利于计算机对每一帧的计算处理，但是这意味着重建结果的细节将会有一定程度的缺失
	m_cDepthWidth = NUI_DEPTH_RAW_WIDTH;    //512
	m_cDepthHeight = NUI_DEPTH_RAW_HEIGHT;  //424
	m_cDepthImagePixels = m_cDepthWidth*m_cDepthHeight; //像素点的格式 高X宽

	// create heap storage for depth pixel data in RGBX format
	m_pDepthRGBX = new BYTE[m_cDepthImagePixels * cBytesPerPixel];

	//定义一个立方Kinect融合重建体积，Kinect位于前面的中心，体积直接位于Kinect的前面。
	m_reconstructionParams.voxelsPerMeter = 256;// 1000mm / 256vpm = ~3.9mm/voxel    （体素）
	m_reconstructionParams.voxelCountX = 384;   // 384 / 256vpm = 1.5m wide reconstruction
	m_reconstructionParams.voxelCountY = 384;   // Memory = 384*384*384 * 4bytes per voxel
	m_reconstructionParams.voxelCountZ = 384;   // This will require a GPU with at least 256MB

	// These parameters are for optionally clipping the input depth image 
	//这些参数用于可选地裁剪输入深度图像。
	m_fMinDepthThreshold = NUI_FUSION_DEFAULT_MINIMUM_DEPTH;   // min depth in meters
	m_fMaxDepthThreshold = 8.0f;    // max depth in meters

	//这个参数是用于重建的深度积分的时间平均参数。
	m_cMaxIntegrationWeight = NUI_FUSION_DEFAULT_INTEGRATION_WEIGHT;	// Reasonable for static scenes

	//此参数设置是否使用GPU或CPU处理。请注意，CPU可能会太慢，无法实时处理。
	m_processorType = NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_AMP; //AMP:GPU  CPU:CPU

	//如果选择了GPU处理，我们可以通过设置这个基于零的索引参数来选择要用于处理的设备的索引。
	//注意，设置-1将导致自动选择最合适的设备（特别是具有最大内存的DirectX11兼容设备），
	//当只需要一个重建卷时，这对于具有多个GPU的系统很有用。
	//注意，自动选择不会跨多个GPU负载平衡，因此当需要多个重建卷时，用户应该手动选择GPU索引，每个重建卷在一个单独的设备上。

	m_deviceIndex = -1;    // automatically choose device index for processing

	//初始化变量m_worldToCameraTransform ，m_defaultWorldToVolumeTransform
	SetIdentityMatrix(m_worldToCameraTransform);
	SetIdentityMatrix(m_defaultWorldToVolumeTransform);

	//在对象创建时我们不知道这些值，所以我们使用标称值。这些值稍后将根据.MappingChanged事件进行更新。
	//设置相机内参
	m_cameraParameters.focalLengthX = NUI_KINECT_DEPTH_NORM_FOCAL_LENGTH_X;
	m_cameraParameters.focalLengthY = NUI_KINECT_DEPTH_NORM_FOCAL_LENGTH_Y;
	m_cameraParameters.principalPointX = NUI_KINECT_DEPTH_NORM_PRINCIPAL_POINT_X;
	m_cameraParameters.principalPointY = NUI_KINECT_DEPTH_NORM_PRINCIPAL_POINT_Y;

}

//██析构函数
CKinectFusionBasics::~CKinectFusionBasics( )
{
	// Clean up Kinect Fusion
	SafeRelease(m_pVolume);
	SafeRelease(m_pMapper);



	if (nullptr!=m_pMapper)
		m_pMapper->UnsubscribeCoordinateMappingChanged(m_coordinateMappingChangedEvent);

	SAFE_FUSION_RELEASE_IMAGE_FRAME(m_pShadedSurface);
	SAFE_FUSION_RELEASE_IMAGE_FRAME(m_pPointCloud);
	SAFE_FUSION_RELEASE_IMAGE_FRAME(m_pDepthFloatImage);

	// done with depth frame reader
	SafeRelease(m_pDepthFrameReader);

	// Clean up Kinect
	if (m_pNuiSensor)
	{
		m_pNuiSensor->Close( );
		m_pNuiSensor->Release( );
	}

	// clean up the depth pixel array
	SAFE_DELETE_ARRAY(m_pDepthImagePixelBuffer);

	SAFE_DELETE_ARRAY(m_pDepthDistortionMap);
	SAFE_DELETE_ARRAY(m_pDepthDistortionLT);

	//清理DAT2D渲染器
	// clean up Direct2D renderer
	SAFE_DELETE(m_pDrawDepth);

	// done with depth pixel data
	SAFE_DELETE_ARRAY(m_pDepthRGBX);

	// clean up Direct2D
	SafeRelease(m_pD2DFactory);
}

//██创建主窗口并开始处理点云
int CKinectFusionBasics::Run(HINSTANCE hInstance , int nCmdShow)
{
	MSG       msg = { 0 };
	WNDCLASS  wc;

	//Dialog custom window class
	//窗口初始化
	ZeroMemory(&wc , sizeof(wc));
	wc.style = CS_HREDRAW|CS_VREDRAW;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursorW(nullptr , MAKEINTRESOURCE(IDC_ARROW));
	wc.hIcon = LoadIconW(hInstance , MAKEINTRESOURCE(IDI_APP));
	wc.lpfnWndProc = DefDlgProcW;
	wc.lpszClassName = L"KinectFusionBasicsAppDlgWndClass";
	if (!RegisterClassW(&wc))  //窗口注册失败的时候，退出
	{
		return 0;
	}
	// Create main application window
	//在此处绑定 IDD_APP窗口作为程序窗口
	HWND hWndApp = CreateDialogParamW(hInstance , MAKEINTRESOURCE(IDD_APP) , nullptr , (DLGPROC)CKinectFusionBasics::MessageRouter , reinterpret_cast<LPARAM>(this));

	// Show window
	ShowWindow(hWndApp , nCmdShow);
	//m_hWnd = hWndApp;
	//消息循环
	// Main message loop
	while (WM_QUIT!=msg.message)
	{
		//显式地检查Kinect帧事件，因为MSGWAITTimeType对象可以返回其他原因，即使它被发出信号。
		// Explicitly check the Kinect frame event since MsgWaitForMultipleObjects
		// can return for other reasons even though it is signaled.
		Update( );

		while (PeekMessageW(&msg , nullptr , 0 , 0 , PM_REMOVE))
		{
			// If a dialog message will be taken care of by the dialog proc
			if ((hWndApp!=nullptr)&&IsDialogMessageW(hWndApp , &msg))
			{
				continue;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}

//██程序主要的处理过程
/// Main processing function
void CKinectFusionBasics::Update( )
{
	//【】检测硬件初始化
	if (nullptr==m_pNuiSensor)
	{
		return;
	}
	//【】单例程序等待
	if (m_coordinateMappingChangedEvent!=NULL&&WAIT_OBJECT_0==WaitForSingleObject((HANDLE)m_coordinateMappingChangedEvent , 0))
	{
		OnCoordinateMappingChanged( );
		ResetEvent((HANDLE)m_coordinateMappingChangedEvent);
	}
	//【】相机初始化
	if (!m_bHaveValidCameraParameters)
	{
		return;
	}

	m_bResetReconstruction = false;
	//【】深渡帧阅读器初始化检查
	if (!m_pDepthFrameReader)
	{
		return;
	}

	IDepthFrame* pDepthFrame = NULL;

	//【】获取深渡帧
	HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);

	if (SUCCEEDED(hr))
	{
		UINT nBufferSize = 0;
		UINT16 *pBuffer = NULL;
		INT64 currentTimestamp = 0;

		//【】获取深渡帧关联的时间戳
		hr = pDepthFrame->get_RelativeTime(&currentTimestamp);
		if (SUCCEEDED(hr)&&currentTimestamp-m_lastFrameTimeStamp>cResetOnTimeStampSkippedMilliseconds*10000&&0!=m_lastFrameTimeStamp)
		{
			m_bResetReconstruction = true;
		}

		//【】更新最新时间戳
		m_lastFrameTimeStamp = currentTimestamp;

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize , &pBuffer);
		}

		if (SUCCEEDED(hr))
		{
			//copy and remap depth
			//【】赋值, 重构深度数据
			const UINT bufferLength = m_cDepthImagePixels;
			UINT16 * pDepth = m_pDepthImagePixelBuffer;
			for (UINT i = 0; i<bufferLength; i++ , pDepth++)
			{
				const UINT id = m_pDepthDistortionLT[i];
				*pDepth = id<bufferLength ? pBuffer[id] : 0;
			}
			//【】 进行 KinectFusion
			ProcessDepth( );
		}
	}

	SafeRelease(pDepthFrame);
}

//██消息路由
LRESULT CALLBACK CKinectFusionBasics::MessageRouter(HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam)
{
	CKinectFusionBasics* pThis = nullptr;

	if (WM_INITDIALOG==uMsg)
	{
		pThis = reinterpret_cast<CKinectFusionBasics*>(lParam);
		SetWindowLongPtr(hWnd , GWLP_USERDATA , reinterpret_cast<LONG_PTR>(pThis));
	}
	else
	{
		pThis = reinterpret_cast<CKinectFusionBasics*>(::GetWindowLongPtr(hWnd , GWLP_USERDATA));
	}

	if (pThis)
	{
		return pThis->DlgProc(hWnd , uMsg , wParam , lParam);
	}
	return 0;
}

//██窗口过程
//--------------------------------------------------------------------窗口过程---------------------------------------------------------------------------------
LRESULT CALLBACK CKinectFusionBasics::DlgProc(HWND hWnd , UINT message , WPARAM wParam , LPARAM lParam)
{
	int wmid , wmEvent;
	switch (message)
	{
		////【】对话框创建后, 对数据进行初始化
		case WM_INITDIALOG:
		{
			// Bind application window handle
			//[]绑定窗口句柄
			m_hWnd = hWnd;

			// Init Direct2D
			//[]初始化 Direct2D
			D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED , &m_pD2DFactory);

			// Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
			// We'll use this to draw the data we receive from the Kinect to the screen
			//[]初始化Direct2D渲染器,用于绘制从Kinect收到的数据
			m_pDrawDepth = new ImageRenderer( );
			HRESULT hr = m_pDrawDepth->Initialize(GetDlgItem(m_hWnd , IDC_VIDEOVIEW) , m_pD2DFactory , m_cDepthWidth , m_cDepthHeight , m_cDepthWidth*sizeof(int));
			if (FAILED(hr))
			{
				SetStatusMessage(L"Failed to initialize the Direct2D draw device.");
				m_bInitializeError = true;
			}

			//[]Kinect摄像头初始化  Look for a connected Kinect, and create it if found
			hr = CreateFirstConnected( );
			if (FAILED(hr))
			{
				m_bInitializeError = true;
			}
			//[]Kinectfusion初始化
			if (!m_bInitializeError)
			{
				hr = InitializeKinectFusion( );
				if (FAILED(hr))
				{
					m_bInitializeError = true;
				}
			}
		}
		break;


		//case WM_CTLCOLORSTATIC://拦截WM_CTLCOLORSTATIC消息
		//{
		//	if ((HWND)lParam==GetDlgItem(hWnd , IDC_remote))//获得指定标签句柄用来对比
		//	{
		//		SetTextColor((HDC)wParam , RGB(255 , 0 , 0));//设置文本颜色
		//		SetBkMode((HDC)wParam , TRANSPARENT);//设置背景透明
		//	}
		//	return (INT_PTR)GetStockObject((NULL_BRUSH));//返回一个空画刷(必须)
		//}
		// 窗口关闭消息
		case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

		case WM_DESTROY:
		// Quit the main message pump
		PostQuitMessage(0);
		break;

		// Handle button press
		case WM_COMMAND:
		{
			wmid = LOWORD(wParam);               //消息解析
			wmEvent = HIWORD(wParam);

			switch (wmid)
			{
				case IDC_BUTTON_GETFILE:        //showButton的按键消息
				{
					SetStatusMessage(L"IDC_BUTTON_GETFILE");
					INuiFusionColorMesh *mesh = nullptr;
					m_pVolume->CalculateMesh(1 , &mesh);
					SaveMeshFile(mesh , Stl);
					break;
				}
				case IDC_BUTTON_RESET_RECONSTRUCTION:    //showButton的按键消息
				{
					ResetReconstruction( );
					break;
				}
				/*	case IDC_remote:
					{

						break;
					}*/
				case IDC_test:
				{
					SetStatusMessage(L"test");
					//// 设置字体参数
					//LOGFONT LogFont;
					//::memset(&LogFont , 0 , sizeof(LOGFONT));
					//LogFont.lfWeight = 400;
					//LogFont.lfHeight = -44; // 字体大小
					//LogFont.lfCharSet = 134;
					//LogFont.lfOutPrecision = 3;
					//LogFont.lfClipPrecision = 2;
					//LogFont.lfOrientation = 45;
					//LogFont.lfQuality = 1;
					//LogFont.lfPitchAndFamily = 2;

					//// 创建字体
					//HFONT hFont = CreateFontIndirect(&LogFont);

					//// 取得控件句柄
					//HWND hWndStatic = GetDlgItem(hWnd , IDC_remote);

					//// 设置控件字体
					//::SendMessage(hWndStatic , WM_SETFONT , (WPARAM)hFont , 0);




					break;
				}
				case MSG_CONNECT:
				{
					SendDlgItemMessageW(m_hWnd , IDC_remote , WM_SETTEXT , 0 , (LPARAM)(L"控制设备已连接"));
					break;
				}
				case MSG_DISCONNECT:
				{
					SendDlgItemMessageW(m_hWnd , IDC_remote , WM_SETTEXT , 0 , (LPARAM)(L"控制设备已断开"));
					break;
				}

				default:
				{
					break;
				}
			}
		}
		break;
	}

	return FALSE;
}


//██初始化Kinect摄像头
HRESULT CKinectFusionBasics::CreateFirstConnected( )
{
	HRESULT hr;

	hr = GetDefaultKinectSensor(&m_pNuiSensor);
	if (FAILED(hr))
	{
		return hr;
	}

	if (m_pNuiSensor)
	{
		// Initialize the Kinect and get the depth reader
		IDepthFrameSource* pDepthFrameSource = NULL;

		hr = m_pNuiSensor->Open( );

		if (SUCCEEDED(hr))
		{
			hr = m_pNuiSensor->get_CoordinateMapper(&m_pMapper);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pNuiSensor->get_DepthFrameSource(&pDepthFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pMapper->SubscribeCoordinateMappingChanged(&m_coordinateMappingChangedEvent);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
		}

		SafeRelease(pDepthFrameSource);
	}

	if (nullptr==m_pNuiSensor||FAILED(hr))
	{
		SetStatusMessage(L"No ready Kinect found!");
		return E_FAIL;
	}

	return hr;
}


//██
void UpdateIntrinsics(NUI_FUSION_IMAGE_FRAME * pImageFrame , NUI_FUSION_CAMERA_PARAMETERS * params)
{
	if (pImageFrame!=nullptr && pImageFrame->pCameraParameters!=nullptr && params!=nullptr)
	{
		pImageFrame->pCameraParameters->focalLengthX = params->focalLengthX;
		pImageFrame->pCameraParameters->focalLengthY = params->focalLengthY;
		pImageFrame->pCameraParameters->principalPointX = params->principalPointX;
		pImageFrame->pCameraParameters->principalPointY = params->principalPointY;
	}

	// Confirm we are called correctly
	_ASSERT(pImageFrame!=nullptr && pImageFrame->pCameraParameters!=nullptr && params!=nullptr);
}

//██ 设置或更新连接摄像机的不失真计算
HRESULT CKinectFusionBasics::SetupUndistortion( )
{
	HRESULT hr = E_UNEXPECTED;

	if (m_cameraParameters.principalPointX!=0)
	{

		CameraSpacePoint cameraFrameCorners[4] = //at 1 meter distance. Take into account that depth frame is mirrored
		{
			/*LT*/{ -m_cameraParameters.principalPointX/m_cameraParameters.focalLengthX, m_cameraParameters.principalPointY/m_cameraParameters.focalLengthY, 1.f },
			/*RT*/{ (1.f-m_cameraParameters.principalPointX)/m_cameraParameters.focalLengthX, m_cameraParameters.principalPointY/m_cameraParameters.focalLengthY, 1.f },
			/*LB*/{ -m_cameraParameters.principalPointX/m_cameraParameters.focalLengthX, (m_cameraParameters.principalPointY-1.f)/m_cameraParameters.focalLengthY, 1.f },
			/*RB*/{ (1.f-m_cameraParameters.principalPointX)/m_cameraParameters.focalLengthX, (m_cameraParameters.principalPointY-1.f)/m_cameraParameters.focalLengthY, 1.f }
		};

		for (UINT rowID = 0; rowID<m_cDepthHeight; rowID++)
		{
			const float rowFactor = float(rowID)/float(m_cDepthHeight-1);
			const CameraSpacePoint rowStart =
			{
				cameraFrameCorners[0].X+(cameraFrameCorners[2].X-cameraFrameCorners[0].X) * rowFactor,
				cameraFrameCorners[0].Y+(cameraFrameCorners[2].Y-cameraFrameCorners[0].Y) * rowFactor,
				1.f
			};

			const CameraSpacePoint rowEnd =
			{
				cameraFrameCorners[1].X+(cameraFrameCorners[3].X-cameraFrameCorners[1].X) * rowFactor,
				cameraFrameCorners[1].Y+(cameraFrameCorners[3].Y-cameraFrameCorners[1].Y) * rowFactor,
				1.f
			};

			const float stepFactor = 1.f/float(m_cDepthWidth-1);
			const CameraSpacePoint rowDelta =
			{
				(rowEnd.X-rowStart.X) * stepFactor,
				(rowEnd.Y-rowStart.Y) * stepFactor,
				0
			};

			_ASSERT(m_cDepthWidth==NUI_DEPTH_RAW_WIDTH);
			CameraSpacePoint cameraCoordsRow[NUI_DEPTH_RAW_WIDTH];

			CameraSpacePoint currentPoint = rowStart;
			for (UINT i = 0; i<m_cDepthWidth; i++)
			{
				cameraCoordsRow[i] = currentPoint;
				currentPoint.X += rowDelta.X;
				currentPoint.Y += rowDelta.Y;
			}

			hr = m_pMapper->MapCameraPointsToDepthSpace(m_cDepthWidth , cameraCoordsRow , m_cDepthWidth , &m_pDepthDistortionMap[rowID * m_cDepthWidth]);
			if (FAILED(hr))
			{
				SetStatusMessage(L"Failed to initialize Kinect Coordinate Mapper.");
				return hr;
			}
		}

		if (nullptr==m_pDepthDistortionLT)
		{
			SetStatusMessage(L"Failed to initialize Kinect Fusion depth image distortion Lookup Table.");
			return E_OUTOFMEMORY;
		}

		UINT* pLT = m_pDepthDistortionLT;
		for (UINT i = 0; i<m_cDepthImagePixels; i++ , pLT++)
		{
			//nearest neighbor depth lookup table 
			UINT x = UINT(m_pDepthDistortionMap[i].X+0.5f);
			UINT y = UINT(m_pDepthDistortionMap[i].Y+0.5f);

			*pLT = (x<m_cDepthWidth && y<m_cDepthHeight) ? x+y*m_cDepthWidth : UINT_MAX;
		}
		m_bHaveValidCameraParameters = true;
	}
	else
	{
		m_bHaveValidCameraParameters = false;
	}
	return S_OK;
}


//██ 初始化KinectFusion容积和处理过程
HRESULT CKinectFusionBasics::OnCoordinateMappingChanged( )
{
	HRESULT hr = E_UNEXPECTED;

	// Calculate the down sampled image sizes, which are used for the AlignPointClouds calculation frames
	CameraIntrinsics intrinsics = {};  //表示深度相机的校准数据。

	m_pMapper->GetDepthCameraIntrinsics(&intrinsics);  //返回深度相机的校准数据。

	float focalLengthX = intrinsics.FocalLengthX/NUI_DEPTH_RAW_WIDTH;
	float focalLengthY = intrinsics.FocalLengthY/NUI_DEPTH_RAW_HEIGHT;
	float principalPointX = intrinsics.PrincipalPointX/NUI_DEPTH_RAW_WIDTH;
	float principalPointY = intrinsics.PrincipalPointY/NUI_DEPTH_RAW_HEIGHT;

	if (m_cameraParameters.focalLengthX==focalLengthX&&
		m_cameraParameters.focalLengthY==focalLengthY&&
		m_cameraParameters.principalPointX==principalPointX&&
		m_cameraParameters.principalPointY==principalPointY)

		return S_OK;

	m_cameraParameters.focalLengthX = focalLengthX;
	m_cameraParameters.focalLengthY = focalLengthY;
	m_cameraParameters.principalPointX = principalPointX;
	m_cameraParameters.principalPointY = principalPointY;

	_ASSERT(m_cameraParameters.focalLengthX!=0);

	UpdateIntrinsics(m_pDepthFloatImage , &m_cameraParameters);
	UpdateIntrinsics(m_pPointCloud , &m_cameraParameters);
	UpdateIntrinsics(m_pShadedSurface , &m_cameraParameters);

	if (nullptr==m_pDepthDistortionMap)
	{
		SetStatusMessage(L"Failed to initialize Kinect Fusion depth image distortion buffer.");
		return E_OUTOFMEMORY;
	}

	hr = SetupUndistortion( );
	return hr;
}

///██初始化KinectFusion 
/// Initialize Kinect Fusion volume and images for processing 
HRESULT CKinectFusionBasics::InitializeKinectFusion( )
{
	HRESULT hr = S_OK;

	// Check to ensure suitable DirectX11 compatible hardware exists before initializing Kinect Fusion
	WCHAR description[MAX_PATH];
	WCHAR instancePath[MAX_PATH];
	UINT  memorySize = 0;
	//【】设备检查:硬件是否支持
	if (FAILED(hr = NuiFusionGetDeviceInfo(
		m_processorType , //设备类型 CPU or GPU
		m_deviceIndex ,   //-1表示使用默认设备,设备索引
		&description[0] ,  //设备描述符
		ARRAYSIZE(description) , //描述的大小
		&instancePath[0] ,   // 用于重建的GPU中的DirectX2D 实例
		ARRAYSIZE(instancePath) , //The size of the instance path , in bytes.
		&memorySize)))//Gets the amount of dedicated video memory on the GPU being used for reconstruction.
	{
		if (hr==E_NUI_BADINDEX)
		{
			// This error code is returned either when the device index is out of range for the processor
			// type or there is no DirectX11 capable device installed. As we set -1 (auto-select default)
			// for the device index in the parameters, this indicates that there is no DirectX11 capable
			// device. The options for users in this case are to either install a DirectX11 capable device
			// (see documentation for recommended GPUs) or to switch to non-real-time CPU based 
			// reconstruction by changing the processor type to NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_CPU.
			SetStatusMessage(L"No DirectX11 device detected, or invalid device index - Kinect Fusion requires a DirectX11 device for GPU-based reconstruction.");
		}
		else
		{
			SetStatusMessage(L"Failed in call to NuiFusionGetDeviceInfo.");
		}
		return hr;
	}

	//【】创建Fusion 容积重建INuiFusionReconstruction
	//【】
	/*
		参数1: 容积重建参数
		参数2: 设备类型
		参数3 : 设备索引
		参数4 : 世界到相机 坐标转换矩阵
		参数5 : 返回指针*/
		//m_reconstructionParams说明
		//typedef struct _NUI_FUSION_RECONSTRUCTION_PARAMETERS
		//{
		//	FLOAT voxelsPerMeter;  //每米体素(体积元素或者体积像素)数量
		//	UINT voxelCountX;      // 重建X轴体素数量
		//	UINT voxelCountY;  //重建Y轴体素数量
		//	UINT voxelCountZ; //重建Z轴体素数量
		//} 	NUI_FUSION_RECONSTRUCTION_PARAMETERS;
		//  GPU加速即显卡内存 , CPU加速即系统内存 , 这可是一笔大买卖啊。
		//	目前主流显存在1G~2G , 差强人意。

		// Create the Kinect Fusion Reconstruction Volume   m_pVolume
	hr = NuiFusionCreateColorReconstruction(
		&m_reconstructionParams ,
		m_processorType , m_deviceIndex ,
		&m_worldToCameraTransform ,
		&m_pVolume);


	if (FAILED(hr))
	{
		if (E_NUI_GPU_FAIL==hr)
		{
			WCHAR buf[MAX_PATH];
			swprintf_s(buf , ARRAYSIZE(buf) , L"Device %d not able to run Kinect Fusion, or error initializing." , m_deviceIndex);
			SetStatusMessage(buf);
		}
		else if (E_NUI_GPU_OUTOFMEMORY==hr)
		{
			WCHAR buf[MAX_PATH];
			swprintf_s(buf , ARRAYSIZE(buf) , L"Device %d out of memory error initializing reconstruction - try a smaller reconstruction volume." , m_deviceIndex);
			SetStatusMessage(buf);
		}
		else if (NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_CPU!=m_processorType)
		{
			WCHAR buf[MAX_PATH];
			swprintf_s(buf , ARRAYSIZE(buf) , L"Failed to initialize Kinect Fusion reconstruction volume on device %d." , m_deviceIndex);
			SetStatusMessage(buf);
		}
		else
		{
			SetStatusMessage(L"Failed to initialize Kinect Fusion reconstruction volume on CPU.");
		}
		return hr;
	}

	// Save the default world to volume transformation to be optionally used in ResetReconstruction 世界到容积坐标转换矩阵:
	hr = m_pVolume->GetCurrentWorldToVolumeTransform(&m_defaultWorldToVolumeTransform);
	//通过修改世界到容积转换矩阵可以控制重建范围.
	if (FAILED(hr))
	{
		SetStatusMessage(L"Failed in call to GetCurrentWorldToVolumeTransform.");
		return hr;
	}

	if (m_bTranslateResetPoseByMinDepthThreshold)
	{
		// This call will set the world-volume transformation
		hr = ResetReconstruction( );
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// Frames generated from the depth input
		//我们需要将收集的深度数据浮点化，再平滑化(可选)，计算出点云后，输出表面(可选)与法线图像(可选)。
	//即需要5张Fusion图像帧

	//██ 创建平滑浮点深度帧
	hr = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_FLOAT , m_cDepthWidth , m_cDepthHeight , &m_cameraParameters , &m_pDepthFloatImage);
	if (FAILED(hr))
	{
		SetStatusMessage(L"Failed to initialize Kinect Fusion image.");
		return hr;
	}

	//██ 创建点云帧
	// Create images to raycast the Reconstruction Volume
	hr = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_POINT_CLOUD , m_cDepthWidth , m_cDepthHeight , &m_cameraParameters , &m_pPointCloud);
	if (FAILED(hr))
	{
		SetStatusMessage(L"Failed to initialize Kinect Fusion image.");
		return hr;
	}

	// ██创建Fusion图像帧
	// Create images to raycast the Reconstruction Volume
	hr = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_COLOR , m_cDepthWidth , m_cDepthHeight , &m_cameraParameters , &m_pShadedSurface);
	if (FAILED(hr))
	{
		SetStatusMessage(L"Failed to initialize Kinect Fusion image.");
		return hr;
	}

	//【】
	_ASSERT(m_pDepthImagePixelBuffer==nullptr);
	m_pDepthImagePixelBuffer = new(std::nothrow) UINT16[m_cDepthImagePixels];
	if (nullptr==m_pDepthImagePixelBuffer)
	{
		SetStatusMessage(L"Failed to initialize Kinect Fusion depth image pixel buffer.");
		return hr;
	}
	//【】
	_ASSERT(m_pDepthDistortionMap==nullptr);
	m_pDepthDistortionMap = new(std::nothrow) DepthSpacePoint[m_cDepthImagePixels];
	if (nullptr==m_pDepthDistortionMap)
	{
		SetStatusMessage(L"Failed to initialize Kinect Fusion depth image distortion buffer.");
		return E_OUTOFMEMORY;
	}
	//【】
	SAFE_DELETE_ARRAY(m_pDepthDistortionLT);
	m_pDepthDistortionLT = new(std::nothrow) UINT[m_cDepthImagePixels];

	if (nullptr==m_pDepthDistortionLT)
	{
		SetStatusMessage(L"Failed to initialize Kinect Fusion depth image distortion Lookup Table.");
		return E_OUTOFMEMORY;
	}

	// If we have valid parameters, let's go ahead and use them.
	if (m_cameraParameters.focalLengthX!=0)
	{
		SetupUndistortion( );
	}

	m_fStartTime = m_timer.AbsoluteTime( );

	// Set an introductory message
	SetStatusMessage(L"Click ‘Reset Reconstruction' to clear!");
	return hr;
}


//██深度数据重建,点云融合过程
/// Handle new depth data and perform Kinect Fusion processing  处理新的深度数据 并进行Fusion点云融合
void CKinectFusionBasics::ProcessDepth( )
{
	if (m_bInitializeError)
	{
		return;
	}

	HRESULT hr = S_OK;

	// To enable playback of a .xef file through Kinect Studio and reset of the reconstruction
	// if the .xef loops, we test for when the frame timestamp has skipped a large number. 
	// Note: this will potentially continually reset live reconstructions on slow machines which
	// cannot process a live frame in less time than the reset threshold. Increase the number of
	// milliseconds in cResetOnTimeStampSkippedMilliseconds if this is a problem.

	if (m_bAutoResetReconstructionOnTimeout && m_cFrameCounter!=0&&m_bResetReconstruction)
	{
		ResetReconstruction( );
		if (FAILED(hr))
		{
			return;
		}
	}

	// Return if the volume is not initialized
	if (nullptr==m_pVolume)
	{
		SetStatusMessage(L"Kinect Fusion reconstruction volume not initialized. Please try reducing volume size or restarting.");
		return;
	}

	////////////////////////////////////////////////////////
	////【】Depth to DepthFloat
	// Convert the pixels describing extended depth as unsigned short type in millimeters to depth
	// as floating point type in meters.
	//Converts the specified array of Kinect depth pixels to a NUI_FUSION_IMAGE_FRAME object representing a depth float frame.
	hr = m_pVolume->DepthToDepthFloatFrame(m_pDepthImagePixelBuffer , m_cDepthImagePixels*sizeof(UINT16) , m_pDepthFloatImage , m_fMinDepthThreshold , m_fMaxDepthThreshold , m_bMirrorDepthFrame);

	if (FAILED(hr))
	{
		SetStatusMessage(L"Kinect Fusion NuiFusionDepthToDepthFloatFrame call failed.");
		return;
	}

	////////////////////////////////////////////////////////
	// //【】ProcessFrame
	//【】Vihan
	// Perform the camera tracking and update the Kinect Fusion Volume
	// This will create memory on the GPU, upload the image, run camera tracking and integrate the
	// data into the Reconstruction Volume if successful. Note that passing nullptr as the final 
	// parameter will use and update the internal camera pose.
	//Processes the specified depth frame and color frame through the Kinect Fusion pipeline.
	hr = m_pVolume->ProcessFrame(m_pDepthFloatImage , nullptr , NUI_FUSION_DEFAULT_ALIGN_ITERATION_COUNT , m_cMaxIntegrationWeight , 0.0 , nullptr , &m_worldToCameraTransform);

	// Test to see if camera tracking failed. 
	// If it did fail, no data integration or raycast for reference points and normals will have taken 
	// place, and the internal camera pose will be unchanged.
	//【】重建失败
	if (FAILED(hr))
	{
		if (hr==E_NUI_FUSION_TRACKING_ERROR)
		{
			m_cLostFrameCounter++;
			m_bTrackingFailed = true;
			printf("------------移动太快 , 重建失败\n");
			SetStatusMessage(L"Kinect Fusion camera tracking failed! Align the camera to the last tracked position.");
		}
		else
		{
			printf("------------Kinect Fusion ProcessFrame call failed!\n");

			SetStatusMessage(L"Kinect Fusion ProcessFrame call failed!");
			return;
		}
	}
	//【】 如果这一帧重建成功, 那么调整相机姿态 
	else
	{
		Matrix4 calculatedCameraPose;
		hr = m_pVolume->GetCurrentWorldToCameraTransform(&calculatedCameraPose);
		if (SUCCEEDED(hr))
		{

			printf("---------------重建成功------------\n");

			// Set the pose
			m_worldToCameraTransform = calculatedCameraPose;
			//【】设立标志
			m_cLostFrameCounter = 0;
			m_bTrackingFailed = false;
		}
	}

	if (m_bAutoResetReconstructionWhenLost && m_bTrackingFailed && m_cLostFrameCounter>=cResetOnNumberOfLostFrames)
	{
		// Automatically clear volume and reset tracking if tracking fails
		hr = ResetReconstruction( );

		if (FAILED(hr))
		{
			return;
		}

		// Set bad tracking message
		SetStatusMessage(L"Kinect Fusion camera tracking failed, automatically reset volume.");
	}

	////////////////////////////////////////////////////////
	// CalculatePointCloud

	// Raycast all the time, even if we camera tracking failed, to enable us to visualize what is happening with the system
	hr = m_pVolume->CalculatePointCloud(m_pPointCloud , nullptr , &m_worldToCameraTransform);

	if (FAILED(hr))
	{
		SetStatusMessage(L"Kinect Fusion CalculatePointCloud call failed.");
		return;
	}

	////////////////////////////////////////////////////////
	// ShadePointCloud and render渲染
	hr = NuiFusionShadePointCloud(m_pPointCloud , &m_worldToCameraTransform , nullptr , m_pShadedSurface , nullptr);
	if (FAILED(hr))
	{
		SetStatusMessage(L"Kinect Fusion NuiFusionShadePointCloud call failed.");
		return;
	}
	// Draw the shaded raycast volume image
	BYTE * pBuffer = m_pShadedSurface->pFrameBuffer->pBits;
	// Draw the data with Direct2D
	m_pDrawDepth->Draw(pBuffer , m_cDepthWidth * m_cDepthHeight * cBytesPerPixel);

	////////////////////////////////////////////////////////
	// Periodically Display Fps

	// Update frame counter
	m_cFrameCounter++;

	// Display fps count approximately every cTimeDisplayInterval seconds
	double elapsed = m_timer.AbsoluteTime( )-m_fStartTime;
	if ((int)elapsed>=cTimeDisplayInterval)
	{
		double fps = (double)m_cFrameCounter/elapsed;

		// Update status display
		if (!m_bTrackingFailed)
		{
			WCHAR str[MAX_PATH];
			swprintf_s(str , ARRAYSIZE(str) , L"Fps: %5.2f" , fps);
			SetStatusMessage(str);
		}

		m_cFrameCounter = 0;
		m_fStartTime = m_timer.AbsoluteTime( );
	}
}


//██重置重建
/// <summary>
/// Reset the reconstruction camera pose and clear the volume.
/// </summary>
/// <returns>S_OK on success, otherwise failure code</returns>
HRESULT CKinectFusionBasics::ResetReconstruction( )
{
	if (nullptr==m_pVolume)
	{
		return E_FAIL;
	}

	HRESULT hr = S_OK;

	SetIdentityMatrix(m_worldToCameraTransform);

	// Translate the reconstruction volume location away from the world origin by an amount equal
	// to the minimum depth threshold. This ensures that some depth signal falls inside the volume.
	// If set false, the default world origin is set to the center of the front face of the 
	// volume, which has the effect of locating the volume directly in front of the initial camera
	// position with the +Z axis into the volume along the initial camera direction of view.
	if (m_bTranslateResetPoseByMinDepthThreshold)
	{
		Matrix4 worldToVolumeTransform = m_defaultWorldToVolumeTransform;

		// Translate the volume in the Z axis by the minDepthThreshold distance
		float minDist = (m_fMinDepthThreshold<m_fMaxDepthThreshold) ? m_fMinDepthThreshold : m_fMaxDepthThreshold;
		worldToVolumeTransform.M43 -= (minDist * m_reconstructionParams.voxelsPerMeter);

		hr = m_pVolume->ResetReconstruction(&m_worldToCameraTransform , &worldToVolumeTransform);
	}
	else
	{
		hr = m_pVolume->ResetReconstruction(&m_worldToCameraTransform , nullptr);
	}

	m_cLostFrameCounter = 0;
	m_cFrameCounter = 0;
	m_fStartTime = m_timer.AbsoluteTime( );

	if (SUCCEEDED(hr))
	{
		m_bTrackingFailed = false;

		SetStatusMessage(L"Reconstruction has been reset.");
	}
	else
	{
		SetStatusMessage(L"Failed to reset reconstruction.");
	}
	return hr;
}


//██保存生成的Mesh文件
HRESULT SaveMeshFile(INuiFusionColorMesh* pMesh , KinectFusionMeshTypes saveMeshType)
{
	HRESULT hr = S_OK;

	if (nullptr==pMesh)
	{
		return E_INVALIDARG;
	}

	CComPtr<IFileSaveDialog>  pSaveDlg;

	// Create the file save dialog object.
	hr = pSaveDlg.CoCreateInstance(__uuidof(FileSaveDialog));

	if (FAILED(hr))
	{
		return hr;
	}

	// Set the dialog title
	hr = pSaveDlg->SetTitle(L"Save Kinect Fusion Mesh");
	if (SUCCEEDED(hr))
	{
		// Set the button text
		hr = pSaveDlg->SetOkButtonLabel(L"Save");
		if (SUCCEEDED(hr))
		{
			// Set a default filename
			if (Stl==saveMeshType)
			{
				hr = pSaveDlg->SetFileName(L"MeshedReconstruction.stl");
			}
			else if (Obj==saveMeshType)
			{
				hr = pSaveDlg->SetFileName(L"MeshedReconstruction.obj");
			}
			else if (Ply==saveMeshType)
			{
				hr = pSaveDlg->SetFileName(L"MeshedReconstruction.ply");
			}

			if (SUCCEEDED(hr))
			{
				// Set the file type extension
				if (Stl==saveMeshType)
				{
					hr = pSaveDlg->SetDefaultExtension(L"stl");
				}
				else if (Obj==saveMeshType)
				{
					hr = pSaveDlg->SetDefaultExtension(L"obj");
				}
				else if (Ply==saveMeshType)
				{
					hr = pSaveDlg->SetDefaultExtension(L"ply");
				}

				if (SUCCEEDED(hr))
				{
					// Set the file type filters
					if (Stl==saveMeshType)
					{
						COMDLG_FILTERSPEC allPossibleFileTypes[] = {
							{ L"Stl mesh files", L"*.stl" },
							{ L"All files", L"*.*" }
						};

						hr = pSaveDlg->SetFileTypes(
							ARRAYSIZE(allPossibleFileTypes) ,
							allPossibleFileTypes);
					}
					else if (Obj==saveMeshType)
					{
						COMDLG_FILTERSPEC allPossibleFileTypes[] = {
							{ L"Obj mesh files", L"*.obj" },
							{ L"All files", L"*.*" }
						};

						hr = pSaveDlg->SetFileTypes(
							ARRAYSIZE(allPossibleFileTypes) ,
							allPossibleFileTypes);
					}
					else if (Ply==saveMeshType)
					{
						COMDLG_FILTERSPEC allPossibleFileTypes[] = {
							{ L"Ply mesh files", L"*.ply" },
							{ L"All files", L"*.*" }
						};

						hr = pSaveDlg->SetFileTypes(
							ARRAYSIZE(allPossibleFileTypes) ,
							allPossibleFileTypes);
					}

					if (SUCCEEDED(hr))
					{
						// Show the file selection box
						hr = pSaveDlg->Show(m_hWnd);

						// Save the mesh to the chosen file.
						if (SUCCEEDED(hr))
						{
							CComPtr<IShellItem> pItem;
							hr = pSaveDlg->GetResult(&pItem);

							if (SUCCEEDED(hr))
							{
								LPOLESTR pwsz = nullptr;
								hr = pItem->GetDisplayName(SIGDN_FILESYSPATH , &pwsz);

								if (SUCCEEDED(hr))
								{
									//SetStatusMessage(L"Saving mesh file, please wait...");
									SetCursor(LoadCursor(nullptr , MAKEINTRESOURCE(IDC_WAIT)));

									if (Stl==saveMeshType)
									{
										hr = WriteBinarySTLMeshFile(pMesh , pwsz);
									}
									else if (Obj==saveMeshType)
									{
										hr = WriteAsciiObjMeshFile(pMesh , pwsz);
									}
									else if (Ply==saveMeshType)
									{
										hr = WriteAsciiPlyMeshFile(pMesh , pwsz , true , false);
									}
									CoTaskMemFree(pwsz);
								}
							}
						}
					}
				}
			}
		}
	}
	return hr;
}


//██状态栏消息更新
void CKinectFusionBasics::SetStatusMessage(WCHAR * szMessage)
{
	SendDlgItemMessageW(m_hWnd , IDC_STATUS , WM_SETTEXT , 0 , (LPARAM)szMessage);
}

//██手机远程控制线程
void  ThreadFun4Contrllor( )
{
	SendDlgItemMessageW(m_hWnd , IDC_remote , WM_SETTEXT , 0 , (LPARAM)(L"控制设备未连接"));
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

			UINT Msg = WM_COMMAND;
			HWND pWnd = FindWindow(L"KinectFusionBasicsAppDlgWndClass" , nullptr);
			SendMessage(pWnd , Msg , MSG_CONNECT , 0);

			while (TRUE)
			{
				ret = recv(sClient , szMessage , MSGSIZE , 0);
				szMessage[ret] = '\0';
				if (ret==0)
				{
					printf("控制设备已断开\n\n");
					UINT Msg = WM_COMMAND;
					HWND pWnd = FindWindow(L"KinectFusionBasicsAppDlgWndClass" , nullptr);
					SendMessage(pWnd , Msg , MSG_DISCONNECT , 0);
					/*SendDlgItemMessageW(m_hWnd , IDC_remote , WM_SETTEXT , 0 , (LPARAM)(L"控制设备已断开"));*/

					WSACleanup( );
					break;
				}

				printf("Received [%d bytes]: '%s'\n" , ret , szMessage);
				if (strcmp(szMessage , "up")==0)
				{
					UINT Msg = WM_COMMAND;
					HWND pWnd = FindWindow(L"KinectFusionBasicsAppDlgWndClass" , nullptr);
					SendMessage(pWnd , Msg , IDC_BUTTON_GETFILE , 0);
				}
				else if (strcmp(szMessage , "down")==0)
				{
					UINT Msg = WM_COMMAND;
					HWND pWnd = FindWindow(L"KinectFusionBasicsAppDlgWndClass" , nullptr);
					SendMessage(pWnd , Msg , IDC_BUTTON_RESET_RECONSTRUCTION , 0);
				}
			}
		}
	}
}
