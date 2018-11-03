#pragma once

#include "resource.h"
////██ADD BY Vihan
#include <Shlobj.h>
#include <atlbase.h>

#include <NuiKinectFusionApi.h>
#include "ImageRenderer.h"


//Author :Vihan 
//Emainl :vihanmy@gmail.com
//Time   :
//Note   :一共40个全局变量m_（private and public）



// For timing calls
#include "Timer.h"

class CKinectFusionBasics
{
	static const int            cBytesPerPixel = 4;                           // for depth float and int-per-pixel raycast images
	static const int            cResetOnTimeStampSkippedMilliseconds = 1000;  // ms
	static const int            cResetOnNumberOfLostFrames = 100;             //丢帧数目
	static const int            cStatusMessageMaxLen = MAX_PATH*2;
	static const int            cTimeDisplayInterval = 10;                    /* 时间显示间隔*/

public:
	/// Constructor
	CKinectFusionBasics( );

	/// Destructor
	~CKinectFusionBasics( );

	/// Handles window messages, passes most to the class instance to handle
	/// <param name="hWnd">window message is for</param>
	/// <param name="uMsg">message</param>
	/// <param name="wParam">message data</param>
	/// <param name="lParam">additional message data</param>
	/// <returns>result of message processing</returns>
	static LRESULT CALLBACK     MessageRouter(HWND hWnd , UINT message , WPARAM wParam , LPARAM lParam);



	/// Handle windows messages for a class instance
	/// <param name="hWnd">window message is for</param>
	/// <param name="uMsg">message</param>
	/// <param name="wParam">message data</param>
	/// <param name="lParam">additional message data</param>
	/// <returns>result of message processing</returns>
	LRESULT CALLBACK            DlgProc(HWND hWnd , UINT message , WPARAM wParam , LPARAM lParam);

	// Creates the main window and begins processing
	int                         Run(HINSTANCE hInstance , int nCmdShow);

private:
	HWND                        m_hWnd;

	// Kinect设备指针
	IKinectSensor*              m_pNuiSensor;

	// 深度数据阅读器
	IDepthFrameReader*          m_pDepthFrameReader;
	UINT                         m_cDepthWidth;
	UINT                         m_cDepthHeight;
	UINT                         m_cDepthImagePixels;

	INT64                       m_lastFrameTimeStamp;
	bool                        m_bResetReconstruction;

	// Direct2D
	ImageRenderer*              m_pDrawDepth;
	ID2D1Factory*               m_pD2DFactory;

	BYTE*                       m_pDepthRGBX;

	/// Main processing function
	void                        Update( );
	/// Create the first connected Kinect found 
	/// <returns>S_OK on success, otherwise failure code</returns>
	HRESULT                     CreateFirstConnected( );

	/// <summary>
	/// Setup or update the Undistortion calculation for the connected camera 设置或更新连接摄像机的不失真计算
	/// </summary>
	HRESULT                     SetupUndistortion( );


	//██在Kinect设备 或者 kinectStudio连接之后会被调用
	/// Handle Coordinate Mapping changed event.
	/// Note, this happens after sensor connect, or when Kinect Studio connects
	HRESULT                     OnCoordinateMappingChanged( );

	//██Fusion初始化
	/// Initialize Kinect Fusion volume and images for processing
	/// <returns>S_OK on success, otherwise failure code</returns>
	HRESULT                     InitializeKinectFusion( );


	//██处理型的深度数据
	/// Handle new depth data
	void                        ProcessDepth( );

	//██重置相机姿势并清除重建容积
	/// Reset the reconstruction camera pose and clear the volume.
	/// <returns>S_OK on success, otherwise failure code</returns>
	HRESULT                     ResetReconstruction( );

	//██状态栏消息设置函数
	void                        SetStatusMessage(WCHAR* szMessage);


	//██Fusion重建容积
	INuiFusionColorReconstruction*   m_pVolume;


	//██Kinect融合体参数
	/// The Kinect Fusion Volume Parameters
	NUI_FUSION_RECONSTRUCTION_PARAMETERS m_reconstructionParams;

