#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <math.h>
#include <list>
#include <unordered_map>
#include <time.h>
#include "resource.h"


using std::min;
using std::max;

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSImg32.lib")

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 015.Battle Sample";
constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;
constexpr auto SNOW_SIZE = 15;
constexpr auto SNOW_X_SHAKE = 3; // x方向抖动最大距离
constexpr auto SNOW_Y_SPEED = 10; // y方向匀速向下
constexpr auto TRANS_BG_COL = RGB(0, 0, 0);
constexpr auto MSG_COLOR = RGB(205, 255, 205);
constexpr auto MAX_MSG = 8;

struct Snowflake {
	int x = 0;
	int y = 0;
};

struct Character {
	int hp = 0;
	int maxHp = 0;
	int mp = 0;
	int maxMp = 0;
	int level = 0;
	int strength = 0;
	int intelligence = 0;
	int agility = 0;
};

enum class Action {
	Normal,
	Critical,
	Magic,
	Miss,
	Recover,
};

struct BmpRes {
	HBITMAP bmp = nullptr;
	int w = 0;
	int h = 0;
};

struct GameStatus {
	int frame = 0;
	int msgCount = 0;
	wchar_t msg[MAX_MSG][128] = {};
	bool canAttack = false;
	bool gameOver = false;
	Character hero = {};
	Character boss = {};
	Action heroAction = Action::Normal;
	Action bossAction = Action::Normal;
	std::unordered_map<int, BmpRes> bmps = {
		{IDB_VICTORY,			{nullptr, 800, 600}},
		{IDB_GAMEOVER,			{nullptr, 1086, 396}},
		{IDB_HERO,				{nullptr, 360, 360}},
		{IDB_HERO_CRITICAL,		{nullptr, 574, 306}},
		{IDB_HERO_SLASH,		{nullptr, 364, 140}},
		{IDB_HERO_MAGIC,		{nullptr, 364, 140}},
		{IDB_MONSTER,			{nullptr, 360, 360}},
		{IDB_MONSTER_CRITICAL,	{nullptr, 574, 306}},
		{IDB_MONSTER_SLASH,		{nullptr, 234, 188}},
		{IDB_MONSTER_MAGIC,		{nullptr, 387, 254}},
		{IDB_RECOVER,			{nullptr, 150, 150}},
		{IDB_SKILL1,			{nullptr, 50, 50}},
		{IDB_SKILL2,			{nullptr, 50, 50}},
		{IDB_SKILL3,			{nullptr, 50, 50}},
		{IDB_SKILL4,			{nullptr, 50, 50}},
	};

	std::unordered_map<int, RECT> skillsRc = {
		{IDB_SKILL1, {}},
		{IDB_SKILL2, {}},
		{IDB_SKILL3, {}},
		{IDB_SKILL4, {}},
	};

	void loadRes(HINSTANCE hInst) {
		for (auto& bmp : bmps) {
			auto& res = bmp.second;
			res.bmp = (HBITMAP)LoadImageW(hInst, MAKEINTRESOURCEW(bmp.first), IMAGE_BITMAP, res.w, res.h, LR_DEFAULTCOLOR);
		}
	}

	void releaseRes() {
		for (const auto& bmp : bmps) {
			DeleteObject(bmp.second.bmp);			
		}
		bmps.clear();
	}
};


