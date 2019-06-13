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

EDIT_FILE*files;

static int AddFileSorted(char*filename, char*newfile);
static int SaveFileDisk(EDIT_FILE*file, char*filename);
static void Backupfile(char*pathname);
static EDIT_FILE*LoadFile(char*filename, int mode);
static int SaveError(EDIT_FILE*file, char*filename);
static void WriteFileProgress(char*progress, char*data, int len,
    FILE_HANDLE*fp);
static int ReadFileProgress(char*progress, char*data, int len, FILE_HANDLE*fp);

extern int forceHex;
extern int forceText;
extern int createBackups;
extern int stripWhitespace;
extern int force_crlf;

char currentFile[MAX_FILENAME];

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*SwitchFile(EDIT_FILE*file)
{
	if (files == file && !file->next)
		CenterBottomBar(1, "[-] Only One File Is Loaded! [-]");
	else {
		file = NextFile(file);

		file->paint_flags = CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
	}
	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SaveAllModified(EDIT_FILE*file)
{
	EDIT_FILE*walk;
	int write_all = 0;

	walk = files;

	file->paint_flags = CURSOR_FLAG;

	while (walk) {
		if (!SaveModified(walk, &write_all))
			return (0);
		walk = walk->next;
	}
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ForceSaveAllModified(EDIT_FILE*file)
{
	EDIT_FILE*walk;

	walk = files;

	file->paint_flags = CURSOR_FLAG;

	while (walk) {
		if (FileModified(walk))
			if (!SaveFile(walk))
				return (0);

		walk = walk->next;
	}
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*CloseAllUnmodified(EDIT_FILE*file)
{
	EDIT_FILE*walk, *newFile = 0;

	walk = files;

	file->paint_flags = CURSOR_FLAG;

	while (walk) {
		if (!FileModified(walk)) {
			if (file == walk)
				file = 0;

			newFile = CloseFile(walk);
		}

		walk = walk->next;
	}

	if (file)
		return (file);

	if (newFile)
		newFile->paint_flags = CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	return (newFile);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*SelectFile(EDIT_FILE*current, int mode)
{
	int i, total = 0, start = 1, line, numFiles;
	char**list;
	EDIT_FILE*walk, **lut;
	char filename[MAX_FILENAME];
	char format[64];
	char title[256];
	int xd;
	int maxlen = 20;
	int linelen = 0;
	int len;

	xd = GetScreenXDim() - 6;
	numFiles = NumberFiles(FILE_FLAG_ALL);
	walk = files;

	list = (char**)OS_Malloc(numFiles*(int)sizeof(char*));
	lut = (EDIT_FILE**)OS_Malloc(numFiles*(int)sizeof(EDIT_FILE*));

	for (i = 0; i < numFiles; i++) {
		if (!walk)
			break;

		len = strlen(walk->pathname);

		if (len > linelen)
			linelen = len;

		walk = walk->next;
	}

	walk = files;

	if (linelen > (xd - 15))
		maxlen = (xd - 15);

	for (i = 0; i < numFiles; i++) {
		if (!walk)
			break;

		if (walk == current)
			start = i + 1;

		strcpy(filename, walk->pathname);

		len = strlen(filename);

		if (len < linelen) {
			memset(&filename[len], ' ', linelen - len);
			filename[linelen] = 0;
		}

		if (mode == SELECT_FILE_ALL || (mode == SELECT_FILE_MODIFIED && (walk->
		    modified || walk->force_modified)) || (mode ==
		    SELECT_FILE_UNMODIFIED && (!walk->modified &&
		    !walk->force_modified))) {
			list[total] = (char*)OS_Malloc(512);
			lut[total] = walk;

			sprintf(list[total], " %s %-4d %-6d", filename, walk->userUndos,
			    walk->number_lines);
			total++;
		}

		walk = walk->next;
	}

	if (!total) {
		OS_Free(list);
		OS_Free(lut);
		CenterBottomBar(1, "[-] No files to display [-]");
		return (current);
	}

	sprintf(format, "!%%-%ds!%%s!", maxlen - 3);

	sprintf(title, format, "Filename!", "Undo!Lines");
	len = strlen(title);
	for (i = 0; i < len; i++) {
		if (title[i] == ' ')
			title[i] = '@';
		if (title[i] == '!')
			title[i] = ' ';
	}

	line = PickList("List of loaded files", total, linelen + 15, title, list,
	    start);

	for (i = 0; i < total; i++)
		OS_Free(list[i]);

	if (line) {
		current = lut[line - 1];
		current->paint_flags = CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
	}

	OS_Free(list);
	OS_Free(lut);

	current->paint_flags |= CURSOR_FLAG;

	return (current);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*NextFile(EDIT_FILE*file)
{
	if (!file)
		return (files);

	file = file->next;

	if (!file)
		file = files;

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*ValidateFile(EDIT_FILE*file, char*pathname)
{
	EDIT_FILE*walk;

	walk = files;

	while (walk) {
		if (file == walk) {
			if (!OS_Strcasecmp(file->pathname, pathname))
				return (walk);
		}
		walk = walk->next;
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int NumberFiles(int mask)
{
	EDIT_FILE*base;
	int number = 0;

	base = files;

	while (base) {
		if (base->file_flags&mask)
			number++;

		base = base->next;
	}
	return (number);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*AllocFile(char*filename)
{
	EDIT_FILE*new_file;
	EDIT_LINE*lines;

	new_file = (EDIT_FILE*)OS_Malloc(sizeof(EDIT_FILE));

	memset(new_file, 0, sizeof(EDIT_FILE));

	new_file->pathname = (char*)OS_Malloc(strlen(filename) + 1);

	strcpy(new_file->pathname, filename);

	lines = (EDIT_LINE*)OS_Malloc(sizeof(EDIT_LINE));

	memset(lines, 0, sizeof(EDIT_LINE));

	new_file->lines = lines;

	new_file->forceHex = forceHex;
	new_file->forceText = forceText;

	return (new_file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*CreateUntitled(void)
{
	EDIT_FILE*file;

	file = AllocFile("untitled");

	if (forceHex)
		file->hexMode = HEX_MODE_HEX;

	AddFile(file, ADD_FILE_SORTED);

	file->file_flags |= (FILE_FLAG_UNTITLED | FILE_FLAG_NORMAL);

	InitDisplayFile(file);
	InitCursorFile(file);

	/* Display the files as we load them. */
	Paint(file);

	UpdateStatusBar(file);

	file->paint_flags = CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeallocFile(EDIT_FILE*file)
{
	DestroyBookmarks(file);

	if (file->pathname)
		OS_Free(file->pathname);

	if (file->hexData)
		OS_Free(file->hexData);

	if (file->diff1_filename)
		OS_Free(file->diff1_filename);

	if (file->diff2_filename)
		OS_Free(file->diff2_filename);

	if (file->lines)
		DeallocLines(file->lines);

	DeleteUndos(file);

	RemoveAllCallbacks(file);

	OS_Free(file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*LoadFile(char*filename, int mode)
{
	FILE_HANDLE*fp;
	EDIT_FILE*new_file;
	long filesize;
	char*buffer;
	char progress[MAX_FILENAME];
	char loadname[MAX_FILENAME];

	fp = OS_Open(filename, "rb");

	if (!fp) {
		if (mode&LOAD_FILE_EXISTS)
			return (0);

		new_file = AllocFile(filename);

		if (forceHex)
			new_file->hexMode = HEX_MODE_HEX;
	} else {
		new_file = AllocFile(filename);

		filesize = OS_Filesize(fp);

		if (forceHex)
			new_file->hexMode = HEX_MODE_HEX;

		if (filesize) {
			buffer = OS_Malloc(filesize);

			OS_GetFilename(filename, 0, loadname);
			sprintf(progress, "Reading \"%s\"", loadname);

			if (!ReadFileProgress(progress, buffer, filesize, fp)) {
				if (mode&LOAD_FILE_CMDLINE)
					OS_Printf("%s: Error reading \"%s\"\n", PROEDIT_MP_TITLE,
					    filename);
				else {
					CenterBottomBar(1, "[-] Error reading: \"%s\" [-]",
					    filename);
				}

				OS_Close(fp);
				DeallocFile(new_file);
				OS_Free(buffer);
				return (0);
			}

			ImportBuffer(new_file, buffer, filesize);

			OS_Free(buffer);
		}

		OS_Close(fp);
	}

	AddFile(new_file, ADD_FILE_SORTED);

	new_file->file_flags |= FILE_FLAG_NORMAL;

	/* Return success. */
	return (new_file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void AddFile(EDIT_FILE*new_file, int mode)
{
	EDIT_FILE*walk;

	new_file->window_title = new_file->pathname;

	if (!files || mode == ADD_FILE_TOP) {
		/* Set this new file at the top of the linked list. */
		if (files)
			files->prev = new_file;

		new_file->next = files;
		new_file->prev = 0;
		files = new_file;
		return ;
	}

	if (files) {
		walk = files;

		for (; ; ) {
			/* Sort the new file added to the linked list. */
			if (mode == ADD_FILE_SORTED && AddFileSorted(walk->pathname,
			    new_file->pathname)) {
				if (walk->prev)
					walk->prev->next = new_file;

				new_file->prev = walk->prev;
				new_file->next = walk;

				walk->prev = new_file;

				if (files == walk)
					files = new_file;

				break;
			}

			/* Add file to the end of the list. */
			if (!walk->next) {
				walk->next = new_file;
				new_file->prev = walk;
				new_file->next = 0;
				break;
			}

			walk = walk->next;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int AddFileSorted(char*filename, char*newfile)
{
	/* If the path depth is the same, sort based on filename. */
	if (OS_PathDepth(filename) == OS_PathDepth(newfile)) {
		if (OS_Strcasecmp(filename, newfile) > 0)
			return (1);
	}

	if (OS_PathDepth(filename) > OS_PathDepth(newfile))
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ImportBuffer(EDIT_FILE*file, char*buf, long max)
{
	char progress[MAX_FILENAME];
	char savename[MAX_FILENAME];
	EDIT_LINE*current = 0;
	long i, len, index, crlf = 0, lf = 0, block = 0, max_block;

	if (!forceHex) {
		for (i = 0; i < max; i++) {
			if (buf[i] == ED_KEY_LF) {
				if (i && buf[i - 1] == ED_KEY_CR)
					crlf++;
				else
					lf++;
			}

			if (buf[i] == 0) {
				if (!forceText) {
					file->hexMode = HEX_MODE_HEX;
					break;
				}
			}
		}
	}

	if (file->hexMode) {
		file->hexData = OS_Malloc(max);
		memcpy(file->hexData, buf, (int)max);
		file->number_lines = max;
		return ;
	}

	/* If all Linefeeds are preceded by a carrage return, this file is */
	/* a CR/LF text file.                                               */
	if (crlf && lf == 0)
		file->file_flags |= FILE_FLAG_CRLF;

	len = 0;
	index = 0;
	max_block = max / 100;


	ProgressBar(0, 0, 0);

	OS_GetFilename(file->pathname, 0, savename);
	sprintf(progress, "Loading \"%s\"", savename);


	for (i = 0; i <= max; i++) {
		if (((i == max) && len) || ((i < max) && (buf[i] == ED_KEY_LF))) {
			block += len;

			if (block >= max_block) {
				ProgressBar(progress, i, max);
				block = 0;
			}
			current = AddFileLine(file, current, &buf[index], (int)len, 0);
			len = 0;
			index = i + 1;
			continue;
		}
		len++;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_LINE*AddFileLine(EDIT_FILE*file, EDIT_LINE*current, char*line, int len,
    int flags)
{
	EDIT_LINE*new_line;

	if (!current)
		current = file->lines;

	current->allocSize = TabulateLength(line, 0, len, len);

	/* If this line has content, allocate space for it and copy it. */
	if (current->allocSize) {
		current->line = OS_Malloc(current->allocSize + 1);

		TabulateLine(current, line, len);
	} else
		current->len = 0;

	current->flags |= flags;

	new_line = (EDIT_LINE*)OS_Malloc(sizeof(EDIT_LINE));

	memset(new_line, 0, sizeof(EDIT_LINE));

	new_line->prev = current;

	current->next = new_line;

	file->number_lines++;

	return (new_line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeallocLines(EDIT_LINE*lines)
{
	EDIT_LINE*next;

	while (lines) {
		next = lines->next;

		if (lines->allocSize)
			OS_Free(lines->line);

		OS_Free(lines);

		lines = next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*QuitFile(EDIT_FILE*file, int session, int*write_all)
{
	EDIT_FILE*newFile;

	if (!SaveModified(file, write_all))
		return (file);

	if (session)
		LogSession(file);

	newFile = CloseFile(file);

	if (!newFile)
		return (0);

	newFile->paint_flags = CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	return (newFile);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int FileModified(EDIT_FILE*file)
{
	if (file->modified || file->force_modified)
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SaveModified(EDIT_FILE*file, int*write_all)
{
	int ch;
	char filename[MAX_FILENAME];

	if (FileModified(file)) {
		if (write_all && *write_all == 1) {
			if (SaveFile(file))
				return (1);
		}

		for (; ; ) {
			OS_GetFilename(file->pathname, 0, filename);

			if (write_all)
				ch = Question(
				    "%s Modified: [W]rite [A]bandon [S]ave-All [Q]uit:",
				    filename);
			else
				ch = Question("%s Modified: [W]rite [A]bandon [Q]uit:",
				    filename);

			file->paint_flags |= (CURSOR_FLAG);

			if (ch == 'W' || ch == 'w') {
				if (SaveFile(file))
					return (1);
				else
					OS_Bell();
			}

			if (write_all && ((ch == 'S' || ch == 's'))) {
				if (SaveFile(file)) {
					*write_all = 1;
					return (1);
				} else
					OS_Bell();
			}

			if (ch == 'A' || ch == 'a')
				return (1);

			if (ch == 'Q' || ch == 'q' || ch == ED_KEY_ESC)
				return (0);
		}
	}
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SaveError(EDIT_FILE*file, char*pathname)
{
	int ch;
	char filename[MAX_FILENAME];
	char path[MAX_FILENAME];

	file->paint_flags = CURSOR_FLAG;

	OS_GetFilename(pathname, path, 0);

	if (!OS_ValidPath(path)) {
		for (; ; ) {
			ch = Question("\"%s\" Doesn't Exist - [C]reate, [Q]uit:", path);

			if (ch == 'Q' || ch == 'q')
				return (1);

			if (ch == 'C' || ch == 'c') {
				if (!OS_CreatePath(path)) {
					CenterBottomBar(1, "[-] Error creating \"%s\" [-]", path);
					return (1);
				}

				return (0);
			}
		}
	}

	for (; ; ) {
		OS_GetFilename(pathname, 0, filename);

		ch = Question("Error saving \"%s\" - [R]ename, [C]heckout, [I]gnore:",
		    filename);

		if (ch == 'I' || ch == 'i')
			return (1);

		if (ch == 'R' || ch == 'r') {
			Input(HISTORY_NEWFILE, "New Filename:", pathname, MAX_FILENAME);
			break;
		}

		if (ch == 'C' || ch == 'c') {
			CheckoutFile(file, pathname);
			break;
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
EDIT_FILE*CloseFile(EDIT_FILE*file)
{
	EDIT_FILE*next;

	next = DisconnectFile(file);

	DeallocFile(file);

	return (next);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*DisconnectFile(EDIT_FILE*file)
{
	EDIT_FILE*next;

	next = file->next;

	/* Disconnect file from list. */
	if (file->prev)
		file->prev->next = file->next;
	if (file->next)
		file->next->prev = file->prev;

	if (files == file)
		files = file->next;

	if (!next)
		next = files;

	return (next);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*LoadNewFile(EDIT_FILE*file)
{
	char pathname[MAX_FILENAME];

	file->paint_flags |= (CURSOR_FLAG);

	OS_GetFilename(file->pathname, pathname, 0);

	strcpy(currentFile, file->pathname);

	Input(HISTORY_NEWFILE, "Filename:", pathname, MAX_FILENAME);

	if (!strlen(pathname))
		return (file);

	file = LoadFileWildcard(file, pathname, LOAD_FILE_NORMAL |
	    LOAD_FILE_INTERACTIVE);

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*LoadFileWildcard(EDIT_FILE*file, char*pathname, int mode)
{
	EDIT_FILE*newFile;
	char*filename;
	char*fullpath;

	filename = OS_Malloc(MAX_FILENAME);
	fullpath = OS_Malloc(MAX_FILENAME);

	if (OS_GetFullPathname(pathname, fullpath, MAX_FILENAME)) {
		OS_DosFindFirst(fullpath, filename);

		if (!strlen(filename)) {
			OS_GetFilename(fullpath, filename, 0);

			if (!OS_ValidPath(filename)) {
				if ((mode&LOAD_FILE_INTERACTIVE))
					CenterBottomBar(1, "[-] Invalid path \"%s\" [-]", filename);
				else
					OS_Printf("%s: Invalid path \"%s\"\n", PROEDIT_MP_TITLE,
					    filename);
			}
		} else {
			while (strlen(filename)) {
				newFile = FileAlreadyLoaded(filename);

				/* If the file isn't already loaded, load it. */
				if (!newFile) {
					newFile = LoadFile(filename, mode);

					if (newFile) {
						InitDisplayFile(newFile);
						InitCursorFile(newFile);
					}
				}

				if (newFile) {
					if (!(mode&LOAD_FILE_NOPAINT)) {
						/* Display the files as we load them. */
						Paint(newFile);
						UpdateStatusBar(newFile);
						HideCursor();
					}

					newFile->paint_flags = CONTENT_FLAG | CURSOR_FLAG |
					    FRAME_FLAG;
					file = newFile;
				}

				if (mode&LOAD_FILE_NOWILDCARD)
					break;

				OS_DosFindNext(filename);
			}
		}
		OS_DosFindEnd();
	} else {
		if ((mode&LOAD_FILE_INTERACTIVE))
			CenterBottomBar(1, "[-] Invalid Path \"%s\" [-]", pathname);
		else
			OS_Printf("%s error: Invalid Path \"%s\"\n", PROEDIT_MP_TITLE,
			    pathname);
	}

	OS_Free(filename);
	OS_Free(fullpath);

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*LoadFileRecurse(EDIT_FILE*file, char*pathname, int mode)
{
	int i, numDirs;
	char*filename;
	char*path;
	char*mask;
	char*orgDir;
	char**dirs;

	filename = OS_Malloc(MAX_FILENAME);
	orgDir = OS_Malloc(MAX_FILENAME);
	path = OS_Malloc(MAX_FILENAME);
	mask = OS_Malloc(MAX_FILENAME);
	dirs = OS_Malloc(MAX_LIST_FILES*sizeof(char*));

	OS_GetCurrentDir(orgDir);

	file = LoadFileWildcard(file, pathname, mode);

	OS_GetFilename(pathname, path, mask);

	if (!strlen(mask))
		strcpy(mask, "*");

	numDirs = OS_ReadDirectory(path, "*", dirs, MAX_LIST_FILES,
	    OS_NO_SPECIAL_DIR);

	for (i = 0; i < numDirs; i++) {
		OS_JoinPath(filename, dirs[i], mask);

		file = LoadFileRecurse(file, filename, mode);
	}

	OS_DeallocDirs(dirs, numDirs);

	OS_SetCurrentDir(orgDir);

	OS_Free(orgDir);
	OS_Free(path);
	OS_Free(mask);
	OS_Free(dirs);
	OS_Free(filename);

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*FileAlreadyLoaded(char*filename)
{
	EDIT_FILE*walk;

	walk = files;

	while (walk) {
		if (!OS_Strcasecmp(filename, walk->pathname))
			return (walk);

		walk = walk->next;
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void RenameFile(EDIT_FILE*file)
{
	char filename[MAX_FILENAME];

	strcpy(filename, file->pathname);
	Input(HISTORY_NEWFILE, "New Filename:", filename, MAX_FILENAME);
	if (!strlen(filename))
		return ;

	OS_Free(file->pathname);

	file->pathname = (char*)OS_Malloc(strlen(filename) + 1);

	strcpy(file->pathname, filename);

	file->window_title = file->pathname;

	file->paint_flags |= (CURSOR_FLAG | FRAME_FLAG);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void Backupfile(char*pathname)
{
	FILE_HANDLE*src = 0;
	FILE_HANDLE*dst = 0;
	char backupPath[MAX_FILENAME];
	char filename[MAX_FILENAME];
	char newFilename[MAX_FILENAME];
	char timeDate[OS_MAX_TIMEDATE];
	char*buffer;
	long filesize;
	char progress[MAX_FILENAME];

	if (!createBackups)
		return ;

	/* Check if the file exists. */
	src = OS_Open(pathname, "rb");

	if (src) {
		if (OS_GetBackupPath(backupPath, timeDate)) {
			OS_GetFilename(pathname, 0, filename);

			sprintf(progress, "Backing up \"%s\"", filename);

			sprintf(newFilename, "%s%s%s", backupPath, filename, timeDate);

			dst = OS_Open(newFilename, "wb");

			if (dst) {
				filesize = OS_Filesize(src);

				if (filesize) {
					buffer = OS_Malloc(filesize);

					if (buffer) {
						if (OS_Read(buffer, filesize, 1, src))
							WriteFileProgress(progress, buffer, filesize, dst);

						OS_Free(buffer);
					}
				}
			}
		}
	}

	if (src)
		OS_Close(src);

	if (dst)
		OS_Close(dst);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SaveFileDisk(EDIT_FILE*file, char*fname)
{
	char pathname[MAX_FILENAME];
	char savename[MAX_FILENAME];
	char progress[MAX_FILENAME];
	FILE_HANDLE*fp;
	EDIT_LINE*line;
	int i, blocks, counter, total;
	char*newLine = 0;
	int newLineLen;

	strcpy(pathname, fname);

	Backupfile(pathname);

	for (; ; ) {
		fp = OS_Open(pathname, "wb");

		if (fp)
			break;

		if (SaveError(file, pathname))
			return (0);
	}


	OS_GetFilename(pathname, 0, savename);
	sprintf(progress, "Saving \"%s\"", savename);

	if (file->file_flags&FILE_FLAG_UNTITLED) {
		if (OS_GetFullPathname(fname, pathname, MAX_FILENAME))

			if (file->pathname)
				OS_Free(file->pathname);

		file->pathname = OS_Malloc(strlen(pathname) + 1);

		strcpy(file->pathname, pathname);

		file->file_flags &= ~FILE_FLAG_UNTITLED;
	}

	if (file->hexMode) {
		WriteFileProgress(progress, file->hexData, file->number_lines, fp);
	} else {
		line = file->lines;

		switch (force_crlf) {
		case MAINTAIN_CRLF :
			{
				if (file->file_flags&FILE_FLAG_CRLF)
					newLine = "\r\n";
				else
					newLine = "\n";
				break;
			}

		case FORCE_CRLF :
			{
				newLine = "\r\n";
				break;
			}

		case FORCE_LF :
			{
				newLine = "\n";
				break;
			}

			default :
			{
				newLine = "\n";
				break;
			}
		}

		newLineLen = strlen(newLine);

		blocks = file->number_lines / 100;
		counter = blocks;
		total = 0;

		ProgressBar(0, 0, 0);

		while (line) {
			/* If line was padded, check to see if we should strip it. */
			if (line->flags&LINE_FLAG_PADDED)
				StripLinePadding(line);

			for (i = 0; i < line->len; i++) {
				if (line->line[i] != ED_KEY_TABPAD)
					OS_PutByte(line->line[i], fp);
			}

			/* If there is another line, and it's not a word wrapped line, */
			/* then instert a newline into the file.                       */
			if (line->next && (!(line->next->flags&LINE_FLAG_WRAPPED)))
				OS_Write(newLine, newLineLen, 1, fp);

			total++;
			counter++;
			if (counter >= blocks) {
				counter = 0;
				ProgressBar(progress, total, file->number_lines);
			}
			line = line->next;
		}
	}

	OS_Close(fp);

	file->modified = 0;
	file->force_modified = 0;

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void WriteFileProgress(char*progress, char*data, int len, FILE_HANDLE*fp)
{
	int blocks, total, chunk, written;

	blocks = len / 100;

	if (blocks > 100) {
		ProgressBar(0, 0, 0);
		written = 0;
		total = len;
		while (total) {
			chunk = MIN(blocks, total);
			OS_Write(&data[written], chunk, 1, fp);
			total -= chunk;
			written += chunk;
			ProgressBar(progress, written, len);
		}
	} else
		OS_Write(data, len, 1, fp);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ReadFileProgress(char*progress, char*data, int len, FILE_HANDLE*fp)
{
	int blocks, total, chunk, num_read;

	blocks = len / 100;

	if (blocks > 100) {
		ProgressBar(0, 0, 0);
		num_read = 0;
		total = len;
		while (total) {
			chunk = MIN(blocks, total);
			if (!OS_Read(&data[num_read], chunk, 1, fp))
				return (0);
			total -= chunk;
			num_read += chunk;
			ProgressBar(progress, num_read, len);
		}
	} else
		return (OS_Read(data, len, 1, fp));

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SaveFileAs(EDIT_FILE*file, char*pathname)
{
	strcpy(pathname, file->pathname);
	Input(HISTORY_NEWFILE, "Save As:", pathname, MAX_FILENAME);

	file->paint_flags |= (CURSOR_FLAG);

	if (!strlen(pathname))
		return (0);

	return (SaveFileDisk(file, pathname));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SaveFile(EDIT_FILE*file)
{
	char pathname[MAX_FILENAME];

	if (file->file_flags&FILE_FLAG_UNTITLED)
		return (SaveFileAs(file, pathname));

	return (SaveFileDisk(file, file->pathname));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*ExitFiles(EDIT_FILE*file)
{
	EDIT_FILE*next;
	int write_all = 0;

	while (file) {
		next = QuitFile(file, 1, &write_all);

		/* Did we abort the exit procedure? */
		if (next == file) {
			ClearSessions();
			break;
		}

		file = next;

		if (file) {
			if (FileModified(file)) {
				file->paint_flags = CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
				Paint(file);
			}
		}
	}

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ToggleBackups(void)
{
	if (createBackups) {
		CenterBottomBar(1, "[-] File backup has been disabled [-]");
		createBackups = 0;
	} else {
		CenterBottomBar(1, "[+] File backup has been enabled. [+]");
		createBackups = 1;
	}

	SetConfigInt(CONFIG_INT_BACKUPS, createBackups);

	SaveConfig();
}



