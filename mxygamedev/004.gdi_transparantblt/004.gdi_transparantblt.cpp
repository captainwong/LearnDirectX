#include <Windows.h>
#include "resource.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 004.GDI TransparentBlt (Í¸Ã÷É«²Ê·¨) Sample";
constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;
constexpr auto CH1_BMP_WIDTH = 535;
constexpr auto CH1_BMP_HEIGHT = 650;
constexpr auto CH2_BMP_WIDTH = 506;
constexpr auto CH2_BMP_HEIGHT = 650;

enum class DisplayState {
	show_nothing,
	show_bg,
	show_ch1,
	show_ch2,
	ds_count,
};

static const wchar_t* displayStateString(DisplayState ds)
{
	switch (ds) {
	case DisplayState::show_nothing: return L"show_nothing";
	case DisplayState::show_bg:return L"show_bg";
	case DisplayState::show_ch1:return L"show_ch1";
	case DisplayState::show_ch2:return L"show_ch2";
	default:return L"";
	}
}

HDC hdc = nullptr;
HBITMAP bg = nullptr, ch1 = nullptr, ch2 = nullptr;
DisplayState ds = DisplayState::show_nothing;

ATOM myRegisterClass(HINSTANCE hInstance);
bool myCreateWindow(HINSTANCE hInstance, int show);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
void myPaint(HWND hwnd, HDC dc);


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
	RECT rc;

	switch (message) {
	case WM_PAINT:
		dc = BeginPaint(hwnd, &ps);
		myPaint(hwnd, dc);
		EndPaint(hwnd, &ps);
		break;

	case WM_DESTROY:
		DeleteDC(hdc); hdc = nullptr;
		DeleteObject(bg); bg = nullptr;
		DeleteObject(ch1); ch1 = nullptr;
		DeleteObject(ch2); ch2 = nullptr;
		PostQuitMessage(0);
		break;

	case WM_TIMER:
		ds = static_cast<DisplayState>(((int)ds + 1) % (int)DisplayState::ds_count);
		GetClientRect(hwnd, &rc);
		InvalidateRect(hwnd, &rc, TRUE);
		break;

	default:
		return DefWindowProcW(hwnd, message, wp, lp);
	}

	return 0;
}

void myPaint(HWND hwnd, HDC dc)
{
	wchar_t text[1024];
	wsprintf(text, L"%s - DisplayState: %s", WINDOW_TITLE, displayStateString(ds));
	SetWindowTextW(hwnd, text);

	HGDIOBJ old = nullptr;

	if (ds >= DisplayState::show_bg) {
		old = SelectObject(hdc, bg);
		BitBlt(dc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdc, 0, 0, SRCCOPY);
	}

	if (ds >= DisplayState::show_ch1) {
		SelectObject(hdc, ch1);
		TransparentBlt(dc, 50, WINDOW_HEIGHT - CH1_BMP_HEIGHT - 50, CH1_BMP_WIDTH, CH1_BMP_HEIGHT, 
					   hdc, 0, 0, CH1_BMP_WIDTH, CH1_BMP_HEIGHT, RGB(0, 0, 0));
	}

	if (ds >= DisplayState::show_ch2) {
		SelectObject(hdc, ch2);
		TransparentBlt(dc, WINDOW_WIDTH / 2 + 50, WINDOW_HEIGHT - CH2_BMP_HEIGHT - 50, CH2_BMP_WIDTH, CH2_BMP_HEIGHT, 
					   hdc, 0, 0, CH2_BMP_WIDTH, CH2_BMP_HEIGHT, RGB(0, 0, 0));
	}

	if (old) {
		SelectObject(hdc, old);
	}
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

	bg = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BG), IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR);
	ch1 = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_CH1), IMAGE_BITMAP, CH1_BMP_WIDTH, CH1_BMP_HEIGHT, LR_DEFAULTCOLOR);
	ch2 = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_CH2), IMAGE_BITMAP, CH2_BMP_WIDTH, CH2_BMP_HEIGHT, LR_DEFAULTCOLOR);

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);
	HDC dc = GetDC(hwnd);
	hdc = CreateCompatibleDC(dc);
	ReleaseDC(hwnd, dc);

	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstance, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	SetTimer(hwnd, 1, 1500, nullptr);

	return true;
}
