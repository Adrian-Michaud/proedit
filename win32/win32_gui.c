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

#include "resource.h"

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "../osdep.h"
#include "proedit_gui.h"

static int ProcessKey(UINT msg, WPARAM wParam, LPARAM lParam, char*keyState,
    int*mode);
static int ProcessMouse(INPUT_RECORD*in, OS_MOUSE*mouse);
static int ProcessALTKey(int ch);
static int ProcessCTRLKey(int ch);
static int OptimizeScreen(char*new, char*prev, int*y1, int*y2);
static void BuildPalette(void);
static void GetFontSize(HFONT hFont, int*xd, int*yd);
static void OS_Paint_Line(char*buffer, int line);
static void DrawFontString(int x, int y, char*str, int len);

static char*screenBuffer;
static char*original;
static unsigned short cursorX;
static unsigned short cursorY;
static int screenXD;
static int screenYD;
static int displayXD;
static int displayYD;

static int*fontSpacing;
static CLOCK_PFN*clock_pfn;

static char line_text[1024];

static int COLS, LINES;
static int font_size_y, font_size_x, font_descent;

#define FG_COLOR(color) ((color) & 0x0f)
#define BG_COLOR(color) (((color) & 0xf0) >> 4)

#define BACKGROUND_COLOR BG_COLOR(COLOR1)
#define FOREGROUND_COLOR FG_COLOR(COLOR1)

static COLORREF rgb_palette[16];

char*old_buffer = 0;