HINSTANCE hInstMain = nullptr;
HDC hdc = nullptr, hdcMem = nullptr, hdcBuf = nullptr;
HBITMAP bg = nullptr, bgEmpty = nullptr, bmpSnow = nullptr;
ULONGLONG prevShow = 0;
RECT rc{ 0 };
std::list<Snowflake> snowflakes = {};
size_t snowCount = 150;
int snowYs = SNOW_Y_SPEED;
int snowXs = 0;
GameStatus game = {};


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
bool gameInit(HWND hwnd);
void gameMain(HWND hwnd);
void gameShutdown(HWND hwnd);
void paintSnow();
void addMsg(const wchar_t* msg);
void paintMsg();
void dieCheck();
void heroAction();
void heroPaint();
void bossAction();
void bossPaint();


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int show)
{
	hInstMain = hInstance;

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
	if (!RegisterClassExW(&wc)) {
		return 1;
	}

	HWND hwnd = CreateWindow(WINDOW_CLASS, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
							 CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);
	if (!hwnd) { return 1; }
	if (!gameInit(hwnd)) { return 1; }

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		} else if (GetTickCount64() - prevShow >= 50) {
			gameMain(hwnd);
		}
	}

	DeleteDC(hdcBuf); hdcBuf = nullptr;
	DeleteDC(hdcMem); hdcMem = nullptr;
	ReleaseDC(hwnd, hdc); hdc = nullptr;
	DeleteObject(bg); bg = nullptr;
	DeleteObject(bgEmpty); bgEmpty = nullptr;
	DeleteObject(bmpSnow); bmpSnow = nullptr;

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
	{
		game.canAttack = true;
		POINT pt{ LOWORD(lp), HIWORD(lp) };
		if (PtInRect(&game.skillsRc[IDB_SKILL1], pt)) {
			game.heroAction = Action::Normal;
		} else if (PtInRect(&game.skillsRc[IDB_SKILL2], pt)) {
			game.heroAction = Action::Magic;
		} else if (PtInRect(&game.skillsRc[IDB_SKILL3], pt)) {
			game.heroAction = Action::Recover;
		} else {
			game.canAttack = false;
		}
		break;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hwnd, message, wp, lp);
}

bool gameInit(HWND hwnd)
{
	srand((unsigned int)time(nullptr));
	PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE1), hInstMain, SND_RESOURCE | SND_ASYNC | SND_LOOP);

	hdc = GetDC(hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	hdcBuf = CreateCompatibleDC(hdc);
	bgEmpty = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
	SelectObject(hdcMem, bgEmpty);
	bg = (HBITMAP)LoadImageW(hInstMain, MAKEINTRESOURCEW(IDB_BG), IMAGE_BITMAP, WINDOW_WIDTH, WINDOW_HEIGHT, LR_DEFAULTCOLOR);
	bmpSnow = (HBITMAP)LoadImageW(hInstMain, MAKEINTRESOURCEW(IDB_SNOW), IMAGE_BITMAP, SNOW_SIZE, SNOW_SIZE, LR_DEFAULTCOLOR);
	game.loadRes(hInstMain);
	GetClientRect(hwnd, &rc);

	game.hero.hp = game.hero.maxHp = 1000;
	game.hero.level = 6;
	game.hero.mp = game.hero.maxMp = 60;
	game.hero.strength = 10;
	game.hero.agility = 20;
	game.hero.intelligence = 20;

	game.boss.hp = game.boss.maxHp = 2000;
	game.boss.level = 10;
	game.boss.strength = 10;
	game.boss.agility = 10;
	game.boss.intelligence = 10;

	HFONT font = CreateFontW(24, 0, 0, 0, 800, 0, 0, 0, GB2312_CHARSET, 0, 0, 0, 0, L"Consolas");
	SelectObject(hdcMem, font);
	SetBkMode(hdcMem, TRANSPARENT);

	gameMain(hwnd);

	return true;
}

