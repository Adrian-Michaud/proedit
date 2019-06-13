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

#define PROEDIT_DISPLAY_CLASS _T("ProeditDisplayWindow")

#define MAX(a,b) (((a)>(b))?(a):(b))

LRESULT CALLBACK ProEditDisplayWndProc(HWND, UINT, WPARAM, LPARAM);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
BOOL PeCreateDisplayWindow(PROEDIT_WINDOW*pe)
{
	pe->hWndDisplay = CreateWindow(PROEDIT_DISPLAY_CLASS, _T(""), WS_CHILD |
	    WS_VISIBLE, 0, 0, pe->screen_xd, pe->screen_yd, pe->hWndMain, 0, pe->
	    hInst, 0);

	if (!pe->hWndDisplay)
		return  FALSE;

	SetWindowLong(pe->hWndDisplay, 0, (LONG)pe);

	return  TRUE;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeRegisterDisplayClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	//Window class for the ProEdit display window
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = ProEditDisplayWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(void*);
	wcex.hInstance = hInstance;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(NULL, IDC_IBEAM);
	wcex.hbrBackground = (HBRUSH)0;
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = PROEDIT_DISPLAY_CLASS;
	wcex.hIconSm = 0;

	RegisterClassEx(&wcex);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
LRESULT CALLBACK ProEditDisplayWndProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
	char keyState[256];
	PROEDIT_WINDOW*pe;

	pe = (PROEDIT_WINDOW*)GetWindowLong(hWnd, 0);

	switch (message) {
	case WM_NCCREATE :
		return  TRUE;

	case WM_NCDESTROY :
		return  0;

	case WM_SETFOCUS :
		CreateCaret(hWnd, NULL, pe->cursor_sizeX, pe->cursor_sizeY);
		SetCaretPos(pe->cursor_x, pe->cursor_y);
		ShowCaret(hWnd);
		break;

	case PE_SET_CURSOR_POS :
		SetCaretPos(pe->cursor_x, pe->cursor_y);
		return (0);

	case PE_SET_CURSOR_SHOW :
		ShowCaret(hWnd);
		return (0);

	case PE_SET_CURSOR_HIDE :
		HideCaret(hWnd);
		return (0);

	case PE_SET_CURSOR_TYPE :
		DestroyCaret();
		CreateCaret(hWnd, NULL, pe->cursor_sizeX, pe->cursor_sizeY);
		SetCaretPos(pe->cursor_x, pe->cursor_y);
		ShowCaret(hWnd);
		return (0);

	case WM_KEYUP :
	case WM_KEYDOWN :
	case WM_SYSKEYDOWN :
	case WM_SYSKEYUP :
		GetKeyboardState((PBYTE)keyState);
		PeAddInputRecord(message, wParam, lParam, keyState);
		return (0);

	case WM_KILLFOCUS :
		DestroyCaret();
		break;

	case WM_PAINT :
		pe->hdc = BeginPaint(hWnd, &pe->ps);
		SelectObject(pe->hdc, pe->hFont);
		ProeditPaint(pe);
		EndPaint(hWnd, &pe->ps);
		return (0);

		default :
		break;
	}

	return  DefWindowProc(hWnd, message, wParam, lParam);
}


