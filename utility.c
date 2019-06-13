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

typedef struct utilityOps
{
	char*option;
	int id;
}UTIL_OPS;

#define UTIL_CALCULATOR       1
#define UTIL_PASTE_CHARSET    2
#define UTIL_CLOSE_UNMODIFIED 3
#define UTIL_SAVE_MODIFIED    4
#define UTIL_SHOW_MODIFIED    5
#define UTIL_SHOW_UNMODIFIED  6
#define UTIL_EDIT_FILENAMES   7
#define UTIL_EDIT_DICTIONARY  8

UTIL_OPS utilOps[] =
{
	{"Calculator", UTIL_CALCULATOR},
	{"Paste Character Set", UTIL_PASTE_CHARSET},
	{"Close All Unmodified Files", UTIL_CLOSE_UNMODIFIED},
	{"Save All Modified Files", UTIL_SAVE_MODIFIED},
	{"Display All Unmodified Files", UTIL_SHOW_UNMODIFIED},
	{"Display All Modified Files", UTIL_SHOW_MODIFIED},
	{"Edit Custom Dictionary words", UTIL_EDIT_DICTIONARY},
	{"Edit All Loaded Filenames", UTIL_EDIT_FILENAMES},
};

#define NUMBER_OF_OPS (sizeof(utilOps)/sizeof(UTIL_OPS))

