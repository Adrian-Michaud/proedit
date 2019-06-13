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
#include "color_c.h"

#define MAX_FIND 256

static SCR_PTR color_operator;
static SCR_PTR color_number;
static SCR_PTR color_char;
static SCR_PTR color_string;
static SCR_PTR color_comment;
static SCR_PTR color_preprocessor;

#define LINE_FLAG_HEAD  (LINE_FLAG_CUSTOM<<1)
#define LINE_FLAG_TAIL  (LINE_FLAG_CUSTOM<<2)
#define LINE_FLAG_MID   (LINE_FLAG_CUSTOM<<3)

#define LINE_FLAG_OPENS (LINE_FLAG_CUSTOM<<4)
#define LINE_FLAG_OPENQ (LINE_FLAG_CUSTOM<<5)

#define LINE_FLAG_MASK (LINE_FLAG_HEAD|LINE_FLAG_TAIL|LINE_FLAG_MID)

static void ClearMidTail(EDIT_LINE*line);

static int SymbolMatch(char*src, char*dest, int len);
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

static char operatorLUT[] = {"|&,.><=!()[]{}%^:;~-+*/"};

static struct textBlockColorLUT
{
	char*start, *end;
	char escape, len1, len2;
	SCR_PTR*color;
}blockLUT[] =
{
	{"<!--", "-->", 0, 4, 3, &color_comment},
	{"<?", "?>", 0, 2, 2, &color_number},
	{"<!DOC", ">", 0, 5, 1, &color_number},
	{"'", "'", '\\', 1, 1, &color_char},
	{"\"", "\"", '\\', 1, 1, &color_string},
};

#define BLOCK_TYPE_HTML_COMMENT   1
#define BLOCK_TYPE_XML_COMMENT    2
#define BLOCK_TYPE_DOCTYPE        3
#define BLOCK_TYPE_SQUOTE         4
#define BLOCK_TYPE_DQUOTE         5


static char*numberLUT = {"0123456789"};
static char*numberLUT2 = {"0123456789xX.lLfFabcdefABCDEF"};

#define NUMBER_BLOCK_ENTRIES (sizeof(blockLUT) / sizeof(struct textBlockColorLUT))

static struct textColorLUT
{
	char*text;
	int len;
	SCR_PTR*color;
}colorLUT[] =
{

	{"cellpadding", 11, &color_preprocessor},
	{"cellspacing", 11, &color_preprocessor},
	{"http-equiv", 10, &color_preprocessor},
	{"background", 10, &color_preprocessor},
	{"blockquote", 10, &color_preprocessor},
	{"language", 8, &color_preprocessor},
	{"onsubmit", 8, &color_preprocessor},
	{"onchange", 8, &color_preprocessor},
	{"bgcolor", 7, &color_preprocessor},
	{"content", 7, &color_preprocessor},
	{"colspan", 7, &color_preprocessor},
	{"action", 6, &color_preprocessor},
	{"border", 6, &color_preprocessor},
	{"center", 6, &color_preprocessor},
	{"height", 6, &color_preprocessor},
	{"hspace", 6, &color_preprocessor},
	{"method", 6, &color_preprocessor},
	{"nowrap", 6, &color_preprocessor},
	{"script", 6, &color_preprocessor},
	{"valign", 6, &color_preprocessor},
	{"vspace", 6, &color_preprocessor},
	{"select", 6, &color_preprocessor},
	{"target", 6, &color_preprocessor},
	{"option", 6, &color_preprocessor},
	{"class", 5, &color_preprocessor},
	{"small", 5, &color_preprocessor},
	{"align", 5, &color_preprocessor},
	{"input", 5, &color_preprocessor},
	{"style", 5, &color_preprocessor},
	{"table", 5, &color_preprocessor},
	{"title", 5, &color_preprocessor},
	{"value", 5, &color_preprocessor},
	{"vlink", 5, &color_preprocessor},
	{"width", 5, &color_preprocessor},
	{"xmlns", 5, &color_preprocessor},
	{"body", 4, &color_preprocessor},
	{"href", 4, &color_preprocessor},
	{"html", 4, &color_preprocessor},
	{"type", 4, &color_preprocessor},
	{"link", 4, &color_preprocessor},
	{"meta", 4, &color_preprocessor},
	{"name", 4, &color_preprocessor},
	{"size", 4, &color_preprocessor},
	{"span", 4, &color_preprocessor},
	{"code", 4, &color_preprocessor},
	{"text", 4, &color_preprocessor},
	{"face", 4, &color_preprocessor},
	{"font", 4, &color_preprocessor},
	{"form", 4, &color_preprocessor},
	{"lang", 4, &color_preprocessor},
	{"head", 4, &color_preprocessor},
	{"alt", 3, &color_preprocessor},
	{"pre", 3, &color_preprocessor},
	{"src", 3, &color_preprocessor},
	{"div", 3, &color_preprocessor},
	{"dir", 3, &color_preprocessor},
	{"img", 3, &color_preprocessor},
	{"xml", 3, &color_preprocessor},
	{"rel", 3, &color_preprocessor},
	{"h1", 2, &color_preprocessor},
	{"h2", 2, &color_preprocessor},
	{"h3", 2, &color_preprocessor},
	{"h4", 2, &color_preprocessor},
	{"td", 2, &color_preprocessor},
	{"em", 2, &color_preprocessor},
	{"th", 2, &color_preprocessor},
	{"tr", 2, &color_preprocessor},
	{"br", 2, &color_preprocessor},
	{"dl", 2, &color_preprocessor},
	{"dt", 2, &color_preprocessor},
	{"dd", 2, &color_preprocessor},
	{"ol", 2, &color_preprocessor},
	{"li", 2, &color_preprocessor},
	{"ul", 2, &color_preprocessor},
	{"id", 2, &color_preprocessor},
	{"b", 1, &color_preprocessor},
	{"a", 1, &color_preprocessor},
	{"p", 1, &color_preprocessor},
};