PROEDIT_WINDOW proeditWindow;


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetTextPosition(int x_position, int y_position)
{
	proeditWindow.cursor_x = (x_position - 1)*font_size_x;
	proeditWindow.cursor_y = ((y_position - 1)*font_size_y) +
	font_size_y - proeditWindow.cursor_sizeY;

	SendMessage(proeditWindow.hWndDisplay, PE_SET_CURSOR_POS, 0, 0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetCursorType(int overstrike, int visible)
{
	if (overstrike) {
		if (proeditWindow.cursor_sizeY != font_size_y) {
			proeditWindow.cursor_sizeY = font_size_y;
			proeditWindow.cursor_sizeX = font_size_x;

			SendMessage(proeditWindow.hWndDisplay, PE_SET_CURSOR_TYPE, 0, 0);
		}
	} else {
		if (proeditWindow.cursor_sizeY != font_size_y / 4) {
			proeditWindow.cursor_sizeY = font_size_y / 4;
			proeditWindow.cursor_sizeX = font_size_x;

			SendMessage(proeditWindow.hWndDisplay, PE_SET_CURSOR_TYPE, 0, 0);
		}
	}

	if (proeditWindow.cursor_visible != visible) {
		if (visible)
			SendMessage(proeditWindow.hWndDisplay, PE_SET_CURSOR_SHOW, 0, 0);
		else
			SendMessage(proeditWindow.hWndDisplay, PE_SET_CURSOR_HIDE, 0, 0);

		proeditWindow.cursor_visible = visible;
	}

}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
int OS_InitScreen(int xd, int yd)
{
	int i;

	#if 1
	proeditWindow.hFont = CreateFont(16, 12, 0, 0, FW_NORMAL, 0, 0, 0,
	    OEM_CHARSET, 0, 0, 0, 0, "Terminal");
	#else

	proeditWindow.hFont = GetStockObject(ANSI_FIXED_FONT);
	//  proeditWindow.hFont = GetStockObject(SYSTEM_FIXED_FONT);
	//  proeditWindow.hFont = GetStockObject(DEVICE_DEFAULT_FONT);
	//  proeditWindow.hFont = GetStockObject(ANSI_FIXED_FONT);
	//"Terminal");
	#endif

	GetFontSize(proeditWindow.hFont, &font_size_x, &font_size_y);

	displayXD = xd;
	displayYD = yd;

	fontSpacing = (int*)malloc(displayXD*sizeof(int));

	for (i = 0; i < displayXD; i++)
		fontSpacing[i] = font_size_x;

	proeditWindow.screen_xd = displayXD*font_size_x;
	proeditWindow.screen_yd = displayYD*font_size_y;

	proeditWindow.cursor_sizeY = font_size_y / 4;
	proeditWindow.cursor_sizeX = font_size_x;

	proeditWindow.cursor_x = 1*font_size_x;
	proeditWindow.cursor_y = (1*font_size_y) +
	font_size_y - proeditWindow.cursor_sizeY;

	COLS = xd;
	LINES = yd;

	BuildPalette();

	return (1);
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
static void BuildPalette(void)
{
	int i, red, green, blue;

	for (i = 0; i < 16; i++) {
		red = green = blue = 0;

		if (i&AWS_FG_R)
			red = 128;
		if (i&AWS_FG_G)
			green = 128;
		if (i&AWS_FG_B)
			blue = 128;

		if (i&AWS_FG_I) {
			if (i == AWS_FG_I) {
				red = 64;
				green = 64;
				blue = 64;
			} else {
				if (red)
					red = 255;
				if (green)
					green = 255;
				if (blue)
					blue = 255;
			}
		}
		rgb_palette[i] = RGB(red, green, blue);
	}
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
void ProeditPaint(PROEDIT_WINDOW*pe)
{
	int lines, top, bottom;

	if (pe->buffer) {
		top = pe->ps.rcPaint.top / font_size_y;
		bottom = pe->ps.rcPaint.bottom / font_size_y;

		if (bottom >= displayYD)
			bottom = displayYD - 1;

		for (lines = top; lines <= bottom; lines++)
			OS_Paint_Line(pe->buffer, lines);
	}
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
void OS_PaintScreen(char*buffer)
{
	RECT rect;

	int newYPos = 12345, newBottom = 56789;

	proeditWindow.buffer = buffer;

	old_buffer = buffer;

	if (OptimizeScreen(buffer, original, &newYPos, &newBottom)) {
		rect.left = 0;
		rect.right = (displayXD*font_size_x) - 1;

		rect.top = newYPos*font_size_y;
		rect.bottom = ((newBottom + 1)*font_size_y);

		if (proeditWindow.hWndDisplay)
			InvalidateRect(proeditWindow.hWndDisplay, &rect, FALSE);
	}
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
void OS_PaintStatus(char*buffer)
{
	RECT rect;

	proeditWindow.buffer = buffer;

	old_buffer = buffer;

	rect.left = 0;
	rect.right = (displayXD*font_size_x) - 1;

	rect.bottom = (displayYD*font_size_y);
	rect.top = (rect.bottom - font_size_y);

	if (proeditWindow.hWndDisplay)
		InvalidateRect(proeditWindow.hWndDisplay, &rect, FALSE);

}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
int OS_ScreenSize(int*xd, int*yd)
{
	*xd = screenXD = COLS;
	*yd = screenYD = LINES;

	original = OS_Malloc(screenXD*screenYD*SCR_PTR_SIZE);
	memset(original, 0, screenXD*screenYD*SCR_PTR_SIZE);

	return (1);
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
static void OS_Paint_Line(char*buffer, int line)
{
	unsigned char attr, prev;
	int x, counter, xPos;
	int draw_x, draw_y;

	/* Update original buffer line for ScreenOptimize routine */
	memcpy(&original[line*screenXD*SCR_PTR_SIZE], &buffer[line*screenXD*
	    SCR_PTR_SIZE], screenXD*SCR_PTR_SIZE);

	prev = buffer[(line*(screenXD*SCR_PTR_SIZE)) + 1];

	SetTextColor(proeditWindow.hdc, rgb_palette[FG_COLOR(prev)]);
	SetBkColor(proeditWindow.hdc, rgb_palette[BG_COLOR(prev)]);

	counter = 0, xPos = 0;

	for (x = 0; x < screenXD; x++) {
		line_text[counter] = buffer[(x*SCR_PTR_SIZE) + (line*(screenXD*
		    SCR_PTR_SIZE))];

		attr = buffer[(x*SCR_PTR_SIZE) + (line*(screenXD*SCR_PTR_SIZE)) + 1];

		if (attr != prev) {
			draw_x = xPos*font_size_x;
			draw_y = (line)*font_size_y;
			DrawFontString(draw_x, draw_y, line_text, counter);
			counter = 0;
			xPos = x;
			prev = attr;
			SetTextColor(proeditWindow.hdc, rgb_palette[FG_COLOR(attr)]);
			SetBkColor(proeditWindow.hdc, rgb_palette[BG_COLOR(attr)]);
			x--;
			continue;
		}
		counter++;
	}

	if (counter) {
		draw_x = xPos*font_size_x;
		draw_y = (line)*font_size_y;
		DrawFontString(draw_x, draw_y, line_text, counter);
	}
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
static void DrawFontString(int x, int y, char*str, int len)
{
	ExtTextOut(proeditWindow.hdc, x, y, ETO_OPAQUE, NULL, str, len,
	    fontSpacing);
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
static void GetFontSize(HFONT hFont, int*xd, int*yd)
{
	TEXTMETRIC fontInfo;
	HDC hdc;
	HANDLE hold;

	hdc = GetDC(0);

	hold = SelectObject(hdc, hFont);

	GetTextMetrics(hdc, &fontInfo);

	#if 0
	printf("  GetFontSize:\n");
	printf("  fontInfo.tmHeight;   = %d\n", fontInfo.tmHeight);
	printf("  fontInfo.tmAscent;   = %d\n", fontInfo.tmAscent);
	printf("  fontInfo.tmDescent;   = %d\n", fontInfo.tmDescent);
	printf("  fontInfo.tmInternalLeading;   = %d\n",
	    fontInfo.tmInternalLeading);
	printf("  fontInfo.tmExternalLeading;   = %d\n",
	    fontInfo.tmExternalLeading);
	printf("  fontInfo.tmAveCharWidth;   = %d\n", fontInfo.tmAveCharWidth);
	printf("  fontInfo.tmMaxCharWidth;   = %d\n", fontInfo.tmMaxCharWidth);
	printf("  fontInfo.tmWeight;   = %d\n", fontInfo.tmWeight);
	printf("  fontInfo.tmOverhang;   = %d\n", fontInfo.tmOverhang);
	printf("  fontInfo.tmDigitizedAspectX;   = %d\n", fontInfo.
	    tmDigitizedAspectX);
	printf("  fontInfo.tmDigitizedAspectY;   = %d\n", fontInfo.
	    tmDigitizedAspectY);
	printf("  fontInfo.tmItalic;   = %d\n", fontInfo.tmItalic);
	printf("  fontInfo.tmUnderlined;   = %d\n", fontInfo.tmUnderlined);
	printf("  fontInfo.tmStruckOut;   = %d\n", fontInfo.tmStruckOut);
	printf("  fontInfo.tmPitchAndFamily;   = %d\n", fontInfo.tmPitchAndFamily);
	printf("  fontInfo.tmCharSet;   = %d\n", fontInfo.tmCharSet);
	#endif

	SelectObject(hdc, hold);
	ReleaseDC(0, hdc);

	font_descent = fontInfo.tmDescent;

	*yd = fontInfo.tmHeight;
	*xd = fontInfo.tmAveCharWidth;
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
static int OptimizeScreen(char*new, char*prev, int*y1, int*y2)
{
	int y;

	for (y = 0; y < screenYD - 1; y++) {
		if (memcmp(&new[y*screenXD*SCR_PTR_SIZE],
		    &prev[y*screenXD*SCR_PTR_SIZE], screenXD*SCR_PTR_SIZE)) {
			*y1 = y;
			break;
		}
	}

	/* There are no differences in the buffer */
	if (y == screenYD - 1)
		return (0);

	for (y = screenYD - 2; y >= *y1; y--) {
		if (memcmp(&new[y*screenXD*SCR_PTR_SIZE],
		    &prev[y*screenXD*SCR_PTR_SIZE], screenXD*SCR_PTR_SIZE)) {
			*y2 = y;
			break;
		}
	}

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeAddInputRecord(UINT message, WPARAM wParam, LPARAM lParam, char*keyState)
{
	INPUT_QUEUE*tail;

	tail = (INPUT_QUEUE*)malloc(sizeof(INPUT_QUEUE));

	if (!tail)
		return ;

	tail->message = message;
	tail->wParam = wParam;
	tail->lParam = lParam;

	memcpy(tail->keyState, keyState, sizeof(tail->keyState));

	EnterCriticalSection(&proeditWindow.cs);

	if (!proeditWindow.head) {
		tail->prev = 0;
		proeditWindow.head = tail;
	} else {
		proeditWindow.tail->next = tail;
		tail->prev = proeditWindow.tail;
	}
	tail->next = 0;

	proeditWindow.tail = tail;

	LeaveCriticalSection(&proeditWindow.cs);

	SetEvent(proeditWindow.event);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeFlushInputQueue(void)
{
	INPUT_QUEUE*head;

	EnterCriticalSection(&proeditWindow.cs);

	while (proeditWindow.head) {
		head = proeditWindow.head;

		proeditWindow.head = head->next;

		if (proeditWindow.head)
			proeditWindow.head->prev = 0;

		free(head);
	}

	LeaveCriticalSection(&proeditWindow.cs);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_GetKey(int*mode, OS_MOUSE*mouse)
{
	INPUT_QUEUE*head;
	UINT msg = 0;
	WPARAM wParam = 0;
	LPARAM lParam = 0;
	DWORD cnt;
	char keyState[256];
	int ch;

	UNREFERENCED_PARAMETER(mouse);
	for (; ; ) {
		EnterCriticalSection(&proeditWindow.cs);

		if (proeditWindow.head) {
			msg = proeditWindow.head->message;
			wParam = proeditWindow.head->wParam;
			lParam = proeditWindow.head->lParam;
			memcpy(keyState, proeditWindow.head->keyState, sizeof(keyState));

			head = proeditWindow.head;

			proeditWindow.head = head->next;

			if (proeditWindow.head)
				proeditWindow.head->prev = 0;

			free(head);
			cnt = 1;
		} else
			cnt = 0;

		LeaveCriticalSection(&proeditWindow.cs);

		if (!cnt) {
			WaitForSingleObject(proeditWindow.event, 1000);
			if (clock_pfn)
				clock_pfn();
			continue;
		}

		switch (msg) {
		case WM_KEYUP :
		case WM_KEYDOWN :
		case WM_SYSKEYDOWN :
		case WM_SYSKEYUP :
			{
				*mode = 0;
				ch = ProcessKey(msg, wParam, lParam, keyState, mode);
				if (ch)
					return (ch);
			}
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_PeekKey(int*mode, OS_MOUSE*mouse)
{
	INPUT_QUEUE*head;
	UINT msg = 0;
	WPARAM wParam = 0;
	LPARAM lParam = 0;
	DWORD cnt;
	char keyState[256];
	int ch = 0;

	UNREFERENCED_PARAMETER(mouse);

	EnterCriticalSection(&proeditWindow.cs);

	if (proeditWindow.head) {
		msg = proeditWindow.head->message;
		wParam = proeditWindow.head->wParam;
		lParam = proeditWindow.head->lParam;
		memcpy(keyState, proeditWindow.head->keyState, sizeof(keyState));

		head = proeditWindow.head;

		proeditWindow.head = head->next;

		if (proeditWindow.head)
			proeditWindow.head->prev = 0;

		free(head);
		cnt = 1;
	} else
		cnt = 0;

	LeaveCriticalSection(&proeditWindow.cs);

	if (cnt) {
		switch (msg) {
		case WM_KEYUP :
		case WM_KEYDOWN :
		case WM_SYSKEYDOWN :
		case WM_SYSKEYUP :
			{
				*mode = 0;
				ch = ProcessKey(msg, wParam, lParam, keyState, mode);
			}
		}
	}
	return (ch);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ProcessKey(UINT msg, WPARAM wParam, LPARAM lParam, char*keyState,
    int*mode)
{
	int alt, shift, ctrl;
	WORD ascii = 0;

	*mode = 1;

	alt = keyState[VK_MENU]&0x80;
	shift = keyState[VK_SHIFT]&0x80;
	ctrl = keyState[VK_CONTROL]&0x80;

	if (wParam == VK_SHIFT) {
		if (shift)
			return (ED_KEY_SHIFT_DOWN);
		else
			return (ED_KEY_SHIFT_UP);
	}

	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
		switch (wParam) {
		case VK_BACK :
		case VK_TAB :
		case VK_RETURN :
		case VK_ESCAPE :
		case VK_SPACE :
			*mode = 0;
			return (wParam);

		case VK_MENU :
		case VK_CONTROL :
		case VK_CAPITAL :
			return (0);

		case VK_F1 :
			return (ED_F1);

		case VK_F2 :
			return (ED_F2);

		case VK_F3 :
			return (ED_F3);

		case VK_F4 :
			return (ED_F4);

		case VK_F5 :
			return (ED_F5);

		case VK_F6 :
			return (ED_F6);

		case VK_F7 :
			return (ED_F7);

		case VK_F8 :
			return (ED_F8);

		case VK_F9 :
			return (ED_F9);

		case VK_F10 :
			return (ED_F10);

		case VK_F11 :
			return (ED_F11);

		case VK_F12 :
			return (ED_F12);

		case VK_HOME :
			return (ED_KEY_HOME);

		case VK_END :
			return (ED_KEY_END);

		case VK_INSERT :
			if (ctrl)
				return (ED_KEY_CTRL_INSERT);
			if (alt)
				return (ED_KEY_ALT_INSERT);
			return (ED_KEY_INSERT);

		case VK_DELETE :
			return (ED_KEY_DELETE);

		case VK_UP :
			if (alt)
				return (ED_KEY_ALT_UP);
			if (ctrl)
				return (ED_KEY_CTRL_UP);
			return (ED_KEY_UP);

		case VK_DOWN :
			if (alt)
				return (ED_KEY_ALT_DOWN);
			if (ctrl)
				return (ED_KEY_CTRL_DOWN);
			return (ED_KEY_DOWN);

		case VK_LEFT :
			if (alt)
				return (ED_KEY_ALT_LEFT);
			if (ctrl)
				return (ED_KEY_CTRL_LEFT);
			return (ED_KEY_LEFT);

		case VK_RIGHT :
			if (alt)
				return (ED_KEY_ALT_RIGHT);
			if (ctrl)
				return (ED_KEY_CTRL_RIGHT);
			return (ED_KEY_RIGHT);

		case VK_PRIOR :
			return (ED_KEY_PGUP);

		case VK_NEXT :
			return (ED_KEY_PGDN);
		}

		if (wParam >= 0x41 && wParam <= 0x5A) {
			if (alt)
				return (ProcessALTKey(wParam));

			if (ctrl)
				return (ProcessCTRLKey(wParam));
		}

		*mode = 0;

		ToAscii(wParam, (lParam >> 16)&0xff, keyState, &ascii, 1);

		return (ascii);
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessALTKey(int ch)
{
	if (ch >= 'a' && ch <= 'z')
		ch -= 32;

	switch (ch) {
	case 'A' :
		return (ED_ALT_A);
	case 'B' :
		return (ED_ALT_B);
	case 'C' :
		return (ED_ALT_C);
	case 'D' :
		return (ED_ALT_D);
	case 'E' :
		return (ED_ALT_E);
	case 'F' :
		return (ED_ALT_F);
	case 'G' :
		return (ED_ALT_G);
	case 'H' :
		return (ED_ALT_H);
	case 'I' :
		return (ED_ALT_I);
	case 'J' :
		return (ED_ALT_J);
	case 'K' :
		return (ED_ALT_K);
	case 'L' :
		return (ED_ALT_L);
	case 'M' :
		return (ED_ALT_M);
	case 'N' :
		return (ED_ALT_N);
	case 'O' :
		return (ED_ALT_O);
	case 'P' :
		return (ED_ALT_P);
	case 'Q' :
		return (ED_ALT_Q);
	case 'R' :
		return (ED_ALT_R);
	case 'S' :
		return (ED_ALT_S);
	case 'T' :
		return (ED_ALT_T);
	case 'U' :
		return (ED_ALT_U);
	case 'V' :
		return (ED_ALT_V);
	case 'W' :
		return (ED_ALT_W);
	case 'X' :
		return (ED_ALT_X);
	case 'Y' :
		return (ED_ALT_Y);
	case 'Z' :
		return (ED_ALT_Z);
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessCTRLKey(int ch)
{
	if (ch >= 'a' && ch <= 'z')
		ch -= 32;

	switch (ch) {
	case 'A' :
		return (ED_CTRL_A);
	case 'B' :
		return (ED_CTRL_B);
	case 'C' :
		return (ED_CTRL_C);
	case 'D' :
		return (ED_CTRL_D);
	case 'E' :
		return (ED_CTRL_E);
	case 'F' :
		return (ED_CTRL_F);
	case 'G' :
		return (ED_CTRL_G);
	case 'H' :
		return (ED_CTRL_H);
	case 'I' :
		return (ED_CTRL_I);
	case 'J' :
		return (ED_CTRL_J);
	case 'K' :
		return (ED_CTRL_K);
	case 'L' :
		return (ED_CTRL_L);
	case 'M' :
		return (ED_CTRL_M);
	case 'N' :
		return (ED_CTRL_N);
	case 'O' :
		return (ED_CTRL_O);
	case 'P' :
		return (ED_CTRL_P);
	case 'Q' :
		return (ED_CTRL_Q);
	case 'R' :
		return (ED_CTRL_R);
	case 'S' :
		return (ED_CTRL_S);
	case 'T' :
		return (ED_CTRL_T);
	case 'U' :
		return (ED_CTRL_U);
	case 'V' :
		return (ED_CTRL_V);
	case 'W' :
		return (ED_CTRL_W);
	case 'X' :
		return (ED_CTRL_X);
	case 'Y' :
		return (ED_CTRL_Y);
	case 'Z' :
		return (ED_CTRL_Z);
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetWindowTitle(char*pathname)
{
	char filename[MAX_PATH];

	OS_GetFilename(pathname, 0, filename);

	SetWindowText(proeditWindow.hWndMain, filename);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char OS_Frame(int index)
{
	static unsigned char titles[] =
	{0xc9, 0xcd, 0xbb, 0xc8, 0xbc, 0xba, 0xb0, 0xb2, 0x18,
	0x19, 0xfe, 0xbf, 0x07};

	return (titles[index]);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Printf(char*fmt, ...)
{
	UNREFERENCED_PARAMETER(fmt);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Exit()
{
	exit(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessMouse(INPUT_RECORD*in, OS_MOUSE*mouse)
{

	mouse->wheel = 0;
	mouse->buttonStatus = 0;
	mouse->moveStatus = 0;

	if (in->Event.MouseEvent.dwEventFlags == MOUSE_MOVED) {
		mouse->moveStatus = 1;
		mouse->xpos = in->Event.MouseEvent.dwMousePosition.X;
		mouse->ypos = in->Event.MouseEvent.dwMousePosition.Y;
		return (1);
	}

	if (in->Event.MouseEvent.dwEventFlags == 0) {
		mouse->buttonStatus = 1;

		if (in->Event.MouseEvent.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)
			mouse->button1 = OS_BUTTON_PRESSED;
		else
			mouse->button1 = OS_BUTTON_RELEASED;

		if (in->Event.MouseEvent.dwButtonState&RIGHTMOST_BUTTON_PRESSED)
			mouse->button2 = OS_BUTTON_PRESSED;
		else
			mouse->button2 = OS_BUTTON_RELEASED;

		if (in->Event.MouseEvent.dwButtonState&4)
			mouse->button3 = OS_BUTTON_PRESSED;
		else
			mouse->button3 = OS_BUTTON_RELEASED;

		return (1);
	}

	if (in->Event.MouseEvent.dwEventFlags == MOUSE_WHEELED) {
		if (in->Event.MouseEvent.dwButtonState&0xFF000000)
			mouse->wheel = OS_WHEEL_DOWN;
		else
			mouse->wheel = OS_WHEEL_UP;
		return (1);
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetClockPfn(CLOCK_PFN*clockPfn)
{
	clock_pfn = clockPfn;
}


