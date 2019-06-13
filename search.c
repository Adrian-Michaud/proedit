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

static int ReplaceText(EDIT_FILE*file);
static int SearchLine(EDIT_FILE*file, char*source, char*dest, int destLen, int
    offset, int line);
static EDIT_FILE*SearchHexAgain(EDIT_FILE*file);

extern char last_search[MAX_SEARCH];
extern char last_replace[MAX_SEARCH];

extern int ignoreCase;
extern int globalSearch;
extern int searchReplace;
extern int globalSearchReplace;

static int num_replaced;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*SearchFile(EDIT_FILE*file)
{
	file->paint_flags |= CURSOR_FLAG;

	strcpy(last_search, "");

	if (file->hexMode == HEX_MODE_HEX)
		Input(HISTORY_HEX_SEARCH, "Hex Search [Example: 55 aa aa55 55aa55aa]:",
		    last_search, MAX_SEARCH);
	else
		Input(HISTORY_SEARCH, "Search:", last_search, MAX_SEARCH);

	if (!strlen(last_search))
		return (file);

	file = SearchAgain(file);

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*SearchReplace(EDIT_FILE*file)
{
	EDIT_FILE*newFile;

	file->paint_flags |= CURSOR_FLAG;

	strcpy(last_search, "");

	if (file->hexMode == HEX_MODE_HEX)
		Input(HISTORY_HEX_SEARCH, "Hex Search [Example: 55 aa aa55 55aa55aa]:",
		    last_search, MAX_SEARCH);
	else
		Input(HISTORY_SEARCH, "Search:", last_search, MAX_SEARCH);

	if (!strlen(last_search))
		return (file);

	strcpy(last_replace, "");

	if (file->hexMode == HEX_MODE_HEX) {
		if (!Input(HISTORY_HEX_REPLACE, "Hex Replace [Example: 55 aa 4e ff]:",
		    last_replace, MAX_SEARCH))
			return (file);
	} else {
		if (!Input(HISTORY_REPLACE, "Replace:", last_replace, MAX_SEARCH))
			return (file);
	}

	searchReplace = 1;

	newFile = SearchAgain(file);

	newFile->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	searchReplace = 0;
	globalSearchReplace = 0;
	num_replaced = 0;

	return (newFile);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*SearchHexAgain(EDIT_FILE*file)
{
	int hit = 0;

	if (strlen(last_search)) {
		for (; ; ) {
			if (SearchLine(file, last_search, file->hexData, file->number_lines,
			    file->cursor.line_number, 0)) {
				if (!searchReplace)
					return (file);

				if (!ReplaceText(file))
					return (file);

				hit = 1;
			} else
				break;
		}

		if (num_replaced)
			CenterBottomBar(1, "[+] %d occurrences replaced [+]", num_replaced);
		else {
			if (hit)
				CenterBottomBar(1, "[-] No more occurrences found [-]");
			else
				CenterBottomBar(1, "[-] Text/Expression not found [-]");
		}
	} else
		return (SearchFile(file));

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*SearchAgain(EDIT_FILE*file)
{
	EDIT_FILE*origin, *newFile;
	EDIT_LINE*lines;
	int len, line_number, offset, origin_line, wrapped = 0, hit = 0;

	if (file->hexMode)
		return (SearchHexAgain(file));

	origin = file;
	origin_line = file->cursor.line_number;

	len = strlen(last_search);

	lines = file->cursor.line;
	offset = file->cursor.offset;
	line_number = file->cursor.line_number;

	if (len) {
		for (; ; ) {
			while (lines) {
				if (lines->len >= len) {
					if (SearchLine(file, last_search, lines->line, lines->len,
					    offset, line_number)) {
						if (!searchReplace)
							return (file);

						if (!ReplaceText(file))
							return (file);

						hit = 1;
					}
				}

				if (wrapped) {
					if (line_number >= origin_line) {
						if (num_replaced) {
							CenterBottomBar(1,
							    "[+] %d occurrences replaced [+]",
							    num_replaced);
						} else {
							if (hit)
								CenterBottomBar(1,
								    "[-] No more occurrences found [-]");
							else
								CenterBottomBar(1,
								    "[-] Text/Expression not found [-]");
						}
						return (file);
					}
				}

				/* If we hit on this line, keep searching rest of line. */
				if (hit) {
					hit = 0;
					offset = file->cursor.offset;
				} else {
					/* Increase the line count. */
					line_number++;
					/* Move to the next line. */
					lines = lines->next;
					/* Start at the beginning of the line. */
					offset = 0;
				}
			}

			/* Is global file searching on? */
			if (!globalSearch)
				break;

			/* Get the next file. */
			newFile = NextFile(file);

			/* If next file is a Hex mode file, or a NONFILE, keep switching */
			/* until we get non-hex mode/valid file.                         */
			while (newFile->hexMode || (newFile->file_flags&FILE_FLAG_NONFILE))
				newFile = NextFile(newFile);

			/* Is there only one file loaded? */
			if (newFile == file)
				break;

			/* Have we wrapped back to the original file? */
			if (newFile == origin)
				wrapped = 1;

			/* Set file pointer to new file. */
			file = newFile;

			/* Start searching the new file from the top. */
			lines = file->lines;
			offset = 0;
			line_number = 0;
		}

		if (num_replaced)
			CenterBottomBar(1, "[+] %d occurrences replaced [+]", num_replaced);
		else {
			if (hit)
				CenterBottomBar(1, "[-] No more occurrences found [-]");
			else
				CenterBottomBar(1, "[-] Text/Expression not found [-]");
		}
	}

	return (origin);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ReplaceText(EDIT_FILE*file)
{
	int ch;
	int i, len;
	char*hexSearch;

	Paint(file);

	if (file->file_flags&FILE_FLAG_READONLY) {
		CenterBottomBar(1,
		    "[-] Aborted Search/Replace: This file is read-only [-]");
		file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
		return (0);
	}

	if (!globalSearchReplace) {
		file->paint_flags |= CURSOR_FLAG;

		for (; ; ) {
			ch = Question("Replace - [Y]es, [N]o, [G]lobal, [Q]uit:");

			if (ch == 'Q' || ch == 'q' || ch == ED_KEY_ESC) {
				CancelSelectBlock(file);
				file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
				return (0);
			}

			if (ch == 'y' || ch == 'Y')
				break;

			if (ch == 'G' || ch == 'g') {
				CenterBottomBar(0, "[+] Global Searching and Replacing... [+]");
				globalSearchReplace = 1;
				break;
			}

			if (ch == 'n' || ch == 'N') {
				CancelSelectBlock(file);
				file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
				return (1);
			}
		}
	}

	if (AbortRequest()) {
		file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
		return (0);
	}

	DeleteSelectBlock(file);

	num_replaced++;

	if (file->hexMode == HEX_MODE_TEXT) {
		InsertHexBytes(file, last_replace, strlen(last_replace));

		for (i = 0; i < (int)strlen(last_replace); i++)
			CursorRight(file);
	} else
		if (file->hexMode == HEX_MODE_HEX) {
			file->hexMode = HEX_MODE_TEXT;

			hexSearch = OS_Malloc(strlen(last_replace));

			len = ConvertToHex(last_replace, hexSearch);

			InsertHexBytes(file, hexSearch, len);

			for (i = 0; i < len; i++)
				CursorRight(file);

			OS_Free(hexSearch);

			file->hexMode = HEX_MODE_HEX;
		} else
			if (strlen(last_replace)) {
				InsertText(file, last_replace, strlen(last_replace),
				    CAN_WORDWRAP);

				for (i = 0; i < (int)strlen(last_replace); i++)
					CursorRight(file);
			}

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SearchLine(EDIT_FILE*file, char*source, char*dest, int destLen, int
    offset, int line)
{
	int i, j, len, pos, len2, retCode = 0;
	char c1, c2;
	char*hexSearch = 0;

	if (file->hexMode == HEX_MODE_HEX) {
		hexSearch = OS_Malloc(strlen(source));

		len = ConvertToHex(source, hexSearch);
		source = hexSearch;
	} else
		len = strlen(source);

	len2 = (destLen - len) + 1;

	for (i = offset; i < len2; i++) {
		pos = i;

		for (j = 0; j < len; j++) {
			c1 = source[j];

			c2 = dest[i + j];

			if (c1 == '?' && !file->hexMode)
				continue;

			if (c1 == c2) {
				if (c1 == ED_KEY_TAB && !file->hexMode) {
					while ((i < len2) && (dest[i + j + 1] == ED_KEY_TABPAD))
						i++;

					if (i == len2)
						break;
				}
				continue;
			}

			if (file->hexMode == HEX_MODE_HEX)
				break;

			if (!ignoreCase)
				break;

			if (c1 >= 'a' && c1 <= 'z')
				c1 -= 32;

			if (c2 >= 'a' && c1 <= 'z')
				c2 -= 32;

			if (c1 != c2)
				break;
		}

		if (j == len) {
			if (file->hexMode) {
				SetupHexSelectBlock(file, pos, len - 1);
				GotoHex(file, i + len);
			} else {
				SetupSelectBlock(file, line, pos, len + (i - pos));
				GotoPosition(file, line + 1, i + len + 1);
			}
			retCode = 1;
			break;
		}
	}

	if (hexSearch)
		OS_Free(hexSearch);

	return (retCode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ToggleCase(void)
{
	if (ignoreCase) {
		CenterBottomBar(1, "[+] Case Sensitivity On [+]");
		ignoreCase = 0;
	} else {
		CenterBottomBar(1, "[-] Case Sensitivity Off [-]");
		ignoreCase = 1;
	}

	SetConfigInt(CONFIG_INT_CASESENSITIVE, !ignoreCase);

	SaveConfig();

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ToggleFileSearch(void)
{
	if (globalSearch) {
		CenterBottomBar(1, "[-] Global File Searching OFF [-]");
		globalSearch = 0;
	} else {
		CenterBottomBar(1, "[+] Global File Searching ON [+]");
		globalSearch = 1;
	}

	SetConfigInt(CONFIG_INT_GLOBALFILE, globalSearch);

	SaveConfig();
}


