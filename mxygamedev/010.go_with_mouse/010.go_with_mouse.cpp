#define NOMINMAX
#include <Windows.h>
#include "resource.h"
#include <list>
#include <algorithm>

using std::min;
using std::max;

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 010.Go With Mouse Sample";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;
constexpr auto SWORDBLADE_WIDTH = 100;
constexpr auto SWORDBLADE_HEIGHT = 26;
constexpr auto SWORDBLADE_BG_COL = RGB(0, 0, 0);
constexpr auto SWORDBLADE_SPEED = 20;
constexpr auto SWORDMAN_WIDTH = 317;
constexpr auto SWORDMAN_HEIGHT = 283;
constexpr auto SWORDMAN_BG_COL = RGB(0, 0, 0);
constexpr auto SWORDMAN_MOVE_STEP = 25;
constexpr auto BG_ROLLING_STEP = 5;


//struct Pos {
//	int x, y;
//};

using Pos = POINT;


HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuf = nullptr;
HBITMAP bg = nullptr, bmpSwordman = nullptr, bmpSwordblade = nullptr, bgEmpty = nullptr;
ULONGLONG prevShow = 0;
int index = 0;
int bgOffset = 0; // 背景滚动
Pos mouse = { (WINDOW_WIDTH - SWORDMAN_WIDTH) / 2, (WINDOW_HEIGHT - SWORDMAN_HEIGHT) / 2 };
Pos swordMan = mouse;
std::list<Pos> swordblades = {};


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
	DeleteObject(bmpSwordman); bmpSwordman = nullptr;
	DeleteObject(bmpSwordblade); bmpSwordblade = nullptr;


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
		}
		break;

	case WM_MOUSEMOVE:
		mouse.x = min(max((int)LOWORD(lp), 0), WINDOW_WIDTH - SWORDMAN_WIDTH);
		mouse.y = min(max((int)HIWORD(lp), 0), WINDOW_HEIGHT - SWORDMAN_HEIGHT);
		break;

	case WM_LBUTTONDOWN:
		swordblades.emplace_back(Pos{ swordMan.x - SWORDMAN_WIDTH / 4, swordMan.y + SWORDMAN_HEIGHT / 2});
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

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	hdc = GetDC(hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	hdcBuf = CreateCompatibleDC(hdc);
	bgEmpty = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
	SelectObject(hdcMem, bgEmpty);

	bg = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BG), IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR);
	bmpSwordman = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SWORDMAN), IMAGE_BITMAP, SWORDMAN_WIDTH, SWORDMAN_HEIGHT, LR_DEFAULTCOLOR);
	bmpSwordblade = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SWORDBLADE), IMAGE_BITMAP, SWORDBLADE_WIDTH, SWORDBLADE_HEIGHT, LR_DEFAULTCOLOR);

	// 设置光标坐标
	POINT pt = mouse;
	ClientToScreen(hwnd, &pt);
	SetCursorPos(pt.x, pt.y);
	ShowCursor(FALSE);

	// 限制光标范围
	RECT rc;
	GetClientRect(hwnd, &rc);
	POINT lt = { rc.left, rc.top };
	POINT rb = { rc.right, rc.bottom };
	ClientToScreen(hwnd, &lt);
	ClientToScreen(hwnd, &rb);
	rc.left = lt.x;
	rc.top = lt.y;
	rc.right = rb.x;
	rc.bottom = rb.y;
	ClipCursor(&rc);

	myPaint(hwnd);

	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstance, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	return true;
}

void myPaint(HWND hwnd)
{
	wchar_t text[1024];
	wsprintf(text, L"%s - bgOffset: %d, mouse: %d, %d, swordman: %d, %d", 
			 WINDOW_TITLE, bgOffset, mouse.x, mouse.y, swordMan.x, swordMan.y);
	SetWindowTextW(hwnd, text);

	// draw rolling background
	HGDIOBJ old = SelectObject(hdcBuf, bg);
	BitBlt(hdcMem, 0, 0, bgOffset, WINDOW_HEIGHT, hdcBuf, WINDOW_WIDTH - bgOffset, 0, SRCCOPY);
	BitBlt(hdcMem, bgOffset, 0, WINDOW_WIDTH - bgOffset, WINDOW_HEIGHT, hdcBuf, 0, 0, SRCCOPY);

	// draw swordman
	swordMan.x = swordMan.x < mouse.x ? min(swordMan.x + SWORDMAN_MOVE_STEP, mouse.x) : max(swordMan.x - SWORDMAN_MOVE_STEP, mouse.x);
	swordMan.y = swordMan.y < mouse.y ? min(swordMan.y + SWORDMAN_MOVE_STEP, mouse.y) : max(swordMan.y - SWORDMAN_MOVE_STEP, mouse.y);
	SelectObject(hdcBuf, bmpSwordman);
	TransparentBlt(hdcMem, swordMan.x, swordMan.y, SWORDMAN_WIDTH, SWORDMAN_HEIGHT,
				   hdcBuf, 0, 0, SWORDMAN_WIDTH, SWORDMAN_HEIGHT, SWORDMAN_BG_COL);

	// draw swordblades
	SelectObject(hdcBuf, bmpSwordblade);
	for (auto iter = swordblades.begin(); iter != swordblades.end(); ) {
		auto& pos = *iter;
		TransparentBlt(hdcMem, pos.x, pos.y, SWORDBLADE_WIDTH, SWORDBLADE_HEIGHT,
					   hdcBuf, 0, 0, SWORDBLADE_WIDTH, SWORDBLADE_HEIGHT, SWORDBLADE_BG_COL);
		pos.x -= SWORDBLADE_SPEED;
		if (pos.x < -SWORDBLADE_WIDTH) {
			iter = swordblades.erase(iter);
		} else {
			iter++;
		}
	}

	SelectObject(hdcBuf, old);
	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		   hdcMem, 0, 0, SRCCOPY);

	bgOffset += BG_ROLLING_STEP;
	if (bgOffset >= WINDOW_WIDTH) {
		bgOffset = 0;
	}

	prevShow = GetTickCount64();
}
