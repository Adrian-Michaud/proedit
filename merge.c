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

#define TEST_OPT

extern int merge_nows;

typedef struct
{
	unsigned long file1, file2;
	int match;
}INDEX_TABLE;

EDIT_FILE**fileHandles;

static EDIT_FILE*CreateMergeFile(EDIT_FILE*file1, EDIT_FILE*file2);
static int MergeCompareLines(int line1, int numLines1, int line2,
    int numLines2);
static int CompareFilenames(EDIT_FILE*comp1, EDIT_FILE*comp2);
static int MergeLineHandler(EDIT_FILE*file, EDIT_LINE*line, int op, int arg);

typedef struct lineHash
{
	EDIT_LINE*line;
	unsigned int hash;
}EDIT_LINE_HASH;

static EDIT_LINE_HASH*HashLines(EDIT_FILE*file);

static EDIT_LINE_HASH*hashFile1;
static EDIT_LINE_HASH*hashFile2;
static int aborted;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*Merge(EDIT_FILE*current)
{
	char newFilename[MAX_FILENAME*2];
	int i, j, found = 0, total = 0, totalFiles = 0, numFiles;
	EDIT_FILE*original;
	char*flags;
	int*comp1;
	int*comp2;
	int already_merged = 0;

	numFiles = NumberFiles(FILE_FLAG_ALL);

	original = current;

	fileHandles = (EDIT_FILE**)OS_Malloc(numFiles*sizeof(EDIT_FILE*));
	flags = (char*)OS_Malloc(numFiles*sizeof(char));
	comp1 = (int*)OS_Malloc(numFiles*sizeof(int));
	comp2 = (int*)OS_Malloc(numFiles*sizeof(int));

	memset(fileHandles, 0, numFiles*sizeof(EDIT_FILE*));

	for (i = 0; i < numFiles; i++) {
		fileHandles[i] = current;
		current = NextFile(current);

		/* Skip over Hex mode files and previously merged files */
		while (current->hexMode || (current->file_flags&FILE_FLAG_MERGED))
			current = NextFile(current);

		if (current == original)
			break;
	}

	for (i = 0; i < numFiles; i++) {
		flags[i] = 0;

		if (fileHandles[i])
			totalFiles++;
	}

	for (i = 0; i < numFiles; i++)
		if (fileHandles[i]) {
			if (flags[i])
				continue;

			for (j = 0; j < numFiles; j++) {
				if (j == i)
					continue;

				if (!fileHandles[j])
					continue;

				if (flags[j])
					continue;

				if (CompareFilenames(fileHandles[i], fileHandles[j])) {
					if (flags[i]) {
						flags[j] = 1;
						continue;
					}

					comp1[found] = i;
					comp2[found] = j;
					flags[i] = 1;
					flags[j] = 1;
					found++;
				}
			}
		}

	if (!found) {
		if (totalFiles != 2)
			CenterBottomBar(1, "[-] No Duplicate Filenames Found To Merge [-]");
		else {
			comp1[0] = 0;
			comp2[0] = 1;
			found = 1;
		}
	}

	for (i = 0; i < found; i++) {
		CenterBottomBar(1, "[+] Merging %d of %d files [+]", i + 1, found);

		sprintf(newFilename, "%s.dif", fileHandles[comp1[i]]->pathname);

		/* Already a merged version? */
		if (!FileAlreadyLoaded(newFilename)) {
			current = CreateMergeFile(fileHandles[comp1[i]],
			    fileHandles[comp2[i]]);
			if (current)
				total++;
		} else
			already_merged++;
		if (aborted)
			break;
	}

	if (found) {
		if (total) {
			CenterBottomBar(1, "[+] %d Different Files Merged [+]", total);
		} else {
			if (aborted)
				CenterBottomBar(1, "[-] File Merge Aborted... [-]");
			else {
				if (already_merged)
					CenterBottomBar(1, "[-] Nothing more to merge [-]");
				else
					CenterBottomBar(1, "[-] Files are not Different [-]");
			}
		}
	}

	if (!current)
		current = original;

	OS_Free(fileHandles);
	OS_Free(flags);
	OS_Free(comp1);
	OS_Free(comp2);

	current->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	return (current);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int MergeCompareLines(int line1, int numLines1, int line2, int numLines2)
{
	int total = 0;

	while (hashFile1[line1].hash == hashFile2[line2].hash) {
		total++;

		line1++;
		line2++;

		if (line1 >= numLines1)
			break;

		if (line2 >= numLines2)
			break;
	}

	return (total);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int CompareFilenames(EDIT_FILE*comp1, EDIT_FILE*comp2)
{
	char cmp1[MAX_FILENAME], cmp2[MAX_FILENAME];

	OS_GetFilename(comp1->pathname, 0, cmp1);
	OS_GetFilename(comp2->pathname, 0, cmp2);

	if (!OS_Strcasecmp(cmp1, cmp2))
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*CreateMergeFile(EDIT_FILE*file1, EDIT_FILE*file2)
{
	INDEX_TABLE indexTable1;
	INDEX_TABLE indexTable2;
	EDIT_FILE*newFile;
	EDIT_LINE*newLine;
	int file1_line_num, file2_line_num, i, j, match, deltas = 0, stop;
	char newFilename[MAX_FILENAME*2];

	aborted = 0;

	ProgressBar(0, 0, 0);

	hashFile1 = HashLines(file1);
	hashFile2 = HashLines(file2);

	sprintf(newFilename, "%s.dif", file1->pathname);

	file1_line_num = 0;
	file2_line_num = 0;

	newFile = AllocFile(newFilename);
	newLine = newFile->lines;

	newFile->diff1_filename = OS_Malloc(strlen(file1->pathname) + 1);
	strcpy(newFile->diff1_filename, file1->pathname);

	newFile->diff2_filename = OS_Malloc(strlen(file2->pathname) + 1);
	strcpy(newFile->diff2_filename, file2->pathname);

	for (; ; ) {
		if (AbortRequest()) {
			aborted = 1;
			break;
		}

		ProgressBar("Merging", file1_line_num, file1->number_lines);

		match = MergeCompareLines(file1_line_num, file1->number_lines,
		    file2_line_num, file2->number_lines);

		if (match) {
			for (i = 0; i < match; i++) {
				newLine = AddFileLine(newFile, newLine,
				    hashFile1[file1_line_num].line->line, hashFile1[
				    file1_line_num].line->len, 0);

				file1_line_num++;
				file2_line_num++;
			}
			if (AbortRequest()) {
				aborted = 1;
				break;
			}
		} else {
			/* Re-Sync Code */
			indexTable1.match = 0;
			indexTable1.file1 = 0;
			indexTable1.file2 = 0;

			indexTable2.match = 0;
			indexTable2.file1 = 0;
			indexTable2.file2 = 0;

			stop = file1->number_lines;

			for (j = file1_line_num; j < file1->number_lines && j < stop; j++) {
				if (AbortRequest()) {
					aborted = 1;
					break;
				}

				for (i = file2_line_num; i < file2->number_lines && i < stop; i
				    ++) {
					match = MergeCompareLines(j, file1->number_lines, i,
					    file2->number_lines);

					if (match) {
						if (match > indexTable1.match) {
							indexTable1.match = match;
							indexTable1.file1 = j;
							indexTable1.file2 = i;

							if (MAX(i, j) + match < stop)
								stop = MAX(i, j) + match;
						}
						break;
					}
				}

				if (j + indexTable1.match >= stop)
					break;
			}

			if (aborted)
				break;

			stop = file2->number_lines;

			for (j = file2_line_num; j < file2->number_lines && j < stop; j++) {
				if (AbortRequest()) {
					aborted = 1;
					break;
				}

				for (i = file1_line_num; i < file1->number_lines && i < stop; i
				    ++) {
					match = MergeCompareLines(i, file1->number_lines, j,
					    file2->number_lines);

					if (match) {
						if (match > indexTable2.match) {
							indexTable2.match = match;
							indexTable2.file1 = i;
							indexTable2.file2 = j;

							if (MAX(i, j) + match < stop)
								stop = MAX(i, j) + match;
						}
						break;
					}
				}

				if (j + indexTable2.match >= stop)
					break;
			}

			if (aborted)
				break;

			if (indexTable1.match && indexTable1.match >= indexTable2.match) {
				if (file1_line_num < (int)indexTable1.file1)
					AddBookmark(newFile, newLine, 0, 0);

				for (i = file1_line_num; i < (int)indexTable1.file1; i++) {
					newLine = AddFileLine(newFile, newLine, hashFile1[i].line->
					    line, hashFile1[i].line->len, LINE_FLAG_DIFF1);

					deltas = 1;
					file1_line_num++;
				}

				if (file2_line_num < (int)indexTable1.file2)
					AddBookmark(newFile, newLine, 0, 0);

				for (i = file2_line_num; i < (int)indexTable1.file2; i++) {
					newLine = AddFileLine(newFile, newLine, hashFile2[i].line->
					    line, hashFile2[i].line->len, LINE_FLAG_DIFF2);

					deltas = 1;
					file2_line_num++;
				}
			} else
				if (indexTable2.match && indexTable2.match >=
				    indexTable1.match) {
					if (file1_line_num < (int)indexTable2.file1)
						AddBookmark(newFile, newLine, 0, 0);

					for (i = file1_line_num; i < (int)indexTable2.file1; i++) {
						newLine = AddFileLine(newFile, newLine, hashFile1[i].
						    line->line, hashFile1[i].line->len,
						    LINE_FLAG_DIFF1);

						deltas = 1;
						file1_line_num++;
					}


					if (file2_line_num < (int)indexTable2.file2)
						AddBookmark(newFile, newLine, 0, 0);

					for (i = file2_line_num; i < (int)indexTable2.file2; i++) {
						newLine = AddFileLine(newFile, newLine, hashFile2[i].
						    line->line, hashFile2[i].line->len,
						    LINE_FLAG_DIFF2);

						deltas = 1;
						file2_line_num++;
					}
				} else {
					if (file1_line_num < file1->number_lines)
						AddBookmark(newFile, newLine, 0, 0);

					while (file1_line_num < file1->number_lines) {
						newLine = AddFileLine(newFile, newLine, hashFile1[
						    file1_line_num].line->line,
						    hashFile1[file1_line_num].line->len,
						    LINE_FLAG_DIFF1);

						deltas = 1;
						file1_line_num++;
					}

					if (file2_line_num < file2->number_lines)
						AddBookmark(newFile, newLine, 0, 0);

					while (file2_line_num < file2->number_lines) {
						newLine = AddFileLine(newFile, newLine, hashFile2[
						    file2_line_num].line->line,
						    hashFile2[file2_line_num].line->len,
						    LINE_FLAG_DIFF2);

						deltas = 1;
						file2_line_num++;
					}
					break;
				}
		}

		/* Reached End of ONE of the files..... */
		if ((file1_line_num >= file1->number_lines) || (file2_line_num >=
		    file2->number_lines)) {
			if (file1_line_num < file1->number_lines)
				AddBookmark(newFile, newLine, 0, 0);

			while (file1_line_num < file1->number_lines) {
				newLine = AddFileLine(newFile, newLine,
				    hashFile1[file1_line_num].line->line, hashFile1[
				    file1_line_num].line->len, LINE_FLAG_DIFF1);

				deltas = 1;
				file1_line_num++;
			}

			if (file2_line_num < file2->number_lines)
				AddBookmark(newFile, newLine, 0, 0);

			while (file2_line_num < file2->number_lines) {
				newLine = AddFileLine(newFile, newLine,
				    hashFile2[file2_line_num].line->line, hashFile2[
				    file2_line_num].line->len, LINE_FLAG_DIFF2);

				deltas = 1;
				file2_line_num++;
			}
			break;
		}
	}

	OS_Free(hashFile1);
	OS_Free(hashFile2);

	/* Don't create a delta file if the files are the same, of if the */
	/* user aborted.                                                  */
	if (aborted || !deltas) {
		DeallocFile(newFile);
		return (0);
	}

	InitDisplayFile(newFile);
	InitCursorFile(newFile);

	newFile->colorize = ConfigColorize(newFile, file1->pathname);

	newFile->file_flags |= FILE_FLAG_MERGED;

	AddFile(newFile, ADD_FILE_TOP);
	AddLineCallback(newFile, (LINE_PFN*)MergeLineHandler,
	    LINE_OP_GETTING_FOCUS);

	return (newFile);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int MergeLineHandler(EDIT_FILE*file, EDIT_LINE*line, int op, int arg)
{
	arg = arg;

	if (op&LINE_OP_GETTING_FOCUS) {
		if (line->flags&LINE_FLAG_DIFF1)
			file->title = file->diff1_filename;
		else
			if (line->flags&LINE_FLAG_DIFF2)
				file->title = file->diff2_filename;
			else
				file->title = 0;
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_LINE_HASH*HashLines(EDIT_FILE*file)
{
	unsigned int hash;
	EDIT_LINE_HASH*hashTable;
	EDIT_LINE*line;
	int i, index;

	hashTable = (EDIT_LINE_HASH*)
	OS_Malloc((file->number_lines + 2)*sizeof(EDIT_LINE_HASH));

	memset(hashTable, 0, (file->number_lines + 2)*sizeof(EDIT_LINE_HASH));

	line = file->lines;
	index = 0;

	while (line) {
		hash = 5137;

		for (i = 0; i < line->len; i++) {
			if (merge_nows && line->line[i] <= 32)
				continue;

			hash = ((hash << 5) + hash) + line->line[i];
		}

		hashTable[index].hash = hash;
		hashTable[index].line = line;
		index++;

		line = line->next;
	}
	hashTable[index].line = 0;
	hashTable[index].hash = (unsigned int)((unsigned long)hashTable);

	return (hashTable);
}


