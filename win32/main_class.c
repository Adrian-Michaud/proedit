/*
 *
 * ProEdit MP Multi-platform Programming Editor
 * Designed/Developed/Produced by Adrian Michaud
 *
 * MIT License
 *
 * Copyright (c) 2019 Adrian Michaud
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE

#include "resource.h"

#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "../osdep.h"
#include "proedit_gui.h"

#define PROEDIT_MAIN_CLASS _T("ProeditMainWindow")
#define PROEDIT_MAIN_TITLE _T("Proedit MP v2.0")

#define PROEDIT_STYLE WS_POPUPWINDOW|WS_CAPTION|WS_CLIPCHILDREN

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
BOOL PeCreateMainWindow(PROEDIT_WINDOW*pe)
{
	RECT rect;

	rect.top = 0;
	rect.left = 0;
	rect.right = (pe->screen_xd - 1);
	rect.bottom = (pe->screen_yd - 1);

	AdjustWindowRect(&rect, PROEDIT_STYLE, FALSE);

	pe->hWndMain = CreateWindow(PROEDIT_MAIN_CLASS, PROEDIT_MAIN_TITLE,
	    PROEDIT_STYLE, CW_USEDEFAULT, CW_USEDEFAULT, (rect.right - rect.left),
	    (rect.bottom - rect.top), NULL, NULL, pe->hInst, NULL);

	if (!pe->hWndMain)
		return  FALSE;

	SetWindowLong(pe->hWndMain, 0, (LONG)pe);

	return  TRUE;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeRegisterMainClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	//Window class for the main application window
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(void*);
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROEDIT_GUI));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = PROEDIT_MAIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassEx(&wcex);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PROEDIT_WINDOW*pe;
	char keyState[256];

	pe = (PROEDIT_WINDOW*)GetWindowLong(hWnd, 0);

	switch (message) {
	case WM_SETFOCUS :
		if (pe)
			SetFocus(pe->hWndDisplay);
		break;

	case PE_SHUTDOWN :
		DestroyWindow(hWnd);
		return (0);

	case WM_CLOSE :
		memset(keyState, 0, sizeof(keyState));
		keyState[VK_MENU] = 0x80;
		PeAddInputRecord(WM_KEYDOWN, 'X', 0, keyState);
		return (0);

	case WM_DESTROY :
		PostQuitMessage(0);
		break;
	}

	return  DefWindowProc(hWnd, message, wParam, lParam);
}


