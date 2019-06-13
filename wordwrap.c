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
#include "proedit.h"

static int WrapLine(EDIT_FILE*file, EDIT_CLIPBOARD*clipboard);
static void DeWrapLine(EDIT_FILE*file);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void WordWrapLine(EDIT_FILE*file)
{
	EDIT_CLIPBOARD clipboard;
	EDIT_LINE*line;
	int flags, delta, offset;
	int cursor_line = 0, cursor_offset = 0, cursor_set = 0;

	/* Don't wordwrap non-files. */
	if (file->file_flags&FILE_FLAG_NONFILE)
		return ;

	for (; ; ) {
		offset = 0;

		InitClipboard(&clipboard);

		if (!WrapLine(file, &clipboard))
			break;

		flags = file->cursor.line->flags;

		line = clipboard.lines;

		delta = file->cursor.offset;

		DeleteLine(file);

		while (line) {
			if (!cursor_set && (delta >= offset) &&
			    (delta < (offset + line->len))) {
				cursor_set = 1;
				cursor_line = file->cursor.line_number;
				cursor_offset = TabulateLength(line->line, 0, delta - offset,
				    line->len);
			}

			if (!line->next && file->cursor.line->flags&LINE_FLAG_WRAPPED) {
				CursorOffset(file, 0);
				InsertText(file, line->line, line->len, 0);
				break;
			}

			SaveUndo(file, UNDO_INSERT_LINE, 0);

			if (line == clipboard.lines)
				InsertLine(file, line->line, line->len, INS_ABOVE_CURSOR,
				    flags);
			else
				InsertLine(file, line->line, line->len, INS_ABOVE_CURSOR,
				    flags | LINE_FLAG_WRAPPED);

			offset += line->len;

			CursorDown(file);

			line = line->next;
		}

		ReleaseClipboard(&clipboard);
	}

	if (cursor_set)
		GotoPosition(file, cursor_line + 1, cursor_offset + 1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int WrapLine(EDIT_FILE*file, EDIT_CLIPBOARD*clipboard)
{
	int i, lineLen, wsLen;
	int numLines = 0;
	char*line;

	lineLen = file->cursor.line->len;
	line = file->cursor.line->line;
	wsLen = lineLen;

	while (wsLen && (line[wsLen - 1] <= ED_KEY_SPACE))
		wsLen--;

	for (; ; ) {
		if (wsLen <= file->display.columns) {
			if (!numLines)
				return (0);

			AddClipboardLine(clipboard, line, lineLen, COPY_LINE);
			numLines++;
			break;
		}

		for (i = file->display.columns; i >= 0; i--) {
			if (line[i] == ED_KEY_TABPAD)
				continue;

			if (i == 0 || line[i] <= ED_KEY_SPACE) {
				if (i < ((file->display.columns) / 4)) {
					i = (file->display.columns);

					while (i && line[i] == ED_KEY_TABPAD)
						i--;
				}

				AddClipboardLine(clipboard, line, i, COPY_LINE);
				line += i;
				lineLen -= i;
				wsLen -= i;
				numLines++;
				break;
			}
		}
	}
	return (numLines);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int DeWordWrapLine(EDIT_FILE*file)
{
	CURSOR_SAVE save;

	if (file->cursor.line->next && file->cursor.line->next->flags&
	    LINE_FLAG_WRAPPED) {
		/* If the previous line was wrapped, rejoin and re-wrap them. */
		SaveCursor(file, &save);
		CursorEnd(file);
		DeleteCharacter(file, CAN_WORDWRAP);
		SetCursor(file, &save);
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
static void DeWrapLine(EDIT_FILE*file)
{
	EDIT_LINE*line;
	char*text;
	int i, newLen, numLines, flags;

	line = file->cursor.line;
	flags = file->cursor.line->flags;

	if (line->next && (line->next->flags&LINE_FLAG_WRAPPED)) {
		newLen = line->len;
		numLines = 1;
		line = line->next;
	} else
		return ;

	while (line) {
		if (line->flags&LINE_FLAG_WRAPPED) {
			newLen += line->len;
			numLines++;
		} else
			break;

		line = line->next;
	}

	text = OS_Malloc(newLen);

	newLen = 0;

	for (i = 0; i < numLines; i++) {
		memcpy(&text[newLen], file->cursor.line->line, file->cursor.line->len);
		newLen += file->cursor.line->len;
		DeleteLine(file);
	}

	SaveUndo(file, UNDO_INSERT_LINE, 0);

	InsertLine(file, text, newLen, INS_ABOVE_CURSOR, flags);

	OS_Free(text);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void WordWrapFile(EDIT_FILE*file)
{
	ProgressBar(0, 0, 0);

	UndoBegin(file);

	SaveUndo(file, UNDO_WORDWRAP, 0);

	file->wordwrap = 1;

	CursorTopFile(file);

	for (; ; ) {
		ProgressBar("Word Wrapping file...", file->cursor.line_number, file->
		    number_lines);
		WordWrapLine(file);
		if (!CursorDown(file))
			break;
	}
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeWordWrapFile(EDIT_FILE*file)
{
	ProgressBar(0, 0, 0);

	UndoBegin(file);

	SaveUndo(file, UNDO_UNWORDWRAP, 0);

	file->wordwrap = 0;

	CursorTopFile(file);

	for (; ; ) {
		ProgressBar("Un-Word Wrapping file...", file->cursor.line_number,
		    file->number_lines);
		DeWrapLine(file);
		if (!CursorDown(file))
			break;
	}
}




