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
#include "cstyle.h"

extern int tabsize;

#define OPERATION_SPELL_CHECK        1
#define OPERATION_SORT_ALPHA         2
#define OPERATION_SORT_NUMERIC       3
#define OPERATION_SORT_BINARY        4
#define OPERATION_LEFT_JUSTIFY       5
#define OPERATION_RIGHT_JUSTIFY      6
#define OPERATION_CENTER             7
#define OPERATION_UPPERCASE          8
#define OPERATION_LOWERCASE          9
#define OPERATION_FORMAT_SUN         10
#define OPERATION_FORMAT_BSD         11
#define OPERATION_STRIP_WS           12
#define OPERATION_STRIP_ALL_WS       13
#define OPERATION_STRIP_TABS         14
#define OPERATION_CONVERT_TABS       15
#define OPERATION_RETAB              16
#define OPERATION_ASCII_HEX          17
#define OPERATION_ASCII_DEC          18
#define OPERATION_FORMAT_ADRIAN      19
#define OPERATION_DEL_WS_LINES       20
#define OPERATION_ASCII_STRINGS      21
#define OPERATION_UNDO_ALL           22
#define OPERATION_DEL_DUP_LINES      23

#define OPERATION_CONVERT_HEX_TO_BIN 24
#define OPERATION_CONVERT_HEX_TO_DEC 25
#define OPERATION_CONVERT_DEC_TO_HEX 26
#define OPERATION_CONVERT_DEC_TO_BIN 27
#define OPERATION_CONVERT_BIN_TO_HEX 28
#define OPERATION_CONVERT_BIN_TO_DEC 29


#define OP_ABORT    0
#define OP_NO_PASTE 1
#define OP_PASTE    2

typedef struct fileOps
{
	char*option;
	int id;
}FILE_OPS;

FILE_OPS fileOps[] =
{
	{"Spell Checker", OPERATION_SPELL_CHECK},
	{"Sort Alphabetically", OPERATION_SORT_ALPHA},
	{"Sort Numerically", OPERATION_SORT_NUMERIC},
	{"Sort Binary", OPERATION_SORT_BINARY},
	{"Align Left Justify", OPERATION_LEFT_JUSTIFY},
	{"Align Right Justify", OPERATION_RIGHT_JUSTIFY},
	{"Align Center", OPERATION_CENTER},
	{"Convert Tabs to Spaces", OPERATION_CONVERT_TABS},
	{"Re-tabulate", OPERATION_RETAB},
	{"Convert To Uppercase", OPERATION_UPPERCASE},
	{"Convert To Lowercase", OPERATION_LOWERCASE},
	{"Convert To ASCII Hex", OPERATION_ASCII_HEX},
	{"Convert To ASCII Decimal", OPERATION_ASCII_DEC},
	{"Convert To 'C' Strings", OPERATION_ASCII_STRINGS},
	{"Convert HEX to Binary", OPERATION_CONVERT_HEX_TO_BIN},
	{"Convert HEX to Decimal", OPERATION_CONVERT_HEX_TO_DEC},
	{"Convert Decimal to HEX", OPERATION_CONVERT_DEC_TO_HEX},
	{"Convert Decimal to Binary", OPERATION_CONVERT_DEC_TO_BIN},
	{"Convert Binary to HEX", OPERATION_CONVERT_BIN_TO_HEX},
	{"Convert Binary to Decimal", OPERATION_CONVERT_BIN_TO_DEC},
	{"Delete Whitespace Lines", OPERATION_DEL_WS_LINES},
	{"Delete Trailing Whitespace", OPERATION_STRIP_WS},
	{"Delete All Tabs", OPERATION_STRIP_TABS},
	{"Delete All Whitespace", OPERATION_STRIP_ALL_WS},
	{"Delete Duplicate Lines", OPERATION_DEL_DUP_LINES},
	{"Auto Format Sun C-Style", OPERATION_FORMAT_SUN},
	{"Auto Format BSD C-Style", OPERATION_FORMAT_BSD},
	{"Auto Format Adrian C-Style", OPERATION_FORMAT_ADRIAN},
	{"Undo All", OPERATION_UNDO_ALL},
};

