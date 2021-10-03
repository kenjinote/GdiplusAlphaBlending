#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "gdiplus")
#pragma comment(lib, "comctl32")

#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include "resource.h"

using namespace Gdiplus;

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, uDpiY, 72)

TCHAR szClassName[] = TEXT("Window");

BOOL GetScaling(HWND hWnd, UINT* pnX, UINT* pnY)
{
	BOOL bSetScaling = FALSE;
	const HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		HMODULE hShcore = LoadLibrary(TEXT("SHCORE"));
		if (hShcore)
		{
			typedef HRESULT __stdcall GetDpiForMonitor(HMONITOR, int, UINT*, UINT*);
			GetDpiForMonitor* fnGetDpiForMonitor = reinterpret_cast<GetDpiForMonitor*>(GetProcAddress(hShcore, "GetDpiForMonitor"));
			if (fnGetDpiForMonitor)
			{
				UINT uDpiX, uDpiY;
				if (SUCCEEDED(fnGetDpiForMonitor(hMonitor, 0, &uDpiX, &uDpiY)) && uDpiX > 0 && uDpiY > 0)
				{
					*pnX = uDpiX;
					*pnY = uDpiY;
					bSetScaling = TRUE;
				}
			}
			FreeLibrary(hShcore);
		}
	}
	if (!bSetScaling)
	{
		HDC hdc = GetDC(NULL);
		if (hdc)
		{
			*pnX = GetDeviceCaps(hdc, LOGPIXELSX);
			*pnY = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL, hdc);
			bSetScaling = TRUE;
		}
	}
	if (!bSetScaling)
	{
		*pnX = DEFAULT_DPI;
		*pnY = DEFAULT_DPI;
		bSetScaling = TRUE;
	}
	return bSetScaling;
}

Bitmap* LoadImageFromResource(const wchar_t* resid, const wchar_t* restype)
{
	Bitmap* pBmp = nullptr;
	HRSRC hrsrc = FindResourceW(GetModuleHandle(0), resid, restype);
	if (hrsrc)
	{
		DWORD dwResourceSize = SizeofResource(GetModuleHandle(0), hrsrc);
		if (dwResourceSize > 0)
		{
			HGLOBAL hGlobalResource = LoadResource(GetModuleHandle(0), hrsrc);
			if (hGlobalResource)
			{
				void* imagebytes = LockResource(hGlobalResource);
				HGLOBAL hGlobal = GlobalAlloc(GHND, dwResourceSize);
				if (hGlobal)
				{
					void* pBuffer = GlobalLock(hGlobal);
					if (pBuffer)
					{
						memcpy(pBuffer, imagebytes, dwResourceSize);
						IStream* pStream = nullptr;
						HRESULT hr = CreateStreamOnHGlobal(hGlobal, FALSE, &pStream);
						if (SUCCEEDED(hr))
						{
							pBmp = new Bitmap(pStream);
						}
						if (pStream)
						{
							pStream->Release();
							pStream = nullptr;
						}
					}
					GlobalFree(hGlobal);
					hGlobal = nullptr;
				}
			}
		}
	}
	return pBmp;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hSlider;
	static UINT uDpiX = DEFAULT_DPI, uDpiY = DEFAULT_DPI;
	static Bitmap* pBitmap;
	static int nPos;
	static HBITMAP hDoubleBufferBitmap;
	static HDC hDoubleBufferDC;
	static DWORD dwDoubleBufferWidth;
	static DWORD dwDoubleBufferHeight;
	switch (msg)
	{
	case WM_CREATE:
		InitCommonControls();
		hSlider = CreateWindow(L"msctls_trackbar32", 0, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		pBitmap = LoadImageFromResource(MAKEINTRESOURCE(IDB_PNG1), L"PNG");
		break;
	case WM_SIZE:
		MoveWindow(hSlider, POINT2PIXEL(10), POINT2PIXEL(10), LOWORD(256), POINT2PIXEL(32), TRUE);
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			dwDoubleBufferWidth = rect.right;
			dwDoubleBufferHeight = rect.bottom;

			DeleteObject(hDoubleBufferBitmap);
			hDoubleBufferBitmap = 0;
			DeleteDC(hDoubleBufferDC);
			hDoubleBufferDC = 0;

			HDC hdc = GetDC(hWnd);
			hDoubleBufferDC = CreateCompatibleDC(hdc);
			hDoubleBufferBitmap = CreateCompatibleBitmap(hdc, dwDoubleBufferWidth, dwDoubleBufferHeight);
			SelectObject(hDoubleBufferDC, hDoubleBufferBitmap);
			ReleaseDC(hWnd, hdc);

			PatBlt(hDoubleBufferDC, 0, 0, dwDoubleBufferWidth, dwDoubleBufferHeight, WHITENESS);
		}
		break;
	case WM_APP:
		if (hDoubleBufferDC && pBitmap)
		{
			Graphics g(hDoubleBufferDC);
			g.Clear(Color::White);
			ImageAttributes attr;
			ColorMatrix cmat = {
				1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f * ((REAL)nPos / (REAL)100.0f), 0.0f,
				0.0f, 0.0f, 0.0f, 0.0f, 1.0f
			};
			attr.SetColorMatrix(&cmat);
			g.DrawImage(pBitmap, RectF((REAL)0, (REAL)50, (REAL)201 * 2, (REAL)90 * 2), (REAL)0, (REAL)0, (REAL)pBitmap->GetWidth(), (REAL)pBitmap->GetHeight(), Unit::UnitPixel, &attr);
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (hDoubleBufferDC)
			{
				RECT rect;
				GetClientRect(hWnd, &rect);
				StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hDoubleBufferDC, 0, 0, dwDoubleBufferWidth, dwDoubleBufferHeight, SRCCOPY);
			}
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_HSCROLL:
		if ((HWND)lParam == hSlider)
		{
			nPos = (int)SendMessage(hSlider, TBM_GETPOS, NULL, NULL);
			SendMessage(hWnd, WM_APP, 0, 0);
			InvalidateRect(hWnd, 0, 0);
		}
		break;
	case WM_NCCREATE:
		{
			const HMODULE hModUser32 = GetModuleHandle(TEXT("user32.dll"));
			if (hModUser32)
			{
				typedef BOOL(WINAPI*fnTypeEnableNCScaling)(HWND);
				const fnTypeEnableNCScaling fnEnableNCScaling = (fnTypeEnableNCScaling)GetProcAddress(hModUser32, "EnableNonClientDpiScaling");
				if (fnEnableNCScaling)
				{
					fnEnableNCScaling(hWnd);
				}
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_DESTROY:
		DeleteObject(hDoubleBufferBitmap);
		hDoubleBufferBitmap = 0;
		DeleteDC(hDoubleBufferDC);
		hDoubleBufferDC = 0;
		if (pBitmap)
		{
			delete pBitmap;
			pBitmap = 0;
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPWSTR pCmdLine, int nCmdShow)
{
	ULONG_PTR gdiToken;
	GdiplusStartupInput gdiSI;
	GdiplusStartup(&gdiToken, &gdiSI, NULL);
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("Window"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	GdiplusShutdown(gdiToken);
	return (int)msg.wParam;
}
