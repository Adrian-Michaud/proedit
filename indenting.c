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

static int IndentLineHandler(EDIT_FILE*file, EDIT_LINE*line, int op, int arg);
static int GetLineIndent(EDIT_LINE*line);
static EDIT_LINE*GetLastLineIndent(EDIT_LINE*line);
static int IndentClosingBracket(EDIT_FILE*file);

extern int indenting;


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InitFileIndenting(EDIT_FILE*file)
{
	AddLineCallback(file, (LINE_PFN*)IndentLineHandler, LINE_OP_LOSING_FOCUS |
	    LINE_OP_INSERT_CHAR);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void IndentLine(EDIT_FILE*file)
{
	EDIT_LINE*cursorLine, *lastIndent;
	int indent;

	if (!indenting)
		return ;

	cursorLine = file->cursor.line;

	lastIndent = GetLastLineIndent(cursorLine->prev);

	if (lastIndent) {
		indent = GetLineIndent(lastIndent);

		if (indent) {
			if (cursorLine->len) {
				/* Flag so we can remove the indented text if it's not needed. */
				cursorLine->flags |= (LINE_FLAG_INDENTED | LINE_FLAG_PADDED);

				/* Insert the indented text from the previous line. */
				InsertText(file, lastIndent->line, indent, CAN_WORDWRAP);
			}

			/* Move the cursor over to the new indented position. */
			while (file->cursor.offset < indent)
				CursorRight(file);
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorIndentOffset(EDIT_FILE*file)
{
	EDIT_LINE*cursorLine, *lastIndent;
	int indent = 0;

	if (!indenting || (file->file_flags&FILE_FLAG_NONFILE))
		return (0);

	cursorLine = file->cursor.line;

	lastIndent = GetLastLineIndent(cursorLine->prev);

	if (lastIndent)
		indent = GetLineIndent(lastIndent);

	return (indent);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int IndentPadding(EDIT_FILE*file, int offset)
{
	EDIT_LINE*cursorLine, *lastIndent;
	int indent;

	if (!indenting || (file->file_flags&FILE_FLAG_NONFILE))
		return (0);

	cursorLine = file->cursor.line;

	lastIndent = GetLastLineIndent(cursorLine->prev);

	if (lastIndent) {
		indent = GetLineIndent(lastIndent);

		if (indent) {
			/* Was the last identment the same as our offset? */
			if (indent <= offset) {
				file->cursor.line->flags |= LINE_FLAG_PADDED;
				/* Use a previous indentment method for this line too. */
				memcpy(file->cursor.line->line, lastIndent->line, indent);
				/* Pad the rest with spaces. */
				memset(&file->cursor.line->line[indent], 32, offset - indent);
				return (1);
			}
		}
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GetLineIndent(EDIT_LINE*line)
{
	int indent = 0;

	for (indent = 0; indent < line->len; indent++)
		if (line->line[indent] > ED_KEY_SPACE)
			break;

	/* Ignore lines with just whitespace. */
	if (indent == line->len)
		return (0);

	return (indent);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_LINE*GetLastLineIndent(EDIT_LINE*line)
{
	int indent;

	while (line) {
		for (indent = 0; indent < line->len; indent++)
			if (line->line[indent] > ED_KEY_SPACE)
				break;

		if (indent != line->len)
			return (line);

		line = line->prev;
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int IndentLineHandler(EDIT_FILE*file, EDIT_LINE*line, int op, int arg)
{
	if (!indenting || (file->file_flags&FILE_FLAG_NONFILE))
		return (0);

	if ((op&LINE_OP_LOSING_FOCUS) && (line->flags&LINE_FLAG_INDENTED)) {
		/* Is line all whitespace? If no, just cut it. */
		if (WhitespaceLine(line))
			CutLine(file, 0, CAN_REALLOC);

		line->flags &= ~LINE_FLAG_INDENTED;
		return (1);
	}

	if (op&LINE_OP_INSERT_CHAR) {
		if (arg == '}')
			return (IndentClosingBracket(file));
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int IndentClosingBracket(EDIT_FILE*file)
{
	CURSOR_SAVE cursor;
	int indent = 0, currentLine;
	EDIT_LINE*bracketLine;
	int del;

	currentLine = file->cursor.line_number;

	SaveCursor(file, &cursor);

	if (!MatchOperator(file, '}', '{', 0)) {
		SetCursor(file, &cursor);
		return (0);
	}

	if (file->cursor.line_number == currentLine) {
		SetCursor(file, &cursor);
		return (0);
	}

	bracketLine = file->cursor.line;
	indent = GetLineIndent(bracketLine);

	SetCursor(file, &cursor);

	if (file->cursor.line->len) {
		if (WhitespaceLine(file->cursor.line)) {
			CutLine(file, 0, CAN_REALLOC);
		} else {
			if (WhitespaceTest(file, WHITESPACE_BEFORE)) {
				del = file->cursor.offset;
				CursorOffset(file, 0);
				DeleteText(file, del, CAN_WORDWRAP);
			} else {
				CarrageReturn(file, 0);
			}
		}
	}

	CursorOffset(file, 0);

	if (indent)
		InsertText(file, bracketLine->line, indent, CAN_WORDWRAP);

	/* Move the cursor over to the new indented position. */
	while (file->cursor.offset < indent)
		CursorRight(file);

	return (0);
}


