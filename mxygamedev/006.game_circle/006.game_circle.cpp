#include <Windows.h>
#include "resource.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 006.Game Circle Sample";
constexpr auto WINDOW_WIDTH = 356;
constexpr auto WINDOW_HEIGHT = 200;
constexpr auto BMP_COUNT = 11;

HWND hwndMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr;
HBITMAP bmps[BMP_COUNT] = {};
int index = 0;
ULONGLONG prevShow = 0;

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
		} else {
			auto now = GetTickCount64();
			if (now - prevShow >= 100) {
				index = (index + 1) % BMP_COUNT;
				myPaint(hwndMain);
			}
		}
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
	switch (message) {
	case WM_DESTROY:
		DeleteDC(hdcMem); hdcMem = nullptr;
		ReleaseDC(hwnd, hdc); hdc = nullptr;
		for (int i = 0; i < BMP_COUNT; i++) {
			DeleteObject(bmps[i]);
		}
		memset(bmps, 0, sizeof(bmps));
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
	wsprintf(text, L"%s - Bmp Index: %d", WINDOW_TITLE, index);
	SetWindowTextW(hwnd, text);

	HGDIOBJ old = SelectObject(hdcMem, bmps[index]);
	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdc, old);

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

	wchar_t path[1024];
	for (int i = 0; i < BMP_COUNT; i++) {
		wsprintf(path, L"./res/%d.bmp", i);
		bmps[i] = (HBITMAP)LoadImageW(nullptr, path, IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR | LR_LOADFROMFILE);
	}

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);
	hdc = GetDC(hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	myPaint(hwnd);

	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstance, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	return true;
}
