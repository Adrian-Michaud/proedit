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

static void FreeFiles(void);
static int FillList(void);
static int SortFiles(const void*arg1, const void*arg2);
static void SearchRecurse(char*dir, char*filename);
static int IsSingleFile(char*dir);

static char*files[MAX_LIST_FILES];
static char*dirs[MAX_LIST_FILES];
static char*list[MAX_LIST_FILES];

static int numDirs, numFiles, numList, dirCount;

extern char currentFile[MAX_FILENAME];

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SearchForFile(char*dir, char*filename)
{
	int line, i;

	numFiles = 0;

	CenterBottomBar(1, "One Moment, Searching for \"%s\"...", filename);

	SearchRecurse(dir, filename);

	if (!numFiles) {
		strcpy(filename, "");
		return (0);
	}

	line = PickList("Select a File", numFiles, GetScreenXDim() - 6,
	    "Select a File", files, 1);

	if (line)
		strcpy(filename, files[line - 1]);
	else
		strcpy(filename, "");

	for (i = 0; i < numFiles; i++)
		OS_Free(files[i]);

	numFiles = 0;

	return (line);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SearchRecurse(char*dir, char*filename)
{
	char*pathname;
	char*fullpath;
	char*found;
	char*curDir;
	char**dirs;
	int i, numDirs;

	pathname = OS_Malloc(MAX_FILENAME);
	fullpath = OS_Malloc(MAX_FILENAME);
	found = OS_Malloc(MAX_FILENAME);
	curDir = OS_Malloc(MAX_FILENAME);
	dirs = OS_Malloc(MAX_LIST_FILES*sizeof(char*));

	OS_GetCurrentDir(curDir);

	OS_JoinPath(pathname, dir, filename);

	if (OS_GetFullPathname(pathname, fullpath, MAX_FILENAME)) {
		if (OS_DosFindFirst(fullpath, found)) {
			while (strlen(found) && numFiles < MAX_LIST_FILES) {
				files[numFiles] = (char*)OS_Malloc(strlen(found) + 1);
				strcpy(files[numFiles], found);
				numFiles++;
				OS_DosFindNext(found);
			}
		}
		OS_DosFindEnd();
	}

	numDirs = OS_ReadDirectory(dir, "*", dirs, MAX_LIST_FILES,
	    OS_NO_SPECIAL_DIR);

	for (i = 0; i < numDirs; i++)
		SearchRecurse(dirs[i], filename);

	OS_DeallocDirs(dirs, numDirs);

	OS_SetCurrentDir(curDir);

	OS_Free(pathname);
	OS_Free(fullpath);
	OS_Free(found);
	OS_Free(curDir);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int IsSingleFile(char*dir)
{
	char path[MAX_FILENAME];
	char mask[MAX_FILENAME];
	char pathname[MAX_FILENAME];
	char filename[MAX_FILENAME];
	int i, retCode = 0;

	if (!strlen(dir))
		return (0);

	OS_GetFilename(dir, path, mask);

	if (!strlen(mask))
		return (0);

	for (i = 0; i < (int)strlen(mask); i++)
		if (mask[i] == '*' || mask[i] == '?')
			return (0);

	strcat(mask, "*");

	numDirs = OS_ReadDirectory(path, mask, dirs, MAX_LIST_FILES,
	    OS_NO_SPECIAL_DIR);

	if (numDirs <= 1) {
		sprintf(pathname, "%s%s", path, mask);

		if (OS_DosFindFirst(pathname, filename)) {
			for (numFiles = 0; numFiles < MAX_LIST_FILES; numFiles++) {
				if (!strlen(filename))
					break;

				files[numFiles] = (char*)OS_Malloc(strlen(filename) + 1);
				strcpy(files[numFiles], filename);
				OS_DosFindNext(filename);
			}
		}
		OS_DosFindEnd();

		if (numDirs == 1 && !numFiles) {
			OS_JoinPath(dir, dirs[0], "");
			retCode = 1;
		}

		if (!numDirs && numFiles == 1) {
			strcpy(dir, files[0]);
			retCode = 1;
		}
	}

	FreeFiles();

	return (retCode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int QueryFiles(char*dir)
{
	char path[MAX_FILENAME];
	char mask[MAX_FILENAME];
	char filename[MAX_FILENAME];
	char pathname[MAX_FILENAME];
	char orgDir[MAX_FILENAME];
	char*result;
	int xd;
	int line;
	int selected;
	int i;

	if (IsSingleFile(dir))
		return (0);

	result = dir;

	OS_GetCurrentDir(orgDir);

	if (!strlen(dir))
		dir = orgDir;

	OS_GetFilename(dir, path, mask);

	if (!strlen(mask))
		strcpy(mask, "*");

	for (; ; ) {
		numDirs = OS_ReadDirectory(path, mask, dirs, MAX_LIST_FILES, 0);

		sprintf(pathname, "%s%s", path, mask);

		if (OS_DosFindFirst(pathname, filename)) {
			for (numFiles = 0; numFiles < MAX_LIST_FILES; numFiles++) {
				if (!strlen(filename))
					break;

				files[numFiles] = (char*)OS_Malloc(strlen(filename) + 1);
				strcpy(files[numFiles], filename);
				OS_DosFindNext(filename);
			}
		}

		OS_DosFindEnd();

		xd = FillList();

		if (!numList) {
			OS_SetCurrentDir(orgDir);
			return (0);
		}

		selected = 1;

		if (strlen(currentFile)) {
			for (i = 0; i < numFiles; i++) {
				if (!strcmp(files[i], currentFile)) {
					selected = numDirs + i + 1;
					break;
				}
			}
			//	   strcpy(currentFile, "");
		}

		line = PickList("Select a File", numList, xd, pathname, list, selected);

		if (!line) {
			strcpy(result, path);
			break;
		}

		if (line > dirCount) {
			strcpy(result, files[(line - dirCount) - 1]);
			break;
		} else {
			strcpy(path, dirs[line - 1]);
			OS_SetCurrentDir(path);
			OS_GetCurrentDir(path);
			strcpy(mask, "*");
		}

		FreeFiles();
	}

	FreeFiles();

	OS_SetCurrentDir(orgDir);

	return (line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void FreeFiles(void)
{
	int i;

	OS_DeallocDirs(dirs, numDirs);

	for (i = 0; i < numFiles; i++)
		OS_Free(files[i]);

	for (i = 0; i < numList; i++)
		OS_Free(list[i]);

	numDirs = numFiles = numList = 0;
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int FillList(void)
{
	int i;
	char line[80 + MAX_FILENAME];
	char filename[MAX_FILENAME];
	OS_FILE_INFO info;
	char text_num[32];
	int max = 0;
	int len;

	numList = 0;
	dirCount = 0;

	qsort(dirs, numDirs, sizeof(char*), SortFiles);
	qsort(files, numFiles, sizeof(char*), SortFiles);

	for (i = 0; i < numDirs && numList < MAX_LIST_FILES; i++) {
		/* Special case for previous directory. */
		if ((strlen(dirs[i]) >= 2) && !strcmp(&dirs[i][strlen(dirs[i]) - 2],
		    ".."))
			strcpy(filename, "..");
		else
			OS_GetFilename(dirs[i], 0, filename);

		if (OS_GetFileInfo(dirs[i], &info))
			sprintf(line, " %s %-18s %s", info.asciidate, "  <DIR>", filename);
		else
			sprintf(line, " <DIR> %s", filename);

		len = strlen(line) + 1;
		list[numList] = OS_Malloc(len);
		strcpy(list[numList], line);
		if (len > max)
			max = len;
		numList++;
		dirCount++;
	}

	for (i = 0; i < numFiles && numList < MAX_LIST_FILES; i++) {
		OS_GetFilename(files[i], 0, filename);

		if (OS_GetFileInfo(files[i], &info)) {
			sprintf(text_num, "%ld", info.fileSize);
			sprintf(line, " %s %18s %s", info.asciidate, text_num, filename);
		} else
			sprintf(line, "       %s", filename);

		len = strlen(line) + 1;
		list[numList] = OS_Malloc(len);
		strcpy(list[numList], line);
		if (len > max)
			max = len;
		numList++;
	}
	return (max);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SortFiles(const void*arg1, const void*arg2)
{
	char**file1 = (char**)arg1;
	char**file2 = (char**)arg2;

	if (OS_PathDepth(*file1) < OS_PathDepth(*file2))
		return (-1);

	if (OS_PathDepth(*file1) > OS_PathDepth(*file2))
		return (1);

	return (OS_Strcasecmp(*file1, *file2));
}


