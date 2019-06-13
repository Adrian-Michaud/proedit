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

static EDIT_FILE*ProcessHexSpecial(EDIT_FILE*file, int ch);

static void GotoHexAddress(EDIT_FILE*file);
static void ProcessHexNormal(EDIT_FILE*file, int ch);
static void HexInsert(EDIT_FILE*file, int ch);
static void HexOverstrike(EDIT_FILE*file, int ch);
static void DeleteHex(EDIT_FILE*file);
static void DeleteKey(EDIT_FILE*file);
static void HexBackspace(EDIT_FILE*file);
static void DeleteHexLine(EDIT_FILE*file);
static void CopyHexBlock(EDIT_FILE*file);
static void PasteHexBlock(EDIT_FILE*file);
static int DisplayHexLimit(EDIT_FILE*file, int address);
static void AdjustHexDisplayPan(EDIT_FILE*file);
static int GetXPos(EDIT_FILE*file, int address);
static int HexCursorUp(EDIT_FILE*file);
static int HexCursorDown(EDIT_FILE*file);
static int HexDisplayUp(EDIT_FILE*file);
static int HexDisplayDown(EDIT_FILE*file);
static void SetHexYPos(EDIT_FILE*file);
static void SetHexXPos(EDIT_FILE*file);

static char*hexClipboard;
static int hexClipboardLen;
extern int hex_endian;

