//myFusion.cpp: 定义应用程序的入口点。
#include "stdafx.h"
#include "myFusion.h"
#include "resource.h"
#include"CvvImage.h"
#include <opencv2/core/core.hpp>        //OpenCv  
#include <opencv2/highgui/highgui.hpp>
#include<opencv2\opencv.hpp>
#include "kinect.h"                     //Kinect
#include<string.h>

using namespace cv;
using namespace std;

int const  TIMER_ID_SHOWIMAGE = 1;  //定时器常量：展示效果图

HINSTANCE  m_hInstance; //全局句柄
HDC    hdc , mdc;
HBITMAP bg;
HWND m_hwnd;

//Kinect						             
//与数据源相关的指针
IKinectSensor* m_pKinectSensor;                   //kinect设备对象指针
IDepthFrameSource* m_pDepthFrameSource = NULL;    //深度帧数据源
IDepthFrameReader* m_pDepthFrameReader = NULL;    //深度帧数据源读取器
IDepthFrame* pDepthFrame = NULL;                  //深度帧数据
UINT16 *depthData = new UINT16[424*512];          //用于装下深度帧的数据
Mat mat_depth;                                    //深度数据图片矩阵



//-----------------------------------------------------------------一些函数声明---------------------------------------------------------------
BOOL InitKinect( );
void InitApp( );
void ShowMat2PIC(HWND Hp , Mat mat);
void CALLBACK TimerProc_SHOWIMAGE(HWND hWnd , UINT uMsg , UINT idEvent , DWORD dwTime);
INT_PTR CALLBACK MainDlgProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam);  //窗口过程函数声明
Mat GetDepthMat(const UINT16* DepthData , int nWidth , int nHeight , USHORT nMinDepth , USHORT nMaxDepth);
void PixelFilter(unsigned short* depthArray , unsigned short* smoothDepthArray , int innerBandThreshold , int outerBandThreshold);// 像素滤波器
Mat ShowDepthImage(unsigned short* depthData); // 将一个深度图可视化
void SetStatusMessage(WCHAR * szMessage);

//应用Style 声明
#pragma comment(linker,"\"/manifestdependency:type='win32' "\
						"name='Microsoft.Windows.Common-Controls' "\
						"version='6.0.0.0' processorArchitecture='*' "\
						"publicKeyToken='6595b64144ccf1df' language='*'\"")

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

//程序入口
int APIENTRY wWinMain(_In_ HINSTANCE hInstance , _In_opt_ HINSTANCE hPrevInstance , _In_ LPWSTR lpCmdLine , _In_ int nShowCmd)
{
	m_hInstance = hInstance;
	InitApp( );
	DialogBox(hInstance , MAKEINTRESOURCE(IDD_MAIN) , NULL , MainDlgProc);// 在这里绑定窗口的窗口过程MainDlgProc
	return 0;
}

