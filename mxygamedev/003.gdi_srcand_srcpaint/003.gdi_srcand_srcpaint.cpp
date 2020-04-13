#include <Windows.h>
#include "resource.h"

#pragma comment(lib, "winmm.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 003.GDI SRCAND SRCPAINT (透明遮罩法) Sample";
constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;
constexpr auto CH1_BMP_WIDTH = 640;
constexpr auto CH1_BMP_HEIGHT = 579;
constexpr auto CH2_BMP_WIDTH = 800;
constexpr auto CH2_BMP_HEIGHT = 584;

HDC hdc = nullptr;
HBITMAP bg = nullptr, ch1 = nullptr, ch2 = nullptr;

ATOM myRegisterClass(HINSTANCE hInstance);
bool myCreateWindow(HINSTANCE hInstance, int show);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
void myPaint(HDC dc);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int show)
{
	myRegisterClass(hInstance);
	if (!myCreateWindow(hInstance, show)) {
		return 1;
	}

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;
	HDC dc;
	switch (message) {
	case WM_PAINT:
		dc = BeginPaint(hwnd, &ps);
		myPaint(dc);
		EndPaint(hwnd, &ps);
		break;

	case WM_DESTROY:
		DeleteDC(hdc); hdc = nullptr;
		DeleteObject(bg); bg = nullptr;
		DeleteObject(ch1); ch1 = nullptr;
		DeleteObject(ch2); ch2 = nullptr;
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProcW(hwnd, message, wp, lp);
	}

	return 0;
}

void myPaint(HDC dc)
{
	// 角色图片为左右均分

	auto old = SelectObject(hdc, bg);
	BitBlt(dc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdc, 0, 0, SRCCOPY);

	SelectObject(hdc, ch1);
	BitBlt(dc, 50, WINDOW_HEIGHT - CH1_BMP_HEIGHT - 50, CH1_BMP_WIDTH / 2, CH1_BMP_HEIGHT, hdc, CH1_BMP_WIDTH / 2, 0, SRCAND);
	BitBlt(dc, 50, WINDOW_HEIGHT - CH1_BMP_HEIGHT - 50, CH1_BMP_WIDTH / 2, CH1_BMP_HEIGHT, hdc, 0, 0, SRCPAINT);

	SelectObject(hdc, ch2);
	BitBlt(dc, WINDOW_WIDTH / 2 + 50, WINDOW_HEIGHT - CH2_BMP_HEIGHT - 50, CH2_BMP_WIDTH / 2, CH2_BMP_HEIGHT, hdc, CH2_BMP_WIDTH / 2, 0, SRCAND);
	BitBlt(dc, WINDOW_WIDTH / 2 + 50, WINDOW_HEIGHT - CH2_BMP_HEIGHT - 50, CH2_BMP_WIDTH / 2, CH2_BMP_HEIGHT, hdc, 0, 0, SRCPAINT);

	SelectObject(hdc, old);
}

ATOM myRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbWndExtra = 0;
	wc.cbClsExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = WINDOW_CLASS;
	wc.hIconSm = nullptr;
	return RegisterClassExW(&wc);
}

bool myCreateWindow(HINSTANCE hInstance, int show)
{
	HWND hwnd = CreateWindow(WINDOW_CLASS, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
							 CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);
	if (!hwnd) { return false; }

	bg = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BG), IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR);
	ch1 = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_CHARACTER1), IMAGE_BITMAP, CH1_BMP_WIDTH, CH1_BMP_HEIGHT, LR_DEFAULTCOLOR);
	ch2 = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_CHARACTER2), IMAGE_BITMAP, CH2_BMP_WIDTH, CH2_BMP_HEIGHT, LR_DEFAULTCOLOR);

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);
	HDC dc = GetDC(hwnd);
	hdc = CreateCompatibleDC(dc);
	myPaint(dc);
	ReleaseDC(hwnd, dc);

	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstance, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	return true;
}
