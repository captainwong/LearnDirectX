#include <Windows.h>
#include "resource.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 007.Go Right (Í¸Ã÷¶¯»­) Sample";
constexpr auto WINDOW_WIDTH = 798;
constexpr auto WINDOW_HEIGHT = 600;
constexpr auto SPRITE_ITEM_WIDTH = 60;
constexpr auto SPRITE_ITEM_COUNT = 8;
constexpr auto SPRITE_WIDTH = SPRITE_ITEM_WIDTH * SPRITE_ITEM_COUNT;
constexpr auto SPRITE_HEIGHT = 108;
constexpr auto SPRITE_Y = WINDOW_HEIGHT - SPRITE_HEIGHT - 150;
constexpr auto SPRITE_BG_COLOR = RGB(255, 0, 0);

HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuf = nullptr;
HBITMAP bg = nullptr, sprite = nullptr, bgEmpty = nullptr;
ULONGLONG prevShow = 0;
int x = 0, index = 0;

ATOM myRegisterClass(HINSTANCE hInstance);
bool myCreateWindow(HINSTANCE hInstance, int show);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
void myPaint(HWND hwnd);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int show)
{
	myRegisterClass(hInstance);
	if (!myCreateWindow(hInstance, show)) {
		return 1;
	}

	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		} else if (GetTickCount64() - prevShow >= 100) {
			myPaint(hwndMain);
		}
	}

	DeleteDC(hdcBuf); hdcBuf = nullptr;
	DeleteDC(hdcMem); hdcMem = nullptr;
	ReleaseDC(hwndMain, hdc); hdc = nullptr;
	DeleteObject(bg); bg = nullptr;
	DeleteObject(sprite); sprite = nullptr;

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
	switch (message) {
	case WM_DESTROY:		
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProcW(hwnd, message, wp, lp);
	}

	return 0;
}

void myPaint(HWND hwnd)
{
	wchar_t text[1024];
	wsprintf(text, L"%s - Sprite Index: %d, x: %d", WINDOW_TITLE, index, x);
	SetWindowTextW(hwnd, text);

	HGDIOBJ old = SelectObject(hdcBuf, bg);
	BitBlt(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcBuf, 0, 0, SRCCOPY);
	SelectObject(hdcBuf, sprite);
	TransparentBlt(hdcMem, x, SPRITE_Y, SPRITE_ITEM_WIDTH, SPRITE_HEIGHT,
				   hdcBuf, index * SPRITE_ITEM_WIDTH, 0, SPRITE_ITEM_WIDTH, SPRITE_HEIGHT, SPRITE_BG_COLOR);
	SelectObject(hdcBuf, old);

	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		   hdcMem, 0, 0, SRCCOPY);

	index = (index + 1) % SPRITE_ITEM_COUNT;

	x += 10;
	if (x >= WINDOW_WIDTH) {
		x = -SPRITE_ITEM_WIDTH;
	}

	prevShow = GetTickCount64();
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
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
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
	hwndMain = hwnd;

	bg = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BG), IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR);
	sprite = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SPRITE), IMAGE_BITMAP, SPRITE_WIDTH, SPRITE_HEIGHT, LR_DEFAULTCOLOR);

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);
	hdc = GetDC(hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	hdcBuf = CreateCompatibleDC(hdc);
	bgEmpty = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
	SelectObject(hdcMem, bgEmpty);

	myPaint(hwnd);

	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstance, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	return true;
}
