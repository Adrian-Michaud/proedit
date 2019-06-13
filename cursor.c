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

static int CursorSymbol(char ch);
static int DisplayLimit(EDIT_FILE*file);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InitCursorFile(EDIT_FILE*file)
{
	file->cursor.line = file->lines;
	file->cursor.line_number = 0;
	file->overstrike = GetConfigInt(CONFIG_INT_OVERSTRIKE);

	if (!(file->file_flags&FILE_FLAG_NONFILE)) {
		InitFileIndenting(file);
	}

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorLeft(EDIT_FILE*file)
{
	if (file->cursor.xpos == 0) {
		if (file->display.pan) {
			file->display.pan--;
			file->cursor.offset--;

			AdjustCursorTab(file, ADJ_CURSOR_LEFT);

			file->paint_flags |= CONTENT_FLAG;
			return (1);
		}
		return (0);
	}

	file->cursor.xpos--;
	file->cursor.offset--;

	AdjustCursorTab(file, ADJ_CURSOR_LEFT);

	file->paint_flags |= CURSOR_FLAG;
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorRight(EDIT_FILE*file)
{
	if (file->cursor.xpos == file->display.columns - 1) {
		file->display.pan++;
		file->cursor.offset++;

		AdjustCursorTab(file, ADJ_CURSOR_RIGHT);

		file->paint_flags |= CONTENT_FLAG;
		return (1);
	}

	file->cursor.xpos++;
	file->cursor.offset++;

	AdjustCursorTab(file, ADJ_CURSOR_RIGHT);

	file->paint_flags |= CURSOR_FLAG;
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int CursorSymbol(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (1);
	if (ch >= 'A' && ch <= 'Z')
		return (1);

	if (ch == '_')
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorLeftText(EDIT_FILE*file)
{
	if (file->cursor.offset > file->cursor.line->len)
		CursorEnd(file);


	while (!CursorLeft(file)) {
		if (!CursorUp(file))
			return (0);
		CursorEnd(file);
	}

	while (!CursorSymbol(file->cursor.line->line[file->cursor.offset])) {
		if (!CursorLeft(file)) {
			for (; ; ) {
				if (!CursorUp(file))
					return (0);
				CursorEnd(file);
				if (CursorLeft(file))
					break;
			}
		}
	}

	while (file->cursor.offset && CursorSymbol(file->cursor.line->line[file->
	    cursor.offset])) {
		if (!CursorSymbol(file->cursor.line->line[file->cursor.offset - 1]))
			break;

		if (!CursorLeft(file))
			break;
	}

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorRightText(EDIT_FILE*file)
{
	int newline = 0;

	while (file->cursor.offset >= file->cursor.line->len) {
		if (!CursorDown(file))
			return (0);
		CursorHome(file);
		newline = 1;
	}

	if (!newline) {
		while (file->cursor.offset < file->cursor.line->len &&
		    CursorSymbol(file->cursor.line->line[file->cursor.offset])) {
			if (!CursorSymbol(file->cursor.line->line[file->cursor.offset]))
				break;

			if (!CursorRight(file))
				break;
		}
	}

	while (file->cursor.offset < file->cursor.line->len && !CursorSymbol(file->
	    cursor.line->line[file->cursor.offset])) {
		if (!CursorRight(file))
			break;

		while (file->cursor.offset >= file->cursor.line->len) {
			if (!CursorDown(file))
				return (0);
			CursorHome(file);
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
int CursorUp(EDIT_FILE*file)
{
	if (!file->cursor.line->prev)
		return (0);

	if (file->cursor.ypos == 0) {
		if (ScrollUp(file)) {
			CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
			file->cursor.line_number--;
			file->cursor.line = file->cursor.line->prev;

			AdjustCursorTab(file, ADJ_CURSOR_LEFT);
			CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS,
			    0);

			file->paint_flags |= (CONTENT_FLAG | FRAME_FLAG);
			return (1);
		}
		return (0);
	}

	CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
	file->cursor.ypos--;
	file->cursor.line_number--;
	file->cursor.line = file->cursor.line->prev;

	AdjustCursorTab(file, ADJ_CURSOR_LEFT);
	CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS, 0);

	file->paint_flags |= (CURSOR_FLAG | FRAME_FLAG);
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorDown(EDIT_FILE*file)
{
	if (!file->cursor.line->next)
		return (0);

	if (file->cursor.ypos == file->display.rows - 1) {
		if (ScrollDown(file)) {
			CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
			file->cursor.line_number++;
			file->cursor.line = file->cursor.line->next;

			AdjustCursorTab(file, ADJ_CURSOR_LEFT);
			CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS,
			    0);

			file->paint_flags |= (CONTENT_FLAG | FRAME_FLAG);
			return (1);
		}
		return (0);
	}

	CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
	file->cursor.ypos++;
	file->cursor.line_number++;
	file->cursor.line = file->cursor.line->next;

	AdjustCursorTab(file, ADJ_CURSOR_LEFT);
	CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS, 0);

	file->paint_flags |= (CURSOR_FLAG | FRAME_FLAG);
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SaveCursor(EDIT_FILE*file, CURSOR_SAVE*saved)
{
	saved->line = file->cursor.line_number;
	saved->offset = file->cursor.offset;
	saved->xpos = file->cursor.xpos;
	saved->ypos = file->cursor.ypos;
	saved->pan = file->display.pan;
	saved->displayLine = file->display.line_number;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int TestCursor(EDIT_FILE*file, CURSOR_SAVE*saved)
{
	if (file->hexMode) {
		if (saved->line > file->number_lines)
			return (0);
	} else {
		if (saved->line > file->number_lines)
			return (0);
	}

	if (saved->displayLine > file->number_lines - 1)
		return (0);

	if (saved->ypos >= file->display.rows)
		return (0);

	if (saved->xpos >= file->display.columns)
		return (0);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetCursor(EDIT_FILE*file, CURSOR_SAVE*saved)
{
	CursorLine(file, saved->line);

	file->cursor.offset = saved->offset;
	file->cursor.xpos = saved->xpos;
	file->cursor.ypos = saved->ypos;

	DisplayLine(file, saved->displayLine);

	file->display.pan = saved->pan;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorLine(EDIT_FILE*file, int line_number)
{
	if (file->hexMode) {
		file->cursor.line_number = line_number;
		return (1);
	}

	while (line_number > file->cursor.line_number) {
		if (!CursorDown(file))
			return (0);
	}

	while (line_number < file->cursor.line_number) {
		if (!CursorUp(file))
			return (0);
	}
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void AdjustCursorTab(EDIT_FILE*file, int direction)
{
	if (file->cursor.offset < file->cursor.line->len) {
		if (file->cursor.line->line[file->cursor.offset] == ED_KEY_TABPAD) {
			switch (direction) {
			case ADJ_CURSOR_LEFT :
				CursorLeft(file);
				break;

			case ADJ_CURSOR_RIGHT :
				CursorRight(file);
				break;
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
void CursorOffset(EDIT_FILE*file, int offset)
{
	while (file->cursor.offset < offset)
		CursorRight(file);

	while (file->cursor.offset > offset)
		CursorLeft(file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorPgup(EDIT_FILE*file)
{
	int i;

	for (i = 0; i < file->display.rows; i++) {
		if (!file->cursor.line->prev)
			break;

		if (!ScrollUp(file))
			break;

		CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
		file->cursor.line_number--;
		file->cursor.line = file->cursor.line->prev;
		CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS, 0);
	}

	AdjustCursorTab(file, ADJ_CURSOR_LEFT);

	file->paint_flags |= (CONTENT_FLAG | FRAME_FLAG);
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorPgdn(EDIT_FILE*file)
{
	int i;

	for (i = 0; i < file->display.rows; i++) {
		if (DisplayLimit(file))
			break;

		if (!file->cursor.line->next)
			break;

		if (!ScrollDown(file))
			break;

		CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
		file->cursor.line_number++;
		file->cursor.line = file->cursor.line->next;
		CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS, 0);
	}

	AdjustCursorTab(file, ADJ_CURSOR_LEFT);

	file->paint_flags |= (CONTENT_FLAG | FRAME_FLAG);
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CursorHome(EDIT_FILE*file)
{
	if (file->display.pan)
		file->paint_flags |= CONTENT_FLAG;

	file->cursor.xpos = 0;
	file->display.pan = 0;
	file->cursor.offset = 0;

	file->paint_flags |= CURSOR_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CursorTopPage(EDIT_FILE*file)
{
	while (file->cursor.ypos)
		if (!CursorUp(file))
			break;

	CursorHome(file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CursorTopFile(EDIT_FILE*file)
{
	int focusChange = 0;

	if (file->cursor.line != file->lines) {
		CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
		focusChange = 1;
	}

	file->cursor.xpos = 0;
	file->cursor.ypos = 0;
	file->cursor.offset = 0;
	file->cursor.line_number = 0;
	file->cursor.line = file->lines;

	file->display.top_line = file->lines;
	file->display.line_number = 0;
	file->display.pan = 0;

	if (focusChange)
		CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS, 0);

	file->paint_flags |= (CURSOR_FLAG | FRAME_FLAG | CONTENT_FLAG);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CursorEnd(EDIT_FILE*file)
{
	while (file->cursor.offset > file->cursor.line->len)
		if (!CursorLeft(file))
			break;

	while (file->cursor.offset < file->cursor.line->len)
		if (!CursorRight(file))
			break;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CursorEndPage(EDIT_FILE*file)
{
	CursorHome(file);

	while (file->cursor.ypos < file->display.rows - 1)
		if (!CursorDown(file))
			break;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CursorEndFile(EDIT_FILE*file)
{
	CursorHome(file);

	while (CursorDown(file));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void KeyEnd(EDIT_FILE*file)
{
	#if 0
	int indent;
	#endif

	file->end_count++;

	switch (file->end_count) {
	case 1 :
		CursorEnd(file);
		#if 0
		indent = CursorIndentOffset(file);
		/* If the end of the line is shorter than the previous indent */
		/* then move the cursor to the previous indent offset.        */
		while (file->cursor.offset < indent)
			CursorRight(file);
		#endif
		break;
	case 2 :
		CursorEndPage(file);
		break;
	case 3 :
		CursorEndFile(file);
		file->end_count = 0;
		break;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void KeyHome(EDIT_FILE*file)
{

	file->home_count++;

	switch (file->home_count) {
	case 1 :
		CursorHome(file);
		break;
	case 2 :
		CursorTopPage(file);
		break;
	case 3 :
		CursorTopFile(file);
		file->home_count = 0;
		break;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ScrollUp(EDIT_FILE*file)
{
	if (file->display.top_line->prev) {
		file->display.line_number--;
		file->display.top_line = file->display.top_line->prev;
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
int ScrollDown(EDIT_FILE*file)
{
	if (file->display.top_line->next) {
		file->display.top_line = file->display.top_line->next;
		file->display.line_number++;
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
int LineUp(EDIT_FILE*file)
{
	if (!file->cursor.line->prev || !file->display.top_line->prev)
		return (0);

	CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
	file->cursor.line_number--;
	file->cursor.line = file->cursor.line->prev;
	CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS, 0);

	AdjustCursorTab(file, ADJ_CURSOR_LEFT);

	ScrollUp(file);

	file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int LineDown(EDIT_FILE*file)
{
	if (DisplayLimit(file))
		return (0);

	if (!file->cursor.line->next || !file->display.top_line->next)
		return (0);

	CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
	file->cursor.line_number++;
	file->cursor.line = file->cursor.line->next;
	CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS, 0);

	AdjustCursorTab(file, ADJ_CURSOR_LEFT);

	ScrollDown(file);

	file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int DisplayLimit(EDIT_FILE*file)
{
	if (((file->display.line_number + file->display.rows) - 1) >= file->
	    number_lines)
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int DisplayLine(EDIT_FILE*file, int line_number)
{
	if (file->hexMode) {
		file->display.line_number = line_number;
		return (1);
	}

	while (line_number > file->display.line_number) {
		if (!ScrollDown(file))
			return (0);
	}

	while (line_number < file->display.line_number) {
		if (!ScrollUp(file))
			return (0);
	}
	return (1);
}