void gameMain(HWND hwnd)
{
	wchar_t text[1024];
	swprintf_s(text, L"%s - snowCount: %d, speed: %d %d, Frame:%d",
			   WINDOW_TITLE, snowCount, snowXs, snowYs, game.frame);
	SetWindowTextW(hwnd, text);

	HGDIOBJ old = SelectObject(hdcBuf, bg);
	BitBlt(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcBuf, 0, 0, SRCCOPY);

	if (!game.gameOver) {
		paintSnow();
	}
	paintMsg();

	if (game.boss.hp > 0) {
		// draw monster
		SelectObject(hdcBuf, game.bmps[IDB_MONSTER].bmp);
		TransparentBlt(hdcMem, 0, 0, game.bmps[IDB_MONSTER].w, game.bmps[IDB_MONSTER].h,
					   hdcBuf, 0, 0, game.bmps[IDB_MONSTER].w, game.bmps[IDB_MONSTER].h, TRANS_BG_COL);
		// draw hp
		swprintf_s(text, L"%d/%d", game.boss.hp, game.boss.maxHp);
		SetTextColor(hdcMem, RGB(255, 10, 10));
		TextOutW(hdcMem, game.bmps[IDB_MONSTER].w / 2 - 50, 10 + game.bmps[IDB_MONSTER].h, text, wcslen(text));
	}

	if (game.hero.hp > 0) {
		// draw hero
		SelectObject(hdcBuf, game.bmps[IDB_HERO].bmp);
		TransparentBlt(hdcMem, rc.right - game.bmps[IDB_HERO].w, 0, game.bmps[IDB_HERO].w, game.bmps[IDB_HERO].h,
					   hdcBuf, 0, 0, game.bmps[IDB_HERO].w, game.bmps[IDB_HERO].h, TRANS_BG_COL);
		// draw hp
		swprintf_s(text, L"%d/%d", game.hero.hp, game.hero.maxHp);
		SetTextColor(hdcMem, RGB(255, 10, 10));
		TextOutW(hdcMem, rc.right - game.bmps[IDB_HERO].w / 2 - 50, 10 + game.bmps[IDB_HERO].h, text, wcslen(text));
		// draw mp
		swprintf_s(text, L"%d/%d", game.hero.mp, game.hero.maxMp);
		SetTextColor(hdcMem, RGB(10, 10, 255));
		TextOutW(hdcMem, rc.right - game.bmps[IDB_HERO].w / 2 - 50, 35 + game.bmps[IDB_HERO].h, text, wcslen(text));
	}

	if (game.gameOver) {
		if (game.hero.hp <= 0) { // hero lose
			SelectObject(hdcBuf, game.bmps[IDB_GAMEOVER].bmp);
			BitBlt(hdcMem, (rc.right - game.bmps[IDB_GAMEOVER].w / 2) / 2, (rc.bottom - game.bmps[IDB_GAMEOVER].h) / 2, game.bmps[IDB_GAMEOVER].w / 2, game.bmps[IDB_GAMEOVER].h,
				   hdcBuf, game.bmps[IDB_GAMEOVER].w / 2, 0, SRCAND);
			BitBlt(hdcMem, (rc.right - game.bmps[IDB_GAMEOVER].w / 2) / 2, (rc.bottom - game.bmps[IDB_GAMEOVER].h) / 2, game.bmps[IDB_GAMEOVER].w / 2, game.bmps[IDB_GAMEOVER].h,
				   hdcBuf, 0, 0, SRCPAINT);
		} else { // hero win
			SelectObject(hdcBuf, game.bmps[IDB_VICTORY].bmp);
			TransparentBlt(hdcMem, (rc.right - game.bmps[IDB_VICTORY].w) / 2, (rc.bottom - game.bmps[IDB_VICTORY].h) / 2, game.bmps[IDB_VICTORY].w, game.bmps[IDB_VICTORY].h,
						   hdcBuf, 0, 0, game.bmps[IDB_VICTORY].w, game.bmps[IDB_VICTORY].h, TRANS_BG_COL);
		}
	} else if (!game.canAttack) {
		int x = rc.right - game.bmps[IDB_SKILL4].w - 10;
		int y = 60 + game.bmps[IDB_HERO].h;
		for (auto res : { IDB_SKILL4, IDB_SKILL3, IDB_SKILL2, IDB_SKILL1 }) {
			SelectObject(hdcBuf, game.bmps[res].bmp);
			BitBlt(hdcMem, x, y, game.bmps[res].w, game.bmps[res].h,
				   hdcBuf, 0, 0, SRCCOPY);
			// save skill buttons' rect for mouse click test 
			game.skillsRc[res] = { x, y, x + game.bmps[res].w, y + game.bmps[res].h };
			x -= game.bmps[res].w + 10;
		}
	} else { // 一回合30帧		
		game.frame++;

		// 5~10帧，玩家攻击
		if (5 <= game.frame && game.frame <= 10) {
			if (game.frame == 5) {
				heroAction();
				dieCheck();
			}
			heroPaint();
		} else if (game.frame == 15) {
			bossAction();
		} else if (26 <= game.frame && game.frame <= 30) {
			bossPaint();
		} else if (game.frame >= 30) { // 回合结束
			game.canAttack = false;
			game.frame = 0;

			// 英雄恢复魔法值
			if (!game.gameOver) {
				int n = 2 * (rand() % game.hero.intelligence) + 6;
				game.hero.mp = min(game.hero.maxMp, game.hero.mp + n);
				swprintf_s(text, L"回合结束，英雄自动回复【%d】点魔法值", n);
				addMsg(text);
			}
		}
	}


	BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		   hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdcBuf, old);

	prevShow = GetTickCount64();
}

