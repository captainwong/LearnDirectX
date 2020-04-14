#include <Windows.h>
#include "resource.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 009.Go With KeyPress Sample";
constexpr auto WINDOW_WIDTH = 641;
constexpr auto WINDOW_HEIGHT = 480;
constexpr auto SPRITE_ITEM_WIDTH = 60;
constexpr auto SPRITE_ITEM_HEIGHT = 108;
constexpr auto SPRITE_ITEM_COUNT_X = 8;
constexpr auto SPRITE_ITEM_COUNT_Y = 2;
constexpr auto SPRITE_WIDTH = SPRITE_ITEM_WIDTH * SPRITE_ITEM_COUNT_X;
constexpr auto SPRITE_HEIGHT = SPRITE_ITEM_HEIGHT * SPRITE_ITEM_COUNT_Y;
constexpr auto SPRITE_COUNT = 4;
constexpr auto SPRITE_MOVE_STEP = 10; // 单次移动步长

enum Direction {
	Up,
	Down,
	Left,
	Right,
};

struct Sprite {
	int x = (WINDOW_WIDTH - SPRITE_ITEM_WIDTH) / 2;
	int y = (WINDOW_HEIGHT - SPRITE_ITEM_HEIGHT) / 2;
	Direction direction = Direction::Down;
};

HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuf = nullptr;
HBITMAP bg = nullptr, bmpSprites[SPRITE_COUNT] = { nullptr }, bgEmpty = nullptr;
ULONGLONG prevShow = 0;
int index = 0;
Sprite sprite = {};


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
		} else if (GetTickCount64() - prevShow >= 100) {
			myPaint(hwndMain);
		}
	}

	DeleteDC(hdcBuf); hdcBuf = nullptr;
	DeleteDC(hdcMem); hdcMem = nullptr;
	ReleaseDC(hwndMain, hdc); hdc = nullptr;
	DeleteObject(bg); bg = nullptr;
	DeleteObject(bgEmpty); bgEmpty = nullptr;
	for (int i = 0; i < SPRITE_COUNT; i++) {
		DeleteObject(bmpSprites[i]); bmpSprites[i] = nullptr;
	}

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
			sprite.y = max(sprite.y - SPRITE_MOVE_STEP, 0);
			sprite.direction = Direction::Up;
			break;

		case VK_DOWN:
			sprite.y = min(sprite.y + SPRITE_MOVE_STEP, WINDOW_HEIGHT - SPRITE_ITEM_HEIGHT - GetSystemMetrics(SM_CYCAPTION));
			sprite.direction = Direction::Down;
			break;

		case VK_LEFT:
			sprite.x = max(sprite.x - SPRITE_MOVE_STEP, 0);
			sprite.direction = Direction::Left;
			break;

		case VK_RIGHT:
			sprite.x = min(sprite.x + SPRITE_MOVE_STEP, WINDOW_WIDTH - SPRITE_ITEM_WIDTH);
			sprite.direction = Direction::Right;
			break;

		}
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


	bg = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BG), IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR);
	for (int i = 0; i < SPRITE_COUNT; i++) {
		bmpSprites[i] = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_UP + i), IMAGE_BITMAP, SPRITE_WIDTH, SPRITE_HEIGHT, LR_DEFAULTCOLOR);
	}
	
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

void myPaint(HWND hwnd)
{
	wchar_t text[1024];
	wsprintf(text, L"%s - Sprite Index: %d", WINDOW_TITLE, index);
	SetWindowTextW(hwnd, text);

	HGDIOBJ old = SelectObject(hdcBuf, bg);
	BitBlt(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcBuf, 0, 0, SRCCOPY);
	SelectObject(hdcBuf, bmpSprites[sprite.direction]);
	BitBlt(hdcMem, sprite.x, sprite.y, SPRITE_ITEM_WIDTH, SPRITE_ITEM_HEIGHT,
		   hdcBuf, index * SPRITE_ITEM_WIDTH, SPRITE_ITEM_HEIGHT, SRCAND);
	BitBlt(hdcMem, sprite.x, sprite.y, SPRITE_ITEM_WIDTH, SPRITE_ITEM_HEIGHT,
		   hdcBuf, index * SPRITE_ITEM_WIDTH, 0, SRCPAINT);
	SelectObject(hdcBuf, old);

	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		   hdcMem, 0, 0, SRCCOPY);

	index = (index + 1) % SPRITE_ITEM_COUNT_X;

	prevShow = GetTickCount64();
}
