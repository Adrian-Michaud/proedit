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
#include <stdarg.h>
#include "proedit.h"


static int ModifyCommand(EDIT_FILE*file, int ch);
static EDIT_FILE*LineInput(EDIT_FILE*file, int mode, int ch);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*ProcessUserInput(EDIT_FILE*file, USER_INPUT_PFN*pfn)
{
	int mode;
	int ch;

	ch = OS_Key(&mode, &file->mouse);

	if (ch != ED_KEY_HOME)
		file->home_count = 0;

	if (ch != ED_KEY_END)
		file->end_count = 0;

	if (mode && ch == ED_KEY_MOUSE) {
		MouseCommand(file);
		return (file);
	}

	if (pfn)
		return (pfn(file, mode, ch));

	if (file->hexMode)
		file = ProcessHexInput(file, mode, ch);
	else {
		if (!mode) {
			if (file->file_flags&FILE_FLAG_READONLY)
				CenterBottomBar(1, "[-] This file is read-only [-]");
			else
				ProcessNormal(file, ch);
		} else
			file = ProcessSpecial(file, ch);
	}

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*ProcessSpecial(EDIT_FILE*file, int ch)
{
	EDIT_CLIPBOARD*clipboard;
	char filename[MAX_FILENAME];
	char pathname[MAX_FILENAME];

	if (file->file_flags&FILE_FLAG_READONLY) {
		if (ModifyCommand(file, ch)) {
			CenterBottomBar(1, "[-] This file is read-only [-]");
			return (file);
		}
	}

	switch (ch) {
	case ED_KEY_SHIFT_UP :
		file->shift = 0;
		break;

	case ED_KEY_SHIFT_DOWN :
		file->shift = 1;
		break;

	case ED_ALT_N :
		CancelSelectBlock(file);
		file = SwitchFile(file);
		break;

	case ED_ALT_K :
		CancelSelectBlock(file);
		UndoBegin(file);
		CutLine(file, file->cursor.offset, CAN_REALLOC);
		break;

	case ED_ALT_M :
		FindMatch(file);
		break;

	case ED_CTRL_Z :
	case ED_ALT_U :
		CancelSelectBlock(file);
		Undo(file);
		break;

	case ED_ALT_I :
		Overstrike(file);
		break;

	case ED_ALT_V :
		CopyBlock(file, COPY_BLOCK);
		break;

	case ED_ALT_H :
		CancelSelectBlock(file);
		DisplayHelp(file);
		break;

	case ED_ALT_C :
		CopyBlock(file, COPY_LINE);
		break;

	case ED_ALT_Z :
		file = BuildShell(file);
		break;

	case ED_ALT_B :
		file = Build(file);
		break;

	case ED_CTRL_C :
		CopySelectBlock(file);
		break;

	case ED_KEY_CTRL_INSERT :
	case ED_KEY_ALT_INSERT :
		PasteSystemBlock(file);
		break;

	case ED_CTRL_V :
		PasteSelectBlock(file);
		break;

	case ED_ALT_J :
		file = JumpNextBookmark(file);
		break;

	case ED_ALT_P :
		CreateBookmark(file);
		break;

	case ED_ALT_D :
		CancelSelectBlock(file);
		UndoBegin(file);
		DeleteLine(file);
		break;

	case ED_ALT_G :
		CancelSelectBlock(file);
		GotoLine(file);
		break;

	case ED_ALT_Y :
		CancelSelectBlock(file);
		file = Merge(file);
		break;

	case ED_ALT_S :
		CancelSelectBlock(file);
		file = SearchFile(file);
		break;

	case ED_ALT_L :
		SelectBlockCheck(file, file->shift);
		file = SearchAgain(file);
		break;

	case ED_ALT_F :
		CancelSelectBlock(file);
		file = SelectFile(file, SELECT_FILE_ALL);
		break;

	case ED_ALT_R :
		CancelSelectBlock(file);
		file = SearchReplace(file);
		break;

	case ED_ALT_X :
		CancelSelectBlock(file);
		file = ExitFiles(file);
		break;

	case ED_ALT_W :
		{
			CancelSelectBlock(file);
			if (SaveFile(file)) {
				OS_GetFilename(file->pathname, 0, filename);
				CenterBottomBar(1, "[+] \"%s\" has been saved to disk [+]",
				    filename);
			}
			break;
		}

	case ED_ALT_A :
		{
			CancelSelectBlock(file);
			if (SaveFileAs(file, pathname)) {
				OS_GetFilename(pathname, 0, filename);
				CenterBottomBar(1, "[+] \"%s\" has been saved to disk [+]",
				    filename);
			}
			break;
		}

	case ED_ALT_Q :
		CancelSelectBlock(file);
		file = QuitFile(file, 0, 0);
		break;

	case ED_ALT_E :
		CancelSelectBlock(file);
		file = LoadNewFile(file);
		break;

	case ED_ALT_T :
		CancelSelectBlock(file);
		RenameFile(file);
		break;

	case ED_KEY_PGUP :
		SelectBlockCheck(file, file->shift);
		CursorPgup(file);
		break;

	case ED_KEY_PGDN :
		SelectBlockCheck(file, file->shift);
		CursorPgdn(file);
		break;

	case ED_KEY_HOME :
		SelectBlockCheck(file, file->shift);
		KeyHome(file);
		break;

	case ED_KEY_END :
		SelectBlockCheck(file, file->shift);
		KeyEnd(file);
		break;

	case ED_KEY_LEFT :
		SelectBlockCheck(file, file->shift);
		CursorLeft(file);
		break;

	case ED_KEY_RIGHT :
		SelectBlockCheck(file, file->shift);
		CursorRight(file);
		break;

	case ED_KEY_UP :
		SelectBlockCheck(file, file->shift);
		CursorUp(file);
		break;

	case ED_KEY_DOWN :
		SelectBlockCheck(file, file->shift);
		CursorDown(file);
		break;

	case ED_KEY_ALT_UP :
	case ED_KEY_CTRL_UP :
		SelectBlockCheck(file, file->shift);
		LineUp(file);
		break;

	case ED_KEY_ALT_RIGHT :
	case ED_KEY_CTRL_RIGHT :
		SelectBlockCheck(file, file->shift);
		CursorRightText(file);
		break;

	case ED_KEY_ALT_DOWN :
	case ED_KEY_CTRL_DOWN :
		SelectBlockCheck(file, file->shift);
		LineDown(file);
		break;

	case ED_KEY_ALT_LEFT :
	case ED_KEY_CTRL_LEFT :
		SelectBlockCheck(file, file->shift);
		CursorLeftText(file);
		break;

	case ED_ALT_O :
		{
			Operation(file);
			break;
		}

	case ED_KEY_INSERT :
		{
			clipboard = GetClipboard();

			if (file->copyStatus&COPY_ON) {
				SaveBlock(clipboard, file);
				file->paint_flags |= CONTENT_FLAG;
				file->copyStatus = 0;
				CenterBottomBar(1, "[+] Block saved to Clipboard [+]");
			} else {
				if (!clipboard->status)
					CenterBottomBar(1, "[-] Clipboard is Empty [-]");
				else {
					UndoBegin(file);
					InsertBlock(clipboard, file);
				}
			}
			CancelSelectBlock(file);
			break;
		}

	case ED_KEY_DELETE :
		{
			UndoBegin(file);
			if (file->copyStatus&COPY_ON) {
				SaveBlock(GetClipboard(), file);
				DeleteBlock(file);
				CancelSelectBlock(file);
			} else
				DeleteCharacter(file, CAN_WORDWRAP);
			break;
		}

	case ED_F1 :
		CancelSelectBlock(file);
		DefineMacro(file);
		break;

	case ED_F2 :
		CancelSelectBlock(file);
		ReplayMacro();
		break;

	case ED_F3 :
		ToggleBackups();
		break;

	case ED_F4 :
		CancelSelectBlock(file);
		Configure(file);
		break;

	case ED_F5 :
		ToggleCase();
		break;

	case ED_F6 :
		ToggleFileSearch();
		break;

	case ED_F7 :
		ToggleDisplaySpecial(file);
		break;

	case ED_F8 :
		ToggleColorize(file);
		break;

	case ED_F10 :
		CancelSelectBlock(file);
		ToggleWordWrap(file);
		break;

	case ED_F9 :
		CancelSelectBlock(file);
		file = Utilities(file);
		break;
	}

	return (file);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ModifyCommand(EDIT_FILE*file, int ch)
{
	switch (ch) {
	case ED_KEY_INSERT :
		{
			if (file->copyStatus&COPY_ON)
				return (0);
			else
				return (1);
		}

	case ED_ALT_K :
	case ED_CTRL_Z :
	case ED_ALT_U :
	case ED_ALT_I :
	case ED_ALT_A :
	case ED_ALT_W :
	case ED_KEY_CTRL_INSERT :
	case ED_KEY_ALT_INSERT :
	case ED_CTRL_V :
	case ED_ALT_D :
	case ED_ALT_R :
	case ED_ALT_O :
	case ED_KEY_DELETE :
	case ED_F12 :
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
void MouseCommand(EDIT_FILE*file)
{
	if (file->mouse.wheel == OS_WHEEL_UP) {
		if (file->hexMode)
			LineHexUp(file);
		else
			LineUp(file);
	}
	if (file->mouse.wheel == OS_WHEEL_DOWN) {
		if (file->hexMode)
			LineHexDown(file);
		else
			LineDown(file);
	}
	if (file->mouse.button1 == OS_BUTTON_PRESSED) {
		SelectBlockCheck(file, file->shift);
		file->shift = 1;
		if (file->hexMode)
			MouseHexClick(file);
		else
			MouseClick(file);
	}

	if (file->mouse.button1 == OS_BUTTON_RELEASED)
		file->shift = 0;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int MouseClick(EDIT_FILE*file)
{
	int xpos, ypos;

	if ((file->mouse.xpos > file->display.xpos) && (file->mouse.xpos < (file->
	    display.xpos + file->display.xd - 1)) && (file->mouse.ypos > file->
	    display.ypos) &&
	    (file->mouse.ypos < (file->display.ypos + file->display.yd - 2))) {
		xpos = (file->mouse.xpos - file->display.xpos) - 1;
		ypos = (file->mouse.ypos - file->display.ypos) - 1;

		MouseCursor(file, xpos, ypos);

		return (1);
	}

	if (file->mouse.xpos == file->display.xpos + file->display.xd - 1) {
		if (file->mouse.ypos == file->display.ypos + 1)
			LineUp(file);

		if (file->mouse.ypos == file->display.ypos + file->display.yd - 3)
			LineDown(file);
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void MouseCursor(EDIT_FILE*file, int xpos, int ypos)
{
	if (xpos < file->cursor.xpos) {
		while (xpos < file->cursor.xpos)
			if (!CursorLeft(file))
				break;
	} else {
		while (xpos > file->cursor.xpos)
			if (!CursorRight(file))
				break;
	}

	if (ypos < file->cursor.ypos) {
		while (ypos < file->cursor.ypos)
			if (!CursorUp(file))
				break;
	} else {
		while (ypos > file->cursor.ypos)
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
int Question(char*fmt, ...)
{
	OS_MOUSE mouse;
	int ch;
	int mode;
	va_list args;
	char*string;

	string = OS_Malloc(512);
	va_start(args, fmt);
	vsprintf(string, fmt, args);
	va_end(args);

	DisplayBottomBar(string);

	OS_SetTextPosition(strlen(string) + 1, GetScreenYDim());

	OS_SetCursorType(0, 1);

	for (; ; ) {
		ch = OS_Key(&mode, &mouse);

		if (mode)
			continue;

		break;
	}

	OS_Free(string);

	HideCursor();

	return (ch);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int Input(int historyIndex, char*prompt, char*result, int max)
{
	EDIT_FILE*input;

	int i, x, y, xd, yd, len, bar, undo, newLen = 0;

	y = GetScreenYDim() - 1;
	yd = 1;
	x = strlen(prompt);
	xd = GetScreenXDim() - x;

	DisplayBottomBar("%s", prompt);

	input = AllocFile("Input");

	input->file_flags = FILE_FLAG_NOBORDER | FILE_FLAG_EDIT_STATUS |
	FILE_FLAG_SINGLELINE;

	InitDisplayFile(input);

	input->display.xpos = x;
	input->display.ypos = y;
	input->display.xd = xd;
	input->display.yd = yd;
	input->display.columns = xd;
	input->display.rows = yd;

	input->client = 2;
	input->border = 2;

	input->userArg = historyIndex;

	InitCursorFile(input);

	CursorTopFile(input);

	undo = DisableUndo(input);

	InsertText(input, result, strlen(result), 0);
	CursorEnd(input);

	EnableUndo(input, undo);

	input->paint_flags |= CONTENT_FLAG | CURSOR_FLAG;

	bar = DisableBottomBar();

	for (; ; ) {
		Paint(input);

		if (!ProcessUserInput(input, LineInput))
			break;
	}

	EnableBottomBar(bar);

	if (!input->lines) {
		result[0] = 0;
		DeallocFile(input);
		return (0);
	}

	len = input->cursor.line->len;

	if (len > max - 1)
		len = max - 1;

	if (len)
		memcpy(result, input->cursor.line->line, len);

	for (i = 0; i < len; i++) {
		if (result[i] == ED_KEY_TABPAD)
			continue;

		result[newLen++] = result[i];
	}

	result[newLen] = 0;

	if (strlen(result))
		AddToHistory(historyIndex, result);

	DeallocFile(input);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int AbortRequest(void)
{
	int mode = 0;
	int ch;
	OS_MOUSE mouse;

	ch = OS_PeekKey(&mode, &mouse);

	if (mode == 0 && ch == ED_KEY_ESC)
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*LineInput(EDIT_FILE*file, int mode, int ch)
{
	EDIT_FILE*retCode;
	char filename[MAX_FILENAME];
	int index;

	retCode = file;

	if (!mode) {
		if (ch == ED_KEY_TAB && file->userArg == HISTORY_NEWFILE) {
			if (file->cursor.offset < MAX_FILENAME) {
				if (file->cursor.line->line) {
					memcpy(filename, file->cursor.line->line, file->cursor.
					    line->len);
					filename[file->cursor.line->len] = 0;
				} else
					strcpy(filename, "");

				if (QueryFiles(filename)) {
					if (strlen(filename))
						AddToHistory(file->userArg, filename);

					retCode = 0;
				}

				UndoBegin(file);
				SaveUndo(file, UNDO_CURSOR, 0);
				CursorHome(file);
				CutLine(file, 0, 0);
				InsertText(file, filename, strlen(filename), 0);
				CursorEnd(file);
			}

			file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG;
			return (retCode);
		}

		if (ch == ED_KEY_CR)
			return (0);

		if (ch == ED_KEY_ESC) {
			DeallocLines(file->lines);
			file->lines = 0;
			return (0);
		}

		ProcessNormal(file, ch);
	} else {
		switch (ch) {
		case ED_KEY_UP :
			{
				index = SelectHistoryText(file->userArg);

				if (index) {
					UndoBegin(file);

					SaveUndo(file, UNDO_CURSOR, 0);

					CursorHome(file);

					CutLine(file, 0, 0);

					InsertText(file, QueryHistory(file->userArg, index - 1),
					    strlen(QueryHistory(file->userArg, index - 1)), 0);

					CursorEnd(file);
				}
				file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG;
				break;
			}

		case ED_KEY_INSERT :
		case ED_ALT_U :
		case ED_KEY_SHIFT_UP :
		case ED_KEY_SHIFT_DOWN :
		case ED_ALT_K :
		case ED_ALT_M :
		case ED_CTRL_Z :
		case ED_ALT_I :
		case ED_ALT_V :
		case ED_ALT_C :
		case ED_CTRL_C :
		case ED_KEY_CTRL_INSERT :
		case ED_KEY_ALT_INSERT :
		case ED_CTRL_V :
		case ED_ALT_D :
		case ED_KEY_HOME :
		case ED_KEY_END :
		case ED_KEY_LEFT :
		case ED_KEY_RIGHT :
		case ED_KEY_ALT_RIGHT :
		case ED_KEY_CTRL_RIGHT :
		case ED_KEY_ALT_LEFT :
		case ED_KEY_CTRL_LEFT :
		case ED_KEY_DELETE :
			file = ProcessSpecial(file, ch);
			break;
		}
	}

	return (file);
}



