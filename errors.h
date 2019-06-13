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


#ifndef __ERRORS_H
#define __ERRORS_H

typedef struct shellError
{
	char filename[MAX_FILENAME];
	char error[MAX_FILENAME];
	char warning[MAX_FILENAME];
	char library[MAX_FILENAME];
	char object[MAX_FILENAME];
	int lineNo;
	int col;
}SHELL_ERROR;

#define MAX_ERROR   256
#define MAX_WARNING 256
#define MAX_BUILD   256

typedef struct errorLists
{
	char*msg;
	char*error;
	char*org_error;
	char*org_warning;
	char*pathname;
	int lineNo;
	int col;
	int existed;
	int is_warning;
	int is_error;
	EDIT_FILE*file;
	EDIT_BOOKMARK*bookmark;
	struct errorLists*prev;
	struct errorLists*next;
}EDIT_ERROR;

SHELL_ERROR*ProcessErrorLine(char*text, int len);
int AddErrorWarning(char*cwd, SHELL_ERROR*error);
EDIT_FILE*ClearErrorWarnings(EDIT_FILE*current);
EDIT_FILE*CreateErrorPicklist(EDIT_FILE*file);
int NumberOfErrors(void);


#endif /* __ERRORS_H */