void gameShutdown(HWND hwnd)
{

}

void addMsg(const wchar_t* msg)
{
	if (game.msgCount < MAX_MSG) {
		wcscpy_s(game.msg[game.msgCount++], msg);
	} else {
		for (int i = 0; i < MAX_MSG - 1; i++) {
			wcscpy_s(game.msg[i], game.msg[i + 1]);
		}
		wcscpy_s(game.msg[MAX_MSG - 1], msg);
	}
}

void paintMsg()
{
	SetTextColor(hdcMem, MSG_COLOR);
	int y = rc.bottom - MAX_MSG * 26 - 20;
	for (int i = 0; i < game.msgCount; i++) {
		TextOutW(hdcMem, 50, y, game.msg[i], wcslen(game.msg[i]));
		y += 25;
	}
}

void dieCheck()
{
	if (game.hero.hp <= 0) {
		game.gameOver = true;
		PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE_FAILURE), hInstMain, SND_ASYNC | SND_RESOURCE);
		addMsg(L"胜败乃兵家常事，大侠请重新来过");
	} else if (game.boss.hp <= 0) {
		game.gameOver = true;
		PlaySoundW(MAKEINTRESOURCEW(IDR_WAVE_VICTORY), hInstMain, SND_ASYNC | SND_RESOURCE);
		addMsg(L"牛逼！奥利给！");
	}
}

void heroAction()
{
	wchar_t msg[1024];
	switch (game.heroAction) {
	case Action::Normal:
	{
		int damage = int(3 * (rand() % game.hero.agility) + game.hero.level * game.hero.strength + 20);
		if (rand() % 3 == 0) {
			game.heroAction = Action::Critical;
			heroAction();
		} else {
			game.boss.hp -= damage;
			swprintf_s(msg, L"玩家使用了普通攻击【无敌斩】，对怪物造成了【%d】点伤害", damage);
			addMsg(msg);
		}
		break;
	}

	case Action::Critical:
	{
		int damage = int(3 * (rand() % game.hero.agility) + game.hero.level * game.hero.strength + 20);
		int times = 5 + rand() % 5;
		damage *= times;
		game.boss.hp -= damage;
		swprintf_s(msg, L"这下流弊了，触发了【恩赐解脱】，%d倍暴击，对怪物造成了【%d】点伤害", times, damage);
		addMsg(msg);
		break;
	}

	case Action::Magic:
	{
		if (game.hero.mp >= 30) {
			game.hero.mp -= 30;
			int damage = 5 * (2 * (rand() % game.hero.agility) + game.hero.level * game.hero.intelligence);
			game.boss.hp -= damage;
			swprintf_s(msg, L"玩家使用了魔法攻击【烈火剑法】，对怪物造成了【%d】点伤害", damage);
			addMsg(msg);
		} else {
			game.heroAction = Action::Miss;
			addMsg(L"魔法值不足，施法失败！。。。");
		}
		break;
	}

	case Action::Recover:
	{
		if (game.hero.mp >= 40) {
			game.hero.mp -= 40;
			int recover = 5 * (5 * (rand() % game.hero.intelligence) + 40);
			game.hero.hp = min(game.hero.maxHp, game.hero.hp + recover);
			swprintf_s(msg, L"玩家使用了【气疗术】，恢复了【%d】点生命值，感觉好多了", recover);
			addMsg(msg);
		} else {
			addMsg(L"魔法值不足，施法失败！。。。");
		}		
		break;
	}
	}
}