//窗口消息处理函数
INT_PTR CALLBACK MainDlgProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam)
{
	m_hwnd = hwnd;
	BOOL bRet = TRUE;
	int wmid , wmEvent;
	switch (message) //消息区分
	{
		//窗口重绘消息
		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;
			hdc = ::BeginPaint(hwnd , &ps);
			::EndPaint(hwnd , &ps);
			break;
		}

		//窗口创建的消息
		case WM_INITDIALOG:
		{
			if (InitKinect( ))
			{
				cout<<"内容"<<"Kinect 设备初始化成功"<<endl;
			}
			else
			{
				cout<<"内容"<<"Kinect 初始化失败"<<endl;
			}
			cout<<"==================================================================="<<endl;
			break;
		}

		case WM_CLOSE:    //关闭窗口消息
		{
			EndDialog(hwnd , 0);
			break;
		}

		case WM_COMMAND:  //指令
		{
			wmid = LOWORD(wParam);       //消息解析
			wmEvent = HIWORD(wParam);

			switch (wmid)
			{
				case IDC_BUTTON_SHOW:    //showButton的按键消息
				{
					SetTimer(hwnd , TIMER_ID_SHOWIMAGE , 40 , (TIMERPROC)TimerProc_SHOWIMAGE);
					break;
				}
				case IDC_BUTTON_STOP:
				{
					BOOL s = KillTimer(hwnd , TIMER_ID_SHOWIMAGE);    //销毁定时器
					break;
				}

				case IDC_BUTTON_GET_DEPTH:
				{
					UINT16 *depthDataTemp = new UINT16[424*512];          //用于装下深度帧的数据
					depthDataTemp = depthData;
					//获取显示距离的范围
					/*USHORT nMinDepth;
					USHORT* pMinDepth = &nMinDepth;
					pDepthFrame->get_DepthMinReliableDistance(pMinDepth);

					USHORT nMaxDepth;
					USHORT* pMaxDepth = &nMaxDepth;
					pDepthFrame->get_DepthMinReliableDistance(pMaxDepth);*/

					mat_depth = GetDepthMat(depthData , 512 , 424 , 0 , 0);
					if (imwrite("C:/Users/1234/Desktop/i_depth.png" , mat_depth))  //openCV保存图片
					{
						cout<<"深度图片获取成功"<<endl;
						MessageBox(hwnd , TEXT("深度图片获取成功") , TEXT("提示") , NULL);
					}
					else
					{
						cout<<"深度图片获取失败"<<endl;
					}
					break;
				}

				//像素滤波
				case IDC_BUTTON_PIXPRO:
				{
					UINT16 *depthDataTemp = new UINT16[424*512];          //用于装下深度帧的数据处理前
					UINT16 *pixelFilterData = new UINT16[424*512];        //                    处理后

					depthDataTemp = depthData;

					PixelFilter(depthDataTemp , pixelFilterData , 3 , 5);

					mat_depth = GetDepthMat(pixelFilterData , 512 , 424 , 0 , 0);
					if (imwrite("C:/Users/1234/Desktop/i_depth.png" , mat_depth))  //openCV保存图片
					{
						cout<<"深度图片获取成功"<<endl;
						MessageBox(hwnd , TEXT("深度图片获取成功") , TEXT("提示") , NULL);
					}
					else
					{
						cout<<"深度图片获取失败"<<endl;
					}
					break;
				}

				//均匀滤波
				case IDC_BUTTON_AVEPRO:
				{

					WCHAR  szStr3[] = L"Viahnchou";
					SetStatusMessage(szStr3);
					break;
				}

				default:
				{
					break;
				}
			}
		}

		default:
		{
			bRet = FALSE;
			break;
		}
	}
	return bRet;
}

//显示mat图片
void ShowMat2PIC(HWND Hp , Mat mat)
{
	IplImage *Image_depth;
	Image_depth = &IplImage(mat);
	HDC hdc = GetDC(Hp);													//得到图片句柄对应的DC
	RECT rect;															    //定义一个矩形放图片滴
	GetClientRect(Hp , &rect);
	CvvImage cimg;
	cimg.CopyOf(Image_depth);
	cimg.DrawToHDC(hdc , &rect);
}

//Kinect设备初始化
BOOL InitKinect( )
{
	mat_depth.create(424 , 512 , CV_8UC1);
	HRESULT hr;

	//【1】获取Kinect设备  m_pKinectSensor
	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	//如果初始化失败
	if (FAILED(hr))
	{
		cout<<"初始化kinnect设备失败!"<<endl;
		return hr;
	}
	else
	{
		cout<<"初始化kinnect设备成功"<<endl;
	}

	//【2】打开kinect设备
	hr = m_pKinectSensor->Open( );
	if (FAILED(hr))
	{
		cout<<"kinnect设备打开失败!"<<endl;
		return hr;
	}
	else
	{
		cout<<"kinnect设备打开成功"<<endl;
	}

	//【3】根据设备获取帧数据源
	hr = m_pKinectSensor->get_DepthFrameSource(&m_pDepthFrameSource);
	if (FAILED(hr))
	{
		cout<<"帧数据源获取失败"<<endl;
		return hr;
	}
	else
	{
		cout<<"帧数据源获取成功"<<endl;
	}

	//【4】使用帧数据源打开帧数据阅读器
	hr = m_pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
	if (FAILED(hr))
	{
		cout<<"帧阅读器获取失败"<<endl;
		return hr;
	}
	else
	{
		cout<<"帧阅读器获取成功"<<endl;
	}
	return TRUE;
}

//应用初始化
void InitApp( )
{
	//初始化控制台窗口
	AllocConsole( );
	freopen("CONOUT$" , "w+t" , stdout);
	freopen("CONIN$" , "r+t" , stdin);

	//初始化显示句柄
}

