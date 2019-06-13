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

static char*patterns[] =
{
	"\"$F\", line $L: warning: $W",
	"\"$F\", line $L: $E",
	"$F:$L: error: $E",
	"$F:$L: warning: $W",
	"$F($L) : error $E",
	"$F($L) : warning $W",
	"$F($L) : fatal error $E",
	"LINK : fatal error LNK$E",
	"c1 : fatal error C$E",
	"fatal error RC$E",
	"$O: error LNK$E",
	"$A($O) : error LNK$E",
	"$F:$L: $E",
	"$F:$L:$C: $E",
};

#define NUM_PATTERNS (int)(sizeof(patterns)/sizeof(char*))

static char*GetFilename(char*line, int*len, char*filename);
static char*GetLineNo(char*line, int*len, int*lineNo);
static int GetMessage(char*line, int*len, char*msg);
static int FilenameChar(char ch);

static int ErrorWarningExists(EDIT_FILE*file, SHELL_ERROR*error);
static int PatternMatch(char*pattern, char*line, int len, SHELL_ERROR*error);

static EDIT_ERROR*errorList;
static int totalErrors;
static int lastError;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int NumberOfErrors(void)
{
	return (totalErrors);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
SHELL_ERROR*ProcessErrorLine(char*text, int len)
{
	int i;
	SHELL_ERROR*error;

	error = (SHELL_ERROR*)OS_Malloc(sizeof(SHELL_ERROR));
	memset(error, 0, sizeof(SHELL_ERROR));

	for (i = 0; i < NUM_PATTERNS; i++) {
		if (PatternMatch(patterns[i], text, len, error))
			return (error);
	}
	OS_Free(error);
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int PatternMatch(char*pattern, char*line, int len, SHELL_ERROR*error)
{
	int i, patLen;

	patLen = strlen(pattern);

	strcpy(error->filename, "");
	strcpy(error->warning, "");
	strcpy(error->error, "");
	strcpy(error->object, "");
	strcpy(error->library, "");
	error->lineNo = 0;
	error->col = 0;

	for (i = 0; i < patLen; i++) {
		if (pattern[i] == '$') {
			i++;
			switch (pattern[i]) {
			case 'F' :
				line = GetFilename(line, &len, error->filename);
				if (!line) {
					#ifdef DEBUG_PARSE
					printf("Failed filename check at %d\n", i);
					#endif
					return (0);
				}
				break;

			case 'A' :
				line = GetFilename(line, &len, error->library);
				if (!line) {
					#ifdef DEBUG_PARSE
					printf("Failed filename check at %d\n", i);
					#endif
					return (0);
				}
				break;

			case 'O' :
				line = GetFilename(line, &len, error->object);
				if (!line) {
					#ifdef DEBUG_PARSE
					printf("Failed filename check at %d\n", i);
					#endif
					return (0);
				}
				break;

			case 'L' :
				line = GetLineNo(line, &len, &error->lineNo);
				if (!line) {
					#ifdef DEBUG_PARSE
					printf("Failed line check at %d\n", i);
					#endif
					return (0);
				}
				#ifdef DEBUG_PARSE
				printf("After number: '%c%c%c'\n", line[0], line[1], line[2]);
				#endif
				break;

			case 'C' :
				line = GetLineNo(line, &len, &error->col);
				if (!line) {
					#ifdef DEBUG_PARSE
					printf("Failed line check at %d\n", i);
					#endif
					return (0);
				}
				#ifdef DEBUG_PARSE
				printf("After number: '%c%c%c'\n", line[0], line[1], line[2]);
				#endif
				break;

			case 'W' :
				if (!GetMessage(line, &len, error->warning))
					return (0);
				break;

			case 'E' :
				if (!GetMessage(line, &len, error->error))
					return (0);
				break;

			}
			continue;
		}

		if (pattern[i] != *line) {
			#ifdef DEBUG_PARSE
			printf("Failed text check at %d (%c and %c)\n", i, pattern[i],
			    *line);
			#endif
			return (0);
		}
		if (!len)
			return (0);

		line++;
		len--;
	}
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*GetFilename(char*line, int*len, char*filename)
{
	int i;

	if (((line[0] >= 'A' && line[0] <= 'Z') || (line[0] >= 'a' && line[0] <=
	    'z')) && line[1] == ':') {
		filename[0] = line[0];
		filename[1] = line[1];
		i = 2;
	} else
		i = 0;

	for (; i < *len; i++) {
		if (i >= MAX_FILENAME - 1)
			return (0);

		if (FilenameChar(line[i]))
			filename[i] = line[i];
		else {
			filename[i] = 0;
			break;
		}
	}

	if (!i || i == *len)
		return (0);

	*len -= i;

	#ifdef DEBUG
	printf("Got Filename(%s)\n", filename);
	#endif

	return (&line[i]);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int FilenameChar(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (1);

	if (ch >= 'A' && ch <= 'Z')
		return (1);

	if (ch >= '0' && ch <= '9')
		return (1);

	if (ch == '/' || ch == '\\')
		return (1);

	if (ch == '.' || ch == '-' || ch == '_' || ch == 32)
		return (1);

	#ifdef DEBUG_PARSE
	printf("Filename char didn't match: '%c'\n", ch);
	#endif
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*GetLineNo(char*line, int*len, int*lineNo)
{
	int i;
	char num[32];

	for (i = 0; i < *len; i++) {
		if (i >= 32)
			return (0);

		if (line[i] >= '0' && line[i] <= '9')
			num[i] = line[i];
		else {
			num[i] = 0;
			break;
		}
	}

	if (!i || i == *len) {
		#ifdef DEBUG_PARSE
		printf("Line number didn't match: %c%c%c\n", line[0], line[1], line[2]);
		#endif
		return (0);
	}

	*lineNo = atoi(num);

	*len -= i;
	#ifdef DEBUG_PARSE
	printf("Got Line Number(%d)\n", *lineNo);
	#endif


	return (&line[i]);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GetMessage(char*line, int*len, char*msg)
{
	int maxLen;

	if (*len >= MAX_ERROR - 1)
		maxLen = MAX_ERROR - 1;
	else
		maxLen = *len;

	memcpy(msg, line, maxLen);
	msg[maxLen] = 0;
	*len = 0;

	#ifdef DEBUG_PARSE
	printf("Got message(%s)\n", msg);
	#endif

	return (strlen(msg));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int AddErrorWarning(char*cwd, SHELL_ERROR*error)
{
	EDIT_ERROR*err;
	EDIT_FILE*file = 0;
	int existed = 0;
	char filename[MAX_FILENAME];

	if (error->lineNo && strlen(error->filename) && (strlen(error->error) ||
	    strlen(error->warning))) {
		if (OS_GetFullPathname(error->filename, filename, MAX_FILENAME)) {
			file = FileAlreadyLoaded(filename);
			if (file)
				existed = 1;
		}

		if (!file) {
			file = LoadFileWildcard(0, error->filename, LOAD_FILE_EXISTS |
			    LOAD_FILE_NOPAINT | LOAD_FILE_NOWILDCARD);

			if (!file) {
				file = LoadExistingFilename(cwd, error->filename);
				if (!file) {
					file = FindManualLoadFilename(error->filename);
					if (!file) {
						file = ManualLoadFilename(cwd, error->filename);
						if (file) {
							if (file == (EDIT_FILE*) - 1)
								return (1);

							AddManualLoadFilename(file, error->filename);
						}
					}
				}
			}
		}

		if (file) {
			if (ErrorWarningExists(file, error))
				return (0);

			err = (EDIT_ERROR*)OS_Malloc(sizeof(EDIT_ERROR));
			memset(err, 0, sizeof(EDIT_ERROR));

			OS_GetFilename(error->filename, 0, filename);

			err->existed = existed;
			err->file = file;
			err->error = OS_Malloc(strlen(filename) + strlen(error->error) +
			    strlen(error->warning) + 50);
			err->msg = OS_Malloc(strlen(error->error) + strlen(error->warning) +
			    50);
			err->org_warning = OS_Malloc(strlen(error->warning) + 1);
			err->org_error = OS_Malloc(strlen(error->error) + 1);
			err->pathname = OS_Malloc(strlen(file->pathname) + 1);

			strcpy(err->org_warning, error->warning);
			strcpy(err->org_error, error->error);
			strcpy(err->pathname, file->pathname);

			GotoPosition(file, error->lineNo, 1);

			if (strlen(error->error)) {
				err->is_error = 1;
				sprintf(err->error, "%s(%d) %s", filename, error->lineNo,
				    error->error);
				sprintf(err->msg, "Error: %s", error->error);
			} else
				if (strlen(error->warning)) {
					err->is_warning = 1;
					sprintf(err->error, "%s(%d) %s", filename, error->lineNo,
					    error->warning);
					sprintf(err->msg, "Warning: %s", error->warning);
				}

			err->bookmark = AddBookmark(file, file->cursor.line, error->col,
			    err->msg);
			err->lineNo = error->lineNo;
			err->col = error->col;
			err->prev = 0;
			err->next = errorList;

			if (errorList)
				errorList->prev = err;

			totalErrors++;

			errorList = err;
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
static int ErrorWarningExists(EDIT_FILE*file, SHELL_ERROR*error)
{
	EDIT_ERROR*walk;

	walk = errorList;

	while (walk) {
		if (walk->file == file) {
			if (!strcmp(error->error, walk->org_error) &&
			    !strcmp(error->warning, walk->org_warning) && walk->lineNo ==
			    error->lineNo && walk->col == error->col)
				return (1);
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
EDIT_FILE*ClearErrorWarnings(EDIT_FILE*current)
{
	EDIT_ERROR*walk, *next;
	EDIT_FILE*file;

	walk = errorList;

	while (walk) {
		next = walk->next;

		file = ValidateFile(walk->file, walk->pathname);

		if (file) {
			/* Delete the bookmark that we created. */
			if (walk->bookmark)
				DeleteBookmark(file, walk->bookmark);

			/* Close this file that we loaded if it's hasn't been modified. */
			if (!walk->existed && !FileModified(file)) {
				if (file == current)
					current = CloseFile(file);
				else
					CloseFile(file);
			}
		}

		OS_Free(walk->org_error);
		OS_Free(walk->org_warning);
		OS_Free(walk->pathname);
		OS_Free(walk->error);
		OS_Free(walk->msg);
		OS_Free(walk);

		walk = next;
	}

	totalErrors = 0;
	errorList = 0;
	lastError = 0;

	return (current);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*CreateErrorPicklist(EDIT_FILE*file)
{
	int i, total = 0, num_errors = 0, num_warnings = 0;
	char**list;
	EDIT_ERROR*err, **lut;
	char title[MAX_FILENAME];

	list = (char**)OS_Malloc(totalErrors*(int)sizeof(char*));
	lut = (EDIT_ERROR**)OS_Malloc(totalErrors*(int)sizeof(EDIT_ERROR*));

	err = errorList;

	for (i = 0; i < totalErrors; i++) {
		if (!err)
			break;

		if (ValidateFile(err->file, err->pathname)) {
			if (ValidateBookmark(err->file, err->bookmark)) {
				num_errors += err->is_error;
				num_warnings += err->is_warning;

				list[total] = err->error;
				lut[total] = err;
				total++;
			}
		}

		err = err->next;
	}

	if (!total) {
		OS_Free(list);
		OS_Free(lut);
		ClearErrorWarnings(file);
		return (0);
	}

	if (lastError == 0)
		lastError = 1;

	if (lastError > total)
		lastError = total;

	sprintf(title, "%d Error(s) / %d Warning(s)", num_errors, num_warnings);

	lastError = PickList("Errors/Warnings", total, GetScreenXDim() - 6, title,
	    list, lastError);

	if (lastError) {
		err = lut[lastError - 1];

		file = err->file;
		file->paint_flags = CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
		GotoBookmark(file, err->bookmark);
	}

	OS_Free(list);
	OS_Free(lut);

	file->paint_flags |= CURSOR_FLAG;

	return (file);
}


