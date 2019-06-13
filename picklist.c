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

#include "osdep.h" /* Platform dependent interface */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "proedit.h"


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int PickList(char*windowTitle, int count, int width, char*title, char*list[],
    int start)
{
	EDIT_FILE*file;
	EDIT_LINE*current = 0;
	char*scrBuff;
	int i, x, y, xd, yd, len, ch, mode, pan = 0, line_number = 0, retCode;

	if (OS_PickList(windowTitle, count, width, title, list, start, &retCode))
		return (retCode);

	scrBuff = SaveScreen();

	file = AllocFile(title);

	for (i = 0; i < count; i++)
		current = AddFileLine(file, current, list[i], strlen(list[i]), 0);

	file->file_flags |= FILE_FLAG_NONFILE;

	InitDisplayFile(file);

	if (width > file->display.xd - 4)
		width = file->display.xd - 4;

	for (i = 0; i < count; i++) {
		len = InsertTabulate(0, list[i], strlen(list[i]));
		if (len > width)
			pan = 1;
	}

	xd = width + 2;
	yd = count + 3;

	if (yd > file->display.yd - 4)
		yd = file->display.yd - 4;
	else
		file->scrollbar = 0;

	if (xd > file->display.xd - 2)
		xd = file->display.xd - 2;

	x = file->display.xd / 2 - xd / 2;
	y = (file->display.yd - 1) / 2 - yd / 2;

	file->display.xpos = x;
	file->display.ypos = y;
	file->display.xd = xd;
	file->display.yd = yd;
	file->display.columns = xd - 2;
	file->display.rows = yd - 3;

	file->hideCursor = 1;
	file->client = 2;
	file->border = 2;

	file->display_special = ED_SPECIAL_TAB;

	InitCursorFile(file);

	CursorTopFile(file);

	GotoPosition(file, start, 1);

	for (; ; ) {
		SetupSelectBlock(file, file->cursor.line_number, 0,
		    file->display.pan + file->display.xd);

		Paint(file);

		ch = OS_Key(&mode, &file->mouse);

		if (mode) {
			if (ch == ED_ALT_X)
				break;

			if (ch == ED_KEY_MOUSE) {
				if (file->mouse.wheel == OS_WHEEL_UP)
					ch = ED_KEY_UP;

				if (file->mouse.wheel == OS_WHEEL_DOWN)
					ch = ED_KEY_DOWN;

				if (file->mouse.button1 == OS_BUTTON_PRESSED)
					MouseClick(file);

				if (file->mouse.buttonStatus && file->mouse.button1 ==
				    OS_BUTTON_RELEASED) {
					if (MouseClick(file)) {
						line_number = file->cursor.line_number + 1;
						break;
					}
				}
			}

			switch (ch) {
			case ED_KEY_HOME :
				while (CursorUp(file));
				file->display.pan = 0;
				break;

			case ED_KEY_END :
				while (file->cursor.line_number < file->number_lines - 1)
					CursorDown(file);
				file->display.pan = 0;
				break;

			case ED_KEY_PGDN :
				{
					for (i = 0; i < file->display.yd; i++)
						if (file->cursor.line_number < file->number_lines - 1)
							CursorDown(file);
					file->display.pan = 0;
					break;
				}

			case ED_KEY_PGUP :
				{
					for (i = 0; i < file->display.yd; i++)
						CursorUp(file);
					file->display.pan = 0;
					break;
				}

			case ED_KEY_LEFT :
				{
					if (file->display.pan)
						file->display.pan--;
					break;
				}

			case ED_KEY_RIGHT :
				{
					if (pan)
						file->display.pan++;
					break;
				}

			case ED_KEY_UP :
				{
					CursorUp(file);
					break;
				}

			case ED_KEY_DOWN :
				{
					if (file->cursor.line_number < file->number_lines - 1)
						CursorDown(file);
					break;
				}
			}
		} else {
			if (ch == ED_KEY_ESC)
				break;

			if (ch == ED_KEY_CR) {
				line_number = file->cursor.line_number + 1;
				break;
			}
		}
	}

	DeallocFile(file);
	RestoreScreen(scrBuff);

	return (line_number);
}