//从原始数据帧获取深度图mat
Mat GetDepthMat(const UINT16* DepthData , int nWidth , int nHeight , USHORT nMinDepth , USHORT nMaxDepth)
{
	Mat img(nHeight , nWidth , CV_16UC1);
	UINT16* p_Mat_Data = (UINT16*)img.data;

	const UINT16* pBufferEnd = DepthData+(nWidth * nHeight);
	while (DepthData<pBufferEnd)
	{
		USHORT depthTemp = *DepthData;
		//*p_mat = (depthTemp>=nMinDepth)&&(depthTemp<=nMaxDepth) ? depthTemp<<3 : 0;
		*p_Mat_Data = depthTemp;
		p_Mat_Data++;
		++DepthData;
	}
	return img;
}


//显示深度视频帧的定时器回调  在 IDC_BUTTON_SHOW消息中开启该回调就开始显示视频
void CALLBACK TimerProc_SHOWIMAGE(HWND hWnd , UINT uMsg , UINT idEvent , DWORD dwTime)
{
	HRESULT hr;
	//【5】使用帧阅读器来获取帧数据，可以对帧数据进行不间断的获取和显示来作为视频
	hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);

	if (FAILED(hr)||!pDepthFrame)
	{
		//cout<<"数据获取失败--------"<<endl;
		return;
	}
	else
	{
		//TODO
		//cout<<"数据获取成功+++++++++"<<endl;
	}

	// 【】depth拷贝到图片中
	if (SUCCEEDED(hr))
	{
		Mat	matBefore(424 , 512 , CV_8UC1);
		Mat	matAfter(424 , 512 , CV_8UC1);
		hr = pDepthFrame->CopyFrameDataToArray(424*512 , depthData);   //得到一帧depthdata
		//原生数据 depthData


		//【1】滤波前 （matBefore）
		for (int i = 0; i<512*424; i++)
		{
			BYTE intensity = static_cast<BYTE>(depthData[i]);   // 0-255深度图，为了显示明显，只取深度数据的低8位
			reinterpret_cast<BYTE*>(matBefore.data)[i] = intensity;
		}

		//【2】滤波后（matAfter）
		//【】像素滤波
		UINT16 *pixelFilterData = new UINT16[424*512];        //                    处理后
		PixelFilter(depthData , pixelFilterData , 3 , 5);

		for (int i = 0; i<512*424; i++)
		{
			BYTE intensity = static_cast<BYTE>(pixelFilterData[i]);   // 0-255深度图，为了显示明显，只取深度数据的低8位
			reinterpret_cast<BYTE*>(matAfter.data)[i] = intensity;
		}

		//【3】前后显示
		HWND Hp = GetDlgItem(hWnd , IDC_PIC_SHOW);
		ShowMat2PIC(Hp , matBefore);//显示的是处理过的易于现实的

		Hp = GetDlgItem(hWnd , IDC_PIC_SHOW_AFTER);
		ShowMat2PIC(Hp , matAfter);//显示的是处理过的易于现实的

		//【4】
		SafeRelease(pDepthFrame); // 释放资源
	}

}

// 将一个深度图可视化  取高八位
Mat ShowDepthImage(unsigned short* depthData)
{
	Mat result(424 , 512 , CV_8UC1);
	for (int i = 0; i<512*424; i++)
	{
		UINT16 depthValue = depthData[i];
		//if (depthValue==0)
		//{
		//	result.data[i*4] = 255;
		//	result.data[i*4+1] = 0;
		//	result.data[i*4+2] = 0;
		//	result.data[i*4+3] = depthValue%256;
		//}
		//else
		{
			result.data[i*4] = depthValue%256;
			result.data[i*4+1] = depthValue%256;
			result.data[i*4+2] = depthValue%256;
			result.data[i*4+3] = depthValue%256;
		}
	}
	return result;
}

