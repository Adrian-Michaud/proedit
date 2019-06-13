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
#include "errors.h"
#include "find.h"

static EDIT_FILE*FindManualLoadDir(char*pathname);

static void GetLastManualDir(char*filename);
static int VerifyPath(char*file, char*cwd);

static EDIT_FIND*findManual;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*ManualLoadFilename(char*cwd, char*pathname)
{
	char filename[MAX_FILENAME];
	int ch;

	for (; ; ) {
		OS_GetFilename(pathname, 0, filename);

		ch = Question(
		    "Can't find \"%s\" - [L]oad Manually, [F]ind, [S]kip, [Q]uit:",
		    filename);

		if (ch == 'F' || ch == 'f') {
			if (SearchForFile(cwd, filename)) {
				if (strlen(filename))
					return (LoadFileWildcard(0, filename, LOAD_FILE_EXISTS |
					    LOAD_FILE_NOPAINT | LOAD_FILE_NOWILDCARD));
			}
		}

		if (ch == 'L' || ch == 'l') {
			GetLastManualDir(filename);

			if (!strlen(filename))
				strcpy(filename, cwd);

			if (QueryFiles(filename)) {
				if (strlen(filename))
					return (LoadFileWildcard(0, filename, LOAD_FILE_EXISTS |
					    LOAD_FILE_NOPAINT | LOAD_FILE_NOWILDCARD));
			}

			Input(HISTORY_NEWFILE, "Load:", filename, MAX_FILENAME);

			if (!strlen(filename))
				return (0);

			return (LoadFileWildcard(0, filename, LOAD_FILE_EXISTS |
			    LOAD_FILE_NOPAINT | LOAD_FILE_NOWILDCARD));
		}


		if (ch == 'S' || ch == 's')
			return (0);

		if (ch == 'Q' || ch == 'q' || ch == ED_KEY_ESC) {
			return ((EDIT_FILE*) - 1);
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*LoadExistingFilename(char*cwd, char*pathname)
{
	EDIT_FILE*walk;
	char current[MAX_FILENAME];
	char search[MAX_FILENAME];

	OS_GetFilename(pathname, 0, search);

	/* get first file */
	walk = NextFile(0);

	while (walk) {
		OS_GetFilename(walk->pathname, 0, current);

		if (!OS_Strcasecmp(current, search)) {
			if (VerifyPath(walk->pathname, cwd))
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
void AddManualLoadFilename(EDIT_FILE*file, char*pathname)
{
	EDIT_FIND*find;
	char filename[MAX_FILENAME];
	char path[MAX_FILENAME];

	find = findManual;

	/* Does this file already exist in the manually loaded? */
	while (find) {
		if (find->active) {
			if (!OS_Strcasecmp(file->pathname, find->file_pathname)) {
				OS_Printf("%s already existed(!)\n", file->pathname);
				return ;
			}
		}
		find = find->next;
	}

	find = OS_Malloc(sizeof(EDIT_FIND));
	memset(find, 0, sizeof(EDIT_FIND));

	strcpy(find->file_pathname, file->pathname);
	strcpy(find->pathname, pathname);

	OS_GetFilename(file->pathname, path, 0);
	OS_GetFilename(file->pathname, 0, filename);

	strcpy(find->path, path);
	strcpy(find->filename, filename);

	find->active = 1;
	find->next = findManual;

	findManual = find;
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*FindManualLoadFilename(char*pathname)
{
	EDIT_FIND*find;
	char filename[MAX_FILENAME];
	char path[MAX_FILENAME];
	EDIT_FILE*file;

	OS_GetFilename(pathname, 0, filename);

	find = findManual;

	while (find) {
		if (find->active) {
			if (!OS_Strcasecmp(filename, find->filename) && !OS_Strcasecmp(
			    pathname, find->pathname)) {
				OS_JoinPath(path, find->path, filename);

				file = LoadFileWildcard(0, path, LOAD_FILE_EXISTS |
				    LOAD_FILE_NOPAINT | LOAD_FILE_NOWILDCARD);

				if (file)
					return (file);
			}
		}

		find = find->next;
	}

	return (FindManualLoadDir(pathname));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*FindManualLoadDir(char*pathname)
{
	EDIT_FIND*find;
	char filename[MAX_FILENAME];
	char path[MAX_FILENAME];
	EDIT_FILE*file;

	OS_GetFilename(pathname, 0, filename);

	find = findManual;

	while (find) {
		if (find->active) {
			OS_JoinPath(path, find->path, filename);

			file = LoadFileWildcard(0, path, LOAD_FILE_EXISTS |
			    LOAD_FILE_NOPAINT | LOAD_FILE_NOWILDCARD);

			if (file)
				return (file);
		}

		find = find->next;
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void GetLastManualDir(char*filename)
{
	EDIT_FIND*find;

	strcpy(filename, "");

	find = findManual;

	while (find) {
		if (find->active)
			strcpy(filename, find->path);

		find = find->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int VerifyPath(char*file, char*cwd)
{
	char base[MAX_FILENAME];

	if (strlen(file) < strlen(cwd))
		return (0);

	strcpy(base, file);
	base[strlen(cwd)] = 0;

	if (!OS_Strcasecmp(base, cwd))
		return (1);

	return (0);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SaveFindState(char*cwd, char*command)
{
	FILE_HANDLE*fp;
	EDIT_FIND*find, *next;
	char filename[MAX_FILENAME];

	OS_JoinPath(filename, cwd, ".proedit-find");

	if (findManual) {
		fp = OS_Open(filename, "wb");

		if (fp) {
			find = findManual;

			while (find) {
				next = find->next;

				if (find->active)
					strcpy(find->command, command);

				find->sig1 = MANUAL_SIG1;
				find->len = sizeof(EDIT_FIND);
				find->sig2 = MANUAL_SIG2;
				OS_Write(find, sizeof(EDIT_FIND), 1, fp);

				OS_Free(find);

				find = next;
			}

			OS_Close(fp);
		}
	}
	findManual = 0;
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void LoadFindState(char*cwd, char*command)
{
	FILE_HANDLE*fp;
	EDIT_FIND*find;
	char filename[MAX_FILENAME];

	OS_JoinPath(filename, cwd, ".proedit-find");

	fp = OS_Open(filename, "rb");

	if (fp) {
		for (; ; ) {
			find = (EDIT_FIND*)OS_Malloc(sizeof(EDIT_FIND));
			memset(find, 0, sizeof(EDIT_FIND));

			if (!OS_Read(find, sizeof(EDIT_FIND), 1, fp)) {
				OS_Free(find);
				break;
			}

			if (find->len != sizeof(EDIT_FIND) || find->sig1 != MANUAL_SIG1 ||
			    find->sig2 != MANUAL_SIG2) {
				OS_Free(find);
				break;
			}

			if (!OS_Strcasecmp(find->command, command))
				find->active = 1;
			else
				find->active = 0;

			find->next = findManual;
			findManual = find;
		}
		OS_Close(fp);
	}
}



