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
void SelectBlockCheck(EDIT_FILE*file, int enabled)
{
	if (enabled) {
		if (!file->selectblock) {
			file->selectblock = 1;
			file->copyStatus = 0;
			CopyBlock(file, COPY_SELECT);
		}
	} else {
		if (file->selectblock) {
			file->selectblock = 0;
			file->copyStatus = 0;
			file->paint_flags |= CONTENT_FLAG;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteSelectBlock(EDIT_FILE*file)
{
	if (file->selectblock) {
		if (file->copyStatus&COPY_ON) {
			if (file->hexMode) {
				UndoBegin(file);
				DeleteHexBlock(file);
			} else {
				UndoBegin(file);
				DeleteBlock(file);
			}
		}
		file->selectblock = 0;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetupSelectBlock(EDIT_FILE*file, int line, int offset, int len)
{
	file->copyStatus = COPY_ON | COPY_SELECT;

	file->selectblock = 1;

	file->paint_flags |= CONTENT_FLAG;

	file->copyFrom.line = line;
	file->copyFrom.offset = offset;
	file->copyTo.line = line;
	file->copyTo.offset = offset + len;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetupHexSelectBlock(EDIT_FILE*file, int offset, int len)
{
	file->copyStatus = COPY_ON | COPY_SELECT;

	file->selectblock = 1;

	file->paint_flags |= CONTENT_FLAG;

	file->copyFrom.line = offset;
	file->copyFrom.offset = 0;
	file->copyTo.line = offset + len;
	file->copyTo.offset = 0;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CancelSelectBlock(EDIT_FILE*file)
{
	if (file->selectblock) {
		file->selectblock = 0;
		file->copyStatus = 0;
		file->paint_flags |= CONTENT_FLAG;
		Paint(file);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CopySelectBlock(EDIT_FILE*file)
{
	if (file->selectblock && (file->copyStatus&COPY_ON)) {
		SaveBlock(GetClipboard(), file);
		CancelSelectBlock(file);
		CenterBottomBar(1, "[+] Block saved to Clipboard [+]");
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PasteSelectBlock(EDIT_FILE*file)
{
	EDIT_CLIPBOARD*clipboard;

	clipboard = GetClipboard();

	DeleteSelectBlock(file);

	if (!clipboard->status)
		CenterBottomBar(1, "[-] Clipboard is Empty [-]");
	else {
		UndoBegin(file);
		InsertBlock(clipboard, file);
	}
}