// 像素滤波器
void PixelFilter(unsigned short* depthArray , unsigned short* smoothDepthArray , int innerBandThreshold = 3 , int outerBandThreshold = 7)
{
	// 我们用这两个值来确定索引在正确的范围内
	int widthBound = 512-1;
	int heightBound = 424-1;

	// 处理每行像素
	for (int depthArrayRowIndex = 0; depthArrayRowIndex<424; depthArrayRowIndex++)
	{
		// 处理一行像素中的每个像素
		for (int depthArrayColumnIndex = 0; depthArrayColumnIndex<512; depthArrayColumnIndex++)
		{
			int depthIndex = depthArrayColumnIndex+(depthArrayRowIndex*512);

			// 我们认为深度值为0的像素即为候选像素
			if (depthArray[depthIndex]==0)
			{
				// 通过像素索引，我们可以计算得到像素的横纵坐标
				int x = depthIndex%512;
				int y = (depthIndex-x)/512;

				// filter collection 用来计算滤波器内每个深度值对应的频度，在后面
				// 我们将通过这个数值来确定给候选像素一个什么深度值。
				unsigned short filterCollection[24][2] = { 0 };

				// 内外层框内非零像素数量计数器，在后面用来确定候选像素是否滤波
				int innerBandCount = 0;
				int outerBandCount = 0;

				// 下面的循环将会对以候选像素为中心的5 X 5的像素阵列进行遍历。这里定义了两个边界。如果在
				// 这个阵列内的像素为非零，那么我们将记录这个深度值，并将其所在边界的计数器加一，如果计数器
				// 高过设定的阈值，那么我们将取滤波器内统计的深度值的众数（频度最高的那个深度值）应用于候选
				// 像素上
				for (int yi = -2; yi<3; yi++)
				{
					for (int xi = -2; xi<3; xi++)
					{
						// yi和xi为操作像素相对于候选像素的平移量

						// 我们不要xi = 0&&yi = 0的情况，因为此时操作的就是候选像素
						if (xi!=0||yi!=0)
						{
							// 确定操作像素在深度图中的位置
							int xSearch = x+xi;
							int ySearch = y+yi;

							// 检查操作像素的位置是否超过了图像的边界（候选像素在图像的边缘）
							if (xSearch>=0&&xSearch<=widthBound&&
								ySearch>=0&&ySearch<=heightBound)
							{
								int index = xSearch+(ySearch*512);
								// 我们只要非零量
								if (depthArray[index]!=0)
								{
									// 计算每个深度值的频度
									for (int i = 0; i<24; i++)
									{
										if (filterCollection[i][0]==depthArray[index])
										{
											// 如果在 filter collection中已经记录过了这个深度
											// 将这个深度对应的频度加一
											filterCollection[i][1]++;
											break;
										}
										else if (filterCollection[i][0]==0)
										{
											// 如果filter collection中没有记录这个深度
											// 那么记录
											filterCollection[i][0] = depthArray[index];
											filterCollection[i][1]++;
											break;
										}
									}

									// 确定是内外哪个边界内的像素不为零，对相应计数器加一
									if (yi!=2&&yi!=-2&&xi!=2&&xi!=-2)
										innerBandCount++;
									else
										outerBandCount++;
								}
							}
						}
					}
				}

				// 判断计数器是否超过阈值，如果任意层内非零像素的数目超过了阈值，
				// 就要将所有非零像素深度值对应的统计众数
				if (innerBandCount>=innerBandThreshold||outerBandCount>=outerBandThreshold)
				{
					short frequency = 0;
					short depth = 0;
					// 这个循环将统计所有非零像素深度值对应的众数
					for (int i = 0; i<24; i++)
					{
						// 当没有记录深度值时（无非零深度值的像素）
						if (filterCollection[i][0]==0)
							break;
						if (filterCollection[i][1]>frequency)
						{
							depth = filterCollection[i][0];
							frequency = filterCollection[i][1];
						}
					}

					smoothDepthArray[depthIndex] = depth;
				}
				else
				{
					smoothDepthArray[depthIndex] = 0;
				}
			}
			else
			{
				// 如果像素的深度值不为零，保持原深度值
				smoothDepthArray[depthIndex] = depthArray[depthIndex];
			}
		}
	}
}


/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
void SetStatusMessage(WCHAR * szMessage)
{
	SendDlgItemMessageW(m_hwnd , IDC_STATUS , WM_SETTEXT , 0 , (LPARAM)szMessage);
	//【】使用
	//WCHAR  szStr3[] = L"Viahnchou";
	//SetStatusMessage(szStr3);

}