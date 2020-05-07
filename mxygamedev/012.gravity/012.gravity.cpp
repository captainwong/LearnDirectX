#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#include "resource.h"
#include <algorithm>
#include <math.h>

using std::min;
using std::max;

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 012.Gravity & Friction Sample";
constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;
constexpr auto BIRD_ITEM_SIZE = 50;
constexpr auto BIRD_WIDTH = BIRD_ITEM_SIZE * 2;
constexpr auto BIRD_X_SPEED = 50;
constexpr auto BIRD_Y_SPEED = 5;
constexpr auto GRAVITY = 3;
constexpr auto COLLIDE_PARAM = 0.8; // 碰撞时损失20%速度
constexpr auto FRICTION_PARAM = 0.1; // 每次绘制时因为摩擦力损失1个单位速度

struct Bird {
	int x = BIRD_ITEM_SIZE;
	int y = BIRD_ITEM_SIZE;
	float xs = BIRD_X_SPEED;
	float ys = 0;
};

HINSTANCE hInstMain = nullptr;
HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuf = nullptr;
HBITMAP bg = nullptr, bgEmpty = nullptr, bmpBird = nullptr;
ULONGLONG prevShow = 0;
int index = 0;
Bird bird{};
RECT rc{ 0 };
int gravity = GRAVITY;
bool paused = false;
bool stepMode = false;


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
ATOM myRegisterClass(HINSTANCE hInstance);
bool myCreateWindow(HINSTANCE hInstance, int show);
void myPaint(HWND hwnd);
void moveBird();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int show)
{
	myRegisterClass(hInstance);
	if (!myCreateWindow(hInstance, show)) {
		return 1;
	}

	mciSendStringW(L"open ./res/ビルジ湖.wav alias bgm", NULL, 0, NULL);
	mciSendStringW(L"play bgm", NULL, 0, NULL);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		} else if (GetTickCount64() - prevShow >= 10) {
			myPaint(hwndMain);
		}
	}

	mciSendStringW(L"stop bgm", NULL, 0, NULL);

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
			bird.ys -= 50;
			break;

		case VK_DOWN:
			bird.ys += 50;
			break;

		case VK_LEFT:
			bird.xs -= 50;
			break;

		case VK_RIGHT:
			bird.xs += 50;
			break;

		case 'P':
			stepMode = paused = !paused;
			break;

		case VK_SPACE:
			if (stepMode) {
				moveBird();
			}
			break;

		case VK_ADD:
			gravity++;
			break;
			
		case VK_SUBTRACT:
			gravity--;
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
	bmpBird = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BIRD), IMAGE_BITMAP, BIRD_WIDTH, BIRD_ITEM_SIZE, LR_DEFAULTCOLOR);

	GetClientRect(hwnd, &rc);

	myPaint(hwnd);


	return true;
}

void myPaint(HWND hwnd)
{
	wchar_t text[1024];
	swprintf_s(text, L"%s - bird: %d, %d, speed: %.2f %.2f, gravity: %d%s%s",
			 WINDOW_TITLE, bird.x, bird.y, bird.xs, bird.ys, gravity, paused ? L", paused" : L"", stepMode ? L", stepMode" : L"");
	SetWindowTextW(hwnd, text);

	// draw background
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

	if (!paused) {
		moveBird();
	}

	prevShow = GetTickCount64();
}

void moveBird()
{
	bool collide = false;
	if (bird.xs > 0) {
		bird.xs -= FRICTION_PARAM;
	} else if (bird.xs < 0) {
		bird.xs += FRICTION_PARAM;
	}
	if (abs(bird.xs) <= 1) {
		bird.xs = 0;
	}
	bird.x += bird.xs;
	if (bird.x < 0) {
		bird.x = 0;
		bird.xs = -bird.xs * COLLIDE_PARAM;
		collide = true;
	} else if (bird.x >= rc.right - BIRD_ITEM_SIZE) {
		bird.x = rc.right - BIRD_ITEM_SIZE;
		bird.xs = -bird.xs * COLLIDE_PARAM;
		collide = true;
	}

	bird.ys += gravity;
	if (bird.ys > 0) {
		bird.ys -= FRICTION_PARAM;
	} else if (bird.ys < 0) {
		bird.ys += FRICTION_PARAM;
	}
	bird.y += bird.ys;
	if (bird.y < 0) {
		bird.y = 0;
		bird.ys = -bird.ys * COLLIDE_PARAM;
		collide = true;
	} else if (bird.y == rc.bottom - BIRD_ITEM_SIZE) {
		
	} else if (bird.y > rc.bottom - BIRD_ITEM_SIZE - 1) {
		bird.y = rc.bottom - BIRD_ITEM_SIZE;
		bird.ys = -bird.ys * COLLIDE_PARAM;
		if (abs(bird.ys) < gravity) {
			bird.ys = 0;
		}
		collide = true;
	}
	

	if (collide) {
		PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstMain, SND_RESOURCE | SND_ASYNC);
	}
}