static EDIT_FILE*RunUtility(EDIT_FILE*file, int id);
static void EditFilenames(EDIT_FILE*file);
static EDIT_FILE*EditFilesInput(EDIT_FILE*file, int mode, int ch);
static void GetNewFilename(char*line, char*filename);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*Utilities(EDIT_FILE*file)
{
	int line;
	int i;
	char**ops;
	char*title;

	title = "General Utilities";

	ops = (char**)OS_Malloc(NUMBER_OF_OPS*sizeof(char*));

	for (i = 0; i < (int)NUMBER_OF_OPS; i++) {
		ops[i] = OS_Malloc(strlen(utilOps[i].option) + 1);
		strcpy(ops[i], utilOps[i].option);
	}

	line = PickList(title, NUMBER_OF_OPS, 60, title, ops, 1);

	for (i = 0; i < (int)NUMBER_OF_OPS; i++)
		OS_Free(ops[i]);

	OS_Free(ops);

	file->paint_flags |= CURSOR_FLAG;

	if (!line)
		return (file);

	return (RunUtility(file, utilOps[line - 1].id));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*RunUtility(EDIT_FILE*file, int id)
{
	char filename[MAX_FILENAME];

	switch (id) {
	case UTIL_CALCULATOR :
		Calculator(file);
		break;

	case UTIL_PASTE_CHARSET :
		PasteCharset(file);
		break;

	case UTIL_EDIT_DICTIONARY :
		{
			OS_GetSessionFile(filename, SESSION_WORDS, 0);

			file = LoadFileWildcard(file, filename, LOAD_FILE_EXISTS |
			    LOAD_FILE_NORMAL | LOAD_FILE_NOWILDCARD);
			break;
		}

	case UTIL_SAVE_MODIFIED :
		ForceSaveAllModified(file);
		break;

	case UTIL_CLOSE_UNMODIFIED :
		file = CloseAllUnmodified(file);
		break;

	case UTIL_EDIT_FILENAMES :
		EditFilenames(file);
		break;

	case UTIL_SHOW_MODIFIED :
		file = SelectFile(file, SELECT_FILE_MODIFIED);
		break;

	case UTIL_SHOW_UNMODIFIED :
		file = SelectFile(file, SELECT_FILE_UNMODIFIED);
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
static void EditFilenames(EDIT_FILE*file)
{
	char line[MAX_FILENAME + 32];
	char filename[MAX_FILENAME];
	char*scrBuff;
	EDIT_FILE*filelist;
	EDIT_LINE*newLine;
	EDIT_FILE*list, **lut;
	int xd, yd, x, y, index = 0, num;
	int numFiles;

	file->paint_flags |= CURSOR_FLAG;

	xd = file->display.xd - 12;
	yd = file->display.yd / 2;

	x = file->display.xd / 2 - xd / 2;
	y = (file->display.yd - 1) / 2 - yd / 2;

	list = AllocFile("Edit Filenames");
	newLine = list->lines;

	InitDisplayFile(list);

	list->display.xpos = x;
	list->display.ypos = y;
	list->display.xd = xd;
	list->display.yd = yd;
	list->display.columns = xd - 2;
	list->display.rows = yd - 3;

	list->client = 2;
	list->border = 2;

	list->file_flags |= FILE_FLAG_NONFILE;

	InitCursorFile(list);

	CursorTopFile(list);

	GotoPosition(list, 1, 1);

	numFiles = NumberFiles(FILE_FLAG_ALL);

	lut = (EDIT_FILE**)OS_Malloc(numFiles*(int)sizeof(EDIT_FILE*));

	filelist = NextFile(0);

	while (filelist) {
		if (filelist->file_flags&FILE_FLAG_NORMAL) {
			lut[index] = filelist;
			sprintf(line, "%d", index + 1);

			while (strlen(line) < 5)
				strcat(line, " ");
			strcat(line, "\"");
			strcat(line, filelist->pathname);
			strcat(line, "\"");

			newLine = AddFileLine(list, newLine, line, strlen(line), 0);
			index++;
		}

		filelist = filelist->next;
	}

	scrBuff = SaveScreen();

	for (; ; ) {
		Paint(list);

		UpdateStatusBar(list);

		if (!ProcessUserInput(list, EditFilesInput))
			break;
	}

	newLine = list->lines;

	while (newLine) {
		if (newLine->len) {
			if (newLine->len > (int)sizeof(line) - 1)
				newLine->len = sizeof(line) - 1;

			memcpy(line, newLine->line, newLine->len);

			line[newLine->len] = 0;

			if (sscanf(line, "%d", &num) > 0) {
				if (num >= 1 && num <= index) {
					GetNewFilename(line, filename);

					if (strlen(filename) && OS_Strcasecmp(filename,
					    lut[num - 1]->pathname)) {
						lut[num - 1]->paint_flags |= FRAME_FLAG;
						lut[num - 1]->force_modified = 1;
						OS_Free(lut[num - 1]->pathname);
						lut[num - 1]->pathname = OS_Malloc(strlen(filename) +
						    1);
						strcpy(lut[num - 1]->pathname, filename);
					}
				}
			}
		}

		newLine = newLine->next;
	}

	OS_Free(lut);

	RestoreScreen(scrBuff);

	DeallocFile(list);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void GetNewFilename(char*line, char*filename)
{
	int i, j, len, index = 0;

	len = strlen(line);

	for (i = 0; i < len; i++) {
		if (line[i] == '"') {
			for (j = i + 1; j < len; j++) {
				if (line[j] == '"')
					break;

				filename[index++] = line[j];
			}
			break;
		}
	}

	filename[index] = 0;
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*EditFilesInput(EDIT_FILE*file, int mode, int ch)
{
	if (!mode) {
		if (ch == ED_KEY_ESC)
			return (0);

		ProcessNormal(file, ch);
	} else {
		switch (ch) {
		case ED_ALT_A : /* Ignore Save As        */
		case ED_ALT_N : /* Ignore Next File      */
		case ED_ALT_Y : /* Ignore Merge          */
		case ED_ALT_F : /* Ignore File Select    */
		case ED_ALT_W : /* Ignore Write          */
		case ED_ALT_E : /* Ignore Newfile        */
		case ED_ALT_T : /* Ignore Title change   */
		case ED_ALT_H : /* Ignore Help           */
		case ED_ALT_O : /* Ignore Operations     */
		case ED_ALT_B : /* Ignore Build          */
		case ED_ALT_Z : /* Ignore Shell          */
		case ED_F3 : /* File Backups          */
		case ED_F5 : /* Ignore Toggle Case    */
		case ED_F6 : /* Ignore Toggle Global  */
		case ED_F9 : /* Ignore Utils          */
			break;

		case ED_ALT_Q : /* Exit Calculator       */
		case ED_ALT_X : /* Exit Calculator       */
			return (0);

			default :
			file = ProcessSpecial(file, ch);
		}
	}

	return (file);
}