	//██相机变换
	// The Kinect Fusion Camera Transform
	Matrix4                     m_worldToCameraTransform;


	// The default Kinect Fusion World to Volume Transform
	Matrix4                     m_defaultWorldToVolumeTransform;

	/// Frames from the depth input
	UINT16*                     m_pDepthImagePixelBuffer;
	NUI_FUSION_IMAGE_FRAME*     m_pDepthFloatImage;

	/// For depth distortion correction
	//深度失真校正
	ICoordinateMapper*          m_pMapper;   //坐标映射  
	DepthSpacePoint*            m_pDepthDistortionMap;  //表示深度图像中的像素坐标。
	UINT*                       m_pDepthDistortionLT;
	WAITABLE_HANDLE             m_coordinateMappingChangedEvent;

	//██相机参数
	/// Kinect camera parameters.
	NUI_FUSION_CAMERA_PARAMETERS m_cameraParameters;   //
	bool                        m_bHaveValidCameraParameters;

	//██光线投射重建体积生成的帧
	/// Frames generated from ray-casting the Reconstruction Volume
	NUI_FUSION_IMAGE_FRAME*     m_pPointCloud;


	//██用于显示的帧数据
	// Images for display
	NUI_FUSION_IMAGE_FRAME*     m_pShadedSurface;


	//██相机追踪参数
	/// Camera Tracking parameters
	int                         m_cLostFrameCounter;
	bool                        m_bTrackingFailed;


	//██是否设置自动检测跟踪失败    真:表示允许自动检测是否跟踪失败    假:表示永远不自动跟踪失败(在构造器中设置)
	/// Parameter to turn automatic reset of the reconstruction when camera tracking is lost on or off.
	/// Set to true in the constructor to enable auto reset on cResetOnNumberOfLostFrames lost frames,
	/// or set false to never automatically reset.
	bool                        m_bAutoResetReconstructionWhenLost;

	//██确保当相邻帧之间的时间太大的时候能够自动重建
	/// Parameter to enable automatic reset of the reconstruction when there is a large difference in timestamp between subsequent frames.
	/// This should usually be set true as default to enable recorded .xef files to generate a reconstruction reset on looping of the playback or scrubbing,
	/// however, for debug purposes, it can be set false to prevent automatic reset on timeouts.
	bool                        m_bAutoResetReconstructionOnTimeout;


	//██处理参数
	/// Processing parameters
	int                         m_deviceIndex;
	NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE m_processorType;
	bool                        m_bInitializeError;
	float                       m_fMinDepthThreshold;
	float                       m_fMaxDepthThreshold;
	bool                        m_bMirrorDepthFrame;
	unsigned short              m_cMaxIntegrationWeight;
	int                         m_cFrameCounter;
	double                      m_fStartTime;
	Timing::Timer               m_timer;

	/// <summary>
	/// Parameter to translate the reconstruction based on the minimum depth setting. When set to
	/// false, the reconstruction volume +Z axis starts at the camera lens and extends into the scene.
	/// Setting this true in the constructor will move the volume forward along +Z away from the
	/// camera by the minimum depth threshold to enable capture of very small reconstruction volumes
	/// by setting a non-identity camera transformation in the ResetReconstruction call.
	/// Small volumes should be shifted, as the Kinect hardware has a minimum sensing limit of ~0.35m,
	/// inside which no valid depth is returned, hence it is difficult to initialize and track robustly  
	/// when the majority of a small volume is inside this distance.
	/// </summary>
	bool                        m_bTranslateResetPoseByMinDepthThreshold;

	/*参数基于最小深度设置来翻译重建。当设置为假时，重建体积+Z轴从相机镜头开始并延伸到场景中。
	在构造函数中设置此真值将使音量沿着+Z从相机前移最小深度阈值，以便通过在Reset.tructtion调用
	中设置非身份相机变换来捕获非常小的重建音量。由于Kinect硬件具有~0.35m的最小感测极限，其中
	没有返回有效深度，因此当小体积的大部分都在此距离内时，很难进行初始化和鲁棒跟踪。*/
};