void heroPaint()
{
	switch (game.heroAction) {
	case Action::Normal:
		SelectObject(hdcBuf, game.bmps[IDB_HERO_SLASH].bmp);
		TransparentBlt(hdcMem, 50, (game.bmps[IDB_MONSTER].h - game.bmps[IDB_HERO_SLASH].h) / 2, game.bmps[IDB_HERO_SLASH].w, game.bmps[IDB_HERO_SLASH].h,
					   hdcBuf, 0, 0, game.bmps[IDB_HERO_SLASH].w, game.bmps[IDB_HERO_SLASH].h, TRANS_BG_COL);
		break;
	case Action::Critical:
		SelectObject(hdcBuf, game.bmps[IDB_HERO_CRITICAL].bmp);
		TransparentBlt(hdcMem, 50, (game.bmps[IDB_MONSTER].h - game.bmps[IDB_HERO_CRITICAL].h) / 2, game.bmps[IDB_HERO_CRITICAL].w, game.bmps[IDB_HERO_CRITICAL].h,
					   hdcBuf, 0, 0, game.bmps[IDB_HERO_CRITICAL].w, game.bmps[IDB_HERO_CRITICAL].h, TRANS_BG_COL);
		break;
	case Action::Magic:
		SelectObject(hdcBuf, game.bmps[IDB_HERO_MAGIC].bmp);
		TransparentBlt(hdcMem, 50, (game.bmps[IDB_MONSTER].h - game.bmps[IDB_HERO_MAGIC].h) / 2, game.bmps[IDB_HERO_MAGIC].w, game.bmps[IDB_HERO_MAGIC].h,
					   hdcBuf, 0, 0, game.bmps[IDB_HERO_MAGIC].w, game.bmps[IDB_HERO_MAGIC].h, TRANS_BG_COL);
		break;
	case Action::Miss:
		break;
	case Action::Recover:
		SelectObject(hdcBuf, game.bmps[IDB_RECOVER].bmp);
		TransparentBlt(hdcMem, rc.right - (game.bmps[IDB_HERO].w - game.bmps[IDB_RECOVER].w) / 2 - game.bmps[IDB_RECOVER].w,
								(game.bmps[IDB_HERO].h - game.bmps[IDB_RECOVER].h) / 2, 
								game.bmps[IDB_RECOVER].w, game.bmps[IDB_RECOVER].h,
					   hdcBuf, 0, 0, game.bmps[IDB_RECOVER].w, game.bmps[IDB_RECOVER].h, TRANS_BG_COL);
		break;
	default:
		break;
	}
}

void bossAction()
{
	if (game.boss.hp > game.boss.maxHp / 2) {
		switch (rand() % 3) {
		case 0: game.bossAction = Action::Normal; break;
		case 1: game.bossAction = Action::Critical; break;
		case 2: game.bossAction = Action::Magic; break;
		}
	} else {
		switch (rand() % 3) {
		case 0: game.bossAction = Action::Magic; break;
		case 1: game.bossAction = Action::Critical; break;
		case 2: game.bossAction = Action::Recover; break;
		}
	}
}