#define NUMBER_OF_OPS (sizeof(fileOps)/sizeof(FILE_OPS))

static int DoOperation(EDIT_FILE*file, int op, EDIT_CLIPBOARD*clipboard);
static void SortAlphaNumeric(EDIT_CLIPBOARD*clipboard, int op);
static int SortAlphaFunc(const void*arg1, const void*arg2);
static int SortBinaryFunc(const void*arg1, const void*arg2);
static int SortNumericFunc(const void*arg1, const void*arg2);
static void ConvertHexDec(EDIT_CLIPBOARD*clipboard, int op);
static void ConvertBase(EDIT_CLIPBOARD*clipboard, int op);
static void ConvertStrings(EDIT_CLIPBOARD*clipboard);
static void Lowercase(EDIT_CLIPBOARD*clipboard);
static void StripWhitespace(EDIT_CLIPBOARD*clipboard);
static void StripAllWhitespace(EDIT_CLIPBOARD*clipboard);
static void Retabulate(EDIT_CLIPBOARD*clipboard);
static void StripWhitespaceLines(EDIT_CLIPBOARD*clipboard);
static void StripTabs(EDIT_CLIPBOARD*clipboard);
static void ConvertTabs(EDIT_CLIPBOARD*clipboard);
static void Uppercase(EDIT_CLIPBOARD*clipboard);
static void CenterJustify(EDIT_FILE*file, int op, EDIT_CLIPBOARD*clipboard);
static double GetNumber(char*dest, int maxlen, char*in, int maxin);
static int CStyle(EDIT_FILE*file, int op, EDIT_CLIPBOARD*clipboard);
static void DeleteDupLines(EDIT_CLIPBOARD*clipboard);
static int DupLine(EDIT_LINE*line1, EDIT_LINE*line2);
static unsigned long GetHex(char*string, int*bits);
static unsigned long GetDec(char*string, int*bits);
static unsigned long GetBin(char*string, int*bits);
static char*Convert2Bin(unsigned long num, int bits);
static char*Convert2Hex(unsigned long num, int bits);
static char*Convert2Dec(unsigned long num);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void Operation(EDIT_FILE*file)
{
	int line;
	int i, ch;
	char**ops;
	char*title;

	if (file->copyStatus&COPY_ON)
		title = "Select Operation for Highlighted Text";
	else
		title = "Select Operation for current File";

	ops = (char**)OS_Malloc(NUMBER_OF_OPS*sizeof(char*));

	for (i = 0; i < (int)NUMBER_OF_OPS; i++) {
		ops[i] = OS_Malloc(strlen(fileOps[i].option) + 1);
		strcpy(ops[i], fileOps[i].option);
	}

	line = PickList(title, NUMBER_OF_OPS, 60, title, ops, 1);

	for (i = 0; i < (int)NUMBER_OF_OPS; i++)
		OS_Free(ops[i]);

	OS_Free(ops);

	file->paint_flags |= CURSOR_FLAG;

	if (!line)
		return ;

	if (!(file->copyStatus&COPY_ON) && (NumberFiles(FILE_FLAG_NORMAL) > 1)) {
		for (; ; ) {
			ch = Question(
			    "Perform Operation to: [A]ll files, or [C]urrent file:");

			if (ch == ED_KEY_ESC)
				return ;

			if (ch == 'c' || ch == 'C')
				break;

			if (ch == 'a' || ch == 'A') {
				RunOperationAll(file, fileOps[line - 1].id);
				return ;
			}
		}
	}

	RunOperation(file, fileOps[line - 1].id);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int RunOperationAll(EDIT_FILE*file, int op)
{
	EDIT_FILE*base;

	base = file;

	for (; ; ) {
		/* Ignore HEX files for now. */
		if (!file->hexMode) {
			/* Ignore non-files. */
			if (!(file->file_flags&FILE_FLAG_NONFILE)) {
				if (RunOperation(file, op) == OP_ABORT)
					break;
			}
		}

		file = NextFile(file);

		if (file == base)
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
int RunOperation(EDIT_FILE*file, int op)
{
	int cursor = 0, row = 0, column = 0;
	EDIT_CLIPBOARD clip;
	int save_line, save_column;
	int retCode;

	switch (op) {
	case OPERATION_UNDO_ALL :
		{
			CancelSelectBlock(file);
			CenterBottomBar(0, "[+] One Moment, Undoing all changes [+]");

			while (file->numberUndos)
				Undo(file);

			file->paint_flags |= (CURSOR_FLAG | FRAME_FLAG | CONTENT_FLAG);

			return (OP_NO_PASTE);
		}
	}

	CenterBottomBar(0, "[+] One Moment... [+]");

	InitClipboard(&clip);

	save_line = file->cursor.line_number;
	save_column = file->cursor.offset;

	if (!(file->copyStatus&COPY_ON)) {
		SetRegionFile(file);
		CopyFileClipboard(file, &clip);
	} else {
		if (file->copyStatus&COPY_BLOCK) {
			row = MIN(file->copyFrom.line, file->copyTo.line);
			column = MIN(file->copyFrom.offset, file->copyTo.offset);
			cursor = 1;
		}

		SaveBlock(&clip, file);
	}

	retCode = DoOperation(file, op, &clip);

	if (retCode == OP_PASTE) {
		UndoBegin(file);
		SaveUndo(file, UNDO_CURSOR, 0);
		DeleteBlock(file);

		if (cursor) {
			while (file->cursor.offset > column)
				if (!CursorLeft(file))
					break;

			while (file->cursor.line_number > row)
				if (!CursorUp(file))
					break;
		}
		InsertBlock(&clip, file);
		GotoPosition(file, save_line + 1, save_column + 1);
	} else {
		file->copyStatus = 0;
		file->paint_flags |= CONTENT_FLAG;
	}

	ReleaseClipboard(&clip);

	file->paint_flags |= (CURSOR_FLAG | FRAME_FLAG | CONTENT_FLAG);

	return (retCode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int DoOperation(EDIT_FILE*file, int op, EDIT_CLIPBOARD*clipboard)
{
	switch (op) {
	case OPERATION_CONVERT_HEX_TO_BIN :
	case OPERATION_CONVERT_HEX_TO_DEC :
	case OPERATION_CONVERT_DEC_TO_HEX :
	case OPERATION_CONVERT_DEC_TO_BIN :
	case OPERATION_CONVERT_BIN_TO_HEX :
	case OPERATION_CONVERT_BIN_TO_DEC :
		{
			ConvertBase(clipboard, op);
			break;
		}

	case OPERATION_DEL_DUP_LINES :
		{
			DeleteDupLines(clipboard);
			break;
		}

	case OPERATION_DEL_WS_LINES :
		{
			StripWhitespaceLines(clipboard);
			break;
		}

	case OPERATION_SPELL_CHECK :
		{
			if (!SpellCheck(file, clipboard))
				return (OP_ABORT);
			else
				return (OP_NO_PASTE);
		}

	case OPERATION_STRIP_WS :
		{
			StripWhitespace(clipboard);
			break;
		}

	case OPERATION_STRIP_ALL_WS :
		{
			StripAllWhitespace(clipboard);
			break;
		}

	case OPERATION_STRIP_TABS :
		{
			StripTabs(clipboard);
			break;
		}

	case OPERATION_ASCII_STRINGS :
		{
			ConvertStrings(clipboard);
			break;
		}

	case OPERATION_RETAB :
		{
			Retabulate(clipboard);
			break;
		}

	case OPERATION_CONVERT_TABS :
		{
			ConvertTabs(clipboard);
			break;
		}

	case OPERATION_ASCII_HEX :
	case OPERATION_ASCII_DEC :
		{
			ConvertHexDec(clipboard, op);
			break;
		}

	case OPERATION_FORMAT_SUN :
	case OPERATION_FORMAT_ADRIAN :
	case OPERATION_FORMAT_BSD :
		{
			if (CStyle(file, op, clipboard))
				return (OP_PASTE);
			else
				return (OP_ABORT);
		}

	case OPERATION_LEFT_JUSTIFY :
	case OPERATION_CENTER :
	case OPERATION_RIGHT_JUSTIFY :
		{
			CenterJustify(file, op, clipboard);
			break;
		}

	case OPERATION_SORT_NUMERIC :
	case OPERATION_SORT_BINARY :
	case OPERATION_SORT_ALPHA :
		{
			SortAlphaNumeric(clipboard, op);
			break;
		}

	case OPERATION_UPPERCASE :
		{
			Uppercase(clipboard);
			break;
		}

	case OPERATION_LOWERCASE :
		{
			Lowercase(clipboard);
			break;
		}
	}

	return (OP_PASTE);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int CStyle(EDIT_FILE*file, int op, EDIT_CLIPBOARD*clipboard)
{
	EDIT_TOKEN*tokens;
	EDIT_CLIPBOARD newclip;
	int retCode = 0;

	file = file;
	tokens = TokenizeLines(file, clipboard->lines);

	if (!tokens)
		return (retCode);

	memset(&newclip, 0, sizeof(EDIT_CLIPBOARD));

	switch (op) {
	case OPERATION_FORMAT_SUN :
		retCode = SunStyle(file, tokens, &newclip);
		break;

	case OPERATION_FORMAT_ADRIAN :
		retCode = AdrianStyle(file, tokens, &newclip);
		break;

	case OPERATION_FORMAT_BSD :
		retCode = BsdStyle(file, tokens, &newclip);
		break;
	}

	if (retCode) {
		DeallocLines(clipboard->lines);
		memcpy(clipboard, &newclip, sizeof(EDIT_CLIPBOARD));
	} else
		DeallocLines(newclip.lines);

	DeallocTokens(tokens);

	return (retCode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void CenterJustify(EDIT_FILE*file, int op, EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int left, right, center = 0, newLen, len = 0;
	char*line;

	walk = clipboard->lines;

	while (walk) {
		if (walk->len > len)
			len = walk->len;
		walk = walk->next;
	}

	walk = clipboard->lines;

	if (clipboard->status&(COPY_LINE | COPY_SELECT))
		if (len < file->display.xd - 2)
			len = file->display.xd - 2;

	line = OS_Malloc(len);

	while (walk) {
		if (walk->len) {
			for (left = 0; left < walk->len; left++)
				if (walk->line[left] > 32)
					break;

			for (right = walk->len - 1; right >= 0; right--)
				if (walk->line[right] > 32)
					break;

			if (left <= right) {
				memset(line, 32, len);

				if (op == OPERATION_CENTER)
					center = (len / 2) - (((right - left) + 1) / 2);

				if (op == OPERATION_RIGHT_JUSTIFY)
					center = len - ((right - left) + 1);

				if (op == OPERATION_LEFT_JUSTIFY)
					center = 0;

				memcpy(&line[center], &walk->line[left], (right - left) + 1);

				if (clipboard->status&COPY_BLOCK)
					newLen = len;
				else
					newLen = center + (right - left) + 1;

				if (walk->allocSize < newLen) {
					OS_Free(walk->line);
					walk->line = OS_Malloc(newLen);
					walk->allocSize = newLen;
				}
				memcpy(walk->line, line, newLen);
				walk->len = newLen;
			}
		}
		walk = walk->next;
	}
	OS_Free(line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void Uppercase(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int i;

	walk = clipboard->lines;

	while (walk) {
		for (i = 0; i < walk->len; i++)
			if (walk->line[i] >= 'a' && walk->line[i] <= 'z')
				walk->line[i] -= 32;
		walk = walk->next;
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ConvertBase(EDIT_CLIPBOARD*clipboard, int op)
{
	EDIT_LINE*walk;
	char*newLine = 0, *line;
	unsigned long num = 0;
	int bits = 0;

	StripAllWhitespace(clipboard);

	walk = clipboard->lines;

	while (walk) {
		if (walk->len) {

			line = malloc(walk->len + 1);
			memcpy(line, walk->line, walk->len);
			line[walk->len] = 0;

			switch (op) {
			case OPERATION_CONVERT_HEX_TO_BIN :
			case OPERATION_CONVERT_HEX_TO_DEC :
				num = GetHex(line, &bits);
				break;

			case OPERATION_CONVERT_DEC_TO_HEX :
			case OPERATION_CONVERT_DEC_TO_BIN :
				num = GetDec(line, &bits);
				break;

			case OPERATION_CONVERT_BIN_TO_HEX :
			case OPERATION_CONVERT_BIN_TO_DEC :
				num = GetBin(line, &bits);
				break;
			}

			free(line);

			switch (op) {
			case OPERATION_CONVERT_HEX_TO_BIN :
			case OPERATION_CONVERT_DEC_TO_BIN :
				newLine = Convert2Bin(num, bits);
				break;

			case OPERATION_CONVERT_BIN_TO_HEX :
			case OPERATION_CONVERT_DEC_TO_HEX :
				newLine = Convert2Hex(num, bits);
				break;

			case OPERATION_CONVERT_HEX_TO_DEC :
			case OPERATION_CONVERT_BIN_TO_DEC :
				newLine = Convert2Dec(num);
				break;
			}

			OS_Free(walk->line);

			walk->line = newLine;
			walk->len = strlen(newLine);
			walk->allocSize = walk->len + 1;
		}

		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static unsigned long GetHex(char*string, int*bits)
{
	char convert[128];

	if (string[0] == '0' && (string[1] == 'x' || string[1] == 'X'))
		strcpy(convert, string);
	else
		sprintf(convert, "0x%s", string);

	*bits = (strlen(convert) - 2)*4;

	return (strtoul(convert, NULL, 0));
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static unsigned long GetDec(char*string, int*bits)
{
	unsigned long num;

	*bits = 0;

	num = strtoul(string, NULL, 0);

	if (num <= (unsigned long)0xF)
		*bits = 4;

	if (num > (unsigned long)0xF && num <= (unsigned long)0xFF)
		*bits = 8;

	if (num > (unsigned long)0xFF && num <= (unsigned long)0x00000FFF)
		*bits = 12;

	if (num > (unsigned long)0xFFF && num <= (unsigned long)0x0000FFFF)
		*bits = 16;

	if (num > (unsigned long)0xFFFF && num <= (unsigned long)0x000FFFFF)
		*bits = 20;

	if (num > (unsigned long)0xFFFFF && num <= (unsigned long)0x00FFFFFF)
		*bits = 24;

	if (num > (unsigned long)0xFFFFFF && num <= (unsigned long)0x0FFFFFFF)
		*bits = 28;

	if (num > (unsigned long)0xFFFFFFF && num <= (unsigned long)0xFFFFFFFF)
		*bits = 32;

	return (num);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static unsigned long GetBin(char*string, int*bits)
{
	unsigned long data = 0;
	int i, loopCounter;
	char convert[128];

	if (string[0] == '0' && (string[1] == 'b' || string[1] == 'B'))
		strcpy(convert, &string[2]);
	else
		strcpy(convert, string);

	for (loopCounter = 0, i = strlen(convert) - 1; i >= 0; i--, loopCounter++)
		if (convert[i] == '1')
			data |= (1 << loopCounter);

	*bits = strlen(convert);

	return (data);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*Convert2Bin(unsigned long num, int bits)
{
	char convert[128];
	int i;
	unsigned long mask;

	mask = (1 << (bits - 1));

	strcpy(convert, "");

	for (i = 0; i < bits; i++) {
		if ((mask >> i)&num)
			strcat(convert, "1");
		else
			strcat(convert, "0");
	}

	return (strdup(convert));

}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*Convert2Hex(unsigned long num, int bits)
{
	char convert[128];

	switch (bits) {
	case 4 :
		sprintf(convert, "0x%1lx", num);
		break;
	case 8 :
		sprintf(convert, "0x%02lx", num);
		break;
	case 12 :
		sprintf(convert, "0x%03lx", num);
		break;
	case 16 :
		sprintf(convert, "0x%04lx", num);
		break;
	case 20 :
		sprintf(convert, "0x%05lx", num);
		break;
	case 24 :
		sprintf(convert, "0x%06lx", num);
		break;
	case 28 :
		sprintf(convert, "0x%07lx", num);
		break;
	case 32 :
		sprintf(convert, "0x%08lx", num);
		break;
		default :
		sprintf(convert, "0x%lx", num);
		break;
	}

	return (strdup(convert));
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*Convert2Dec(unsigned long num)
{
	char convert[128];

	sprintf(convert, "%ld", num);

	return (strdup(convert));
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ConvertHexDec(EDIT_CLIPBOARD*clipboard, int op)
{
	EDIT_LINE*walk;
	int i;
	char*newLine;
	int newLen, actualLen;
	char val[32];

	walk = clipboard->lines;

	while (walk) {
		if (walk->len) {
			if (op == OPERATION_ASCII_HEX)
				newLen = walk->len*5;
			else
				newLen = walk->len*4;

			newLine = OS_Malloc(newLen);

			for (actualLen = 0, i = 0; i < walk->len; i++) {
				if (op == OPERATION_ASCII_HEX)
					sprintf(val, "0x%02x,", walk->line[i]&0xff);
				else
					sprintf(val, "%d,", walk->line[i]&0xff);
				memcpy(&newLine[actualLen], val, strlen(val));
				actualLen += strlen(val);
			}
			actualLen--;

			OS_Free(walk->line);

			walk->line = newLine;
			walk->len = actualLen;
			walk->allocSize = newLen;
		}

		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void Lowercase(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int i;

	walk = clipboard->lines;

	while (walk) {
		for (i = 0; i < walk->len; i++)
			if (walk->line[i] >= 'A' && walk->line[i] <= 'Z')
				walk->line[i] += 32;
		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void StripWhitespace(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;

	walk = clipboard->lines;

	while (walk) {
		StripLinePadding(walk);
		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void StripAllWhitespace(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int i, newLen;

	walk = clipboard->lines;

	while (walk) {
		for (newLen = 0, i = 0; i < walk->len; i++) {
			if (walk->line[i] <= ED_KEY_SPACE)
				continue;

			walk->line[newLen++] = walk->line[i];
		}
		walk->len = newLen;

		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DeleteDupLines(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk, *next, *find;

	walk = clipboard->lines;

	while (walk) {
		find = walk->next;

		while (find) {
			if (DupLine(walk, find))
				break;

			find = find->next;
		}

		if (find) {
			next = walk->next;

			if (walk == clipboard->lines)
				clipboard->lines = next;

			if (walk->prev)
				walk->prev->next = walk->next;

			if (walk->next)
				walk->next->prev = walk->prev;

			if (walk->allocSize)
				OS_Free(walk->line);

			OS_Free(walk);

			walk = next;

			continue;
		}
		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int DupLine(EDIT_LINE*line1, EDIT_LINE*line2)
{
	int i;
	char ch1, ch2;

	if (!line1->next || !line2->next)
		return (0);

	if (line1->len != line2->len)
		return (0);

	for (i = 0; i < line1->len; i++) {
		ch1 = line1->line[i];
		ch2 = line2->line[i];

		if (ch1 >= 'A' && ch1 <= 'Z')
			ch1 += 32;

		if (ch2 >= 'A' && ch2 <= 'Z')
			ch2 += 32;

		if (ch1 != ch2)
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
static void StripWhitespaceLines(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk, *next;

	walk = clipboard->lines;

	while (walk) {
		if (!walk->len || WhitespaceLine(walk)) {
			next = walk->next;

			if (walk->prev)
				walk->prev->next = walk->next;

			if (walk->next)
				walk->next->prev = walk->prev;

			if (walk->allocSize)
				OS_Free(walk->line);

			OS_Free(walk);

			walk = next;

			continue;
		}
		walk = walk->next;
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void Retabulate(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int i;
	int offset;
	int tabs;

	walk = clipboard->lines;

	while (walk) {
		offset = 0;
		for (i = 0; i < walk->len; i++) {
			if (walk->line[i] > ED_KEY_SPACE)
				break;
			if (walk->line[i] == ED_KEY_TAB)
				offset += tabsize;
			if (walk->line[i] == ED_KEY_SPACE)
				offset++;
		}
		tabs = offset / tabsize;

		if (tabs && ((tabs*tabsize) == offset) && (i > tabs)) {
			memset(walk->line, ED_KEY_TAB, tabs);
			memcpy(&walk->line[tabs], &walk->line[i], walk->len - i);
			walk->len = tabs + (walk->len - i);
		}
		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ConvertTabs(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int i;

	walk = clipboard->lines;

	while (walk) {
		for (i = 0; i < walk->len; i++) {
			if (walk->line[i] == ED_KEY_TAB || walk->line[i] == ED_KEY_TABPAD)
				walk->line[i] = ED_KEY_SPACE;
		}

		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ConvertStrings(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int i, len, index;
	char*newLine;

	walk = clipboard->lines;

	while (walk) {

		len = walk->len + 3;

		for (i = 0; i < walk->len; i++) {
			if (walk->line[i] == '"')
				len++;
		}

		newLine = OS_Malloc(len);

		index = 0;
		newLine[index++] = '"';

		/* Skip trailing space. */
		while (walk->len && (walk->line[walk->len - 1] <= ED_KEY_SPACE))
			walk->len--;

		/* Skip leading space. */
		for (i = 0; i < walk->len; i++)
			if (walk->line[i] > ED_KEY_SPACE)
				break;

		for (i = i; i < walk->len; i++) {
			if (walk->line[i] == '"')
				newLine[index++] = '\\';
			newLine[index++] = walk->line[i];
		}

		newLine[index++] = '"';
		newLine[index++] = ',';

		if (walk->line)
			OS_Free(walk->line);

		walk->len = index;
		walk->line = newLine;
		walk->allocSize = len;

		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void StripTabs(EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int i, newLen;

	walk = clipboard->lines;

	while (walk) {
		for (newLen = 0, i = 0; i < walk->len; i++) {
			if (walk->line[i] == ED_KEY_TAB || walk->line[i] == ED_KEY_TABPAD)
				continue;

			walk->line[newLen++] = walk->line[i];
		}
		walk->len = newLen;

		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SortAlphaNumeric(EDIT_CLIPBOARD*clipboard, int op)
{
	EDIT_LINE*walk;
	int number_lines = 0, index;
	EDIT_LINE*lines;

	walk = clipboard->lines;

	while (walk) {
		number_lines++;
		walk = walk->next;
	}

	lines = (EDIT_LINE*)OS_Malloc(number_lines*sizeof(EDIT_LINE));

	walk = clipboard->lines;

	index = 0;
	while (walk) {
		memcpy(&lines[index++], walk, sizeof(EDIT_LINE));
		walk = walk->next;
	}

	if (op == OPERATION_SORT_NUMERIC)
		qsort(lines, number_lines, sizeof(EDIT_LINE), SortNumericFunc);

	if (op == OPERATION_SORT_ALPHA)
		qsort(lines, number_lines, sizeof(EDIT_LINE), SortAlphaFunc);

	if (op == OPERATION_SORT_BINARY)
		qsort(lines, number_lines, sizeof(EDIT_LINE), SortBinaryFunc);

	walk = clipboard->lines;

	index = 0;
	while (walk) {
		walk->line = lines[index].line;
		walk->len = lines[index].len;
		walk->allocSize = lines[index].allocSize;
		walk->flags = lines[index].flags;
		index++;
		walk = walk->next;
	}

	OS_Free(lines);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SortAlphaFunc(const void*arg1, const void*arg2)
{
	EDIT_LINE*line1 = (EDIT_LINE*)arg1;
	EDIT_LINE*line2 = (EDIT_LINE*)arg2;
	char ch1 = 0, ch2 = 0;
	int i1, i2, index1 = 0, index2 = 0;

	for (; ; ) {
		for (i1 = index1; i1 < line1->len; i1++) {
			ch1 = line1->line[index1++];
			if (ch1 >= 'a' && ch1 <= 'z') {
				ch1 -= 32;
				break;
			}
			if (ch1 >= 'A' && ch1 <= 'Z')
				break;
		}

		for (i2 = index2; i2 < line2->len; i2++) {
			ch2 = line2->line[index2++];
			if (ch2 >= 'a' && ch2 <= 'z') {
				ch2 -= 32;
				break;
			}
			if (ch2 >= 'A' && ch2 <= 'Z')
				break;
		}

		if (i1 >= line1->len && i2 < line2->len)
			return (1);

		if (i1 < line1->len && i2 >= line2->len)
			return (-1);

		if (i1 >= line1->len && i2 >= line2->len)
			break;

		if (ch1 != ch2)
			return (ch1 - ch2);
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SortBinaryFunc(const void*arg1, const void*arg2)
{
	EDIT_LINE*line1 = (EDIT_LINE*)arg1;
	EDIT_LINE*line2 = (EDIT_LINE*)arg2;
	int i;

	for (i = 0; i < line1->len && i < line2->len; i++) {
		if (line1->line[i] != line2->line[i])
			return (line1->line[i]-line2->line[i]);
	}
	return (line1->len - line2->len);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SortNumericFunc(const void*arg1, const void*arg2)
{
	EDIT_LINE*line1 = (EDIT_LINE*)arg1;
	EDIT_LINE*line2 = (EDIT_LINE*)arg2;
	double value1 = 0.00, value2 = 0.00;
	char num[64];
	int i;

	for (i = 0; i < line1->len; i++) {
		if (line1->line[i] >= '0' && line1->line[i] <= '9') {
			value1 = GetNumber(num, sizeof(num), &line1->line[i],
			    line1->len - i);
			break;
		}
	}

	for (i = 0; i < line2->len; i++) {
		if (line2->line[i] >= '0' && line2->line[i] <= '9') {
			value2 = GetNumber(num, sizeof(num), &line2->line[i],
			    line2->len - i);
			break;
		}
	}

	if (value1 < value2)
		return (-1);
	if (value1 > value2)
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static double GetNumber(char*dest, int maxlen, char*in, int maxin)
{
	int i, index = 0;
	int hex = 0;
	int floating = 0;
	double result;
	unsigned long intVal = 0;

	if ((maxin > 1) && (in[0] == '0') && (in[1] == 'x' || in[1] == 'X'))
		hex = 2;

	for (i = hex; i < maxin; i++) {
		if (hex) {
			if (!((in[i] >= '0' && in[i] <= '9') || (in[i] >= 'A' && in[i] <=
			    'F') || (in[i] >= 'a' && in[i] <= 'f')))
				break;
		} else
			if (in[i] == ',')
				continue;
			else
				if (in[i] == '.') {
					if (floating)
						break;
					floating = 1;
				} else
					if (!(in[i] >= '0' && in[i] <= '9'))
						break;

		dest[index++] = in[i];

		if (index >= maxlen - 1)
			break;
	}

	dest[index] = 0;

	if (hex) {
		scanf(dest, "%lx", &intVal);
		result = (double)intVal;
	} else
		if (floating)
			result = atof(dest);
		else {
			intVal = atol(dest);
			result = (double)intVal;
		}

	return (result);
}


