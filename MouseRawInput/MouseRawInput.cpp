// MouseRawInput.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "strsafe.h"
#include "MouseRawInput.h"
#include "opencv2\opencv.hpp"
#include <process.h>
#include "glog\logging.h"

#define MAX_LOADSTRING 100
//#define GLOG_NO_ABBREVIATED_SEVERITIES

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

UINT count = 0;
LPTSTR logMsg = new TCHAR[256];
HBITMAP hBitmap = NULL;  
HDC hDDC = NULL;  
HDC hCDC = NULL;  
int nWidth;  
int nHeight;
IplImage *img = NULL;
bool gdiScreenCapture();
void initScreenCapture();
HANDLE hThread = NULL;
HANDLE captureEvent = NULL;
bool exitCaptureThread = FALSE;
DWORD threadID = 0;
DWORD dwTimerId = 0;

void initScreenCapture(){
	// get screen resolution
	nWidth = GetSystemMetrics(SM_CXSCREEN);
	nHeight = GetSystemMetrics(SM_CYSCREEN);
	// create a rgba format IplImage
	img = cvCreateImage(cvSize(nWidth,nHeight),8,4);
	// Get desktop DC, create a compatible dc, create a comaptible bitmap and select into compatible dc.  
	// acquire the screen
	hDDC = GetDC(NULL);
	hCDC = CreateCompatibleDC(hDDC);
	// acquire the bitmap
	hBitmap =CreateCompatibleBitmap(hDDC,nWidth,nHeight);
	SelectObject(hCDC,hBitmap);
}

bool gdiScreenCapture(){ 
	BitBlt(hCDC,0,0,nWidth,nHeight,hDDC,0,0,SRCCOPY);  
	//add mouse icon
	CURSORINFO cursor = { sizeof(cursor) };
	GetCursorInfo(&cursor);
	if (cursor.flags == CURSOR_SHOWING) {
		ICONINFO info = { sizeof(info) };
		GetIconInfo(cursor.hCursor, &info);
		const int x = cursor.ptScreenPos.x - info.xHotspot;
		const int y = cursor.ptScreenPos.y - info.yHotspot;

		BITMAP bmpCursor = { 0 };
		GetObject(info.hbmColor, sizeof(bmpCursor), &bmpCursor);
		DrawIconEx(hCDC, x, y, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight,
			0, NULL, DI_NORMAL);
	}
	GetBitmapBits(hBitmap,nWidth*nHeight*4,img->imageData); 

	SYSTEMTIME lpsystime;
	GetLocalTime(&lpsystime);
	WCHAR dirPath[256];
	//folder name correction
	wsprintf(dirPath,L"C:\\Touch_Log\\PIC_LOG\\%04d%02d%02d", lpsystime.wYear, lpsystime.wMonth, lpsystime.wDay);

	if (GetFileAttributesW(dirPath) & FILE_ATTRIBUTE_DIRECTORY) {
		if (!CreateDirectory(dirPath, NULL))
		{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{

			}
			else
			{
				return FALSE;
			}
		}
	}
	else {
		return FALSE;
	}

	CHAR wPath[256];
	sprintf(wPath, "C:\\Touch_Log\\PIC_LOG\\%04d%02d%02d\\%02d_%02d_%02d_%04d.jpg", lpsystime.wYear, lpsystime.wMonth, lpsystime.wDay, lpsystime.wHour, lpsystime.wMinute, lpsystime.wSecond, lpsystime.wMilliseconds);

	cvSaveImage(wPath,img);  
	cvWaitKey();    
	return TRUE;
} 

DWORD WINAPI ThreadProc1(LPVOID lpParam)
{
	if(!gdiScreenCapture())
	{
		return 1;
	}
	return 0;
}

void __cdecl ThreadProc2(void *para)
{
	while(1)
	{
		WaitForSingleObject(captureEvent,INFINITE);
		if(!exitCaptureThread)
		{
			if(!gdiScreenCapture())
			{

			}
		}
		else
		{
			return;
		}
	}
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MOUSERAWINPUT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MOUSERAWINPUT));

	google::InitGoogleLogging(".\\");             // 全局初始化glog
	google::SetStderrLogging(google::GLOG_INFO);         // 设置glog的输出级别，这里的含义是输出INFO级别以上的信息
	google::SetLogDestination(google::GLOG_INFO, ".\\test.log");

	LOG(INFO) << "Application Start";     // 像C++标准流一样去使用就可以了，这时把这条信息定义为INFO级别


	// 主消息循环:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if(hThread)
	{
		exitCaptureThread = TRUE;
		SetEvent(captureEvent);
		WaitForSingleObject(hThread,INFINITE);
		//CloseHandle(hThread);
	}
	if(logMsg)
	{
		delete[] logMsg;
	}
	if(hDDC)
	{
		DeleteDC(hDDC);
	}
	if(hCDC)
	{
		ReleaseDC(NULL, hCDC);

	}	
	if(hBitmap)
	{
		DeleteObject(hBitmap);
	}
	if(dwTimerId != 0)
	{
		KillTimer(NULL, dwTimerId);
	}
	LOG(INFO) << "Exit Application!"; 
	google::ShutdownGoogleLogging();                // 全局关闭glog

	return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
