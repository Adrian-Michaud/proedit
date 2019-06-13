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
#include <stdlib.h>
#include "proedit.h"

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void GotoLine(EDIT_FILE*file)
{
	int len, line, col = 0;

	char search[32];

	strcpy(search, "");
	Input(HISTORY_GOTOLINE, "Goto (Line [,column]):", search, sizeof(search));

	file->paint_flags |= CURSOR_FLAG;

	len = strlen(search);

	if (len) {
		if (strstr(search, ",")) {
			col = atol(strstr(search, ",") + 1);
			*strstr(search, ",") = 0;
		}

		line = atol(search);

		if (!line)
			line = file->cursor.line_number + 1;

		GotoPosition(file, line, col);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void GotoPosition(EDIT_FILE*file, int line, int column)
{
	int i, j;

	if ((line < file->display.line_number + 1) || line > (file->display.
	    line_number + file->display.rows)) {
		if (line) {
			CursorLine(file, line - 1);

			if (file->cursor.line_number == file->display.line_number) {
				for (i = 0; i < (file->display.yd - 2) / 2; i++) {
					if (!CursorUp(file))
						break;
				}

				for (j = 0; j < i; j++)
					if (!CursorDown(file))
						break;
			} else {
				for (i = 0; i < (file->display.yd - 2) / 2; i++) {
					if (file->cursor.line_number + 1 >= file->number_lines)
						break;
					if (!CursorDown(file))
						break;
				}

				for (j = 0; j < i; j++)
					if (!CursorUp(file))
						break;
			}
		}
	} else {
		if (line)
			CursorLine(file, line - 1);
	}

	if (column) {
		CursorHome(file);
		CursorOffset(file, column - 1);
	}

	file->paint_flags |= (CURSOR_FLAG | FRAME_FLAG | CONTENT_FLAG);
}


