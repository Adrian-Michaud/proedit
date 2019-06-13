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
void InsertBlock(EDIT_CLIPBOARD*clipboard, EDIT_FILE*file)
{
	EDIT_LINE*line;
	CURSOR_SAVE save;
	int offset, number_lines = 0;
	int block, counter = 0;

	line = clipboard->lines;

	/* Special case if the file is a single line file. We only      */
	/* paste the first line from the clipboard and ignore the rest. */
	if (file->file_flags&FILE_FLAG_SINGLELINE) {
		if (line) {
			offset = InsertTabulate(file->cursor.offset, line->line, line->len);
			InsertText(file, line->line, line->len, 0);
			CursorOffset(file, file->cursor.offset + offset);
		}
		file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);
		file->copyStatus = 0;
		return ;
	}

	ProgressBar(0, 0, 0);

	block = clipboard->number_lines / 100;

	if (clipboard->status&COPY_SELECT) {
		while (line) {
			offset = InsertTabulate(file->cursor.offset, line->line, line->len);

			InsertText(file, line->line, line->len, CAN_INDENT);

			CursorOffset(file, file->cursor.offset + offset);

			if (line->next)
				CarrageReturn(file, NO_INDENT);

			line = line->next;

			if (block >= 100) {
				number_lines++;
				if (++counter > block) {
					counter = 0;
					ProgressBar("Pasting", number_lines,
					    clipboard->number_lines);
				}
			}
		}
	} else
		if (clipboard->status&COPY_LINE) {
			while (line) {
				SaveUndo(file, UNDO_INSERT_LINE, 0);
				InsertLine(file, line->line, line->len, INS_ABOVE_CURSOR, 0);
				CursorDown(file);
				line = line->next;

				if (block >= 100) {
					number_lines++;
					if (++counter > block) {
						counter = 0;
						ProgressBar("Pasting", number_lines, clipboard->
						    number_lines);
					}
				}
			}
		} else
			if (clipboard->status&COPY_BLOCK) {
				SaveCursor(file, &save);

				while (line) {
					if (!(WhitespaceLine(line) && (file->cursor.offset >=
					    file->cursor.line->len)))
						InsertText(file, line->line, line->len, CAN_INDENT);

					if (line->next) {
						if (!CursorDown(file)) {
							SaveUndo(file, UNDO_INSERT_LINE, 0);
							InsertLine(file, 0, 0, INS_BELOW_CURSOR, 0);
							CursorDown(file);
						}
					}
					line = line->next;

					if (block >= 100) {
						number_lines++;
						if (++counter > block) {
							counter = 0;
							ProgressBar("Pasting", number_lines, clipboard->
							    number_lines);
						}
					}
				}
				SetCursor(file, &save);
			}

	file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);

	file->copyStatus = 0;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteBlock(EDIT_FILE*file)
{
	int start, stop, x1, x2, len, line, status, cursor = 0;
	CURSOR_SAVE save;
	int number_lines = 0, num_lines;
	int block, counter = 0;

	start = MIN(file->copyFrom.line, file->copyTo.line);
	stop = MAX(file->copyFrom.line, file->copyTo.line);
	x1 = MIN(file->copyFrom.offset, file->copyTo.offset);
	x2 = MAX(file->copyFrom.offset, file->copyTo.offset);

	num_lines = stop - start;

	ProgressBar(0, 0, 0);

	block = num_lines / 100;

	status = file->copyStatus;

	/* Move the cursor so it looks nicer after the block has been deleted. */
	if (status&COPY_BLOCK)
		CursorOffset(file, x1);

	if ((file->copyFrom.line > file->copyTo.line) || (status&COPY_BLOCK)) {
		SaveCursor(file, &save);
		cursor = 1;
	}

	if (status&COPY_SELECT) {
		if (start == stop)
			status |= COPY_BLOCK;
		else {
			for (line = stop; line >= start; line--) {
				if (CursorLine(file, line))
					DeleteSelectLine(file, line);

				if (block >= 100) {
					number_lines++;
					if (++counter > block) {
						counter = 0;
						ProgressBar("Cutting", number_lines, num_lines);
					}
				}
			}
		}
	}

	if (status&COPY_LINE) {
		if (file->copyFrom.line < file->copyTo.line)
			stop--;

		for (line = stop; line >= start; line--) {
			if (CursorLine(file, line))
				DeleteLine(file);

			if (block >= 100) {
				number_lines++;
				if (++counter > block) {
					counter = 0;
					ProgressBar("Cutting", number_lines, num_lines);
				}
			}
		}
	}

	if (status&COPY_BLOCK) {
		len = (x2 - x1) + 1;

		if (status&COPY_SELECT)
			len--;

		if (len) {
			for (line = start; line <= stop; line++) {
				if (CursorLine(file, line)) {
					CursorOffset(file, x1);
					DeleteText(file, len, 0);
				}

				if (block >= 100) {
					number_lines++;
					if (++counter > block) {
						counter = 0;
						ProgressBar("Cutting", number_lines, num_lines);
					}
				}
			}
			CursorLine(file, file->copyTo.line);
		}
	}

	file->copyStatus = 0;

	if (cursor)
		SetCursor(file, &save);

	file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteSelectLine(EDIT_FILE*file, int line)
{
	int start, stop;

	start = MIN(file->copyFrom.line, file->copyTo.line);
	stop = MAX(file->copyFrom.line, file->copyTo.line);

	if (line < stop && line > start)
		DeleteLine(file);

	if (file->copyFrom.line < file->copyTo.line) {
		if (line == file->copyFrom.line) {
			CursorOffset(file, file->copyFrom.offset);
			DeleteText(file, file->cursor.line->len - file->copyFrom.offset, 0);
			DeleteCharacter(file, 0);
		}

		if (line == file->copyTo.line) {
			CursorOffset(file, 0);
			DeleteText(file, MIN(file->copyTo.offset, file->cursor.line->len),
			    0);
		}
	} else {
		if (line == file->copyFrom.line) {
			CursorOffset(file, 0);
			DeleteText(file, MIN(file->copyFrom.offset, file->cursor.line->len),
			    0);
		}

		if (line == file->copyTo.line) {
			CursorOffset(file, file->copyTo.offset);
			DeleteText(file, file->cursor.line->len - file->copyTo.offset, 0);
			DeleteCharacter(file, 0);
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SaveBlock(EDIT_CLIPBOARD*clipboard, EDIT_FILE*file)
{
	int start, stop, x1, x2, i, j, len = 0, line_len, status;
	EDIT_LINE*line;
	char*buffer;
	int number_lines = 0, num_lines;
	int block, counter = 0;

	ReleaseClipboard(clipboard);

	start = MIN(file->copyFrom.line, file->copyTo.line);
	stop = MAX(file->copyFrom.line, file->copyTo.line);
	x1 = MIN(file->copyFrom.offset, file->copyTo.offset);
	x2 = MAX(file->copyFrom.offset, file->copyTo.offset);

	num_lines = stop - start;

	block = num_lines / 100;

	status = file->copyStatus;

	if (start == stop && (status&COPY_SELECT))
		status |= COPY_BLOCK;

	if (status&COPY_LINE)
		if (file->copyFrom.line < file->copyTo.line)
			stop--;

	line = GetLine(file, start);

	for (i = start; i <= stop; i++) {
		if (status&COPY_BLOCK) {
			if (!line)
				line_len = 0;
			else
				line_len = line->len;

			len = (x2 - x1) + 1;

			if (status&COPY_SELECT)
				len--;

			if (len) {
				buffer = OS_Malloc(len);

				for (j = 0; j < len; j++) {
					if (x1 + j >= line_len)
						buffer[j] = 32;
					else
						buffer[j] = line->line[x1 + j];
				}

				AddClipboardLine(clipboard, buffer, len, status);
				OS_Free(buffer);
			}
		} else
			if ((status&COPY_SELECT) && line) {
				buffer = GetSelectLine(file, line, i, &len);
				if (buffer) {
					AddClipboardLine(clipboard, buffer, len, status);
					OS_Free(buffer);
				} else
					AddClipboardLine(clipboard, "", 0, status);
			} else
				if ((status&COPY_LINE) && line)
					AddClipboardLine(clipboard, line->line, line->len, status);

		line = line->next;

		if (block >= 100) {
			number_lines++;
			if (++counter > block) {
				counter = 0;
				ProgressBar("Copying", number_lines, num_lines);
			}
		}
	}

	AddGlobalClipboard(clipboard);

	file->paint_flags |= CURSOR_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char*GetSelectLine(EDIT_FILE*file, EDIT_LINE*line, int lineNo, int*len)
{
	int start, stop;
	char*ptr = 0, *buffer = 0;

	start = MIN(file->copyFrom.line, file->copyTo.line);
	stop = MAX(file->copyFrom.line, file->copyTo.line);

	if (lineNo > start && lineNo < stop) {
		*len = line->len;
		ptr = line->line;
	} else {
		if (file->copyFrom.line < file->copyTo.line) {
			if (lineNo == file->copyFrom.line) {
				if (file->copyFrom.offset < line->len) {
					*len = line->len - file->copyFrom.offset;
					ptr = &line->line[file->copyFrom.offset];
				}
			}

			if (lineNo == file->copyTo.line) {
				*len = MIN(file->copyTo.offset, line->len);
				ptr = line->line;
			}
		} else {
			if (lineNo == file->copyFrom.line) {
				*len = MIN(file->copyFrom.offset, line->len);
				ptr = line->line;
			}

			if (lineNo == file->copyTo.line) {
				if (file->copyTo.offset < line->len) {
					*len = line->len - file->copyTo.offset;
					ptr = &line->line[file->copyTo.offset];
				}
			}
		}
	}

	if (ptr) {
		buffer = OS_Malloc(*len);
		memcpy(buffer, ptr, *len);
	}

	return (buffer);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PasteSystemBlock(EDIT_FILE*file)
{
	char*sysClip, *tabLine;
	int len, i, index = 0, tablen;
	EDIT_CLIPBOARD clip;
	char*line;

	sysClip = OS_GetClipboard();

	if (!sysClip) {
		CenterBottomBar(1, "[-] System Clipboard is Empty [-]");
		return ;
	}

	InitClipboard(&clip);

	len = strlen(sysClip);

	line = OS_Malloc(len);

	for (i = 0; i <= len; i++) {
		if (sysClip[i] == ED_KEY_CR || sysClip[i] == ED_KEY_LF || sysClip[i] ==
		    0) {
			tabLine = TabulateString(line, index, &tablen);
			AddClipboardLine(&clip, tabLine, tablen, COPY_SELECT);
			OS_Free(tabLine);
			index = 0;
			continue;
		}
		line[index++] = sysClip[i];
	}

	OS_Free(line);

	UndoBegin(file);
	InsertBlock(&clip, file);

	ReleaseClipboard(&clip);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CopyBlock(EDIT_FILE*file, int type)
{
	if (file->copyStatus&COPY_ON)
		file->copyStatus = 0;
	else {
		file->copyStatus = COPY_ON | COPY_UPDATE | type;

		file->copyFrom.line = file->cursor.line_number;
		file->copyFrom.offset = file->cursor.offset;
	}

	file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetRegionFile(EDIT_FILE*file)
{
	file->copyStatus = COPY_ON | COPY_LINE;
	file->copyFrom.line = 0;
	file->copyFrom.offset = 0;

	file->copyTo.line = file->number_lines;
	file->copyTo.offset = 0;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CopyRegionUpdate(EDIT_FILE*file)
{
	if (file->copyStatus&COPY_UPDATE) {
		file->copyTo.line = file->cursor.line_number;
		file->copyTo.offset = file->cursor.offset;

		file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CopyCheck(EDIT_FILE*file, EDIT_LINE*line, int x, int y)
{
	int start, stop, x1, x2;

	x1 = MIN(file->copyFrom.offset, file->copyTo.offset);
	x2 = MAX(file->copyFrom.offset, file->copyTo.offset);
	start = MIN(file->copyFrom.line, file->copyTo.line);
	stop = MAX(file->copyFrom.line, file->copyTo.line);

	if (file->copyStatus&COPY_BLOCK) {
		if (y >= start && y <= stop) {
			/* Is the rightmost select anchor sitting on a TAB? If so, lets */
			/* Include all of the tab padding in the display selection.     */
			if (x2 < line->len && line->line[x2] == ED_KEY_TAB)
				while (((x2 + 1) < line->len) && (line->line[x2 + 1] ==
				    ED_KEY_TABPAD))
					x2++;

			if (x >= x1 && x <= x2)
				return (1);
			return (0);
		}
	}

	if (file->copyStatus&COPY_SELECT) {
		if (y >= start && y <= stop) {
			if (y > start && y < stop)
				return (1);

			if (start == stop) {
				if (x >= x1 && x < x2)
					return (1);
				return (0);
			}

			if (file->copyFrom.line < file->copyTo.line) {
				if (y == file->copyTo.line)
					if (x < file->copyTo.offset)
						return (1);

				if (y == file->copyFrom.line)
					if (x >= file->copyFrom.offset)
						return (1);
			} else {
				if (y == file->copyFrom.line)
					if (x < file->copyFrom.offset)
						return (1);

				if (y == file->copyTo.line)
					if (x >= file->copyTo.offset)
						return (1);
			}
		}
	}

	if (file->copyStatus&COPY_LINE) {
		if (y >= start && y <= stop) {
			if (y > start && y < stop)
				return (1);

			if (start == stop)
				return (1);

			if (file->copyFrom.line < file->copyTo.line) {
				if (y == file->copyFrom.line)
					return (1);
			} else {
				if (y == file->copyTo.line)
					return (1);
				if (y == file->copyFrom.line)
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
void PasteCharset(EDIT_FILE*file)
{
	EDIT_CLIPBOARD clipboard;
	char set[64];
	int rows, cols;
	int index = 0;

	UndoBegin(file);

	if (file->hexMode) {
		PasteHexCharset(file);
		return ;
	}

	memset(&clipboard, 0, sizeof(EDIT_CLIPBOARD));

	for (rows = 0; rows < 4; rows++) {
		for (cols = 0; cols < 64; cols++) {
			set[cols] = (char)index++;

			if (set[cols] == ED_KEY_BS || set[cols] == ED_KEY_TAB || set[cols]
			    == ED_KEY_TABPAD || set[cols] == ED_KEY_LF || set[cols] ==
			    ED_KEY_CR)
				set[cols] = 32;
		}

		AddClipboardLine(&clipboard, set, cols, COPY_LINE);
	}

	InsertBlock(&clipboard, file);

	ReleaseClipboard(&clipboard);
}



