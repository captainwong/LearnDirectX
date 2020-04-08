#define WIN32_LEAN_AND_MEAN
#define INITGUID

#include <Windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <math.h>
#include <time.h>
#include <chrono>

#include <ddraw.h>
#pragma comment(lib, "ddraw.lib")

enum class GameState {
	Init,
	StartLevel,
	Run,
	Shutdown,
	Exit,
};

static constexpr int SCREEN_WIDTH = 1920;
static constexpr int SCREEN_HEIGHT = 1080;
static constexpr int SCREEN_BITS_PER_PIXEL = 8;
static constexpr int MAX_COLORS = 256;

static constexpr int WINDOW_WIDTH = 800;
static constexpr int WINDOW_HEIGHT = 600;

static constexpr int BLOCK_WIDTH = 64;
static constexpr int BLOCK_HEIGHT = 16;
static constexpr int BLOCK_ORIGIN_X = 8;
static constexpr int BLOCK_ORIGIN_Y = 8;
static constexpr int BLOCK_X_GAP = 80;
static constexpr int BLOCK_Y_GAP = 32;
static constexpr int BLOCK_ROWS = (SCREEN_HEIGHT / 2 - (BLOCK_Y_GAP - BLOCK_HEIGHT)) / (BLOCK_Y_GAP);
static constexpr int BLOCK_COLS = (SCREEN_WIDTH - (BLOCK_X_GAP - BLOCK_WIDTH)) / (BLOCK_X_GAP);

static constexpr int PADDLE_START_X = (SCREEN_WIDTH / 2 - 16);
static constexpr int PADDLE_START_Y = (SCREEN_HEIGHT - 32);
static constexpr int PADDLE_WIDTH = 64;
static constexpr int PADDLE_HEIGHT = 16;
static constexpr int PADDLE_COLOR = 191;

static constexpr int BALL_START_Y = SCREEN_HEIGHT / 2;
static constexpr int BALL_SIZE = 4;

static constexpr LPCWSTR WINDOW_CLASS_NAME = L"FREAKOUT_CLASS";
static constexpr LPCWSTR WINDOW_TITLE = L"FreakOut";


static LPDIRECTDRAW7 lpdd = nullptr;
static LPDIRECTDRAWSURFACE7 lpddsPrimary = nullptr;
static LPDIRECTDRAWSURFACE7 lpddsBack = nullptr;
static LPDIRECTDRAWPALETTE lpddPalette = nullptr;
static LPDIRECTDRAWCLIPPER lpddClipper = nullptr;
static PALETTEENTRY palette[256] = {};
static PALETTEENTRY savedPalett[256] = {};
static DDSURFACEDESC2 ddsd = {};
static DDBLTFX ddBltfx = {};
static DDSCAPS2 ddScaps; // surface capabilities
static HRESULT ddRet = S_OK;
static DWORD startTime = 0;

static int minClipX = 0;
static int maxClipX = 0;
static int minClipY = 0;
static int maxClipY = 0;

static int screenWidth = 0;
static int screenHeight = 0;
static int screenBpp = 0;

static HWND hWndMainWindow = nullptr;
static HINSTANCE hInstanceMain = nullptr;
static GameState gameState = GameState::Init;

static int paddleX = 0, paddleY = 0;
static int ballX = 0, ballY = 0, ballDx = 0, ballDy = 0;
static int score = 0;
static int level = 1;
static int blocksHit = 0;

using clock_type = std::chrono::steady_clock;
static clock_type::time_point gameClock = {};

static unsigned char blocks[BLOCK_ROWS][BLOCK_COLS] = { 0 };

static void startClock() { gameClock = clock_type::now(); }
static clock_type::duration getClock() { return clock_type::now() - gameClock; }


static bool isKeyDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) ? true : false; }
static bool isKeyUp(int vk) { return !isKeyDown(vk); }


static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (msg) {
	case WM_CREATE:return 0;
	
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static int ddFillSurface(LPDIRECTDRAWSURFACE7 lpdds, int color)
{
	DDBLTFX ddbltfx = { 0 };
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwFillColor = color;
	lpdds->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT | DDBLT_ASYNC, &ddbltfx);
	return 1;
}