#define HEX_ROUND(a,file)  (((a)/file->hex_columns) * file->hex_columns)


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*ProcessHexInput(EDIT_FILE*file, int mode, int ch)
{
	if (!mode)
		ProcessHexNormal(file, ch);
	else
		file = ProcessHexSpecial(file, ch);

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ProcessHexNormal(EDIT_FILE*file, int ch)
{
	if (ch == ED_KEY_ESC) {
		CancelSelectBlock(file);
		if (file->copyStatus&COPY_ON) {
			file->paint_flags |= CONTENT_FLAG;
			file->copyStatus = 0;
		}
		return ;
	}

	if (ch == ED_KEY_BS) {
		HexBackspace(file);
		return ;
	}

	if (ch == ED_KEY_TAB) {
		if (file->hexMode == HEX_MODE_HEX)
			file->hexMode = HEX_MODE_TEXT;
		else
			file->hexMode = HEX_MODE_HEX;

		SetHexXPos(file);

		return ;
	}

	if (file->overstrike)
		HexOverstrike(file, ch);
	else
		HexInsert(file, ch);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void HexBackspace(EDIT_FILE*file)
{
	if (file->hexMode == HEX_MODE_HEX) {
		if (file->cursor.xpos&1) {
			CursorHexLeft(file);
			return ;
		}
	}

	if (file->cursor.line_number) {
		UndoBegin(file);
		SaveUndo(file, UNDO_CURSOR, 0);
		CursorHexLeft(file);
		DeleteHex(file);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void HexInsert(EDIT_FILE*file, int ch)
{
	int nibble;
	char data;

	if (file->hexMode == HEX_MODE_HEX) {
		if (file->cursor.xpos&0x01) {
			HexOverstrike(file, ch);
			return ;
		}

		if (ch >= 'a' && ch <= 'f')
			nibble = 10 + ch - 'a';
		else
			if (ch >= 'A' && ch <= 'F')
				nibble = 10 + ch - 'A';
			else
				if (ch >= '0' && ch <= '9')
					nibble = ch - '0';
				else {
					OS_Bell();
					return ;
				}
		ch = (nibble << 4);
	}

	UndoBegin(file);

	data = (char)ch;

	InsertHexBytes(file, &data, 1);

	CursorHexRight(file);

	file->paint_flags |= CONTENT_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void HexOverstrike(EDIT_FILE*file, int ch)
{
	int nibble;
	char data;

	if ((file->cursor.line_number) == file->number_lines) {
		if (file->cursor.xpos&0x01)
			CursorHexLeft(file);

		HexInsert(file, ch);
		return ;
	}

	if (file->hexMode == HEX_MODE_HEX) {
		if (ch >= 'a' && ch <= 'f')
			nibble = 10 + ch - 'a';
		else
			if (ch >= 'A' && ch <= 'F')
				nibble = 10 + ch - 'A';
			else
				if (ch >= '0' && ch <= '9')
					nibble = ch - '0';
				else {
					OS_Bell();
					return ;
				}

		if (file->cursor.xpos&0x01)
			ch = (file->hexData[file->cursor.line_number]&0xF0) | nibble;
		else
			ch = (file->hexData[file->cursor.line_number]&0x0F) | (nibble << 4);
	}

	UndoBegin(file);

	data = ch;

	OverstrikeHexBytes(file, &data, 1);

	CursorHexRight(file);

	file->paint_flags |= CONTENT_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*ProcessHexSpecial(EDIT_FILE*file, int ch)
{
	switch (ch) {
	case ED_KEY_SHIFT_UP :
	case ED_KEY_SHIFT_DOWN :
	case ED_ALT_S :
	case ED_ALT_B :
	case ED_ALT_Z :
	case ED_ALT_L :
	case ED_ALT_R :
	case ED_ALT_N :
	case ED_CTRL_Z :
	case ED_ALT_U :
	case ED_ALT_H :
	case ED_ALT_F :
	case ED_ALT_X :
	case ED_ALT_W :
	case ED_ALT_Q :
	case ED_ALT_E :
	case ED_ALT_T :
	case ED_ALT_C :
	case ED_ALT_V :
	case ED_F1 :
	case ED_F2 :
	case ED_F4 :
	case ED_F5 :
	case ED_F9 :
		return (ProcessSpecial(file, ch));

	case ED_ALT_I :
		Overstrike(file);
		break;

	case ED_ALT_D :
		UndoBegin(file);
		DeleteHexLine(file);
		break;

	case ED_CTRL_C :
		CopyHexBlock(file);
		break;

	case ED_CTRL_V :
		PasteHexBlock(file);
		break;

	case ED_KEY_INSERT :
		{
			if (file->copyStatus&COPY_ON) {
				CopyHexBlock(file);
				file->paint_flags |= CONTENT_FLAG;
				file->copyStatus = 0;
			} else
				PasteHexBlock(file);
			break;
		}

	case ED_KEY_DELETE :
		{
			if (file->copyStatus&COPY_ON) {
				UndoBegin(file);
				DeleteHexBlock(file);
				CancelSelectBlock(file);
			} else
				DeleteKey(file);
			break;
		}

	case ED_ALT_G :
		{
			CancelSelectBlock(file);
			GotoHexAddress(file);
			break;
		}

	case ED_KEY_LEFT :
		{
			SelectBlockCheck(file, file->shift);
			CursorHexLeft(file);
			break;
		}

	case ED_KEY_RIGHT :
		SelectBlockCheck(file, file->shift);
		CursorHexRight(file);
		break;

	case ED_KEY_UP :
		SelectBlockCheck(file, file->shift);
		CursorHexUp(file);
		break;

	case ED_KEY_DOWN :
		SelectBlockCheck(file, file->shift);
		CursorHexDown(file);
		break;

	case ED_KEY_ALT_UP :
	case ED_KEY_CTRL_UP :
		SelectBlockCheck(file, file->shift);
		LineHexUp(file);
		break;

	case ED_KEY_ALT_DOWN :
	case ED_KEY_CTRL_DOWN :
		SelectBlockCheck(file, file->shift);
		LineHexDown(file);
		break;


	case ED_KEY_ALT_RIGHT :
	case ED_KEY_CTRL_RIGHT :
		SelectBlockCheck(file, file->shift);
		file->cursor.xpos |= 0x01;
		CursorHexRight(file);
		file->cursor.xpos &= 0xFFF0;
		AdjustHexDisplayPan(file);
		break;

	case ED_KEY_ALT_LEFT :
	case ED_KEY_CTRL_LEFT :
		SelectBlockCheck(file, file->shift);
		file->cursor.xpos &= 0xFFF0;
		CursorHexLeft(file);
		file->cursor.xpos &= 0xFFF0;
		AdjustHexDisplayPan(file);
		break;

	case ED_KEY_PGUP :
		SelectBlockCheck(file, file->shift);
		CursorHexPgup(file);
		break;

	case ED_KEY_PGDN :
		SelectBlockCheck(file, file->shift);
		CursorHexPgdn(file);
		break;

	case ED_KEY_HOME :
		SelectBlockCheck(file, file->shift);
		CursorHexHome(file);
		break;

	case ED_KEY_END :
		SelectBlockCheck(file, file->shift);
		CursorHexEnd(file);
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
static void AdjustHexDisplayPan(EDIT_FILE*file)
{
	int xp = 0;
	int offset = 2;

	if (file->file_flags&FILE_FLAG_NOBORDER)
		offset = 1;

	if (file->hexMode&HEX_MODE_HEX)
		xp = (12 + file->display.xpos + ((file->cursor.xpos >> 4)*
		(3)) + offset + (file->cursor.xpos&0x0F));
	else
		if (file->hexMode&HEX_MODE_TEXT)
			xp = (12 + file->hex_columns*(3) +
			file->display.xpos + (file->cursor.xpos >> 4) + offset);

	if (file->display.pan && ((xp - (offset)) < file->display.columns)) {
		file->display.pan = 0;
		file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
	} else
		if ((xp - offset) < file->display.pan) {
			file->display.pan = (xp - offset);
			file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
		} else
			if (((xp - (offset)) - file->display.pan) >=
			    file->display.columns) {
				file->display.pan += ((((xp - (offset)) - file->display.pan) -
				file->display.columns));
				file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
			}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void PasteHexBlock(EDIT_FILE*file)
{
	if (!hexClipboardLen) {
		CenterBottomBar(1, "[-] Clipboard is Empty [-]");
		return ;
	}

	UndoBegin(file);

	if (file->overstrike)
		OverstrikeHexBytes(file, hexClipboard, hexClipboardLen);
	else
		InsertHexBytes(file, hexClipboard, hexClipboardLen);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PasteHexCharset(EDIT_FILE*file)
{
	unsigned char*charset;
	int i;

	charset = OS_Malloc(0x100);

	for (i = 0; i < 0x100; i++)
		charset[i] = (unsigned char)i;

	if (file->overstrike)
		OverstrikeHexBytes(file, (char*)charset, i);
	else
		InsertHexBytes(file, (char*)charset, i);

	OS_Free(charset);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void CopyHexBlock(EDIT_FILE*file)
{
	int from, to, len;

	if (file->copyStatus&COPY_ON) {
		from = MIN(file->copyFrom.line, file->copyTo.line);
		to = MAX(file->copyFrom.line, file->copyTo.line);

		len = (to - from) + 1;

		if (to >= file->number_lines)
			len--;

		if (len) {
			if (hexClipboard)
				OS_Free(hexClipboard);

			hexClipboard = OS_Malloc(len);
			memcpy(hexClipboard, &file->hexData[from], len);
			hexClipboardLen = len;
			CenterBottomBar(1, "[+] Block saved to Clipboard [+]");
		}
		CancelSelectBlock(file);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteHexBlock(EDIT_FILE*file)
{
	int from, to, len;

	from = MIN(file->copyFrom.line, file->copyTo.line);
	to = MAX(file->copyFrom.line, file->copyTo.line);

	len = (to - from) + 1;

	if (to >= file->number_lines)
		len--;

	if (len) {
		SaveUndo(file, UNDO_CURSOR, 0);

		GotoHex(file, from);

		DeleteHexBytes(file, len);
		file->copyStatus = 0;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int HexCopyCheck(EDIT_FILE*file, int offset)
{
	int from, to;

	from = MIN(file->copyFrom.line, file->copyTo.line);
	to = MAX(file->copyFrom.line, file->copyTo.line);

	if ((offset >= from) && (offset <= to))
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DeleteKey(EDIT_FILE*file)
{
	if (file->hexMode == HEX_MODE_HEX) {
		if (file->cursor.xpos&0x01)
			CursorHexLeft(file);
	}

	if ((file->cursor.line_number) < file->number_lines) {
		UndoBegin(file);
		DeleteHex(file);

		if ((file->cursor.line_number) == file->number_lines) {
			CursorHexLeft(file);
			file->cursor.xpos &= 0xFFF0;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DeleteHex(EDIT_FILE*file)
{
	if ((file->cursor.line_number) < file->number_lines) {
		if (file->hexMode == HEX_MODE_HEX) {
			if (file->cursor.xpos&0x01)
				CursorHexLeft(file);
		}

		DeleteHexBytes(file, 1);
		file->paint_flags |= CONTENT_FLAG | FRAME_FLAG;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OverstrikeHexBytes(EDIT_FILE*file, char*data, int len)
{
	int delta;
	int line_number;

	if (file->cursor.line_number + len > file->number_lines)
		delta = (file->cursor.line_number + len) - file->number_lines;
	else
		delta = 0;

	SaveUndo(file, UNDO_HEX_OVERSTRIKE, len - delta);

	memcpy(&file->hexData[file->cursor.line_number], data, len - delta);

	if (delta) {
		line_number = file->cursor.line_number;

		file->cursor.line_number = file->number_lines;

		InsertHexBytes(file, &data[len - delta], delta);

		file->cursor.line_number = line_number;
	}

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InsertHexBytes(EDIT_FILE*file, char*data, int len)
{
	char*hexData;

	SaveUndo(file, UNDO_HEX_INSERT, len);

	hexData = OS_Malloc(file->number_lines + len);

	memcpy(hexData, file->hexData, file->cursor.line_number);

	memcpy(&hexData[file->cursor.line_number], data, len);

	memcpy(&hexData[file->cursor.line_number + len],
	    &file->hexData[file->cursor.line_number], (file->number_lines - (file->
	    cursor.line_number)));

	file->number_lines += len;

	if (file->hexData)
		OS_Free(file->hexData);

	file->hexData = hexData;

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DeleteHexLine(EDIT_FILE*file)
{
	int line_number, xpos;

	line_number = file->cursor.line_number;
	xpos = file->cursor.xpos;

	file->cursor.line_number = HEX_ROUND(file->cursor.line_number, file);
	file->cursor.xpos = 0;

	DeleteHexBytes(file, file->hex_columns);

	file->cursor.line_number = line_number;
	file->cursor.xpos = xpos;

	SetHexXPos(file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteHexBytes(EDIT_FILE*file, int len)
{
	if ((file->cursor.line_number + len) > file->number_lines)
		len = file->number_lines - file->cursor.line_number;

	if (!len)
		return ;

	SaveUndo(file, UNDO_HEX_DELETE, len);

	memmove(&file->hexData[file->cursor.line_number], &file->hexData[file->
	    cursor.line_number + len], (file->number_lines - (file->cursor.
	    line_number)) - len);

	file->number_lines -= len;

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void GotoHexAddress(EDIT_FILE*file)
{
	unsigned int address = 0;
	int len, i = 0;
	char offset = 0;

	char search[32];

	strcpy(search, "");
	Input(HISTORY_GOTOADDRESS, "Goto Address or Offset [(+)(-)Offset]:", search,
	    sizeof(search));

	file->paint_flags |= CURSOR_FLAG;

	len = strlen(search);

	if (len) {
		if (search[0] == '-' || search[0] == '+') {
			offset = search[0];
			i++;
		}

		if (search[i] == '0' && (search[i + 1] == 'x' || search[i + 1] == 'X'))
			sscanf(&search[i + 2], "%x", &address);
		else
			address = atol(&search[i]);

		if (offset) {
			if (offset == '+')
				address = file->cursor.line_number + address;
			else {
				if (address > (unsigned int)file->cursor.line_number)
					address = 0;
				else
					address =
					(file->cursor.line_number) - address;
			}
		}

		if (address > (unsigned int)file->number_lines)
			address = file->number_lines;

		GotoHex(file, address);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void GotoHex(EDIT_FILE*file, long address)
{
	int i;

	file->cursor.line_number = address;

	file->display.line_number = HEX_ROUND(file->cursor.line_number, file);

	for (i = 0; i < (file->display.rows) / 2; i++)
		HexDisplayUp(file);

	while (DisplayHexLimit(file, file->display.line_number)) {
		if (!HexDisplayUp(file))
			break;
	}

	file->cursor.xpos = GetXPos(file, address) << 4;

	SetHexYPos(file);

	AdjustHexDisplayPan(file);

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorHexPgup(EDIT_FILE*file)
{
	int i;

	for (i = 0; i < file->display.rows; i++)
		if (!LineHexUp(file))
			return (0);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorHexPgdn(EDIT_FILE*file)
{
	int i;

	for (i = 0; i < file->display.rows; i++) {
		if (!LineHexDown(file))
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
void CursorHexEnd(EDIT_FILE*file)
{
	file->end_count++;

	switch (file->end_count) {
	case 1 :
		file->cursor.line_number =
		HEX_ROUND(file->cursor.line_number, file) +
		(file->hex_columns - 1);

		if (file->cursor.line_number > file->number_lines)
			file->cursor.line_number = file->number_lines;

		SetHexXPos(file);
		break;
	case 2 :
		file->cursor.line_number = HEX_ROUND(file->display.line_number + (file->
		    display.rows - 1)*file->hex_columns, file);

		if (file->cursor.line_number >= file->number_lines)
			file->cursor.line_number =
			HEX_ROUND(file->number_lines - 1, file);

		file->cursor.xpos = 0;
		SetHexYPos(file);
		AdjustHexDisplayPan(file);
		break;
	case 3 :
		GotoHex(file, file->number_lines);
		file->end_count = 0;
		break;
	}
	file->paint_flags |= CURSOR_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CursorHexHome(EDIT_FILE*file)
{
	file->home_count++;

	switch (file->home_count) {
	case 1 :
		file->cursor.line_number = HEX_ROUND(file->cursor.line_number, file);
		file->cursor.xpos = 0;
		break;
	case 2 :
		file->cursor.line_number = file->display.line_number;
		file->cursor.xpos = 0;
		file->cursor.ypos = 0;
		break;
	case 3 :
		GotoHex(file, 0);
		file->home_count = 0;
		break;
	}
	AdjustHexDisplayPan(file);
	file->paint_flags |= CURSOR_FLAG;
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorHexLeft(EDIT_FILE*file)
{
	int xp, pos;

	xp = (file->cursor.xpos) >> 4;
	pos = (file->cursor.xpos&0xF);

	if (pos && file->hexMode == HEX_MODE_HEX)
		pos--;
	else {
		if (file->cursor.line_number >= 1)
			file->cursor.line_number -= 1;
		else
			return (0);

		if (file->cursor.line_number < file->display.line_number)
			HexDisplayUp(file);

		xp = GetXPos(file, file->cursor.line_number);

		pos = 1;
	}


	if (file->hexMode == HEX_MODE_TEXT)
		pos = 0;

	file->cursor.xpos = (xp << 4) | pos;

	SetHexYPos(file);

	AdjustHexDisplayPan(file);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorHexRight(EDIT_FILE*file)
{
	int xp, pos;

	xp = (file->cursor.xpos) >> 4;
	pos = (file->cursor.xpos&0x0F);

	if ((pos == 1) || file->hexMode == HEX_MODE_TEXT) {
		if ((file->cursor.line_number + 1) <= file->number_lines)
			file->cursor.line_number += 1;
		else
			return (0);

		xp = GetXPos(file, file->cursor.line_number);

		if (((file->cursor.line_number - file->display.line_number) / file->
		    hex_columns) >= file->display.rows)
			HexDisplayDown(file);

		pos = 0;
	} else
		pos++;

	file->cursor.xpos = (xp << 4) | pos;

	SetHexYPos(file);

	AdjustHexDisplayPan(file);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int LineHexUp(EDIT_FILE*file)
{
	if (!HexDisplayUp(file))
		return (0);

	HexCursorUp(file);

	SetHexYPos(file);

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int LineHexDown(EDIT_FILE*file)
{
	if (DisplayHexLimit(file, file->display.line_number + file->hex_columns))
		return (0);

	HexCursorDown(file);
	HexDisplayDown(file);
	SetHexYPos(file);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int DisplayHexLimit(EDIT_FILE*file, int address)
{
	if ((((address / file->hex_columns) + file->display.rows) - 2) >= (file->
	    number_lines / file->hex_columns))
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int HexDisplayUp(EDIT_FILE*file)
{
	if (file->display.line_number >= file->hex_columns) {
		file->display.line_number -= file->hex_columns;
		file->paint_flags |= CONTENT_FLAG | FRAME_FLAG;
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
static int HexDisplayDown(EDIT_FILE*file)
{
	file->display.line_number += file->hex_columns;
	file->paint_flags |= CONTENT_FLAG | FRAME_FLAG;
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int HexCursorUp(EDIT_FILE*file)
{
	if (file->cursor.line_number >= file->hex_columns) {
		file->cursor.line_number -= file->hex_columns;
		file->paint_flags |= CURSOR_FLAG;
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
static int HexCursorDown(EDIT_FILE*file)
{
	if (file->number_lines - file->cursor.line_number >= file->hex_columns) {
		file->cursor.line_number += file->hex_columns;
		file->paint_flags |= CURSOR_FLAG;
		return (1);
	} else {
		file->cursor.line_number = file->number_lines;
		SetHexXPos(file);
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorHexUp(EDIT_FILE*file)
{
	if (!HexCursorUp(file))
		return (0);

	if (file->cursor.line_number < file->display.line_number) {
		file->paint_flags |= CONTENT_FLAG | FRAME_FLAG;
		HexDisplayUp(file);
	}

	SetHexYPos(file);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CursorHexDown(EDIT_FILE*file)
{
	if (file->cursor.line_number >= file->number_lines)
		return (0);

	HexCursorDown(file);

	if (((file->cursor.line_number - file->display.line_number) / file->
	    hex_columns) >= file->display.rows) {
		if (!DisplayHexLimit(file,
		    file->display.line_number + file->hex_columns))
			HexDisplayDown(file);
	}

	SetHexYPos(file);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SetHexYPos(EDIT_FILE*file)
{
	file->cursor.ypos =
	(file->cursor.line_number - file->display.line_number) / file->hex_columns;

	file->paint_flags |= CURSOR_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GetXPos(EDIT_FILE*file, int address)
{
	return (address%file->hex_columns);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SetHexXPos(EDIT_FILE*file)
{
	if (file->cursor.line_number > file->number_lines)
		file->cursor.line_number = file->number_lines;

	file->cursor.xpos = (GetXPos(file, file->cursor.line_number) << 4);

	AdjustHexDisplayPan(file);

	file->paint_flags |= CURSOR_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ConvertToHex(char*hexString, char*hexData)
{
	int i, len = 0, size;
	unsigned int data;

	unsigned int data32;
	unsigned short data16;
	unsigned char data8;

	char w2[2];
	char w4[4];

	for (i = 0; i < (int)strlen(hexString); i++) {
		if (hexString[i] <= 32)
			continue;

		if (hexString[i] == '0' && (hexString[i + 1] == 'x' ||
		    hexString[i + 1] == 'X') && (hexString[i + 2]))
			i += 2;

		size = 0;
		data = 0;

		if (!sscanf(&hexString[i], "%x", &data))
			break;

		while (hexString[i] && hexString[i] > 32) {
			size++;
			i++;
		}

		data32 = (unsigned int)(data&0xFFFFFFFF);
		data16 = (unsigned short)(data&0xFFFF);
		data8 = (unsigned char)(data&0xFF);

		switch (hex_endian) {
		case HEX_SEARCH_DEFAULT :
			{
				memcpy(w4, &data32, sizeof(data32));
				memcpy(w2, &data16, sizeof(data16));
				break;
			}

		case HEX_SEARCH_BIG_ENDIAN :
			{
				w4[0] = (char)((data32&0xFF000000) >> 24);
				w4[1] = (char)((data32&0x00FF0000) >> 16);
				w4[2] = (char)((data32&0x0000FF00) >> 8);
				w4[3] = (char)((data32&0x000000FF));

				w2[0] = (char)((data16&0xFF00) >> 8);
				w2[1] = (char)((data16&0x00FF));
				break;
			}

		case HEX_SEARCH_LITTLE_ENDIAN :
			{
				w4[3] = (char)((data32&0xFF000000) >> 24);
				w4[2] = (char)((data32&0x00FF0000) >> 16);
				w4[1] = (char)((data32&0x0000FF00) >> 8);
				w4[0] = (char)((data32&0x000000FF));

				w2[1] = (char)((data16&0xFF00) >> 8);
				w2[0] = (char)((data16&0x00FF));
				break;
			}
		}

		if (data&0xFFFF0000 || size >= 5) {
			if (hexData)
				memcpy(&hexData[len], w4, 4);

			len += 4;
		} else
			if (data&0xFF00 || size >= 3) {
				if (hexData)
					memcpy(&hexData[len], w2, 2);

				len += 2;
			} else {
				if (hexData)
					memcpy(&hexData[len], &data8, 1);

				len++;
			}
	}
	return (len);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int MouseHexClick(EDIT_FILE*file)
{
	int xpos, ypos;
	int address;

	if ((file->mouse.xpos > file->display.xpos) && (file->mouse.xpos < (file->
	    display.xpos + file->display.xd - 1)) && (file->mouse.ypos > file->
	    display.ypos) &&
	    (file->mouse.ypos < (file->display.ypos + file->display.yd - 2))) {
		xpos = ((file->mouse.xpos - file->display.xpos) - 1) + file->display.
		    pan;
		ypos = (file->mouse.ypos - file->display.ypos) - 1;

		if ((xpos >= 12) && (xpos < (12 + file->hex_columns*3) + file->
		    hex_columns)) {
			file->cursor.line_number = HEX_ROUND(file->cursor.line_number,
			    file);
			file->cursor.xpos = 0;

			address = file->display.line_number + (ypos*file->hex_columns);

			while (file->cursor.line_number < address)
				if (!CursorHexDown(file))
					break;

			while (file->cursor.line_number > address)
				if (!CursorHexUp(file))
					break;

			if (xpos >= (12 + file->hex_columns*3)) {
				file->cursor.line_number += xpos - (12 + file->hex_columns*3);
				file->hexMode = HEX_MODE_TEXT;
				SetHexXPos(file);
			} else {
				file->cursor.line_number += (xpos - 12) / 3;
				file->hexMode = HEX_MODE_HEX;
				SetHexXPos(file);
			}

			file->paint_flags |= CURSOR_FLAG;
		}

		return (1);
	}

	if (file->mouse.xpos == file->display.xpos + file->display.xd - 1) {
		if (file->mouse.ypos == file->display.ypos + 1)
			LineHexUp(file);

		if (file->mouse.ypos == file->display.ypos + file->display.yd - 3)
			LineHexDown(file);
	}
	return (0);
}



