#define NOMINMAX
#include <Windows.h>
#include "resource.h"
#include <algorithm>

using std::min;
using std::max;

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 011.Collision Sample";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;
constexpr auto BIRD_ITEM_SIZE = 50;
constexpr auto BIRD_WIDTH = BIRD_ITEM_SIZE * 2;
constexpr auto BIRD_X_SPEED = 5;
constexpr auto BIRD_Y_SPEED = 5;

struct Bird {
	int x = (WINDOW_WIDTH - BIRD_ITEM_SIZE) / 2;
	int y = (WINDOW_HEIGHT - BIRD_ITEM_SIZE) / 2;
	int xs = BIRD_X_SPEED;
	int ys = BIRD_Y_SPEED;
};

HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuf = nullptr;
HBITMAP bg = nullptr, bgEmpty = nullptr, bmpBird = nullptr;
ULONGLONG prevShow = 0;
int index = 0;
Bird bird{};
RECT rc{0};


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
ATOM myRegisterClass(HINSTANCE hInstance);
bool myCreateWindow(HINSTANCE hInstance, int show);
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
		} else if (GetTickCount64() - prevShow >= 10) {
			myPaint(hwndMain);
		}
	}

	DeleteDC(hdcBuf); hdcBuf = nullptr;
	DeleteDC(hdcMem); hdcMem = nullptr;
	ReleaseDC(hwndMain, hdc); hdc = nullptr;
	DeleteObject(bg); bg = nullptr;
	DeleteObject(bgEmpty); bgEmpty = nullptr;
	DeleteObject(bmpBird); bmpBird = nullptr;
	ShowCursor(TRUE);

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
	switch (message) {
	case WM_KEYDOWN:
		switch (wp) {
		case VK_ESCAPE:
			PostQuitMessage(0);
			return 0;
		
		case VK_UP:
			if (bird.ys == 0) { bird.ys = BIRD_Y_SPEED; } else { bird.ys *= 1.5; }
			break;

		case VK_DOWN:
			if (bird.ys == 0) { bird.ys = BIRD_Y_SPEED; } else { bird.ys *= 0.75; }
			break;

		case VK_LEFT:
			if (bird.xs == 0) { bird.xs = BIRD_X_SPEED; } else { bird.xs *= 0.75; }
			break;

		case VK_RIGHT:
			if (bird.xs == 0) { bird.xs = BIRD_X_SPEED; } else { bird.xs *= 1.5; }
			break;

		}
		break;

	case WM_MOUSEMOVE:
		break;

	case WM_LBUTTONDOWN:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hwnd, message, wp, lp);
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
	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstance, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	hdc = GetDC(hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	hdcBuf = CreateCompatibleDC(hdc);
	bgEmpty = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
	SelectObject(hdcMem, bgEmpty);

	bg = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BG), IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR);
	bmpBird = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BIRD), IMAGE_BITMAP, BIRD_WIDTH, BIRD_ITEM_SIZE, LR_DEFAULTCOLOR);

	GetClientRect(hwnd, &rc);

	myPaint(hwnd);


	return true;
}

void myPaint(HWND hwnd)
{
	wchar_t text[1024];
	wsprintf(text, L"%s - bird: %d, %d, speed: %d, %d",
			 WINDOW_TITLE, bird.x, bird.y, bird.xs, bird.ys);
	SetWindowTextW(hwnd, text);

	// draw rolling background
	HGDIOBJ old = SelectObject(hdcBuf, bg);
	BitBlt(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcBuf, 0, 0, SRCCOPY);

	// draw bird
	SelectObject(hdcBuf, bmpBird);
	BitBlt(hdcMem, bird.x, bird.y, BIRD_ITEM_SIZE, BIRD_ITEM_SIZE,
		   hdcBuf, BIRD_ITEM_SIZE, 0, SRCAND);
	BitBlt(hdcMem, bird.x, bird.y, BIRD_ITEM_SIZE, BIRD_ITEM_SIZE,
		   hdcBuf, 0, 0, SRCPAINT);

	SelectObject(hdcBuf, old);
	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		   hdcMem, 0, 0, SRCCOPY);

	bird.x += bird.xs;
	if (bird.x < 0) {
		bird.x = 0;
		bird.xs = -bird.xs;
	} else if (bird.x >= rc.right - BIRD_ITEM_SIZE) {
		bird.x = rc.right - BIRD_ITEM_SIZE;
		bird.xs = -bird.xs;
	}

	bird.y += bird.ys;
	if (bird.y < 0) {
		bird.y = 0;
		bird.ys = -bird.ys;
	} else if (bird.y >= rc.bottom - BIRD_ITEM_SIZE) {
		bird.y = rc.bottom - BIRD_ITEM_SIZE;
		bird.ys = -bird.ys;
	}

	prevShow = GetTickCount64();
}
