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
constexpr const auto WINDOW_TITLE = L"LearnDX - 013.Particle-Snowflake Sample";
constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;
constexpr auto SNOW_SIZE = 15;
constexpr auto SNOW_BG_COL = RGB(0, 0, 0);
constexpr auto SNOW_X_SHAKE = 3; // x方向抖动最大距离
constexpr auto SNOW_Y_SPEED = 10; // y方向匀速向下

struct Snowflake {
	int x = 0;
	int y = 0;
};


HINSTANCE hInstMain = nullptr;
HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuf = nullptr;
HBITMAP bg = nullptr, bgEmpty = nullptr, bmpSnow = nullptr;
ULONGLONG prevShow = 0;
RECT rc{ 0 };
std::list<Snowflake> snowflakes = {};
size_t snowCount = 150;
int snowYs = SNOW_Y_SPEED;
int snowXs = 0;


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
	DeleteObject(bmpSnow); bmpSnow = nullptr;
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
			snowYs = max(1, snowYs - 1);
			break;

		case VK_DOWN:
			snowYs++;
			break;

		case VK_LEFT:
			snowXs--;
			break;

		case VK_RIGHT:
			snowXs++;
			break;

		case 'P':
			break;

		case VK_SPACE:
			break;

		case VK_ADD:
			snowCount += 10;
			break;

		case VK_SUBTRACT:
			if (snowCount >= 10) {
				snowCount -= 10;
			}
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
	bmpSnow = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SNOW), IMAGE_BITMAP, SNOW_SIZE, SNOW_SIZE, LR_DEFAULTCOLOR);

	GetClientRect(hwnd, &rc);
	srand((unsigned int)time(nullptr));

	myPaint(hwnd);

	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstMain, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	return true;
}

void myPaint(HWND hwnd)
{
	wchar_t text[1024];
	swprintf_s(text, L"%s - snowCount: %d, speed: %d %d",
			   WINDOW_TITLE, snowCount, snowXs, snowYs);
	SetWindowTextW(hwnd, text);

	// draw rolling background
	HGDIOBJ old = SelectObject(hdcBuf, bg);
	BitBlt(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcBuf, 0, 0, SRCCOPY);

	if (snowflakes.size() < snowCount) {
		snowflakes.emplace_back(Snowflake{ rand() % rc.right, rc.top });
	}

	SelectObject(hdcBuf, bmpSnow);
	for (auto iter = snowflakes.begin(); iter != snowflakes.end(); ) {
		auto& snowflake = *iter;
		TransparentBlt(hdcMem, snowflake.x, snowflake.y, SNOW_SIZE, SNOW_SIZE,
						hdcBuf, 0, 0, SNOW_SIZE, SNOW_SIZE, SNOW_BG_COL);

		if (rand() % 2) {
			snowflake.x += rand() % (SNOW_X_SHAKE + 1);
		} else {
			snowflake.x -= rand() % (SNOW_X_SHAKE + 1);
		}
		snowflake.x += snowXs;
		snowflake.y += snowYs;
		if ((snowflake.x < rc.left - SNOW_SIZE || snowflake.x > rc.right || snowflake.y >= rc.bottom)) {
			if (snowflakes.size() > snowCount) {
				iter = snowflakes.erase(iter);
				continue;
			} else {
				if (snowXs > 0) { // 西风
					if (rand() % (rc.right + rc.bottom) > min(rc.right, rc.bottom)) { // 从上界飘下
						snowflake.x = rand() % rc.right;
						snowflake.y = 0;
					} else { // 从左边界飘落
						snowflake.x = 0;
						snowflake.y = rand() % (rc.bottom - SNOW_SIZE);
					}
				} else if (snowXs < 0) { // 东风
					if (rand() % (rc.right + rc.bottom) > min(rc.right, rc.bottom)) { // 从上界飘下
						snowflake.x = rand() % rc.right;
						snowflake.y = 0;
					} else { // 从右边界飘落
						snowflake.x = rc.right;
						snowflake.y = rand() % (rc.bottom - SNOW_SIZE);
					}
				} else { // 没风
					snowflake.x = rand() % rc.right;
					snowflake.y = 0;
				}
				
			}
		}

		iter++;
	}

	SelectObject(hdcBuf, old);
	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		   hdcMem, 0, 0, SRCCOPY);

	prevShow = GetTickCount64();
}
