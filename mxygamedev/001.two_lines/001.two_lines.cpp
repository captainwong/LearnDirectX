
#define USE_LATEST_D3D 0

#if USE_LATEST_D3D
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")



#else
#include <d3d9.h>

#pragma comment(lib, "d3d9.lib")
//#pragma comment(lib, "d3dx9.lib")


LPDIRECT3D9 d3d = nullptr;
LPDIRECT3DDEVICE9 device = nullptr;
LPDIRECT3DVERTEXBUFFER9 vertexBuff = nullptr;

struct MyVertex {
	float x, y, z, rhw;
	unsigned long color;
};

// flexible vertex format
#define MY_D3DFVF_VERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

#endif 

constexpr const auto WINDOW_CLASS = L"LEARN_DX";
constexpr const auto WINDOW_TITLE = L"LearnDX - 001.Two Lines";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;


bool initObjects()
{
	unsigned long color = D3DCOLOR_XRGB(255, 255, 255);
	MyVertex objs[] = {
		{WINDOW_WIDTH / 3.f, WINDOW_HEIGHT / 3.f, 0.5f, 1.f, color},
		{WINDOW_WIDTH / 3.f, WINDOW_HEIGHT * 2.f / 3.f, 0.5f, 1.f, color},
		{WINDOW_WIDTH * 2.f / 3.f, WINDOW_HEIGHT / 3.f, 0.5f, 1.f, color},
		{WINDOW_WIDTH * 2.f / 3.f, WINDOW_HEIGHT * 2 / 3.f, 0.5f, 1.f, color},
	};
	if (FAILED(device->CreateVertexBuffer(sizeof(objs), 0, MY_D3DFVF_VERTEX, D3DPOOL_DEFAULT, &vertexBuff, nullptr))) {
		return false;
	}
	void* ptr = nullptr;
	if (FAILED(vertexBuff->Lock(0, sizeof(objs), (void**)&ptr, 0))) {
		return false;
	}
	memcpy(ptr, objs, sizeof(objs));
	vertexBuff->Unlock();
	return true;
}

bool initD3D(HWND hwnd, bool fullScreen)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d) { return false; }

	D3DDISPLAYMODE dm;
	if (FAILED(d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm))) {
		return false;
	}

	D3DPRESENT_PARAMETERS pp = { 0 };
	if (fullScreen) {
		pp.Windowed = FALSE;
		pp.BackBufferWidth = WINDOW_WIDTH;
		pp.BackBufferHeight = WINDOW_HEIGHT;
	} else {
		pp.Windowed = TRUE;
	}

	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.BackBufferFormat = dm.Format;
	if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device))) {
		return false;
	}

	return initObjects();
}


void render()
{
	// clear back buffer
	device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	device->BeginScene();

	device->SetStreamSource(0, vertexBuff, 0, sizeof(MyVertex));
	device->SetFVF(MY_D3DFVF_VERTEX);
	device->DrawPrimitive(D3DPT_LINELIST, 0, 2);

	device->EndScene();
	device->Present(nullptr, nullptr, nullptr, nullptr);
}

void releaseD3D()
{
	if (device) {
		device->Release();
		device = nullptr;
	}

	if (d3d) {
		d3d->Release();
		d3d = nullptr;
	}

	if (vertexBuff) {
		vertexBuff->Release();
		vertexBuff = nullptr;
	}
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_KEYUP:
		if (wparam == VK_ESCAPE) {
			PostQuitMessage(0);
		}
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE prevhInst, LPSTR cmdLine, int show)
{
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), // UINT        cbSize;
		CS_CLASSDC,			// UINT        style;
		WndProc,			// WNDPROC     lpfnWndProc;
		0,					// int         cbClsExtra;
		0,					// int         cbWndExtra;
		hInst,				// HINSTANCE   hInstance;
		nullptr,			// HICON       hIcon;
		LoadCursor(nullptr, IDC_ARROW),			// HCURSOR     hCursor;
		nullptr,			// HBRUSH      hbrBackground;
		nullptr,			// LPCWSTR     lpszMenuName;
		WINDOW_CLASS,		// LPCWSTR     lpszClassName;
		nullptr				// HICON       hIconSm;
	};

	if (!RegisterClassEx(&wc)) {
		return 1;
	}

	HWND hwnd = CreateWindow(WINDOW_CLASS, WINDOW_TITLE,
							 WS_OVERLAPPEDWINDOW,
							 CW_USEDEFAULT, CW_USEDEFAULT,
							 WINDOW_WIDTH, WINDOW_HEIGHT,
							 GetDesktopWindow(), nullptr, hInst, nullptr);
	if (!hwnd) { return 1; }
	if (!initD3D(hwnd, false)) {
		return 1;
	}

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	MSG msg;
	while (1) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		} else {
			render();
		}
	}

	releaseD3D();
	UnregisterClass(WINDOW_CLASS, hInst);
	return 0;
}
