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

#ifndef __PROEDIT_GUI_H__
#define __PROEDIT_GUI_H__

typedef struct inputQueue
{
	char keyState[256];
	UINT message;
	WPARAM wParam;
	LPARAM lParam;

	struct inputQueue*next;
	struct inputQueue*prev;
}INPUT_QUEUE;

typedef struct proeditWindow_t
{
	HINSTANCE hInst; // current instance
	HANDLE event;

	INPUT_QUEUE*head;
	INPUT_QUEUE*tail;

	CRITICAL_SECTION cs;

	PAINTSTRUCT ps;
	int cursor_x;
	int cursor_y;
	int cursor_sizeX;
	int cursor_sizeY;
	int cursor_visible;

	HDC hdc;
	char*buffer;
	int screen_xd;
	int screen_yd;
	HFONT hFont;

	HWND hWndMain;
	HWND hWndDisplay;

	int lineNumber;
	int numberLines;
	int column;
	int undo;
}PROEDIT_WINDOW;


void PeAddInputRecord(UINT message, WPARAM wParam, LPARAM lParam,
    char*keyState);

void PeRegisterDisplayClass(HINSTANCE hInstance);
BOOL PeCreateDisplayWindow(PROEDIT_WINDOW*pe);

BOOL PeCreateMainWindow(PROEDIT_WINDOW*pe);
void PeRegisterMainClass(HINSTANCE hInstance);

void PeFlushInputQueue(void);

void ProeditPaint(PROEDIT_WINDOW*pe);

#define PE_BASE             (WM_USER)

#define PE_SET_CURSOR_POS   (PE_BASE+0)
#define PE_SET_CURSOR_TYPE  (PE_BASE+1)
#define PE_SET_CURSOR_HIDE  (PE_BASE+2)
#define PE_SET_CURSOR_SHOW  (PE_BASE+3)
#define PE_SHUTDOWN         (PE_BASE+4)

#endif /* __PROEDIT_GUI_H__ */



