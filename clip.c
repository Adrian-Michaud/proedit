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
#include "proedit.h"

EDIT_CLIPBOARD global_clipboard;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_CLIPBOARD*GetClipboard(void)
{
	return (&global_clipboard);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CopyFileClipboard(EDIT_FILE*file, EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*lines;

	ReleaseClipboard(clipboard);

	lines = file->lines;

	while (lines) {
		AddClipboardLine(clipboard, lines->line, lines->len, COPY_LINE);
		lines = lines->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ReleaseClipboard(EDIT_CLIPBOARD*clipboard)
{
	DeallocLines(clipboard->lines);

	clipboard->status = 0;
	clipboard->lines = 0;
	clipboard->number_lines = 0;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InitClipboard(EDIT_CLIPBOARD*clipboard)
{
	memset(clipboard, 0, sizeof(EDIT_CLIPBOARD));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void AddGlobalClipboard(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int i, len = 0, index = 0;
	char*string;

	walk = clipboard->lines;

	while (walk) {
		len += (walk->len + 2);
		walk = walk->next;
	}

	string = OS_Malloc(len + 1);

	walk = clipboard->lines;

	while (walk) {
		for (i = 0; i < walk->len; i++) {
			if (walk->line[i] != ED_KEY_TABPAD)
				string[index++] = walk->line[i];
		}
		if (walk->next) {
			string[index++] = ED_KEY_CR;
			string[index++] = ED_KEY_LF;
		}

		walk = walk->next;
	}
	string[index++] = 0;

	OS_SetClipboard(string, index);

	OS_Free(string);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void AddClipboardLine(EDIT_CLIPBOARD*clipboard, char*buffer, int len,
    int status)
{
	EDIT_LINE*line, *walk;

	clipboard->status |= status;
	clipboard->number_lines++;

	line = OS_Malloc(sizeof(EDIT_LINE));

	line->len = len;

	if (len) {
		line->line = OS_Malloc(len);
		memcpy(line->line, buffer, len);
	} else
		line->line = 0;

	line->allocSize = len;
	line->prev = clipboard->tail;
	line->next = 0;
	line->flags = 0;

	walk = clipboard->lines;

	if (!walk) {
		clipboard->lines = line;
		clipboard->tail = line;
	} else {
		clipboard->tail->next = line;
		clipboard->tail = line;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DisplayClipboard(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*lines;
	int i;

	lines = clipboard->lines;

	OS_Printf("\n\n\n\n\n\nCLIPBOARD:\n\n\n");
	while (lines) {
		OS_Printf("->");
		for (i = 0; i < lines->len; i++)
			OS_Printf("%c", lines->line[i]);
		OS_Printf("<-\n");
		lines = lines->next;
	}

	OS_Printf("--------------\n");
}


