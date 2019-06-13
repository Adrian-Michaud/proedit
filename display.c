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

extern int colorizing;
extern int force_rows;
extern int force_cols;
extern int ignoreCase;
extern int globalSearch;

SCR_PTR*screen;
int screenXDIM;
int screenYDIM;
int pendingStatus = 0;
int clockEnabled = 0;

static int paintEnable = 1;
static int bottomBarEnable = 1;

unsigned char defaultColorTable[] =
{COLOR1, COLOR2, COLOR3, COLOR4, COLOR5, COLOR6, COLOR7, COLOR8, COLOR9};

unsigned char defaultColorTableBorder[] =
{COLOR2, COLOR1, COLOR3, COLOR4, COLOR4, COLOR4, COLOR4, COLOR4, COLOR9};

unsigned char defaultColorTableSelect[] =
{COLOR9, COLOR9, COLOR9, COLOR9, COLOR9, COLOR9, COLOR9, COLOR9, COLOR4};

static SCR_PTR color_highlight;
static SCR_PTR color_diff1;
static SCR_PTR color_diff2;
static SCR_PTR color_bottom;
static SCR_PTR color_bookmark;

static void DrawBottomBar(char*text);

static void UpdateClockPfn(void);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int GetScreenYDim(void)
{
	return (screenYDIM);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int GetScreenXDim(void)
{
	return (screenXDIM);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int InitDisplay(void)
{
	int x, y;
	int cols, rows;
	int defColor;

	defColor = GetConfigInt(CONFIG_INT_FG_COLOR) |
	GetConfigInt(CONFIG_INT_BG_COLOR);

	if (force_cols)
		cols = force_cols;
	else
		cols = GetConfigInt(CONFIG_INT_DEFAULT_COLS);

	if (force_rows)
		rows = force_rows;
	else
		rows = GetConfigInt(CONFIG_INT_DEFAULT_ROWS);

	if (!OS_InitScreen(cols + 2, rows + 3)) /* Add extra for borders... */
		return (0);

	if (!OS_ScreenSize(&screenXDIM, &screenYDIM))
		return (0);

	screen = OS_Malloc(screenXDIM*screenYDIM*SCR_PTR_SIZE);

	for (x = 0; x < screenXDIM; x++)
		for (y = 0; y < screenYDIM; y++) {
			screen[y*screenXDIM*2 + x*2] = ED_KEY_SPACE;
			screen[y*screenXDIM*2 + x*2 + 1] = (SCR_PTR)defColor;
		}

	OS_PaintScreen((char*)screen);

	OS_SetClockPfn(UpdateClockPfn);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char*SaveScreen(void)
{
	char*scr;

	scr = OS_Malloc(screenXDIM*screenYDIM*SCR_PTR_SIZE);
	memcpy(scr, screen, screenXDIM*screenYDIM*SCR_PTR_SIZE);
	return (scr);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void RestoreScreen(char*scr)
{
	memcpy(screen, scr, screenXDIM*screenYDIM*SCR_PTR_SIZE);
	OS_Free(scr);
	if (paintEnable)
		OS_PaintScreen((char*)screen);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CloseDisplay(void)
{
	OS_Free(screen);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InitDisplayFile(EDIT_FILE*file)
{
	file->paint_flags = CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
	file->colorize = ConfigColorize(file, file->pathname);
	file->scrollbar = 1;
	file->display.top_line = file->lines;
	file->display.line_number = 0;
	file->display.xpos = 0;
	file->display.ypos = 0;
	file->display.xd = screenXDIM;
	file->display.yd = screenYDIM;
	file->display.pan = 0;
	file->display.columns = screenXDIM - 2;
	file->display.rows = screenYDIM - 3;

	if (file->hexMode)
		file->hex_columns = GetConfigInt(CONFIG_INT_HEX_COLS);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int DisableBottomBar(void)
{
	int bar;

	bar = bottomBarEnable;

	bottomBarEnable = 0;

	return (bar);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void EnableBottomBar(int bar)
{
	bottomBarEnable = bar;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DisplayBottomBar(char*fmt, ...)
{
	va_list args;
	char*text;

	if (!bottomBarEnable)
		return ;

	text = OS_Malloc(OS_MAX_SCREEN_XDIM);
	va_start(args, fmt);
	vsprintf(text, fmt, args);
	va_end(args);

	if (OS_DisplayBottomBar(text)) {
		OS_Free(text);
		return ;
	}

	DrawBottomBar(text);
	OS_Free(text);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DrawBottomBar(char*text)
{
	int len, i;

	len = strlen(text);

	for (i = 0; i < screenXDIM; i++) {
		if (i < len)
			screen[screenXDIM*2*(screenYDIM - 1) + i*2] = text[i];
		else
			screen[screenXDIM*2*(screenYDIM - 1) + i*2] = ED_KEY_SPACE;

		screen[screenXDIM*2*(screenYDIM - 1) + i*2 + 1] = color_bottom;
	}

	if (paintEnable)
		OS_PaintStatus((char*)screen);

	clockEnabled = 0;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void Paint(EDIT_FILE*file)
{

	if (file->copyStatus&COPY_ON)
		CopyRegionUpdate(file);

	if (file->paint_flags&FRAME_FLAG) {
		if (!(file->file_flags&FILE_FLAG_NOBORDER))
			PaintFrame(file);
	}

	if (file->paint_flags&CONTENT_FLAG)
		PaintContent(file);

	if (file->paint_flags&(FRAME_FLAG | CONTENT_FLAG)) {
		if (paintEnable) {
			if (file->file_flags&FILE_FLAG_EDIT_STATUS)
				OS_PaintStatus((char*)screen);
			else
				OS_PaintScreen((char*)screen);
		}
	}

	if (file->paint_flags&CURSOR_FLAG)
		PaintCursor(file);

	file->paint_flags = 0;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PaintCursor(EDIT_FILE*file)
{
	int xp, yp;
	int offset = 2;

	if (file->file_flags&FILE_FLAG_NOBORDER)
		offset = 1;

	if (file->hexMode&HEX_MODE_HEX)
		xp = (12 + file->display.xpos + ((file->cursor.xpos >> 4)*
		(3)) + offset + (file->cursor.xpos&0x0F)) -
		file->display.pan;
	else
		if (file->hexMode&HEX_MODE_TEXT)
			xp = (12 + file->hex_columns*(3) +
			file->display.xpos + (file->cursor.xpos >> 4) + offset) - file->
			    display.pan;
		else
			xp = file->display.xpos + file->cursor.xpos + offset;

	yp = file->display.ypos + file->cursor.ypos + offset;

	OS_SetCursorType(file->overstrike, file->hideCursor ? 0 : 1);

	OS_SetTextPosition(xp, yp);

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void HideCursor(void)
{
	OS_SetCursorType(0, 0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetupColors(void)
{
	color_highlight = (SCR_PTR)(GetConfigInt(CONFIG_INT_HIGHLIGHT_FG_COLOR) |
	GetConfigInt(CONFIG_INT_BG_COLOR));

	color_diff1 = (SCR_PTR)(GetConfigInt(CONFIG_INT_DIFF1_FG_COLOR) |
	GetConfigInt(CONFIG_INT_DIFF1_BG_COLOR));

	color_diff2 = (SCR_PTR)(GetConfigInt(CONFIG_INT_DIFF2_FG_COLOR) |
	GetConfigInt(CONFIG_INT_DIFF2_BG_COLOR));

	color_bookmark = (SCR_PTR)(GetConfigInt(CONFIG_INT_BOOKMARK_FG_COLOR) |
	GetConfigInt(CONFIG_INT_BOOKMARK_BG_COLOR));

	defaultColorTable[0] =
	(SCR_PTR)(GetConfigInt(CONFIG_INT_FG_COLOR) |
	GetConfigInt(CONFIG_INT_BG_COLOR));

	defaultColorTableBorder[0] =
	(SCR_PTR)(GetConfigInt(CONFIG_INT_BORDER_COLOR) |
	GetConfigInt(CONFIG_INT_BG_COLOR));

	defaultColorTableSelect[0] =
	(SCR_PTR)(GetConfigInt(CONFIG_INT_SELECT_FG_COLOR) |
	GetConfigInt(CONFIG_INT_SELECT_BG_COLOR));

	defaultColorTable[2] =
	(SCR_PTR)(GetConfigInt(CONFIG_INT_DLG_FG_COLOR) |
	GetConfigInt(CONFIG_INT_DLG_BG_COLOR));

	defaultColorTableBorder[2] =
	(SCR_PTR)(GetConfigInt(CONFIG_INT_DLG_FG_COLOR) |
	GetConfigInt(CONFIG_INT_DLG_BG_COLOR));

	defaultColorTableSelect[2] =
	(SCR_PTR)(GetConfigInt(CONFIG_INT_SELECT_FG_COLOR) |
	GetConfigInt(CONFIG_INT_SELECT_BG_COLOR));

	color_bottom = (SCR_PTR)(GetConfigInt(CONFIG_INT_BOTTOM_BG_COLOR) |
	GetConfigInt(CONFIG_INT_BOTTOM_FG_COLOR));

	SetupColorizers();

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PaintContent(EDIT_FILE*file)
{
	int xp, yp, xd, yd, column, pan, line;
	EDIT_LINE*cur_line;
	unsigned char attr;

	if (file->hexMode) {
		PaintHex(file);
		return ;
	}

	if (file->file_flags&FILE_FLAG_NOBORDER) {
		xp = file->display.xpos;
		yp = file->display.ypos;
	} else {
		xp = file->display.xpos + 1;
		yp = file->display.ypos + 1;
	}

	xd = file->display.columns;
	yd = file->display.rows;

	cur_line = file->display.top_line;
	pan = file->display.pan;

	for (line = 0; line < yd && cur_line; line++) {
		column = 0;

		if (pan < cur_line->len) {
			column = cur_line->len - pan;

			if (column > xd)
				column = xd;

			ProcessContent(file, cur_line, pan, column, line,
			    (xp + ((yp + line)*screenXDIM))*2);
		}

		/* PAD rest of the line out in the buffer. */
		while (column < xd) {
			if (cur_line->flags&LINE_FLAG_HIGHLIGHT)
				attr = color_highlight;
			else
				if (cur_line->flags&LINE_FLAG_DIFF1)
					attr = color_diff1;
				else
					if (cur_line->flags&LINE_FLAG_DIFF2)
						attr = color_diff2;
					else
						if (cur_line->flags&LINE_FLAG_BOOKMARK)
							attr = color_bookmark;
						else
							attr = defaultColorTable[file->client];

			if (file->copyStatus&COPY_ON)
				if (CopyCheck(file, cur_line, pan + column, file->display.
				    line_number + line))
					attr = defaultColorTableSelect[file->client];

			if (pan + column == cur_line->len && (file->display_special&
			    ED_SPECIAL_SPACE))
				screen[((xp + column) + ((yp + line)*screenXDIM))*2] = OS_Frame(
				    11);
			else
				screen[((xp + column) + ((yp + line)*screenXDIM))*2] =
				    ED_KEY_SPACE;

			screen[((xp + column) + ((yp + line)*screenXDIM))*2 + 1] = attr;
			column++;
		}

		cur_line = cur_line->next;
	}

	attr = defaultColorTable[file->client];

	/* PAD rest of page. */
	while (line < yd) {
		/* PAD rest of the line out in the buffer. */
		for (column = 0; column < xd; column++) {
			screen[((xp + column) + ((yp + line)*screenXDIM))*2] = ED_KEY_SPACE;
			screen[((xp + column) + ((yp + line)*screenXDIM))*2 + 1] = attr;
		}
		line++;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void HighlightLine(EDIT_FILE*file, EDIT_LINE*line, int highlight)
{
	if (highlight)
		line->flags |= LINE_FLAG_HIGHLIGHT;
	else
		line->flags &= ~LINE_FLAG_HIGHLIGHT;

	file->paint_flags |= CONTENT_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PaintHex(EDIT_FILE*file)
{
	static char temp2[64];
	int x, y, x2, len;
	int xp, yp, xd, yd;
	unsigned int curByte;
	char addressColor, hexColor, textColor;
	unsigned char*text;
	unsigned char*attrs;
	int size;
	int address_size;
	int text_offset;
	char byte[64];

	addressColor = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_HEXADDR_FG_COLOR));

	hexColor = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_HEXBYTE_FG_COLOR));

	textColor = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_HEXTEXT_FG_COLOR));

	sprintf(temp2, "0x%08x: ", (unsigned int)0);
	address_size = strlen(temp2);

	text_offset = address_size + (file->hex_columns*3);

	size = text_offset + file->hex_columns;

	text = OS_Malloc(size);
	attrs = OS_Malloc(size);

	if (file->file_flags&FILE_FLAG_NOBORDER) {
		xp = file->display.xpos;
		yp = file->display.ypos;
	} else {
		xp = file->display.xpos + 1;
		yp = file->display.ypos + 1;
	}

	xd = file->display.columns;
	yd = file->display.rows;

	curByte = file->display.line_number;

	for (y = 0; y < yd; y++) { /* Paint line-by-line     */
		memset(attrs, hexColor, size);
		memset(&attrs[text_offset], textColor, (size - text_offset));
		memset(text, 32, size);

		if (curByte <= (unsigned int)file->number_lines)
			sprintf((char*)text, "0x%08x: ", (unsigned int)curByte);

		/* 0x00000000: */
		len = address_size;

		memset(attrs, addressColor, len);

		/* xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx */
		for (x = 0; x < file->hex_columns; x++) {
			if ((curByte + x) < (unsigned int)file->number_lines) {
				sprintf(byte, "%02x",
				    (unsigned char)file->hexData[curByte + x]);
				text[len] = byte[0];
				text[len + 1] = byte[1];

				if ((file->copyStatus&COPY_ON) && HexCopyCheck(file,
				    curByte + x)) {
					attrs[len] = defaultColorTableSelect[file->client];
					attrs[len + 1] = defaultColorTableSelect[file->client];
				}
			}
			len += 3;
		}

		for (x = 0; x < file->hex_columns; x++) {
			if ((curByte + x) < (unsigned int)file->number_lines) {
				text[text_offset + x] = file->hexData[curByte + x];

				if ((file->copyStatus&COPY_ON) && HexCopyCheck(file,
				    curByte + x))
					attrs[text_offset + x] = defaultColorTableSelect[file->
					    client];
			}
		}

		for (x2 = 0; x2 < text_offset + x && x2 < xd; x2++) {
			if ((x2 + file->display.pan) < size) {
				screen[(yp + y)*screenXDIM*2 + (xp + x2)*2] =
				text[x2 + file->display.pan];

				screen[(yp + y)*screenXDIM*2 + (xp + x2)*2 + 1] =
				attrs[x2 + file->display.pan];
			} else
				break;
		}

		/* Clear rest of line */
		for (; x2 < xd; x2++) {
			screen[(yp + y)*screenXDIM*2 + (xp + x2)*2] = 32;
			screen[(yp + y)*screenXDIM*2 + (xp + x2)*2 + 1] =
			defaultColorTable[file->client];
		}

		if ((curByte + file->hex_columns) >= (unsigned int)file->number_lines)
			break;

		curByte += file->hex_columns;
	}

	/* Clear rest of window */
	for (y++; y < yd; y++) {
		for (x = 0; x < xd; x++) {
			screen[(yp + y)*screenXDIM*2 + (xp + x)*2] = 32;
			screen[(yp + y)*screenXDIM*2 + (xp + x)*2 + 1] =
			defaultColorTable[file->client];
		}
	}

	if (paintEnable)
		OS_PaintScreen((char*)screen);

	OS_Free(text);
	OS_Free(attrs);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void EnablePainting(int enable)
{
	paintEnable = enable;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ProcessContent(EDIT_FILE*file, EDIT_LINE*line, int pan, int len, int
    line_number, int address)
{
	int column, length, address2;
	char ch, attr = 0;

	length = pan + len;

	if (line->flags&LINE_FLAG_HIGHLIGHT)
		attr = color_highlight;
	else
		if (line->flags&LINE_FLAG_DIFF1)
			attr = color_diff1;
		else
			if (line->flags&LINE_FLAG_DIFF2)
				attr = color_diff2;
			else
				if (line->flags&LINE_FLAG_BOOKMARK)
					attr = color_bookmark;
				else
					if (file->colorize && colorizing)
						file->colorize(line, pan, len, screen, address + 1,
						    defaultColorTable[file->client]);
					else
						attr = defaultColorTable[file->client];

	if (attr) {
		address2 = address + 1;

		for (column = pan; column < length; column++) {
			screen[address2] = attr;
			address2 += 2;
		}
	}

	/* Write out paned line. */
	for (column = 0; column < length; column++) {
		ch = line->line[column];

		if (ch == ED_KEY_TAB && (file->display_special&ED_SPECIAL_TAB))
			ch = OS_Frame(10);

		if (ch == ED_KEY_SPACE && (file->display_special&ED_SPECIAL_SPACE))
			ch = OS_Frame(12);

		if (ch == ED_KEY_TAB || ch == ED_KEY_TABPAD)
			ch = ED_KEY_SPACE;

		if (column >= pan) {
			screen[address] = ch;

			if (file->copyStatus&COPY_ON)
				if (CopyCheck(file, line, column, file->display.line_number +
				    line_number))
					screen[address + 1] = defaultColorTableSelect[file->client];

			address += 2;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PaintFrame(EDIT_FILE*file)
{
	int i, xp, yp, len, xd, yd;
	char title[MAX_FILENAME];
	char*ptr;
	char*filename;

	xp = file->display.xpos;
	yp = file->display.ypos;
	xd = file->display.xd - 1;
	yd = file->display.rows;

	if (file->window_title)
		OS_SetWindowTitle(file->window_title);

	if (!(file->file_flags&FILE_FLAG_NONFILE)) {
		if (OS_PaintFrame(file->window_title, file->cursor.line_number, file->
		    number_lines))
			return ;
	}

	if (file->title)
		filename = file->title;
	else
		filename = file->pathname;

	/* A hack to make the listboxes look nicer. */
	if (filename[0] == '@')
		sprintf(title, "%s", filename);
	else
		sprintf(title, " %s ", filename);

	for (i = 0; i < (int)strlen(title); i++)
		if (title[i] == '@')
			title[i] = OS_Frame(1);

	if ((int)strlen(title) > xd - 4) {
		ptr = &title[strlen(title) - (xd - 4)];
		memcpy(ptr, " ...", 4);
	} else
		ptr = title;

	len = strlen(ptr);

	for (i = xp + 1; i < (xd / 2 - len / 2 + xp); i++) {
		screen[(i + (yp*screenXDIM))*2] = OS_Frame(1);
		screen[(i + (yp*screenXDIM))*2 + 1] = defaultColorTableBorder[file->
		    border];
	}

	for (i = xd / 2 - len / 2 + xp; i < xd / 2 - len / 2 + xp + len; i++) {
		screen[(i*2) + (yp*(((screenXDIM - 1) + 1)*2))] = ptr[i - (xd / 2 - len
		    / 2 + xp)];
		screen[(i*2) + (yp*(((screenXDIM - 1) + 1)*2)) + 1] =
		defaultColorTableBorder[file->border];
	}

	for (i = (xd / 2 - len / 2 + xp) + len; i < xp + xd; i++) {
		screen[(i + (yp*screenXDIM))*2] = OS_Frame(1);
		screen[(i + (yp*screenXDIM))*2 + 1] = defaultColorTableBorder[file->
		    border];
	}

	for (i = xp + 1; i < xp + xd; i++) {
		screen[(i + ((yp + yd + 1)*screenXDIM))*2] = OS_Frame(1);
		screen[(i + ((yp + yd + 1)*screenXDIM))*2 + 1] =
		defaultColorTableBorder[file->border];
	}

	for (i = yp + 1; i < yp + yd + 1; i++) {
		screen[i*(((screenXDIM - 1) + 1)*2) + xp*2] = OS_Frame(5);
		screen[i*(((screenXDIM - 1) + 1)*2) + xp*2 + 1] =
		defaultColorTableBorder[file->border];

		if (!file->scrollbar || yd <= 2) {
			screen[i*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2] = OS_Frame(5);
			screen[i*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2 + 1] =
			defaultColorTableBorder[file->border];
		}
	}

	if (file->scrollbar) {
		if (yd > 2) {
			for (i = yp + 2; i < yp + yd; i++) {
				screen[i*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2] = OS_Frame(
				    6);
				screen[i*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2 + 1] = COLOR4;
			}

			screen[(yp + 1)*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2] = OS_Frame
			    (8);
			screen[(yp + 1)*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2 + 1] =
			    COLOR4;

			screen[(yp + yd)*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2] =
			    OS_Frame(9);
			screen[(yp + yd)*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2 + 1] =
			    COLOR4;
		}
	}

	screen[(xp + (yp*screenXDIM))*2] = OS_Frame(0);
	screen[(xp + (yp*screenXDIM))*2 + 1] =
	defaultColorTableBorder[file->border];

	screen[(xp + xd + (yp*screenXDIM))*2] = OS_Frame(2);
	screen[(xp + xd + (yp*screenXDIM))*2 + 1] =
	defaultColorTableBorder[file->border];

	screen[(xp + ((yp + yd + 1)*screenXDIM))*2] = OS_Frame(3);
	screen[(xp + ((yp + yd + 1)*screenXDIM))*2 + 1] =
	defaultColorTableBorder[file->border];

	screen[(xp + xd + ((yp + yd + 1)*screenXDIM))*2] = OS_Frame(4);
	screen[(xp + xd + ((yp + yd + 1)*screenXDIM))*2 + 1] =
	defaultColorTableBorder[file->border];

	if (file->scrollbar) {
		if (yd > 2) {
			int len = yd - 2;
			int offset = len - 1;

			if (file->cursor.line_number <= file->number_lines && file->
			    number_lines) {
				offset = (int)(((double)file->cursor.line_number*
				((double)len / (double)file->number_lines)));

				if (offset > len - 1)
					offset = len - 1;
			}

			screen[(yp + offset + 2)*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2] =
			    OS_Frame(7);
			screen[(yp + offset + 2)*(((screenXDIM - 1) + 1)*2) + (xd + xp)*2 +
			    1] = COLOR4;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ProgressBar(char*title, int current, int total)
{
	int progress;
	static int last_current, last_progress;

	if (!title) {
		last_current = -1;
		last_progress = -1;
		HideCursor();
		return ;
	}

	if (current == last_current)
		return ;

	if (total)
		progress = (int)((((float)current / (float)total)*100));
	else
		progress = 0;

	if (progress != last_progress)
		CenterBottomBar(0, "[+] %s... %d%% [+]", title, progress);

	last_current = current;
	last_progress = progress;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CenterBottomBar(int pending, char*fmt, ...)
{
	int len, i;
	char*temp;
	va_list args;
	char*text;

	if (!bottomBarEnable)
		return ;

	text = OS_Malloc(OS_MAX_SCREEN_XDIM);
	va_start(args, fmt);
	vsprintf(text, fmt, args);
	va_end(args);

	if (!OS_CenterBottomBar(text)) {
		if ((int)strlen(text) < screenXDIM)
			len = (screenXDIM / 2) - (strlen(text) / 2);
		else
			len = 0;

		temp = OS_Malloc(OS_MAX_SCREEN_XDIM);
		strcpy(temp, "");

		for (i = 0; i < len; i++)
			strcat(temp, " ");

		strcat(temp, text);

		DrawBottomBar(temp);

		OS_Free(temp);
	}

	pendingStatus = pending;
	OS_Free(text);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void UpdateStatusBar(EDIT_FILE*file)
{
	char buffer[OS_MAX_SCREEN_XDIM];
	char time[OS_MAX_TIMEDATE], title[80 + MAX_FILENAME];
	char tm[OS_MAX_TIMEDATE];
	char fileInfo[80];
	char line[32], col[32], page[32], undo[32];

	if (pendingStatus) {
		pendingStatus = 0;
		return ;
	}

	memset(buffer, 32, screenXDIM);
	memset(fileInfo, 32, sizeof(fileInfo));

	sprintf(title, "%s  ", PROEDIT_MP_TITLE);
	memcpy(buffer, title, strlen(title));

	OS_Time(tm);
	sprintf(time, "%s", tm);
	memcpy(&buffer[screenXDIM - strlen(time)], time, strlen(time));

	sprintf(undo, "Undo:%d", file->userUndos);

	if (file->hexMode) {
		sprintf(line, "Address:0x%x", file->cursor.line_number);
		sprintf(col, "File Size:0x%x", file->number_lines);
		sprintf(page, "Col:%d",
		    (file->cursor.line_number%file->hex_columns) + 1);

		memcpy(fileInfo, line, strlen(line));
		memcpy(&fileInfo[19], col, strlen(col));
		memcpy(&fileInfo[40], page, strlen(page));
		strcpy(&fileInfo[49], undo);
	} else {
		sprintf(line, "Line:%d/%d", file->cursor.line_number + 1, file->
		    number_lines);
		sprintf(col, "Col:%d", file->cursor.offset + 1);
		sprintf(page, "Pg:%d/%d",
		    (file->cursor.line_number / file->display.rows) + 1, (file->
		    number_lines / file->display.rows) + 1);

		memcpy(fileInfo, line, strlen(line));
		memcpy(&fileInfo[22], col, strlen(col));
		memcpy(&fileInfo[34], page, strlen(page));
		strcpy(&fileInfo[49], undo);
	}

	memcpy(&buffer[strlen(title)], fileInfo, strlen(fileInfo));

	buffer[screenXDIM] = 0;
	DisplayBottomBar(buffer);

	clockEnabled = 1;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void UpdateClockPfn(void)
{
	char tm[OS_MAX_TIMEDATE];
	int i, len;

	if (pendingStatus || !clockEnabled)
		return ;

	OS_Time(tm);

	len = strlen(tm);

	for (i = 0; i < len; i++) {
		screen[screenXDIM*2*(screenYDIM - 1) + (i + screenXDIM - len)*2] = tm[i
		    ];
		screen[screenXDIM*2*(screenYDIM - 1) + (i + screenXDIM - len)*2 + 1] =
		    color_bottom;
	}

	if (paintEnable)

		OS_PaintStatus((char*)screen);
}



