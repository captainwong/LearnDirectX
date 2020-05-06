#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <math.h>
#include <list>
#include <time.h>
#include "resource.h"

using std::min;
using std::max;

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 014.Particle-Star Sample";
constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;
constexpr auto STAR_SIZE = 20;
constexpr auto STAR_BG_COL = RGB(0, 0, 0);
constexpr auto STAR_COUNT = 100U; // 星光数量
constexpr auto STAR_LAST_FRAMES = 60; // 星光存续帧数，超出后消失
constexpr auto STAR_SPEED_BASE = 15;

struct Star {
	int x = 0;
	int y = 0;
	int vx = 0;
	int vy = 0;
	int timestamp = 0;

};


HINSTANCE hInstMain = nullptr;
HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuf = nullptr;
HBITMAP bg = nullptr, bgEmpty = nullptr, bmpStar = nullptr;
ULONGLONG prevShow = 0;
RECT rc{ 0 };
std::list<Star> stars = {};


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
		} else if (GetTickCount64() - prevShow >= 50) {
			myPaint(hwndMain);
		}
	}

	DeleteDC(hdcBuf); hdcBuf = nullptr;
	DeleteDC(hdcMem); hdcMem = nullptr;
	ReleaseDC(hwndMain, hdc); hdc = nullptr;
	DeleteObject(bg); bg = nullptr;
	DeleteObject(bgEmpty); bgEmpty = nullptr;
	DeleteObject(bmpStar); bmpStar = nullptr;
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
			break;

		case VK_DOWN:
			break;

		case VK_LEFT:
			break;

		case VK_RIGHT:
			break;

		case 'P':
			break;

		case VK_SPACE:
			break;

		case VK_ADD:
			break;

		case VK_SUBTRACT:
			break;
		}

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
	hInstMain = hInstance;
	hwndMain = hwnd;

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	hdc = GetDC(hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	hdcBuf = CreateCompatibleDC(hdc);
	bgEmpty = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
	SelectObject(hdcMem, bgEmpty);

	bg = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BG), IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR);
	bmpStar = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_STAR), IMAGE_BITMAP, STAR_SIZE, STAR_SIZE, LR_DEFAULTCOLOR);

	GetClientRect(hwnd, &rc);

	srand((unsigned int)time(nullptr));

	myPaint(hwnd);

	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstMain, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	return true;
}

void myPaint(HWND hwnd)
{
	/*wchar_t text[1024];
	swprintf_s(text, L"%s - snowCount: %d, speed: %d %d",
			   WINDOW_TITLE, snowCount, snowXs, snowYs);
	SetWindowTextW(hwnd, text);*/

	// draw rolling background
	HGDIOBJ old = SelectObject(hdcBuf, bg);
	BitBlt(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcBuf, 0, 0, SRCCOPY);

	if (stars.empty()) {
		int x = rand() % rc.right;
		int y = rand() % rc.bottom;

		for (size_t i = 0; i < STAR_COUNT; i++) {
			Star star{ x, y};
			switch (i % 4) {
			case 0: // 左上
			{
				star.vx = -(1 + rand() % STAR_SPEED_BASE);
				star.vy = -(1 + rand() % STAR_SPEED_BASE);
				break;
			}

			case 1: // 右上
			{
				star.vx = (1 + rand() % STAR_SPEED_BASE);
				star.vy = -(1 + rand() % STAR_SPEED_BASE);
				break;
			}

			case 2: // 右下
			{
				star.vx = (1 + rand() % STAR_SPEED_BASE);
				star.vy = (1 + rand() % STAR_SPEED_BASE);
				break;
			}

			case 3: // 左下
			{
				star.vx = -(1 + rand() % STAR_SPEED_BASE);
				star.vy = (1 + rand() % STAR_SPEED_BASE);
				break;
			}
			}

			stars.emplace_back(star);
		}
	}

	SelectObject(hdcBuf, bmpStar);
	for (auto iter = stars.begin(); iter != stars.end(); ) {
		auto& star = *iter;
		TransparentBlt(hdcMem, star.x, star.y, STAR_SIZE, STAR_SIZE,
					   hdcBuf, 0, 0, STAR_SIZE, STAR_SIZE, STAR_BG_COL);
		star.x += star.vx;
		star.y += star.vy;
		if (++star.timestamp > STAR_LAST_FRAMES || (star.x < rc.left - STAR_SIZE || star.x > rc.right || star.y >= rc.bottom)) {
			iter = stars.erase(iter);
		} else {
			iter++;
		}
	}

	SelectObject(hdcBuf, old);
	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		   hdcMem, 0, 0, SRCCOPY);

	prevShow = GetTickCount64();
}