//  注释:
//
//    仅当希望
//    此代码与添加到 Windows 95 中的“RegisterClassEx”
//    函数之前的 Win32 系统兼容时，才需要此函数及其用法。调用此函数十分重要，
//    这样应用程序就可以获得关联的
//    “格式正确的”小图标。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSERAWINPUT));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_MOUSERAWINPUT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // 将实例句柄存储在全局变量中

	//hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	//	CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 250,155 , NULL, NULL, hInstance, NULL);


	if (!hWnd)
	{
		return FALSE;
	}

	dwTimerId = SetTimer(hWnd,1,1000*3600,NULL);

	RAWINPUTDEVICE Rid[2];

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = RIDEV_INPUTSINK;   // adds HID mouse and also ignores legacy mouse messages
	Rid[0].hwndTarget = hWnd;

	//Rid[0].dwFlags = RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
	//Rid[0].hwndTarget = 0;

	Rid[1].usUsagePage = 0x01;
	Rid[1].usUsage = 0x06;
	Rid[1].dwFlags = RIDEV_INPUTSINK;   // adds HID keyboard and also ignores legacy keyboard messages
	Rid[1].hwndTarget = hWnd;

	//Rid[1].dwFlags = RIDEV_NOLEGACY;   // adds HID keyboard and also ignores legacy keyboard messages
	//Rid[1].hwndTarget = 0;

	if (RegisterRawInputDevices(Rid, 2, sizeof(RAWINPUTDEVICE)) == FALSE) {
		return FALSE;
		//registration failed. Call GetLastError for the cause of the error
	}
	captureEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("captureSignal"));
	ResetEvent(captureEvent);
	hThread =(HANDLE)_beginthread(ThreadProc2,0,NULL);
	initScreenCapture();

	//ShowWindow(hWnd, nCmdShow);
	//ShowWindow(hWnd, SW_HIDE);

	UpdateWindow(hWnd);

	return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此添加任意绘图代码...
		EndPaint(hWnd, &ps);
		break;
	case WM_TIMER:
		{
			SYSTEMTIME lpsystime;
			GetLocalTime(&lpsystime);
			CHAR Path[256];
			sprintf(Path, "C:\\Touch_Log\\STATUS_LOG\\%04d%02d%02d.txt", lpsystime.wYear, lpsystime.wMonth, lpsystime.wDay);
			std::ofstream openfile(Path, std::ios::app);
			CHAR log[256];
			sprintf(log, "%02d:%02d:%02d.%04d：self test!\r\n", lpsystime.wHour, lpsystime.wMinute, lpsystime.wSecond, lpsystime.wMilliseconds);
			openfile << log;
			openfile.close();
		}
		break;
	case WM_INPUT:
		{
			//get rawInputData size
			UINT rawInputSize;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &rawInputSize,sizeof(RAWINPUTHEADER));
			//according to rawInputData size, allocation needed memory
			LPBYTE rawInputData = new BYTE[rawInputSize];
			if (rawInputData == NULL)
			{
				return 0;
			}
			//get rawInputData
			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputData, &rawInputSize, sizeof(RAWINPUTHEADER)) != rawInputSize)
			{
				OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));
			}

			RAWINPUT* rawInput = (RAWINPUT*)rawInputData;

			if (rawInput->header.dwType == RIM_TYPEMOUSE)
			{
				if (rawInput->data.mouse.ulButtons == 1 || rawInput->data.mouse.ulButtons == 4)
				{
					SetEvent(captureEvent);
					//HRESULT hResult = StringCchPrintf(logMsg, STRSAFE_MAX_CCH, TEXT("Mouse[%04d]: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"),
					//	count++,
					//	rawInput->data.mouse.usFlags,
					//	rawInput->data.mouse.ulButtons,
					//	rawInput->data.mouse.usButtonFlags,
					//	rawInput->data.mouse.usButtonData,
					//	rawInput->data.mouse.ulRawButtons,
					//	rawInput->data.mouse.lLastX,
					//	rawInput->data.mouse.lLastY,
					//	rawInput->data.mouse.ulExtraInformation);
					//if (FAILED(hResult))
					//{
					//	// TODO: write error handler
					//}
					//OutputDebugString(logMsg);
					//"C:\\HMI_PIC\\%04d%02d%2d\\%02d_%02d_%02d_%04d.jpg", lpsystime.wYear, lpsystime.wMonth, lpsystime.wDay, lpsystime.wHour, lpsystime.wMinute, lpsystime.wSecond, lpsystime.wMilliseconds
					//	SYSTEMTIME lpsystime;
					//GetLocalTime(&lpsystime);
					//WCHAR dirPath[256];
					//wsprintf(dirPath,L"C:\\HMI_PIC\\%04d%02d%2d", lpsystime.wYear, lpsystime.wMonth, lpsystime.wDay);

					//if (GetFileAttributesW(dirPath) & FILE_ATTRIBUTE_DIRECTORY) {
					//	if (!CreateDirectory(dirPath, NULL))
					//	{
					//		if (GetLastError() == ERROR_ALREADY_EXISTS)
					//		{

					//		}
					//		else
					//		{
					//			return FALSE;
					//		}
					//	}
					//}
					//else {
					//	return FALSE;
					//}

					//LOG(INFO) << "Mouse Click."; 
					//HRESULT hResult = StringCchPrintf(logMsg, STRSAFE_MAX_CCH, TEXT("C:\\HMI_PIC\\%04d%02d%2d\\%04d/%02d/%02d %02d_%02d_%02d_%04d :Mouse Click\r\n"),
					//	lpsystime.wYear, 
					//	lpsystime.wMonth, 
					//	lpsystime.wDay, 
					//	lpsystime.wHour, 
					//	lpsystime.wMinute, 
					//	lpsystime.wSecond, 
					//	lpsystime.wMilliseconds);
					//if (FAILED(hResult))
					//{
					//	// TODO: write error handler
					//}
				}
			}

			//delete[] logMsg;
			delete[] rawInputData;
			return 0;
		}
		break;
	case WM_DESTROY:
		LOG(INFO) << "WM_DESTROY"; 
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
