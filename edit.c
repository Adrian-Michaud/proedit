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


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ProcessNormal(EDIT_FILE*file, int ch)
{
	if (file->file_flags&FILE_FLAG_READONLY)
		return ;

	/* Ignore the ESC key. */
	if (ch == ED_KEY_ESC) {
		CancelSelectBlock(file);
		if (file->copyStatus&COPY_ON) {
			file->paint_flags |= CONTENT_FLAG;
			file->copyStatus = 0;
		}
		return ;
	}

	DeleteSelectBlock(file);

	if (ch == ED_KEY_BS) {
		if (CanBackspace(file)) {
			UndoBegin(file);
			ProcessBackspace(file);
		}
		return ;
	}

	UndoBegin(file);

	InsertCharacter(file, ch);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InsertCharacter(EDIT_FILE*file, int ch)
{
	char byte;

	SaveUndo(file, UNDO_CURSOR, 0);

	if (ch == ED_KEY_CR) {
		CarrageReturn(file, CAN_INDENT);
	} else {
		byte = (char)ch;

		CallLineCallbacks(file, file->cursor.line, LINE_OP_INSERT_CHAR, ch);

		if (file->overstrike)
			OverstrikeCharacter(file, byte, CAN_INDENT | CAN_WORDWRAP);
		else
			InsertText(file, &byte, 1, CAN_INDENT | CAN_WORDWRAP);

		CursorRight(file);
	}

	file->paint_flags |= CONTENT_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CarrageReturn(EDIT_FILE*file, int indent)
{
	int len;
	char*line;

	/* Are we just inserting a new line? */
	if (file->cursor.offset == 0) {
		SaveUndo(file, UNDO_INSERT_LINE, 0);
		InsertLine(file, 0, 0, INS_ABOVE_CURSOR, 0);
		CursorDown(file);
		return ;
	}

	SaveUndo(file, UNDO_CARRAGE_RETURN, 0);

	/* Are we breaking this line? */
	if (file->cursor.offset < file->cursor.line->len) {
		len = (file->cursor.line->len) - (file->cursor.offset);
		line = &file->cursor.line->line[file->cursor.offset];

		CutLine(file, file->cursor.offset, NO_UNDO);
		InsertLine(file, line, len, INS_BELOW_CURSOR, 0);
		ReallocLine(file->cursor.line, file->cursor.offset);
	} else
		InsertLine(file, 0, 0, INS_BELOW_CURSOR, 0);

	CursorHome(file);
	CursorDown(file);

	if (indent == CAN_INDENT)
		IndentLine(file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CutLine(EDIT_FILE*file, int offset, int options)
{
	if (offset < file->cursor.line->len) {
		if (!(options&NO_UNDO))
			SaveUndo(file, UNDO_CUTLINE, offset);

		file->cursor.line->len = offset;
		file->paint_flags |= CONTENT_FLAG;

		if (options&CAN_REALLOC)
			ReallocLine(file->cursor.line, offset);

		CallLineCallbacks(file, file->cursor.line, LINE_OP_EDIT, 0);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CallLineCallbacks(EDIT_FILE*file, EDIT_LINE*line, int op, int arg)
{
	LINE_CALLBACK*walk;
	int retCode = 0;

	walk = file->linePfns;

	while (walk) {
		if ((walk->opMask&op)&~file->callbackMask)
			retCode |= walk->pfn(file, line, ((walk->opMask&op)&~file->
			    callbackMask), arg);
		walk = walk->next;
	}
	return (retCode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void LineCallbackMask(EDIT_FILE*file, int mask)
{
	file->callbackMask |= mask;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void LineCallbackUnMask(EDIT_FILE*file, int mask)
{
	file->callbackMask &= ~mask;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void AddLineCallback(EDIT_FILE*file, LINE_PFN*pfn, int opMask)
{
	LINE_CALLBACK*cb;

	cb = file->linePfns;

	while (cb) {
		if (cb->pfn == pfn) {
			cb->opMask |= opMask;
			return ;
		}

		cb = cb->next;
	}

	cb = (LINE_CALLBACK*)OS_Malloc(sizeof(LINE_CALLBACK));

	cb->pfn = pfn;
	cb->opMask = opMask;

	cb->next = file->linePfns;
	cb->prev = 0;

	if (file->linePfns)
		file->linePfns->prev = cb;

	file->linePfns = cb;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void RemoveLineCallback(EDIT_FILE*file, LINE_PFN*pfn)
{
	LINE_CALLBACK*cb;

	cb = file->linePfns;

	while (cb) {
		if (cb->pfn == pfn) {
			if (cb->prev)
				cb->prev->next = cb->next;

			if (cb->next)
				cb->next->prev = cb->prev;

			if (file->linePfns == cb)
				file->linePfns = cb->next;

			OS_Free(cb);
			return ;
		}

		cb = cb->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void RemoveAllCallbacks(EDIT_FILE*file)
{
	while (file->linePfns)
		RemoveLineCallback(file, file->linePfns->pfn);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CanBackspace(EDIT_FILE*file)
{
	if (file->cursor.line == file->lines && file->cursor.offset == 0)
		return (0);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ProcessBackspace(EDIT_FILE*file)
{
	if (file->cursor.offset > file->cursor.line->len) {
		if (!CursorLeft(file)) {
			if (CursorUp(file))
				CursorEnd(file);
		}
		return ;
	}

	SaveUndo(file, UNDO_CURSOR, 0);

	if (!CursorLeft(file)) {
		if (!CursorUp(file))
			return ;

		CursorEnd(file);
	}

	DeleteCharacter(file, CAN_WORDWRAP);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InsertLine(EDIT_FILE*file, char*text, int len, int below, int flags)
{
	EDIT_LINE*newline;

	newline = OS_Malloc(sizeof(EDIT_LINE));

	memset(newline, 0, sizeof(EDIT_LINE));

	newline->flags = flags;

	if (below == INS_BELOW_CURSOR) {
		/* Insert line below the cursor. */
		newline->prev = file->cursor.line;
		newline->next = file->cursor.line->next;

		file->cursor.line->next = newline;

		if (newline->next)
			newline->next->prev = newline;

		if (len)
			TabulateLine(newline, text, len);

		file->number_lines++;

		CallLineCallbacks(file, newline, LINE_OP_INSERT, 0);
		return ;
	}

	CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);
	/* Insert line above the cursor. */
	newline->prev = file->cursor.line->prev;
	newline->next = file->cursor.line;

	file->cursor.line->prev = newline;

	if (newline->prev)
		newline->prev->next = newline;

	if (len)
		TabulateLine(newline, text, len);

	if (file->display.top_line == file->cursor.line)
		file->display.top_line = newline;

	if (file->lines == file->cursor.line)
		file->lines = newline;

	file->cursor.line = newline;

	file->number_lines++;

	CallLineCallbacks(file, newline, LINE_OP_INSERT | LINE_OP_GETTING_FOCUS, 0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OverstrikeCharacter(EDIT_FILE*file, char ch, int options)
{
	if (file->cursor.offset < file->cursor.line->len)
		DeleteText(file, 1, 0);

	InsertText(file, &ch, 1, options);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InsertText(EDIT_FILE*file, char*text, int len, int options)
{
	int allocLen, offset, insertLen;


	if (!(options&NO_UNDO)) {
		insertLen = InsertTabulate(file->cursor.offset, text, len);
		SaveUndo(file, UNDO_INSERT_TEXT, insertLen);
	}

	/* Are we inserting past the end of the line? Calculate new length. */
	if (file->cursor.offset >= file->cursor.line->len)
		allocLen = file->cursor.offset + len;
	else
		allocLen = file->cursor.line->len + len;

	/* Do we need to re-allocate the line? */
	if (allocLen > file->cursor.line->allocSize)
		ReallocLine(file->cursor.line, allocLen + EXTRA_LINE_PADDING);

	/* Are we inserting past the end of the line? Insert padding*/
	if (file->cursor.offset > file->cursor.line->len) {
		offset = (file->cursor.offset - file->cursor.line->len);

		/* Should we try to add indented padding from a previous line instead? */
		if ((options&CAN_INDENT) && file->cursor.line->len == 0) {
			if (!IndentPadding(file, offset))
				memset(&file->cursor.line->line[file->cursor.line->len], 32,
				    offset);
		} else
			memset(&file->cursor.line->line[file->cursor.line->len], 32,
			    offset);

		/* Mark line as padded. */
		file->cursor.line->flags |= LINE_FLAG_PADDED;

		file->cursor.line->len += offset;
	} else
		/* Do we need to make room for this insert? */
		if (file->cursor.offset < file->cursor.line->len) {
			memmove(&file->cursor.line->line[file->cursor.offset + len],
			    &file->cursor.line->line[file->cursor.offset],
			    file->cursor.line->len - file->cursor.offset);
		}

	/* Insert the text. */
	if (len)
		memcpy(&file->cursor.line->line[file->cursor.offset], text, len);

	file->cursor.line->len += len;

	TabulateLine(file->cursor.line, 0, 0);

	CallLineCallbacks(file, file->cursor.line, LINE_OP_EDIT, 0);

	if (file->wordwrap && (options&CAN_WORDWRAP))
		WordWrapLine(file);

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ReallocLine(EDIT_LINE*line, int allocLen)
{
	char*newline;
	int origAllocLen;

	origAllocLen = line->allocSize;

	line->allocSize = allocLen;

	newline = OS_Malloc(line->allocSize);

	memcpy(newline, line->line, line->len);

	/* Free the old line. */
	if (origAllocLen)
		OS_Free(line->line);

	/* Setup the new line. */
	line->line = newline;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_LINE*GetLine(EDIT_FILE*file, int line_number)
{
	EDIT_LINE*line;
	int number = 0;

	line = file->lines;

	while (line) {
		if (number == line_number)
			return (line);

		line = line->next;
		number++;
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteCharacter(EDIT_FILE*file, int options)
{
	CURSOR_SAVE save;
	int flags = 0;

	if (file->cursor.offset == 0 && file->cursor.line->len == 0) {
		DeleteLine(file);
		return ;
	}

	if (file->cursor.offset < file->cursor.line->len) {
		DeleteText(file, 1, CAN_WORDWRAP);
		return ;
	}

	if (file->cursor.line->next) {
		SaveUndo(file, UNDO_CURSOR, 0);

		if (file->cursor.line->next->len) {
			if (!file->cursor.line->len)
				flags = file->cursor.line->next->flags;

			InsertText(file, file->cursor.line->next->line, file->cursor.line->
			    next->len, CAN_INDENT);

			if (flags)
				file->cursor.line->flags = flags;
		}

		SaveCursor(file, &save);
		if (CursorDown(file))
			DeleteLine(file);
		SetCursor(file, &save);

		if (file->wordwrap && (options&CAN_WORDWRAP))
			WordWrapLine(file);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteText(EDIT_FILE*file, int len, int options)
{
	if ((file->cursor.offset + len) >= file->cursor.line->len) {
		CutLine(file, file->cursor.offset, 0);
		return ;
	}

	len = TabulateLength(file->cursor.line->line, file->cursor.offset, len,
	    file->cursor.line->len);

	SaveUndo(file, UNDO_DELETE_TEXT, len);

	memmove(&file->cursor.line->line[file->cursor.offset], &file->cursor.line->
	    line[file->cursor.offset + len],
	    file->cursor.line->len - (file->cursor.offset + len));

	file->cursor.line->len -= len;

	file->paint_flags |= CONTENT_FLAG;

	TabulateLine(file->cursor.line, 0, 0);

	if (file->wordwrap && (options&CAN_WORDWRAP))
		DeWordWrapLine(file);

	CallLineCallbacks(file, file->cursor.line, LINE_OP_EDIT, 0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteLine(EDIT_FILE*file)
{
	EDIT_LINE*line = file->cursor.line;

	if (!line->next) {
		if (file->cursor.line->len) {
			CutLine(file, 0, 0);
			CallLineCallbacks(file, file->cursor.line, LINE_OP_EDIT, 0);
		}
		return ;
	}

	if (file->display.top_line == line && !file->display.top_line->next)
		return ;

	CallLineCallbacks(file, file->cursor.line, LINE_OP_LOSING_FOCUS, 0);

	SaveUndo(file, UNDO_DELETE_LINE, 0);

	CallLineCallbacks(file, line, LINE_OP_DELETE, 0);

	if (file->display.top_line == line)
		file->display.top_line = file->display.top_line->next;

	if (file->cursor.line == line) {
		file->cursor.line = line->next;
		AdjustCursorTab(file, ADJ_CURSOR_LEFT);
		CallLineCallbacks(file, file->cursor.line, LINE_OP_GETTING_FOCUS, 0);
	}

	if (file->lines == line) {
		file->lines = line->next;
		file->lines->prev = 0;
	} else {
		if (line->prev)
			line->prev->next = line->next;

		if (line->next)
			line->next->prev = line->prev;
	}

	if (line->allocSize)
		OS_Free(line->line);

	OS_Free(line);

	file->number_lines--;

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int WhitespaceLine(EDIT_LINE*line)
{
	int i;

	if (!line->len)
		return (0);

	for (i = 0; i < line->len; i++)
		if (line->line[i] > ED_KEY_SPACE)
			break;

	if (i == line->len)
		return (1);

	return (0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int WhitespaceTest(EDIT_FILE*file, int mode)
{
	int i;

	switch (mode) {
	case WHITESPACE_BEFORE :
		for (i = 0; i < file->cursor.offset; i++) {
			if (file->cursor.line->line[i] > ED_KEY_SPACE)
				return (0);
		}
		return (1);

	case WHITESPACE_AFTER :
		break;
	}
	return (0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void StripLinePadding(EDIT_LINE*line)
{
	while (line->len && line->line[line->len - 1] <= ED_KEY_SPACE)
		line->len--;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PrintLine(char*msg, EDIT_LINE*line)
{
	int i;

	OS_Printf("%s: (len=%d) '", msg, line->len);

	for (i = 0; i < line->len; i++)
		OS_Printf("%c", line->line[i]);

	OS_Printf("'\n");
}


