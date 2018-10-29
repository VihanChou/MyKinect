#include <Windows.h>
#include "resource.h"

#include"CvvImage.h"


#include <opencv2/core/core.hpp>    //OpenCv  
#include <opencv2/highgui/highgui.hpp>
#include<opencv2\opencv.hpp>

using namespace cv;

HINSTANCE  m_hInstance;

HDC hdc , mdc;
HBITMAP bg;

#pragma comment(linker,"\"/manifestdependency:type='win32' "\
						"name='Microsoft.Windows.Common-Controls' "\
						"version='6.0.0.0' processorArchitecture='*' "\
						"publicKeyToken='6595b64144ccf1df' language='*'\"")

INT_PTR CALLBACK MainDlgProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance , _In_opt_ HINSTANCE hPrevInstance , _In_ LPWSTR lpCmdLine , _In_ int nShowCmd)
{
	m_hInstance = hInstance;
	DialogBox(hInstance , MAKEINTRESOURCE(IDD_MAIN) , NULL , MainDlgProc);

	return 0;
}


//
INT_PTR CALLBACK MainDlgProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam)
{


	BOOL bRet = TRUE;
	int wmid , wmEvent;
	static HWND hwndList1;
	static HWND hwndList2;

	switch (message)
	{
		case WM_CLOSE:
		{
			EndDialog(hwnd , 0);
		}
		break;

		case WM_COMMAND:
		{
			wmid = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			hwndList1 = GetDlgItem(hwnd , IDC_LIST1);
			hwndList2 = GetDlgItem(hwnd , IDC_LIST2);


			int index , lenth;
			TCHAR *pVatChar;

			switch (wmid)
			{
				case IDC_BT_ADD:
				{
					SendMessage(hwndList1 , LB_ADDSTRING , 0 , (LPARAM)TEXT("Hello"));
					SendMessage(hwndList1 , LB_ADDSTRING , 0 , (LPARAM)TEXT("Hello"));
					SendMessage(hwndList1 , LB_ADDSTRING , 0 , (LPARAM)TEXT("Hello"));
					break;
				}
				case IDC_BT_DEL:
				{
					SendMessage(hwndList1 , LB_DELETESTRING , 2 , 0);
					break;
				}
				case IDC_BT_RESET:
				{
					SendMessage(hwndList1 , LB_RESETCONTENT , 0 , 0);
					break;
				}

				case IDC_BT_SELFSET:
				{
					SendMessage(hwndList1 , LB_SETCURSEL , 2 , 0);
					break;
				}

				case IDC_BT_CANCLESEL:
				{
					SendMessage(hwndList1 , LB_SETCURSEL , -1 , 0);  //取消选中
					break;
				}
				//列表框消息处理
				case IDC_LIST1:
				{
					if (wmEvent==LBN_SELCHANGE)
					{
						index = SendMessage(hwndList1 , LB_GETCURSEL , 0 , 0);
						lenth = SendMessage(hwndList1 , LB_GETTEXTLEN , index , 0)+1;
						pVatChar = (TCHAR*)(calloc(lenth , sizeof(TCHAR)));
						SendMessage(hwndList1 , LB_GETTEXT , index , (LPARAM)pVatChar);
					}
					break;
				}

				case IDC_BT_MOVE:
				{
					index = SendMessage(hwndList1 , LB_GETCURSEL , 0 , 0);
					lenth = SendMessage(hwndList1 , LB_GETTEXTLEN , index , 0)+1;
					pVatChar = (TCHAR*)(calloc(lenth , sizeof(TCHAR)));
					SendMessage(hwndList1 , LB_GETTEXT , index , (LPARAM)pVatChar);
					SendMessage(hwndList2 , LB_ADDSTRING , -1 , (LPARAM)pVatChar);
					SendMessage(hwndList1 , LB_DELETESTRING , index , 0);
					break;
				}

				case IDC_BUTTON_INSERT:
				{
					SendMessage(hwndList2 , LB_INSERTSTRING , -1 , (LPARAM)TEXT("one"));
					break;
				}

				case IDC_BUTTON_LOADPIC:
				{

					/*			HANDLE hBitmap;
								hBitmap = LoadImageW(m_hInstance , L"‪C:\\Users\\1234\\Desktop\\rgb.bmp" , IMAGE_BITMAP , 640 , 480 , LR_LOADFROMFILE);
								HWND hP = GetDlgItem(hwnd , IDC_PIC);
								SendMessage(hP , STM_SETIMAGE , IMAGE_BITMAP , LPARAM(hBitmap));*/


					IplImage *img = cvLoadImage("C:\\Users\\1234\\Desktop\\rgb.png");      //加载你的图片
						//        cvNamedWindow("example1",CV_WINDOW_AUTOSIZE);   //这是opencv用对话框显示得，咱们不用
						//  cvShowImage("example1",img);

					HWND Hp = GetDlgItem(hwnd , IDC_PIC);      //得到你图片的句柄
					HDC hdc = GetDC(Hp);                        //得到图片句柄对应的DC
					RECT rect;                                 //定义一个矩形放图片滴
					::GetClientRect(Hp , &rect);
					CvvImage cimg;
					cimg.CopyOf(img);
					cimg.DrawToHDC(hdc , &rect);
					break;
				}

				default:
				break;
			}
		}
		default:
		bRet = FALSE;
		break;
	}
	return bRet;
}