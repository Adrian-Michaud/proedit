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

static void DeallocUndo(EDIT_FILE*file, EDIT_UNDOS*undo);
static EDIT_UNDOS*AllocUndo(EDIT_FILE*file);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int DisableUndo(EDIT_FILE*file)
{
	int undo;

	undo = file->undo_disabled;

	file->undo_disabled = 1;

	return (undo);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void EnableUndo(EDIT_FILE*file, int undo)
{
	file->undo_disabled = undo;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void UndoBegin(EDIT_FILE*file)
{
	if (file->undoStatus&UNDO_RUNNING)
		return ;

	file->undoStatus |= UNDO_BEGIN;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteUndos(EDIT_FILE*file)
{
	while (file->undoHead)
		DeallocUndo(file, file->undoHead);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SaveUndo(EDIT_FILE*file, int type, int arg)
{
	EDIT_UNDOS*undo;
	int op;

	if (file->undo_disabled)
		return ;

	if (file->undoStatus&UNDO_RUNNING)
		return ;

	if (file->userUndos >= MAX_UNDOS) {
		file->modified++;

		while (file->undoHead) {
			file->numberUndos--;

			op = file->undoHead->operationStatus;

			DeallocUndo(file, file->undoHead);

			if (op&UNDO_DONE) {
				file->userUndos--;
				break;
			}
		}
	}

	undo = AllocUndo(file);

	SaveCursor(file, &undo->cursor);

	undo->arg = arg;

	undo->operationStatus = type;

	if (file->undoStatus&UNDO_BEGIN) {
		undo->operationStatus |= UNDO_DONE;
		file->undoStatus &= ~UNDO_BEGIN;
		file->userUndos++;
		file->modified++;
	}

	if (undo->operationStatus&(UNDO_HEX_OVERSTRIKE | UNDO_HEX_DELETE)) {
		undo->len = arg;
		undo->buffer = OS_Malloc(undo->len);

		memcpy(undo->buffer, &file->hexData[file->cursor.line_number],
		    undo->len);
	}

	if (undo->operationStatus&UNDO_INSERT_TEXT)
		undo->len = file->cursor.line->len;

	if (undo->operationStatus&UNDO_CUTLINE) {
		undo->len = file->cursor.line->len - undo->arg;
		undo->buffer = OS_Malloc(undo->len);
		memcpy(undo->buffer, &file->cursor.line->line[undo->arg], undo->len);
	}

	if (undo->operationStatus&UNDO_DELETE_TEXT) {
		undo->len = arg;
		undo->buffer = OS_Malloc(arg);
		memcpy(undo->buffer, &file->cursor.line->line[file->cursor.offset],
		    arg);
	}

	if (undo->operationStatus&(UNDO_DELETE_LINE)) {
		undo->line_flags = file->cursor.line->flags;

		if (file->cursor.line->len) {
			undo->len = file->cursor.line->len;
			undo->buffer = OS_Malloc(undo->len);

			memcpy(undo->buffer, file->cursor.line->line, undo->len);
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void Undo(EDIT_FILE*file)
{
	EDIT_UNDOS*undo;

	if (!file->numberUndos) {
		CenterBottomBar(1, "[-] Nothing To Undo [-]");
		return ;
	}

	file->undoStatus |= UNDO_RUNNING;

	while (file->undoTail) {
		undo = file->undoTail;

		SetCursor(file, &undo->cursor);

		if (undo->operationStatus&UNDO_INSERT_TEXT) {
			DeleteText(file, undo->arg, 0);
			file->cursor.line->len = undo->len;
		}

		if (undo->operationStatus&UNDO_WORDWRAP)
			file->wordwrap = 0;

		if (undo->operationStatus&UNDO_UNWORDWRAP)
			file->wordwrap = 1;

		if (undo->operationStatus&UNDO_INSERT_LINE)
			DeleteLine(file);

		if (undo->operationStatus&UNDO_DELETE_LINE)
			InsertLine(file, undo->buffer, undo->len, INS_ABOVE_CURSOR, 0);

		if (undo->operationStatus&UNDO_DELETE_TEXT)
			InsertText(file, undo->buffer, undo->len, 0);

		if (undo->operationStatus&UNDO_CUTLINE) {
			file->cursor.offset = undo->arg;
			InsertText(file, undo->buffer, undo->len, 0);
			SetCursor(file, &undo->cursor);
		}

		if (undo->operationStatus&UNDO_HEX_INSERT)
			DeleteHexBytes(file, undo->arg);

		if (undo->operationStatus&UNDO_HEX_DELETE)
			InsertHexBytes(file, undo->buffer, undo->len);

		if (undo->operationStatus&UNDO_HEX_OVERSTRIKE) {
			memcpy(&file->hexData[file->cursor.line_number], undo->buffer,
			    undo->len);
		}

		if (undo->operationStatus&UNDO_CARRAGE_RETURN)
			DeleteCharacter(file, 0);

		if (undo->operationStatus&UNDO_DONE) {
			file->userUndos--;

			if (!file->modified)
				file->force_modified = 1;

			file->modified--;

			DeallocUndo(file, undo);
			break;
		}

		DeallocUndo(file, undo);
	}

	if (!file->numberUndos)
		file->userUndos = 0;

	file->undoStatus &= ~UNDO_RUNNING;

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DeallocUndo(EDIT_FILE*file, EDIT_UNDOS*undo)
{
	file->numberUndos--;

	if (undo->buffer)
		OS_Free(undo->buffer);

	if (undo->prev)
		undo->prev->next = undo->next;

	if (undo->next)
		undo->next->prev = undo->prev;

	if (undo == file->undoHead)
		file->undoHead = undo->next;

	if (undo == file->undoTail)
		file->undoTail = undo->prev;

	OS_Free(undo);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_UNDOS*AllocUndo(EDIT_FILE*file)
{
	EDIT_UNDOS*undo;

	undo = (EDIT_UNDOS*)OS_Malloc(sizeof(EDIT_UNDOS));

	file->numberUndos++;

	memset(undo, 0, sizeof(EDIT_UNDOS));

	undo->prev = file->undoTail;

	if (!file->undoHead)
		file->undoHead = undo;

	if (file->undoTail)
		file->undoTail->next = undo;

	file->undoTail = undo;

	return (undo);
}