#define NUMBER_TEXT_ENTRIES (sizeof(colorLUT)/sizeof(struct textColorLUT))

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ColorLines_Html(EDIT_LINE*lines)
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
void SetupColors_Html(void)
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

	color_preprocessor = (SCR_PTR)(GetConfigInt(CONFIG_INT_BG_COLOR) |
	GetConfigInt(CONFIG_INT_PREPROCESSOR_COLOR));
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int Colorize_Html_LinePfn(EDIT_FILE*file, EDIT_LINE*line, int op, int arg)
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
void Colorize_Html(EDIT_LINE*line, int pan, int len, SCR_PTR*screen,
    int address, char defaultAttr)
{
	int column, cf = 0, cf2 = 0, length, i, skip = 0, openA = 0;
	char ch, attr, prev;

	attr = defaultAttr;
	prev = ED_KEY_SPACE;

	length = pan + len;

	if (line->flags&LINE_FLAG_MID) {
		attr = color_comment;
		cf = length;
	} else
		if (line->flags&LINE_FLAG_TAIL) {
			cf2 = BLOCK_TYPE_HTML_COMMENT;
			attr = color_comment;
		}

	/* Write out paned line. */
	for (column = 0; column < length; column++) {
		ch = line->line[column];

		if (ch == '<')
			openA++;

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

		if (!cf && !cf2 && openA) {
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

		if (ch == '>' && openA)
			openA--;

		if (column >= pan) {
			if (!openA && !cf && !cf2)
				screen[address] = defaultAttr;
			else
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

		if (i + 4 <= line->len) {
			if (!strncmp("<!--", &line->line[i], 4) &&
			    !(*state&LINE_FLAG_HEAD)) {
				if (!(*state&(LINE_FLAG_OPENS | LINE_FLAG_OPENQ))) {
					*state |= LINE_FLAG_HEAD;
					*index = i + 4;
					return (LINE_FLAG_HEAD);
				}
			}
		}

		if (i + 3 <= line->len) {
			if (!strncmp("-->", &line->line[i], 3)) {
				if (found < MAX_FIND) {
					foundIndex[found] = i + 3;
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

	if (SymbolMatch(colorLUT[index].text, &line->line[column], colorLUT[index].
	    len)) {
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
static int SymbolMatch(char*src, char*dest, int len)
{
	int i;
	char ch1, ch2;

	for (i = 0; i < len; i++) {
		ch1 = src[i];
		ch2 = dest[i];

		if (ch1 >= 'a' && ch1 <= 'z')
			ch1 -= 32;

		if (ch2 >= 'a' && ch2 <= 'z')
			ch2 -= 32;

		if (ch1 != ch2)
			return (0);
	}
	return (1);
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

	if (ch == '_' || ch == '-')
		return (0);

	return (1);
}