// create a clipper from the sent clipList and attach it to the sent surface
static LPDIRECTDRAWCLIPPER ddAttachClipper(LPDIRECTDRAWSURFACE7 lpdds, int nRects, LPRECT clipList)
{
	LPDIRECTDRAWCLIPPER clipper;
	if (lpdd->CreateClipper(0, &clipper, nullptr) != DD_OK) {
		return nullptr;
	}

	char* raw = new char[sizeof(RGNDATAHEADER) + nRects * sizeof(RECT)];
	LPRGNDATA rgn = (LPRGNDATA)(raw);
	memcpy(rgn->Buffer, clipList, sizeof(RECT) * nRects);
	rgn->rdh.dwSize = sizeof(RGNDATAHEADER);
	rgn->rdh.iType = RDH_RECTANGLES;
	rgn->rdh.nCount = nRects;
	rgn->rdh.nRgnSize = nRects * sizeof(RECT);
	rgn->rdh.rcBound.left = rgn->rdh.rcBound.top = 64000;
	rgn->rdh.rcBound.right = rgn->rdh.rcBound.bottom = -64000;

	// find bounds of all clipping regions
	for (int i = 0; i < nRects; i++) {
		if (clipList[i].left < rgn->rdh.rcBound.left) {
			rgn->rdh.rcBound.left = clipList[i].left;
		}
		if (clipList[i].top < rgn->rdh.rcBound.top) {
			rgn->rdh.rcBound.top = clipList[i].top;
		}
		if (clipList[i].right > rgn->rdh.rcBound.right) {
			rgn->rdh.rcBound.right = clipList[i].right;
		}
		if (clipList[i].bottom > rgn->rdh.rcBound.bottom) {
			rgn->rdh.rcBound.bottom = clipList[i].bottom;
		}
	}

	if (clipper->SetClipList(rgn, 0) != DD_OK) {
		delete[] raw;
		return nullptr;
	}

	// attach the clipper to the surface
	if (lpdds->SetClipper(clipper) != DD_OK) {
		delete[] raw;
		return nullptr;
	}

	delete[] raw;
	return clipper;
}

static int ddInit(int w, int h, int bpp)
{
	if (DirectDrawCreateEx(nullptr, (void**)&lpdd, IID_IDirectDraw7, nullptr) != DD_OK) {
		return 0;
	}

	if (lpdd->SetCooperativeLevel(hWndMainWindow, DDSCL_ALLOWMODEX | DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT) != DD_OK) {
		return 0;
	}

	if (lpdd->SetDisplayMode(w, h, bpp, 0, 0) != DD_OK) {
		return 0;
	}

	screenWidth = w;
	screenHeight = h;
	screenBpp = bpp;

	// create primary surface
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;

	// we want a complex flippable surface
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

	// set backbuffer count to 1
	ddsd.dwBackBufferCount = 1;

	// create primary surface
	lpdd->CreateSurface(&ddsd, &lpddsPrimary, nullptr);

	// query for the backbuffer, i.e the secondary surface
	ddScaps.dwCaps = DDSCAPS_BACKBUFFER;
	lpddsPrimary->GetAttachedSurface(&ddScaps, &lpddsBack);

	// create and attach palette

	// create palette, clear all entries
	memset(palette, 0, sizeof(palette));

	// create R G B GR gradient palette
	for (int i = 0; i < sizeof(palette) / sizeof(PALETTEENTRY); i++) {
		if (i < 64) {
			palette[i].peRed = i * 4;
		} else if (i < 128) {
			palette[i].peGreen = (i - 64) * 4;
		} else if (i < 192) {
			palette[i].peBlue = (i - 128) * 4;
		} else if (i < 256) {
			palette[i].peRed = palette[i].peGreen = palette[i].peBlue = (i - 192) * 4;
		}
		palette[i].peFlags = PC_NOCOLLAPSE;
	}

	// create palette
	if (lpdd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256, palette, &lpddPalette, nullptr) != DD_OK) {
		return 0;
	}

	// attach the palette to primary surface
	if (lpddsPrimary->SetPalette(lpddPalette) != DD_OK) {
		return 0;
	}

	// clear both primary and secondary surface
	ddFillSurface(lpddsPrimary, 0);
	ddFillSurface(lpddsBack, 0);

	// attach a clipper to the screen
	RECT rc = { 0, 0, screenWidth, screenHeight };
	lpddClipper = ddAttachClipper(lpddsBack, 1, &rc);

	return 1;
}

