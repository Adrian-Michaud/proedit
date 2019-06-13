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

static int ValidQuote(EDIT_LINE*line, int offset, int forward, int inside,
    char quote);

typedef struct operatorMatch
{
	int forward;
	char match;
	char find;
}OPERATOR_MATCH;

static OPERATOR_MATCH matchOps[] =
{
	{1, '{', '}'},
	{1, '[', ']'},
	{1, '(', ')'},
	{1, '<', '>'},
	{0, '}', '{'},
	{0, ']', '['},
	{0, ')', '('},
	{0, '>', '<'},
};

#define NUM_OPERATORS (sizeof(matchOps)/sizeof(OPERATOR_MATCH))

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void FindMatch(EDIT_FILE*file)
{
	char match, find = 0;
	int forward = 0, i;
	EDIT_LINE*line;

	line = file->cursor.line;

	if (file->cursor.offset < file->cursor.line->len)
		match = line->line[file->cursor.offset];
	else
		match = 0;

	for (i = 0; i < (int)NUM_OPERATORS; i++) {
		if (matchOps[i].match == match) {
			find = matchOps[i].find;
			forward = matchOps[i].forward;
			break;
		}
	}

	if (i == NUM_OPERATORS) {
		CenterBottomBar(1,
		    "[-] Cursor Must Sit On An Operator: {} [] () <>.. [-]");
		return ;
	}

	if (!MatchOperator(file, match, find, forward)) {
		CenterBottomBar(1, "[-] Could Not Find Matching Operator For '%c' [-]",
		    match);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int MatchOperator(EDIT_FILE*file, char match, char find, int forward)
{
	EDIT_LINE*line;
	int openQ = 0, openS = 0, open = 0;
	int line_number, offset;
	int i;

	line = file->cursor.line;

	line_number = file->cursor.line_number;

	if (forward) {
		offset = file->cursor.offset + 1;

		while (line) {
			for (i = offset; i < line->len; i++) {
				if (line->line[i] == '\\' && (openS || openQ)) {
					i++;
					continue;
				}

				if (line->line[i] == '"' && !openS && ValidQuote(line, i, 1,
				    openQ, line->line[i])) {
					if (!openQ)
						openQ = 1;
					else
						openQ = 0;
				}

				if (line->line[i] == '\'' && !openQ && ValidQuote(line, i, 1,
				    openS, line->line[i])) {
					if (!openS)
						openS = 1;
					else
						openS = 0;
				}

				if (line->line[i] == match && !openQ && !openS)
					open++;

				if (line->line[i] == find && !openQ && !openS) {
					if (!open) {
						GotoPosition(file, line_number + 1, i + 1);
						return (1);
					}
					open--;
				}
			}
			line_number++;
			offset = 0;
			line = line->next;
		}
	} else {
		offset = file->cursor.offset - 1;

		while (line) {
			if (line->line) {
				for (i = offset; i >= 0; i--) {
					if (line->line[i] == '"' && !openS && ValidQuote(line, i, 0,
					    openQ, line->line[i])) {
						if (!openQ)
							openQ = 1;
						else {
							if (!i || line->line[i - 1] != '\\')
								openQ = 0;
						}
					}

					if (line->line[i] == '\'' && !openQ && ValidQuote(line, i,
					    0, openS, line->line[i])) {
						if (!openS)
							openS = 1;
						else {
							if (!i || line->line[i - 1] != '\\')
								openS = 0;
						}
					}

					if (line->line[i] == match && !openQ && !openS)
						open++;

					if (line->line[i] == find && !openQ && !openS) {
						if (!open) {
							GotoPosition(file, line_number + 1, i + 1);
							return (1);
						}
						open--;
					}
				}
			}

			line_number--;
			line = line->prev;

			if (line) {
				if (line->len)
					offset = line->len - 1;
				else
					offset = 0;
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
static int ValidQuote(EDIT_LINE*line, int offset, int forward, int inside,
    char quote)
{
	int i, total = 0;

	if (inside)
		return (1);

	if (forward) {
		for (i = offset + 1; i < line->len; i++) {
			if (line->line[i] == '\\') {
				i++;
				total++;
				continue;
			}
			if (line->line[i] == quote)
				return (1);

			if ((++total > 2) && quote == '\'')
				return (0);
		}
	} else {
		for (i = offset - 1; i >= 0; i--) {
			if (i && line->line[i - 1] == '\\') {
				i--;
				total++;
				continue;
			}
			if (line->line[i] == quote)
				return (1);
			if ((++total > 2) && quote == '\'')
				return (0);
		}
	}

	return (0);
}


