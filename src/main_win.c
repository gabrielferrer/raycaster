#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wingdi.h>
#include "defs.h"
#include "state.h"
#include "render.h"
#include "main.h"

const char* WINCLASSNAME = "RaycasterClass";
const char* WINDOWNAME = "Raycaster";

HINSTANCE HInst;
HWND Hwnd;
char* Scene;
BITMAPINFO* BI;
HBITMAP DIBSection;

void M_Draw () {
	HDC hdc, chdc;
	HBITMAP hbmp;

#ifdef DEBUG
	R_RendTextures ();
#else
	R_Rend ();
#endif
	if ((hdc = GetDC (Hwnd)) == NULL) {
		return;
	}

	if ((chdc = CreateCompatibleDC (hdc)) == NULL) {
		return;
	}

	if ((hbmp = SelectObject (chdc, DIBSection)) == NULL) {
		return;
	}

	if (!BitBlt (hdc, 0, 0, WINDOWWIDTH, WINDOWHEIGHT, chdc, 0, 0, SRCCOPY)) {
		return;
	}

	ReleaseDC (Hwnd, hdc);
	SelectObject (chdc, hbmp);
	DeleteDC (chdc);
}

LRESULT CALLBACK M_WProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_LEFT:
					ST_CalcDirection (-1.0);
					break;
				case VK_RIGHT:
					ST_CalcDirection (1.0);
					break;
				case VK_UP:
					ST_CalcPosition (1.0);
					break;
				case VK_DOWN:
					ST_CalcPosition (-1.0);
					break;
			}

			return 0;

		case WM_DESTROY:
			PostQuitMessage (0);
			return 0;
	}

	return DefWindowProc (hwnd, uMsg, wParam, lParam);
}

bool_t M_Init (HINSTANCE hInst) {
	HDC hdc;
	WNDCLASS wc;
	RECT r;
	DWORD wStyle;

	HInst = hInst;

	wc.style = 0;
	wc.lpfnWndProc = (void*) &M_WProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = HInst;
	wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor (NULL, IDC_NO);
	wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINCLASSNAME;

	if (!RegisterClass (&wc)) {
		return false;
	}

	wStyle = WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
	r.left = 0;
	r.top = 0;
	r.right = WINDOWWIDTH;
	r.bottom = WINDOWHEIGHT;

	AdjustWindowRect (&r, wStyle, FALSE);

	if ((Hwnd = CreateWindow(WINCLASSNAME, WINDOWNAME, wStyle, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
			NULL, NULL, HInst, NULL)) == NULL) {
		return false;
	}

	UpdateWindow (Hwnd);
	ShowWindow (Hwnd, SW_SHOW);

	if ((hdc = GetDC (Hwnd)) == NULL) {
		return false;
	}

	if ((BI = (BITMAPINFO*) malloc (sizeof (BITMAPINFO))) == NULL) {
		return false;
	}

	BI->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	BI->bmiHeader.biWidth = WINDOWWIDTH;
	BI->bmiHeader.biHeight = -WINDOWHEIGHT;
	BI->bmiHeader.biPlanes = 1;
	BI->bmiHeader.biBitCount = 32;
	BI->bmiHeader.biCompression = BI_RGB;
	BI->bmiHeader.biSizeImage = 0;
	BI->bmiHeader.biXPelsPerMeter = 0;
	BI->bmiHeader.biYPelsPerMeter = 0;
	BI->bmiHeader.biClrUsed = 0;
	BI->bmiHeader.biClrImportant = 0;
	BI->bmiColors[0] = (RGBQUAD) {0,0,0,0};

	if ((DIBSection = CreateDIBSection (hdc, BI, DIB_RGB_COLORS, (void*) &Scene, NULL, 0)) == NULL) {
		return false;
	}

	int bpl = (BI->bmiHeader.biBitCount >> 3) * WINDOWWIDTH;
	memset (Scene, 0, bpl * WINDOWHEIGHT);

	ST_Init ();
	ST_SetWindowWidth (WINDOWWIDTH);
	ST_SetWindowHeight (WINDOWHEIGHT);
	ST_SetImageBpl (bpl);

	/* Pass image data pointer to render */
	R_Init (Scene);
	ReleaseDC (Hwnd, hdc);

	return true;
}

void M_Terminate () {
	UnregisterClass (WINCLASSNAME, HInst);

	if (DIBSection) {
		DeleteObject (DIBSection);
	}
}

int M_MainLoop () {
	MSG msg;
	bool_t exit;
	long prevTick, tick, elapsed;

	exit = false;
	while (!exit) {
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				exit = true;
			} else {
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}

		M_Draw ();
		tick = GetTickCount ();

		/* Manage wraparound */
		if (prevTick > tick) {
			elapsed = 0xffffffff - prevTick+tick;
		} else {
			elapsed = tick - prevTick;
		}

		prevTick = tick;
	}

	return msg.wParam;
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) {
	if (!M_Init (hThisInstance)) {
		return -1;
	}

	M_MainLoop ();
	M_Terminate ();

	return 0;
}