static int ddFlip()
{
	while (lpddsPrimary->Flip(nullptr, DDFLIP_WAIT) != DD_OK) {

	}
	return 1;
}

static int ddShutdown()
{
	if (lpddClipper) {
		lpddClipper->Release();
	}

	if (lpddPalette) {
		lpddPalette->Release();
	}

	if (lpddsBack) {
		lpddsBack->Release();
	}

	if (lpddsPrimary) {
		lpddsPrimary->Release();
	}

	if (lpdd) {
		lpdd->Release();
	}

	return 1;
}

static int drawRectangle(int l, int t, int r, int b, int color, LPDIRECTDRAWSURFACE7 lpdds)
{
	DDBLTFX bltfx = { 0 };
	bltfx.dwSize = sizeof(bltfx);
	bltfx.dwFillColor = color;

	RECT rc;
	rc.left = l;
	rc.top = t;
	rc.right = r + 1;
	rc.bottom = b + 1;

	lpdds->Blt(&rc, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT | DDBLT_ASYNC, &bltfx);
	
	return 1;
}

static int drawTextGDI(const std::wstring& text, int x, int y, int color, LPDIRECTDRAWSURFACE7 lpdds)
{
	HDC hdc;
	if (lpdds->GetDC(&hdc) != DD_OK) { return 0; }
	SetTextColor(hdc, RGB(palette[color].peRed, palette[color].peGreen, palette[color].peBlue));
	SetBkMode(hdc, TRANSPARENT);
	TextOutW(hdc, x, y, text.data(), text.size());
	lpdds->ReleaseDC(hdc);
	return 1;
}

static void initBlocks()
{
	for (int row = 0; row < BLOCK_ROWS; row++) {
		for (int col = 0; col < BLOCK_COLS; col++) {
			blocks[row][col] = row * 16 + col * 3 + 16;
		}
	}
}

static void drawBlocks()
{
	int x = BLOCK_ORIGIN_X;
	int y = BLOCK_ORIGIN_Y;

	for (int row = 0; row < BLOCK_ROWS; row++) {
		x = BLOCK_ORIGIN_X;

		for (int col = 0; col < BLOCK_COLS; col++) {
			if (blocks[row][col] != 0) {
				drawRectangle(x - 4, y + 4, x + BLOCK_WIDTH - 4, y + BLOCK_HEIGHT + 4, 0, lpddsBack);
				drawRectangle(x, y, x + BLOCK_WIDTH, y + BLOCK_HEIGHT, blocks[row][col], lpddsBack);
			}

			x += BLOCK_X_GAP;
		}

		y += BLOCK_Y_GAP;
	}
}

static void processBall()
{
	// check if ball hit paddle
	if (ballY > (SCREEN_HEIGHT / 2) && ballDy > 0) {
		int x = ballX + (BALL_SIZE / 2);
		int y = ballY + (BALL_SIZE / 2);

		// ball hit paddle
		if ((paddleX <= x && x <= paddleX + PADDLE_WIDTH) &&
			(paddleY <= y && y <= paddleY + PADDLE_HEIGHT)) {
			// reflect ball
			ballDy = -ballDy;
			ballY += ballDy;

			if (isKeyDown(VK_RIGHT)) {
				ballDx -= rand() % 3;
			} else if (isKeyDown(VK_LEFT)) {
				ballDx += rand() % 3;
			} else {
				ballDx += -1 + rand() % 3;
			}

			if (blocksHit >= BLOCK_ROWS * BLOCK_COLS) {
				gameState = GameState::StartLevel;
				level++;
			}

			MessageBeep(MB_OK);
			return;
		}
	} else {
		int x = BLOCK_ORIGIN_X;
		int y = BLOCK_ORIGIN_Y;
		int bx = ballX + BALL_SIZE / 2; // ball center x
		int by = ballY + BALL_SIZE / 2; // ball center y

		// check if ball hit block
		for (int row = 0; row < BLOCK_ROWS; row++) {
			x = BLOCK_ORIGIN_X;
			for (int col = 0; col < BLOCK_COLS; col++) {
				// ball exists
				if (blocks[row][col]) {
					// ball hit block
					if (x < bx && bx < x + BLOCK_WIDTH &&
						y < by && by < y + BLOCK_HEIGHT) {
						// remove block
						blocks[row][col] = 0;
						blocksHit++;
						// bounce the ball
						ballDy = -ballDy;
						ballDx += -1 + rand() % 3;
						MessageBeep(MB_OK);
						score += 5 * (level + abs(ballDx));
						return;
					}
				}

				x += BLOCK_X_GAP;
			}
			y += BLOCK_Y_GAP;
		}
	}
}


