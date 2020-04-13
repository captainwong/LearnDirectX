#include <Windows.h>
#include "resource.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 008.Go Random Direction (ÌùÍ¼) Sample";
constexpr auto WINDOW_WIDTH = 1000;
constexpr auto WINDOW_HEIGHT = 600;
constexpr auto SPRITE_ITEM_WIDTH = 96;
constexpr auto SPRITE_ITEM_COUNT = 4;
constexpr auto SPRITE_WIDTH = SPRITE_ITEM_WIDTH * SPRITE_ITEM_COUNT;
constexpr auto SPRITE_HEIGHT = 96;
constexpr auto SPRITE_COUNT = 4;
constexpr auto SPRITE_Y = WINDOW_HEIGHT - SPRITE_HEIGHT - 150;
constexpr auto SPRITE_BG_COLOR = RGB(0, 0, 0);
constexpr auto SPRITES_TO_SHOW = 10;

enum Direction {
	Down,
	Left,
	Right,
	Up,
};

struct Sprite {
	int x, y;
	Direction direction;
};

Sprite sprites[SPRITES_TO_SHOW] = {0};
HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuff = nullptr;
HBITMAP bg = nullptr, bmpSprites[SPRITE_COUNT] = { nullptr }, bgEmpty = nullptr;
ULONGLONG prevShow = 0;
int index = 0;

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

	DeleteDC(hdcBuff); hdcBuff = nullptr;
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
	wsprintf(text, L"%s - Sprite Index: %d", WINDOW_TITLE, index);
	SetWindowTextW(hwnd, text);

	HGDIOBJ old = SelectObject(hdcBuff, bg);
	BitBlt(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcBuff, 0, 0, SRCCOPY);
	for (int i = 0; i < SPRITES_TO_SHOW; i++) {
		SelectObject(hdcBuff, bmpSprites[sprites[i].direction]);
		TransparentBlt(hdcMem, sprites[i].x, sprites[i].y, SPRITE_ITEM_WIDTH, SPRITE_HEIGHT,
					   hdcBuff, index * SPRITE_ITEM_COUNT, 0, SPRITE_ITEM_WIDTH, SPRITE_HEIGHT, SPRITE_BG_COLOR);
	}
	SelectObject(hdcBuff, old);

	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		   hdcMem, 0, 0, SRCCOPY);

	index = (index + 1) % SPRITE_ITEM_COUNT;

	static int changeD = 0;
	if (++changeD % 6 == 0) {
		changeD = 0;
	}
	for (int i = 0; i < SPRITES_TO_SHOW; i++) {
		if (changeD == 0) {
			switch (sprites[i].direction) {
			case Up:
			case Down:
				sprites[i].direction = rand() % 2 ? Direction::Left : Direction::Right;
				break;
			case Left:
			case Right:
				sprites[i].direction = rand() % 2 ? Direction::Up : Direction::Down;
				break;
			}
		} else {
			switch (sprites[i].direction) {
			case Down:
				sprites[i].y = min(sprites[i].y + SPRITE_HEIGHT / 4, WINDOW_HEIGHT - SPRITE_HEIGHT);
				break;
			case Left:
				sprites[i].x = max(sprites[i].x - SPRITE_ITEM_WIDTH / 4, 0);
				break;
			case Right:
				sprites[i].x = min(sprites[i].x + SPRITE_ITEM_WIDTH / 4, WINDOW_WIDTH - SPRITE_ITEM_WIDTH);
				break;
			case Up:
				sprites[i].y = max(sprites[i].y - SPRITE_HEIGHT / 4, 0);
				break;
			}
		}
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
	for (int i = 0; i < SPRITE_COUNT; i++) {
		bmpSprites[i] = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SPRITE1 + i), IMAGE_BITMAP, SPRITE_WIDTH, SPRITE_HEIGHT, LR_DEFAULTCOLOR);
	}

	srand((unsigned int)GetTickCount64());
	for (int i = 0; i < SPRITES_TO_SHOW; i++) {
		sprites[i].x = rand() % WINDOW_WIDTH;
		sprites[i].y = rand() % WINDOW_HEIGHT;
		sprites[i].direction = Direction(rand() % 4);
	}

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);
	hdc = GetDC(hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	hdcBuff = CreateCompatibleDC(hdc);
	bgEmpty = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
	SelectObject(hdcMem, bgEmpty);

	myPaint(hwnd);

	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstance, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	return true;
}
