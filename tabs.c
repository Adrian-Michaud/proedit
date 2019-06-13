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

extern int tabsize;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char*TabulateString(char*string, int len, int*newLen)
{
	EDIT_LINE line;

	line.allocSize = TabulateLength(string, 0, len, len) + 1;
	line.line = OS_Malloc(line.allocSize);

	TabulateLine(&line, string, len);

	*newLen = line.len;

	line.line[line.len] = 0;

	return (line.line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int InsertTabulate(int offset, char*buf, int len)
{
	int i, base;

	base = offset;

	for (i = 0; i < len; i++) {
		if (buf[i] == ED_KEY_TABPAD)
			continue;

		if (buf[i] == ED_KEY_TAB)
			offset += (tabsize - (offset%tabsize));
		else
			offset++;
	}

	return (offset - base);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int TabulateLength(char*buf, int index, int len, int max)
{
	int i, offset;

	offset = index;

	for (i = index; i < index + len; i++) {
		if (i >= max)
			break;

		if (buf[i] == ED_KEY_LF || buf[i] == ED_KEY_CR || buf[i] ==
		    ED_KEY_TABPAD)
			continue;

		if (buf[i] == ED_KEY_TAB)
			offset += (tabsize - (offset%tabsize));
		else
			offset++;
	}

	return (offset - index);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void TabulateLine(EDIT_LINE*line, char*buf, int len)
{
	int i, pad, tablen, newlen = 0;
	char*alloced = 0;

	if (!buf) {
		buf = OS_Malloc(line->len);
		len = line->len;
		alloced = buf;

		memcpy(buf, line->line, line->len);
	}

	tablen = TabulateLength(buf, 0, len, len);

	if (tablen > line->allocSize)
		ReallocLine(line, tablen + EXTRA_LINE_PADDING);

	for (i = 0; i < len; i++) {
		if (buf[i] == ED_KEY_LF || buf[i] == ED_KEY_CR || buf[i] ==
		    ED_KEY_TABPAD)
			continue;

		if (buf[i] == ED_KEY_TAB) {
			pad = tabsize - (newlen%tabsize);

			line->line[newlen++] = ED_KEY_TAB;

			pad--;

			while (pad--)
				line->line[newlen++] = ED_KEY_TABPAD;
		} else
			line->line[newlen++] = buf[i];
	}

	if (newlen > tablen) {
		OS_Printf("newlen=%d, tablen=%d\n", newlen, tablen);
		OS_Exit();
	}

	line->len = newlen;

	if (alloced)
		OS_Free(alloced);
}


