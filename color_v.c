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
#include <errno.h>
#include <stdlib.h>
#include "proedit.h"
#include "color_v.h"

#define MAX_FIND 256

static SCR_PTR color_operator;
static SCR_PTR color_number;
static SCR_PTR color_char;
static SCR_PTR color_string;
static SCR_PTR color_comment;
static SCR_PTR color_keyword;
static SCR_PTR color_preprocessor;

#define LINE_FLAG_HEAD  (LINE_FLAG_CUSTOM<<1)
#define LINE_FLAG_TAIL  (LINE_FLAG_CUSTOM<<2)
#define LINE_FLAG_MID   (LINE_FLAG_CUSTOM<<3)

#define LINE_FLAG_OPENS (LINE_FLAG_CUSTOM<<4)
#define LINE_FLAG_OPENQ (LINE_FLAG_CUSTOM<<5)

#define LINE_FLAG_MASK (LINE_FLAG_HEAD|LINE_FLAG_TAIL|LINE_FLAG_MID)

static void ClearMidTail(EDIT_LINE*line);

static int GetLineFlags(EDIT_LINE*line);
static void ProcessEdit(EDIT_LINE*line);
static void ProcessInsert(EDIT_LINE*line);
static void ProcessDelete(EDIT_LINE*line);
static int ProcessComments(EDIT_LINE*line, int*index, int*state);
static int SymbolCheck(int index, EDIT_LINE*line, int column);
static int NotAChar(char ch);
static void FindNewTail(EDIT_LINE*line);

/* Macro used for colorizing algorythim */
#define NOTANUM(ch)  ((ch>='0'&&ch<='9')?(0):(1))

static char operatorLUT[] = {"|&,.><=!()[]{}%^:;~-+*/@"};

static struct textBlockColorLUT
{
	char*start, *end;
	char escape, len1, len2;
	SCR_PTR*color;
}blockLUT[] =
{
	//      {"'", "'"   , '\\', 1,1, &color_char },
	{"\"", "\"", '\\', 1, 1, &color_string},
	{"/*", "*/", 0, 2, 2, &color_comment},
	{"//", "\r", 0, 2, 1, &color_comment},
};

#define BLOCK_TYPE_SQUOTE      1
#define BLOCK_TYPE_DQUOTE      1
#define BLOCK_TYPE_C_COMMENT   2
#define BLOCK_TYPE_CPP_COMMENT 3

static char*numberLUT = {"0123456789"};
static char*numberLUT2 = {"0123456789abcdefABCDEF'hbzxHBZX"};

#define NUMBER_BLOCK_ENTRIES (sizeof(blockLUT) / sizeof(struct textBlockColorLUT))