static int gameInit(void* p = nullptr) { return 1; }
static int gameShutdown(void* p = nullptr) { return 1; }

static int gameMain(void* p = nullptr)
{
	switch (gameState) {
	case GameState::Init:
	{
		ddInit(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BITS_PER_PIXEL);
		srand((unsigned int)time(nullptr));
		paddleX = PADDLE_START_X;
		paddleY = PADDLE_START_Y;

		ballX = 8 + rand() % (SCREEN_WIDTH - 16);
		ballY = BALL_START_Y;
		ballDx = -4 + rand() % (8 + 1);
		ballDy = 6 + rand() % 2;
		gameState = GameState::StartLevel;
		break;
	}

	case GameState::StartLevel:
	{
		initBlocks();
		blocksHit = 0;
		gameState = GameState::Run;
		break;
	}

	case GameState::Run:
	{
		startClock();
		drawRectangle(0, 0, screenWidth - 1, screenHeight - 1, 200, lpddsBack);

		if (isKeyDown(VK_RIGHT)) {
			paddleX = min(paddleX + 8, SCREEN_WIDTH - PADDLE_WIDTH);
		} else if (isKeyDown(VK_LEFT)) {
			paddleX = max(paddleX - 8, 0);
		}

		drawBlocks();

		ballX += ballDx;
		ballY += ballDy;

		if (ballX > (SCREEN_WIDTH - BALL_SIZE) || ballX < 0) {
			ballDx = -ballDx;
			ballX += ballDx;
		}

		if (ballY < 0) {
			ballDy = -ballDy;
			ballY += ballDy;
		} else if (ballY > (SCREEN_HEIGHT - BALL_SIZE)) {
			ballDy = -ballDy;
			ballY += ballDy;
			score -= 100;
		}

		if (ballDx > 8) { ballDx = 8; }
		else if (ballDx < -8) { ballDx = -8; }

		processBall();

		// draw paddle shadow
		drawRectangle(paddleX - 8, paddleY + 8, paddleX - 8 + PADDLE_WIDTH, paddleY + 8 + PADDLE_HEIGHT, 0, lpddsBack);
		// draw paddle
		drawRectangle(paddleX, paddleY, paddleX + PADDLE_WIDTH, paddleY + PADDLE_HEIGHT, PADDLE_COLOR, lpddsBack);
		// draw ball shadow
		drawRectangle(ballX - 4, ballY + 4, ballX - 4 + BALL_SIZE, ballY + 4 + BALL_SIZE, 0, lpddsBack);
		// draw ball
		drawRectangle(ballX, ballY, ballX + BALL_SIZE, ballY + BALL_SIZE, 255, lpddsBack);
		// draw info
		wchar_t buff[80]; 
		wsprintf(buff, L"F R E A K O U T           Score %d             Level %d", score, level);
		drawTextGDI(buff, 8, SCREEN_HEIGHT - 16, 127, lpddsBack);

		ddFlip();

		//Sleep(100);

		if (isKeyDown(VK_ESCAPE)) {
			PostMessage(hWndMainWindow, WM_DESTROY, 0, 0);
			gameState = GameState::Shutdown;
		}

		break;
	}

	case GameState::Shutdown:
	{
		ddShutdown();
		break;
	}

	case GameState::Exit:
	default:
		break;
	}

	return 1;
}




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS wndClass;
	wndClass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = WINDOW_CLASS_NAME;
	if (!RegisterClass(&wndClass)) {
		return 0;
	}

	HWND hWnd = CreateWindowExW(0, WINDOW_CLASS_NAME, WINDOW_TITLE, WS_POPUP | WS_VISIBLE,
								0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
								nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) { return 0; }

	ShowCursor(FALSE);

	hWndMainWindow = hWnd;
	hInstanceMain = hInstance;

	gameInit();

	MSG msg;
	while (1) {
		if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) { break; }

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		gameMain();
	}

	gameShutdown();
	ShowCursor(TRUE);

	return msg.wParam;
}