void bossPaint()
{
	wchar_t msg[1024];
	switch (game.bossAction) {
	case Action::Normal:
		SelectObject(hdcBuf, game.bmps[IDB_MONSTER_SLASH].bmp);
		TransparentBlt(hdcMem, rc.right - (game.bmps[IDB_HERO].w - game.bmps[IDB_MONSTER_SLASH].w) / 2 - game.bmps[IDB_MONSTER_SLASH].w,
								(game.bmps[IDB_HERO].h - game.bmps[IDB_MONSTER_SLASH].h) / 2, 
								game.bmps[IDB_MONSTER_SLASH].w, game.bmps[IDB_MONSTER_SLASH].h,
					   hdcBuf, 0, 0, game.bmps[IDB_MONSTER_SLASH].w, game.bmps[IDB_MONSTER_SLASH].h, TRANS_BG_COL);
		if (game.frame == 30) {
			int damage = rand() % game.boss.agility + game.boss.level * game.boss.strength;
			game.hero.hp -= damage;
			swprintf_s(msg, L"黄金魔龙君释放了幽冥鬼火，对玩家照成了【%d】点伤害", damage);
			addMsg(msg);
			dieCheck();
		}
		break;
	case Action::Critical:
		SelectObject(hdcBuf, game.bmps[IDB_MONSTER_CRITICAL].bmp);
		TransparentBlt(hdcMem, rc.right - (game.bmps[IDB_HERO].w - game.bmps[IDB_MONSTER_CRITICAL].w) / 2 - game.bmps[IDB_MONSTER_CRITICAL].w,
					   (game.bmps[IDB_HERO].h - game.bmps[IDB_MONSTER_CRITICAL].h) / 2,
					   game.bmps[IDB_MONSTER_CRITICAL].w, game.bmps[IDB_MONSTER_CRITICAL].h,
					   hdcBuf, 0, 0, game.bmps[IDB_MONSTER_CRITICAL].w, game.bmps[IDB_MONSTER_CRITICAL].h, TRANS_BG_COL);
		if (game.frame == 30) {
			int damage = 2 * (rand() % game.boss.agility + game.boss.level * game.boss.strength);
			game.hero.hp -= damage;
			swprintf_s(msg, L"黄金魔龙君触发了致命一击，对玩家照成了【%d】点伤害", damage);
			addMsg(msg);
			dieCheck();
		}
		break;
	case Action::Magic:
		SelectObject(hdcBuf, game.bmps[IDB_MONSTER_MAGIC].bmp);
		TransparentBlt(hdcMem, rc.right - (game.bmps[IDB_HERO].w - game.bmps[IDB_MONSTER_MAGIC].w) / 2 - game.bmps[IDB_MONSTER_MAGIC].w,
					   (game.bmps[IDB_HERO].h - game.bmps[IDB_MONSTER_MAGIC].h) / 2,
					   game.bmps[IDB_MONSTER_MAGIC].w, game.bmps[IDB_MONSTER_MAGIC].h,
					   hdcBuf, 0, 0, game.bmps[IDB_MONSTER_MAGIC].w, game.bmps[IDB_MONSTER_MAGIC].h, TRANS_BG_COL);
		if (game.frame == 30) {
			int damage = 2 * (2 * rand() % game.boss.agility + game.boss.strength * game.boss.intelligence);
			game.hero.hp -= damage;
			int recover = int(damage * 0.2f);
			game.boss.hp += recover;
			swprintf_s(msg, L"黄金魔龙君释放了嗜血咒，对玩家照成了【%d】点伤害，自身恢复了【%d】点生命值", damage, recover);
			addMsg(msg);
			dieCheck();
		}
		break;
	case Action::Miss:
		break;
	case Action::Recover:
		SelectObject(hdcBuf, game.bmps[IDB_RECOVER].bmp);
		TransparentBlt(hdcMem, 50, (game.bmps[IDB_MONSTER].h - game.bmps[IDB_RECOVER].h) / 2, game.bmps[IDB_RECOVER].w, game.bmps[IDB_RECOVER].h,
					   hdcBuf, 0, 0, game.bmps[IDB_RECOVER].w, game.bmps[IDB_RECOVER].h, TRANS_BG_COL); 
		if (game.frame == 30) {
			int recover = 2 * game.boss.intelligence * game.boss.intelligence;
			game.boss.hp = min(game.boss.maxHp, game.boss.hp + recover);
			swprintf_s(msg, L"黄金魔龙君释放了梅肯斯姆，恢复了【%d】点生命值", recover);
			addMsg(msg);
			dieCheck();
		}
		break;
	default:
		break;
	}
}

void paintSnow()
{
	if (snowflakes.size() < snowCount) {
		snowflakes.emplace_back(Snowflake{ rand() % rc.right, rc.top });
	}

	SelectObject(hdcBuf, bmpSnow);
	for (auto iter = snowflakes.begin(); iter != snowflakes.end(); ) {
		auto& snowflake = *iter;
		TransparentBlt(hdcMem, snowflake.x, snowflake.y, SNOW_SIZE, SNOW_SIZE,
					   hdcBuf, 0, 0, SNOW_SIZE, SNOW_SIZE, TRANS_BG_COL);

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
}