static struct textColorLUT
{
	char*text;
	int len;
	SCR_PTR*color;
}colorLUT[] =
{
	{"always", 6, &color_keyword},
	{"and", 3, &color_keyword},
	{"assign", 6, &color_keyword},
	{"begin", 5, &color_keyword},
	{"buf", 3, &color_keyword},
	{"bufif0", 6, &color_keyword},
	{"bufif1", 6, &color_keyword},
	{"case", 4, &color_keyword},
	{"casex", 5, &color_keyword},
	{"casez", 5, &color_keyword},
	{"cmos", 4, &color_keyword},
	{"deassign", 8, &color_keyword},
	{"default", 7, &color_keyword},
	{"defparam", 8, &color_keyword},
	{"disable", 7, &color_keyword},
	{"edge", 4, &color_keyword},
	{"else", 4, &color_keyword},
	{"endcase", 7, &color_keyword},
	{"end", 3, &color_keyword},
	{"endfunction", 11, &color_keyword},
	{"endmodule", 9, &color_keyword},
	{"endprimitive", 12, &color_keyword},
	{"endspecify", 10, &color_keyword},
	{"endtable", 8, &color_keyword},
	{"endtask", 7, &color_keyword},
	{"event", 5, &color_keyword},
	{"force", 5, &color_keyword},
	{"for", 3, &color_keyword},
	{"forever", 7, &color_keyword},
	{"function", 8, &color_keyword},
	{"highz0", 6, &color_keyword},
	{"highz1", 6, &color_keyword},
	{"if", 2, &color_keyword},
	{"ifnone", 6, &color_keyword},
	{"initial", 7, &color_keyword},
	{"inout", 5, &color_keyword},
	{"input", 5, &color_keyword},
	{"integer", 7, &color_keyword},
	{"join", 4, &color_keyword},
	{"large", 5, &color_keyword},
	{"macromodule", 11, &color_keyword},
	{"medium", 6, &color_keyword},
	{"module", 6, &color_keyword},
	{"nand", 4, &color_keyword},
	{"negedge", 7, &color_keyword},
	{"nmos", 4, &color_keyword},
	{"nor", 3, &color_keyword},
	{"not", 3, &color_keyword},
	{"notif1", 6, &color_keyword},
	{"notif0", 6, &color_keyword},
	{"or", 2, &color_keyword},
	{"output", 6, &color_keyword},
	{"parameter", 9, &color_keyword},
	{"pmos", 4, &color_keyword},
	{"posedge", 7, &color_keyword},
	{"primitive", 9, &color_keyword},
	{"pull0", 5, &color_keyword},
	{"pull1", 5, &color_keyword},
	{"pulldown", 8, &color_keyword},
	{"pullup", 6, &color_keyword},
	{"rcmos", 5, &color_keyword},
	{"real", 4, &color_keyword},
	{"realtime", 8, &color_keyword},
	{"reg", 3, &color_keyword},
	{"release", 7, &color_keyword},
	{"repeat", 6, &color_keyword},
	{"rnmos", 5, &color_keyword},
	{"rpmos", 5, &color_keyword},
	{"rtran", 5, &color_keyword},
	{"rtranif0", 8, &color_keyword},
	{"rtranif1", 8, &color_keyword},
	{"scalared", 8, &color_keyword},
	{"small", 5, &color_keyword},
	{"specify", 7, &color_keyword},
	{"specparam", 9, &color_keyword},
	{"strong0", 7, &color_keyword},
	{"strong1", 7, &color_keyword},
	{"supply1", 7, &color_keyword},
	{"supply0", 7, &color_keyword},
	{"table", 5, &color_keyword},
	{"task", 4, &color_keyword},
	{"time", 4, &color_keyword},
	{"tran", 4, &color_keyword},
	{"tranif0", 7, &color_keyword},
	{"tranif1", 7, &color_keyword},
	{"triand", 6, &color_keyword},
	{"tri", 3, &color_keyword},
	{"tri0", 4, &color_keyword},
	{"tri1", 4, &color_keyword},
	{"trior", 5, &color_keyword},
	{"trireg", 6, &color_keyword},
	{"vectored", 8, &color_keyword},
	{"wait", 4, &color_keyword},
	{"wand", 4, &color_keyword},
	{"weak1", 5, &color_keyword},
	{"weak0", 5, &color_keyword},
	{"while", 5, &color_keyword},
	{"wire", 4, &color_keyword},
	{"wor", 3, &color_keyword},
	{"xnor", 4, &color_keyword},
	{"xor", 3, &color_keyword},



	{"`celldefine", 11, &color_preprocessor},
	{"`endcelldefine", 14, &color_preprocessor},
	{"`default_nettype", 16, &color_preprocessor},
	{"`define", 7, &color_preprocessor},
	{"`undef", 6, &color_preprocessor},
	{"`ifdef", 6, &color_preprocessor},
	{"`ifndef", 7, &color_preprocessor},
	{"`else", 5, &color_preprocessor},
	{"`endif", 6, &color_preprocessor},
	{"`include", 8, &color_preprocessor},
	{"`resetall", 9, &color_preprocessor},
	{"`timescale", 10, &color_preprocessor},
	{"`unconnected_drive", 18, &color_preprocessor},
	{"`nounconnected_drive", 20, &color_preprocessor},
	{"`uselib", 7, &color_preprocessor},


};

#define NUMBER_TEXT_ENTRIES (sizeof(colorLUT)/sizeof(struct textColorLUT))

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ColorLines_V(EDIT_LINE*lines)
{
	EDIT_LINE*walk;

	walk = lines;

	/* Strip existing colorizing flags from all lines. */
	while (walk) {
		walk->flags &= ~LINE_FLAG_MASK;
		walk = walk->next;
	}

	walk = lines;

	/* Colorize comments. */
	while (walk) {
		ProcessEdit(walk);
		walk = walk->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetupColors_V(void)
{
	color_operator = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_OPERATOR_COLOR));

	color_number = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_NUMERIC_COLOR));

	color_char = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_CHARACTER_COLOR));

	color_string = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_STRING_COLOR));

	color_comment = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_COMMENTS_COLOR));

	color_keyword = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_KEYWORD_COLOR));

	color_preprocessor = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_PREPROCESSOR_COLOR));
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int Colorize_V_LinePfn(EDIT_FILE*file, EDIT_LINE*line, int op, int arg)
{
	file = file;
	arg = arg;

	if (op&LINE_OP_EDIT)
		ProcessEdit(line);

	if (op&LINE_OP_INSERT)
		ProcessInsert(line);

	if (op&LINE_OP_DELETE)
		ProcessDelete(line);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ProcessInsert(EDIT_LINE*line)
{
	int flags, prev = 0, next = 0;

	line->flags &= ~LINE_FLAG_MASK;

	flags = GetLineFlags(line);

	if (line->prev)
		prev = line->prev->flags;

	if (line->next)
		next = line->next->flags;

	if (!flags && (prev&(LINE_FLAG_MID | LINE_FLAG_HEAD))) {
		line->flags |= LINE_FLAG_MID;
		return ;
	}

	if (!flags && (next&(LINE_FLAG_MID | LINE_FLAG_TAIL))) {
		line->flags |= LINE_FLAG_MID;
		return ;
	}

	if (!flags)
		return ;

	/* Insert a head */
	if (flags == LINE_FLAG_HEAD)
		FindNewTail(line->next);

	/* Insert a tail inside of a comment */
	if ((flags == LINE_FLAG_TAIL) && (prev&(LINE_FLAG_MID | LINE_FLAG_HEAD)))
		ClearMidTail(line->next);

	/* Insert a bogus tail */
	if ((flags == LINE_FLAG_TAIL) && !(prev&(LINE_FLAG_MID | LINE_FLAG_HEAD)))
		return ;

	line->flags |= flags;
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ProcessDelete(EDIT_LINE*line)
{
	int prev = 0;

	if (line->prev)
		prev = line->prev->flags;

	if ((line->flags&LINE_FLAG_HEAD) && (line->flags&LINE_FLAG_TAIL))
		return ;

	if (line->flags&LINE_FLAG_MID)
		return ;

	if (line->flags&LINE_FLAG_TAIL)
		FindNewTail(line->next);

	if (line->flags&LINE_FLAG_HEAD) {
		if (!(prev&(LINE_FLAG_HEAD | LINE_FLAG_MID)))
			ClearMidTail(line->next);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ProcessEdit(EDIT_LINE*line)
{
	int flags, newFlags = 0, prev = 0;

	if (line->prev)
		prev = line->prev->flags;

	flags = GetLineFlags(line);

	if (!flags && line->flags&LINE_FLAG_MID)
		return ;

	if (flags == (line->flags&LINE_FLAG_MASK))
		return ;

	/* Insert a tail inside of a comment */
	if ((flags == LINE_FLAG_TAIL) && (line->flags&(LINE_FLAG_MID |
	    LINE_FLAG_HEAD))) {
		if (!(line->flags&LINE_FLAG_TAIL))
			ClearMidTail(line->next);
	}

	/* Add a bogus tail */
	if ((flags == LINE_FLAG_TAIL) && !(line->flags&LINE_FLAG_MASK))
		return ;

	/* Add a head to form a dual Tail/Head configuration. */
	if (flags == (LINE_FLAG_TAIL | LINE_FLAG_HEAD))
		FindNewTail(line->next);

	/* Add a new standalone head */
	if ((flags == LINE_FLAG_HEAD) && (!(line->flags&(LINE_FLAG_HEAD |
	    LINE_FLAG_MID))))
		FindNewTail(line->next);

	/* Remove the head */
	if ((line->flags&LINE_FLAG_HEAD) && (!(flags&LINE_FLAG_HEAD))) {
		/* Remove the head from a dual Tail/Head configuration. */
		if (line->flags&LINE_FLAG_TAIL)
			ClearMidTail(line->next);
		else {
			if (!(prev&(LINE_FLAG_HEAD | LINE_FLAG_MID)))
				ClearMidTail(line->next);
			else
				newFlags |= LINE_FLAG_MID;
		}
	}

	/* Remove the tail */
	if ((line->flags&LINE_FLAG_TAIL) && (!(flags&LINE_FLAG_TAIL))) {
		newFlags |= LINE_FLAG_MID;
		FindNewTail(line->next);
	}

	/* Clear the bogus tail if any. */
	if ((flags&LINE_FLAG_TAIL) && (!(prev&(LINE_FLAG_MID | LINE_FLAG_HEAD))))
		flags &= ~LINE_FLAG_TAIL;

	line->flags &= ~LINE_FLAG_MASK;
	line->flags |= (flags | newFlags);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GetLineFlags(EDIT_LINE*line)
{
	int type, flags = 0, index = 0, head = 0, state = 0;

	for (; ; ) {
		type = ProcessComments(line, &index, &state);

		if (type == LINE_FLAG_TAIL)
			flags = LINE_FLAG_TAIL;
		else
			break;
	}

	for (; ; ) {
		if (type == LINE_FLAG_HEAD) {
			head++;
			type = ProcessComments(line, &index, &state);

			if (type == LINE_FLAG_TAIL) {
				head--;
				type = ProcessComments(line, &index, &state);
				continue;
			}
			break;
		} else
			break;
	}

	if (head)
		flags |= LINE_FLAG_HEAD;

	if (type == LINE_FLAG_TAIL)
		flags |= LINE_FLAG_TAIL;

	return (flags);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ClearMidTail(EDIT_LINE*line)
{
	while (line) {
		if (line->flags&LINE_FLAG_MID)
			line->flags &= ~LINE_FLAG_MASK;
		else {
			if (line->flags == LINE_FLAG_HEAD)
				return ;

			if (line->flags&LINE_FLAG_TAIL)
				line->flags &= ~LINE_FLAG_TAIL;
			return ;
		}

		line = line->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void FindNewTail(EDIT_LINE*line)
{
	int type, index, state = 0;

	while (line) {
		index = 0;

		type = ProcessComments(line, &index, &state);

		if (type == LINE_FLAG_HEAD) {
			line->flags |= LINE_FLAG_TAIL;
			return ;
		}

		if (type == LINE_FLAG_TAIL) {
			line->flags |= LINE_FLAG_TAIL;
			line->flags &= ~LINE_FLAG_MID;
			return ;
		}

		line->flags &= ~LINE_FLAG_MASK;
		line->flags |= LINE_FLAG_MID;

		line = line->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void Colorize_V(EDIT_LINE*line, int pan, int len, SCR_PTR*screen, int address,
    char defaultAttr)
{
	int column, cf = 0, cf2 = 0, length, i, skip = 0;
	char ch, attr, prev;

	attr = defaultAttr;
	prev = ED_KEY_SPACE;

	length = pan + len;

	if (line->flags&LINE_FLAG_MID) {
		attr = color_comment;
		cf = length;
	} else
		if (line->flags&LINE_FLAG_TAIL) {
			cf2 = BLOCK_TYPE_C_COMMENT;
			attr = color_comment;
		}

	/* Write out paned line. */
	for (column = 0; column < length; column++) {
		ch = line->line[column];

		if (ch == ED_KEY_TAB || ch == ED_KEY_TABPAD)
			ch = ED_KEY_SPACE;

		if (cf2 && !skip) {
			if (blockLUT[cf2 - 1].escape && blockLUT[cf2 - 1].escape == line->
			    line[column])
				skip = 2;
			else {
				if (column + blockLUT[cf2 - 1].len2 <= length) {
					if (!strncmp(blockLUT[cf2 - 1].end, &line->line[column],
					    blockLUT[cf2 - 1].len2)) {
						cf = blockLUT[cf2 - 1].len2;
						cf2 = 0;
					}
				}
			}
		}

		if (!cf && !cf2) {
			for (i = 0; i < (int)NUMBER_BLOCK_ENTRIES; i++) {
				if (column + blockLUT[i].len1 <= length) {
					if (!strncmp(blockLUT[i].start, &line->line[column],
					    blockLUT[i].len1)) {
						cf2 = i + 1;
						attr = *blockLUT[i].color;
						break;
					}
				}
			}

			if (i == NUMBER_BLOCK_ENTRIES) {
				if (ch != ED_KEY_SPACE && NotAChar(prev)) {

					if (NOTANUM(prev) && strchr(numberLUT, ch)) {
						cf = 1;
						while (column + cf < length && strchr(numberLUT2,
						    line->line[column + cf]))
							cf++;
						attr = color_number;
					} else {
						for (i = 0; i < (int)NUMBER_TEXT_ENTRIES; i++) {
							if (SymbolCheck(i, line, column)) {
								cf = colorLUT[i].len;
								attr = *colorLUT[i].color;
								break;
							}
						}
					}
				}

				if (!cf) {
					if (strchr(operatorLUT, ch)) {
						attr = color_operator;
						cf = 1;
					}
				}
			}
		}
		prev = ch;

		if (column >= pan) {
			screen[address] = attr;
			address += 2;
		}

		if (cf) {
			if (!--cf)
				attr = defaultAttr;
		}
		if (skip)
			skip--;
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ProcessComments(EDIT_LINE*line, int*index, int*state)
{
	int i, expire = 0, found = 0;
	int foundIndex[MAX_FIND];
	int foundState[MAX_FIND];

	for (i = *index; i < line->len; i++) {
		if (!(*state&LINE_FLAG_HEAD)) {
			/* Skip over escape character */
			if (line->line[i] == '\\') {
				i++;
				continue;
			}

			if (!(*state&LINE_FLAG_OPENS) && line->line[i] == '"') {
				if (*state&LINE_FLAG_OPENQ)
					*state &= ~LINE_FLAG_OPENQ;
				else
					*state |= LINE_FLAG_OPENQ;
			}

			if (!(*state&LINE_FLAG_OPENQ) && line->line[i] == '\'' && !expire) {
				if (*state&LINE_FLAG_OPENS)
					*state &= ~LINE_FLAG_OPENS;
				else {
					*state |= LINE_FLAG_OPENS;
					expire = 2;
				}
			}
		}

		if (i + 2 <= line->len) {
			if (!strncmp("//", &line->line[i], 2)) {
				if (!(*state&(LINE_FLAG_OPENS | LINE_FLAG_OPENQ)) && (*state !=
				    LINE_FLAG_HEAD)) {
					*index = line->len;
					return (0);
				}
			}

			if (!strncmp("/*", &line->line[i], 2) && !(*state&LINE_FLAG_HEAD)) {
				if (!(*state&(LINE_FLAG_OPENS | LINE_FLAG_OPENQ))) {
					*state |= LINE_FLAG_HEAD;
					*index = i + 2;
					return (LINE_FLAG_HEAD);
				}
			}

			if (!strncmp("*/", &line->line[i], 2)) {
				if (found < MAX_FIND) {
					foundIndex[found] = i + 2;
					foundState[found] = (*state)&(LINE_FLAG_OPENS |
					    LINE_FLAG_OPENQ);
					found++;
				}

				if (!(*state&(LINE_FLAG_OPENS | LINE_FLAG_OPENQ))) {
					*state &= ~LINE_FLAG_HEAD;
					*index = i + 2;
					return (LINE_FLAG_TAIL);
				}
			}
		}

		if (expire) {
			expire--;

			if (!expire)
				*state &= ~LINE_FLAG_OPENS;
		}
	}

	for (i = 0; i < found; i++) {
		/* Was this a bogus single/double quote failure? */
		if (foundState[i] == (*state&(LINE_FLAG_OPENS | LINE_FLAG_OPENQ))) {
			*state &= ~LINE_FLAG_HEAD;
			*index = foundIndex[i];
			return (LINE_FLAG_TAIL);
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
static int SymbolCheck(int index, EDIT_LINE*line, int column)
{
	if (column + colorLUT[index].len > line->len)
		return (0);

	if (!memcmp(colorLUT[index].text, &line->line[column],
	    colorLUT[index].len)) {
		if (column + colorLUT[index].len == line->len)
			return (1);

		if (NotAChar(line->line[column + colorLUT[index].len]))
			return (1);
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int NotAChar(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (0);

	if (ch >= 'A' && ch <= 'Z')
		return (0);

	if (ch == '_')
		return (0);

	return (1);
}


