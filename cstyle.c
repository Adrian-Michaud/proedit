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
#include "cstyle.h"

#if 0
#define NESTED_COMMENT_ERRORS
#endif


static char*keywords[] =
{"int", "double", "float", "long", "char", "short",
	"void", "struct", "signed", "unsigned", "volatile", "const",
	"extern", "static", "for", "sizeof", "break", "case",
	"goto", "if", "switch", "continue", "default", "typedef",
"union", "do", "else", "register", "return", "while"};

#define NUMBER_KEYWORDS (sizeof(keywords)/sizeof(char *))

static EDIT_TOKEN*GetLineToken(EDIT_FILE*file, EDIT_TOKEN*prev, EDIT_LINE*line,
    int lineNumber, int*index);
static int IsKeyword(char*keyword);
static EDIT_TOKEN*StringConstant(char*text, int i, int len, EDIT_FILE*file,
    int lineNo, int*pos);
static EDIT_TOKEN*Preprocessor(char*text, int i, int len, int lineNo, int*pos);
static int ValidBreakToken(EDIT_TOKEN*token);
static EDIT_TOKEN*CreateToken(int type, int line, int column, char*string, int
    len);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_TOKEN*TokenizeLines(EDIT_FILE*file, EDIT_LINE*lines)
{
	EDIT_TOKEN*tokens, *walk, *end, *prev;
	EDIT_LINE*line;
	int index, lineNumber;

	if (file)
		lineNumber = MIN(file->copyFrom.line, file->copyTo.line);
	else
		lineNumber = 0;

	index = 0;
	tokens = 0;
	prev = 0;
	end = 0;

	line = lines;

	while (line) {
		walk = GetLineToken(file, prev, line, lineNumber, &index);

		if (index == -1) {
			DeallocTokens(tokens);
			return (0);
		}

		if (walk)
			prev = walk;

		if (walk) {
			if (end) {
				end->next = walk;
				walk->prev = end;
				end = walk;
			} else {
				tokens = walk;
				end = walk;
			}
		}

		if (index == 0) {
			line = line->next;
			lineNumber++;

			walk = CreateToken(TOKEN_NEWLINE, lineNumber, 0, "", 1);

			if (end) {
				end->next = walk;
				walk->prev = end;
				end = walk;
			} else {
				tokens = walk;
				end = walk;
			}
		}
	}

	#if 0
	DumpTokens(tokens);
	#endif

	return (tokens);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*CreateToken(int type, int line, int column, char*string, int
    len)
{
	EDIT_TOKEN*token;

	token = (EDIT_TOKEN*)OS_Malloc(sizeof(EDIT_TOKEN));

	token->type = type;
	token->next = 0;
	token->ident = 0;
	token->prev = 0;
	token->line = line;
	token->column = column;
	token->string = OS_Malloc(len);
	token->stringLen = len;

	memcpy(token->string, string, len);

	return (token);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DisplayToken(EDIT_TOKEN*token)
{
	char*type;
	int i;

	switch (token->type) {
	case TOKEN_SYMBOL :
		type = "TOKEN_SYMBOL        "; break;
	case TOKEN_WS :
		type = "TOKEN_WS            "; break;
	case TOKEN_IDENT :
		type = "TOKEN_IDENT         "; break;
	case TOKEN_NUMBER :
		type = "TOKEN_NUMBER        "; break;
	case TOKEN_HEX :
		type = "TOKEN_HEX           "; break;
	case TOKEN_STRING :
		type = "TOKEN_STRING        "; break;
	case TOKEN_COMMENT_BEGIN :
		type = "TOKEN_COMMENT_BEGIN "; break;
	case TOKEN_COMMENT_LINE :
		type = "TOKEN_COMMENT_LINE  "; break;
	case TOKEN_COMMENT_END :
		type = "TOKEN_COMMENT_END   "; break;
	case TOKEN_PREPROC :
		type = "TOKEN_PREPROC       "; break;
	case TOKEN_FLOATING :
		type = "TOKEN_FLOATING      "; break;
	case TOKEN_KEYWORD :
		type = "TOKEN_KEYWORD       "; break;
	case TOKEN_TAB_PLUS :
		type = "TOKEN_TAB_PLUS      "; break;
	case TOKEN_TAB_MINUS :
		type = "TOKEN_TAB_MINUS     "; break;
	case TOKEN_NEWLINE :
		type = "TOKEN_NEWLINE       "; break;
		default : type = "Unknown!            "; break;
	}

	#if 0
	OS_Printf("Type   : %s\n", type);
	OS_Printf("Line   : %d\n", token->line);
	OS_Printf("Col    : %d\n", token->column);
	OS_Printf("Ident  : %d\n", token->ident);
	OS_Printf("strlen : %d\n", token->stringLen);
	OS_Printf("String : ");

	for (i = 0; i < token->stringLen; i++)
		OS_Printf("%c", token->string[i]);

	OS_Printf("\n\n");
	#else

	OS_Printf("%s: ", type);
	for (i = 0; i < token->stringLen; i++)
		OS_Printf("%c", token->string[i]);

	OS_Printf("\n");
	#endif

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DumpTokens(EDIT_TOKEN*token)
{
	while (token) {
		DisplayToken(token);
		token = token->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*GetLineToken(EDIT_FILE*file, EDIT_TOKEN*prev, EDIT_LINE*line,
    int lineNo, int*pos)
{
	int i, offset = 0;
	char*text;
	int index, hex, ident;
	char sym[256];
	EDIT_TOKEN*token = 0;

	text = line->line;

	if (prev && ((prev->type == TOKEN_PREPROC) && (prev->ident ==
	    TOKEN_IDENT_BACKSLASH)))
		return (Preprocessor(text, *pos, line->len, lineNo, pos));

	if (prev && ((prev->type == TOKEN_STRING) && (prev->ident ==
	    TOKEN_IDENT_BACKSLASH)))
		return (StringConstant(text, *pos, line->len, file, lineNo, pos));

	if (prev && ((prev->type == TOKEN_COMMENT_BEGIN) || (prev->type ==
	    TOKEN_COMMENT_LINE))) {
		index = 0;
		offset = *pos;

		for (i = offset; i < line->len; i++) {
			#ifdef NESTED_COMMENT_ERRORS
			if (i + 1 < line->len && text[i] == '/' && text[i + 1] == '*') {
				if (file) {
					GotoPosition(file, lineNo + 1, i + 1);
					CenterBottomBar(1, "[-] Error: Nested Comment Found [-]");
				}
				*pos = -1;
				return (0);
			}
			#endif
			if (i + 1 < line->len && text[i] == '*' && text[i + 1] == '/') {
				if (!index) {
					*pos = i + 2;
					return (CreateToken(TOKEN_COMMENT_END, lineNo, offset, "*/",
					    2));
				}
				break;
			}
			index++;
		}

		if (offset < prev->column) {
			for (ident = offset; ident < prev->column &&
			    ident < (offset + index); ident++)
				if (!TokenCheck(text[ident], TOKEN_WS))
					break;

			if ((ident - offset) < index) {
				index -= (ident - offset);
				offset = ident;
			}
		}
		if (index) {
			text = OS_Malloc(index + 1);
			memcpy(text, &line->line[offset], index);
			token = CreateToken(TOKEN_COMMENT_LINE, lineNo, prev->column, text,
			    index);
			OS_Free(text);
		}

		if (i < line->len)
			*pos = i;
		else
			*pos = 0;

		return (token);
	}

	for (i = *pos; i < line->len; i++) {
		offset = i;

		if (TokenCheck(text[i], TOKEN_WS))
			continue;

		if (i + 1 < line->len && text[i] == '/' && text[i + 1] == '/') {
			*pos = 0;
			return (CreateToken(TOKEN_CPP_COMMENT, lineNo, i, &text[i], line->
			    len - i));
		}

		if (i + 1 < line->len && text[i] == '/' && text[i + 1] == '*') {
			*pos = i + 2;
			return (CreateToken(TOKEN_COMMENT_BEGIN, lineNo, i, "/*", 2));
		}

		if (TokenCheck(text[i], TOKEN_NUMBER) || ((i + 1 < line->len) &&
		    text[i] == '.' && TokenCheck(text[i + 1], TOKEN_NUMBER))) {
			if ((i + 1 < line->len) && text[i] == '0' && (text[i + 1] == 'x' ||
			    text[i + 1] == 'X'))
				hex = 1;
			else
				hex = 0;

			index = 0;

			while (i < line->len && TokenCheck(text[i], hex ? TOKEN_HEX |
			    TOKEN_NUMBER : TOKEN_NUMBER | TOKEN_FLOATING))
				sym[index++] = text[i++];

			*pos = i;
			return (CreateToken(TOKEN_NUMBER, lineNo, offset, sym, index));
		}

		if (text[i] == '"')
			return (StringConstant(text, i, line->len, file, lineNo, pos));

		/* Is this a preprocessor directive? Copy capture enture line. */
		if (text[i] == '#')
			return (Preprocessor(text, i, line->len, lineNo, pos));

		if (text[i] == '\'') {
			index = 0;
			offset = i;
			sym[index++] = text[i++];
			while (i < line->len) {
				sym[index++] = text[i++];

				if (sym[index - 1] == '\\')
					sym[index++] = text[i++];
				else
					if (sym[index - 1] == '\'')
						break;
			}

			if (index < 2 || sym[index - 1] != '\'') {
				if (file) {
					CenterBottomBar(1,
					    "[-] Error: Missing closing single quote on line %d:%d [-]", lineNo + 1, offset + 1);

					GotoPosition(file, lineNo + 1, offset + 1);
				}
				*pos = -1;
				return (0);
			}

			*pos = i;
			return (CreateToken(TOKEN_STRING, lineNo, offset, sym, index));
		}

		if (TokenCheck(text[i], TOKEN_SYMBOL)) {
			index = 0;
			offset = i;
			while (i < line->len && TokenCheck(text[i], TOKEN_SYMBOL |
			    TOKEN_NUMBER))
				sym[index++] = text[i++];
			sym[index] = 0;

			*pos = i;
			ident = IsKeyword(sym);

			if (ident) {
				token = CreateToken(TOKEN_KEYWORD, lineNo, offset, sym, index);
				token->ident = ident - 1;
				return (token);
			} else
				return (CreateToken(TOKEN_SYMBOL, lineNo, offset, sym, index));
		}

		if (TokenCheck(text[i], TOKEN_IDENT)) {
			ident = 0;
			offset = i;

			switch (text[i]) {
			case '(' :
				ident = TOKEN_IDENT_OPENP;
				break;
			case ')' :
				ident = TOKEN_IDENT_CLOSEP;
				break;
			case '+' :
				if (i + 1 < line->len && text[i + 1] == '+') {
					ident = TOKEN_IDENT_INC; i++;
				} else
					if (i + 1 < line->len && text[i + 1] == '=') {
						ident = TOKEN_IDENT_PLUS_EQ; i++;
					} else {
						if (prev && ((prev->type == TOKEN_NUMBER) ||
						    (prev->type == TOKEN_SYMBOL) || (prev->type ==
						    TOKEN_IDENT && prev->string[0] == ')')))
							ident = TOKEN_IDENT_PLUS;
						else
							ident = TOKEN_IDENT_UPLUS;
					}
				break;
			case '-' :
				if (i + 1 < line->len && text[i + 1] == '-') {
					ident = TOKEN_IDENT_DEC; i++;
				} else
					if (i + 1 < line->len && text[i + 1] == '=') {
						ident = TOKEN_IDENT_MINUS_EQ; i++;
					} else
						if (i + 1 < line->len && text[i + 1] == '>') {
							ident = TOKEN_IDENT_POINTER; i++;
						} else {
							if (prev && ((prev->type == TOKEN_NUMBER) ||
							    (prev->type == TOKEN_SYMBOL) || (prev->type ==
							    TOKEN_IDENT && prev->string[0] == ')')))
								ident = TOKEN_IDENT_MINUS;
							else
								ident = TOKEN_IDENT_UMINUS;
						}
				break;
			case '*' :
				if (i + 1 < line->len && text[i + 1] == '=') {
					ident = TOKEN_IDENT_TIMES_EQ; i++;
				} else {
					while (i < line->len - 1 && text[i + 1] == '*')
						i++;
					ident = TOKEN_IDENT_TIMES;
				}
				break;
			case '/' :
				if (i + 1 < line->len && text[i + 1] == '=') {
					ident = TOKEN_IDENT_DIV_EQ; i++;
				} else
					ident = TOKEN_IDENT_DIV;
				break;
			case '~' :
				ident = TOKEN_IDENT_INV;
				break;
			case '%' :
				ident = TOKEN_IDENT_REM;
				break;
			case ',' :
				ident = TOKEN_IDENT_COMMA;
				break;
			case '[' :
				ident = TOKEN_IDENT_OPENI;
				break;
			case ']' :
				ident = TOKEN_IDENT_CLOSEI;
				break;
			case '{' :
				ident = TOKEN_IDENT_OPENB;
				break;
			case '}' :
				ident = TOKEN_IDENT_CLOSEB;
				break;
			case '^' :
				ident = TOKEN_IDENT_EOR;
				break;
			case '\\' :
				ident = TOKEN_IDENT_BACKSLASH;
				break;
			case '?' :
				ident = TOKEN_IDENT_QUESTION;
				break;
			case ':' :
				ident = TOKEN_IDENT_COLEN;
				break;
			case ';' :
				ident = TOKEN_IDENT_SEMI;
				break;
			case '.' :
				if (i + 1 < line->len && TokenCheck(text[i + 1], TOKEN_SYMBOL))
					ident = TOKEN_IDENT_STRUCT;
				else
					ident = TOKEN_IDENT_PERIOD;
				break;
			case '!' :
				if (i + 1 < line->len && text[i + 1] == '=') {
					ident = TOKEN_IDENT_NOTEQ; i++;
				} else
					ident = TOKEN_IDENT_NOT;
				break;

			case '|' :
				if (i + 1 < line->len && text[i + 1] == '|') {
					ident = TOKEN_IDENT_OR; i++;
				} else
					if (i + 1 < line->len && text[i + 1] == '=') {
						ident = TOKEN_IDENT_BITOR_EQ; i++;
					} else
						ident = TOKEN_IDENT_BITOR;
				break;

			case '&' :
				if (i + 1 < line->len && text[i + 1] == '&') {
					ident = TOKEN_IDENT_AND; i++;
				} else
					if (i + 1 < line->len && text[i + 1] == '=') {
						ident = TOKEN_IDENT_BITAND_EQ; i++;
					} else
						ident = TOKEN_IDENT_BITAND;
				break;

			case '>' :
				if (i + 1 < line->len && text[i + 1] == '>') {
					ident = TOKEN_IDENT_SHFR; i++;
				} else
					if (i + 1 < line->len && text[i + 1] == '=') {
						ident = TOKEN_IDENT_GTEQ; i++;
					} else
						ident = TOKEN_IDENT_GT;
				break;

			case '<' :
				if (i + 1 < line->len && text[i + 1] == '<') {
					ident = TOKEN_IDENT_SHFL; i++;
				} else
					if (i + 1 < line->len && text[i + 1] == '=') {
						ident = TOKEN_IDENT_LTEQ; i++;
					} else
						ident = TOKEN_IDENT_LT;
				break;
			case '=' :
				if (i + 1 < line->len && text[i + 1] == '=') {
					ident = TOKEN_IDENT_EQ; i++;
				} else
					ident = TOKEN_IDENT_ASSIGN;
				break;
				default :
				if (file) {
					CenterBottomBar(1, "[-] Misplaced identifier '%c' [-]",
					    text[i]);
					GotoPosition(file, lineNo + 1, i + 1);
				}
				*pos = -1;
				return (0);
			}

			*pos = i + 1;

			for (i = offset; i < *pos; i++)
				sym[i - offset] = text[i];

			token = CreateToken(TOKEN_IDENT, lineNo, offset, sym, i - offset);
			token->ident = ident;
			return (token);
		}

		if (file) {
			CenterBottomBar(1, "[-] Syntax Error '%c' on line %d:%d [-]",
			    text[i], lineNo + 1, i + 1);

			GotoPosition(file, lineNo + 1, i + 1);
		}
		*pos = -1;
		return (0);
	}

	*pos = 0;
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeallocTokens(EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*walk;

	while (tokens) {
		OS_Free(tokens->string);
		walk = tokens->next;
		OS_Free(tokens);
		tokens = walk;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*Preprocessor(char*text, int i, int len, int lineNo, int*pos)
{
	char*sym;
	EDIT_TOKEN*token = 0;
	int skip = 0, index = 0, offset, openQ = 0, openS = 0, comment = 0;

	offset = i;
	sym = OS_Malloc(len + 2);

	while (i < len) {
		if (text[i] == '\\')
			skip = 2;

		if (!skip && text[i] == '"')
			openQ = ~openQ;

		if (!skip && text[i] == '\'')
			openS = ~openS;

		/* Check if there is a comment inside of this preprocessor directive */
		if (!openQ && !openS && (i + 1 < len) && text[i] == '/' && text[i + 1]
		    == '/')
			break;

		if (!openQ && !openS && (i + 1 < len) && text[i] == '/' && text[i + 1]
		    == '*')
			comment = index;

		if (!openQ && !openS && (i + 1 < len) && text[i] == '*' && text[i + 1]
		    == '/')
			comment = 0;

		sym[index++] = text[i++];

		if (skip)
			skip--;
	}

	if (sym[index - 1] == '\\') {
		token = CreateToken(TOKEN_PREPROC, lineNo, offset, sym, index);
		token->ident = TOKEN_IDENT_BACKSLASH;
		*pos = 0;
		OS_Free(sym);
		return (token);
	}

	if (comment) {
		index = comment;
		i = offset + index;
	}

	*pos = i;
	token = CreateToken(TOKEN_PREPROC, lineNo, offset, sym, index);
	OS_Free(sym);
	return (token);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*StringConstant(char*text, int i, int len, EDIT_FILE*file,
    int lineNo, int*pos)
{
	char*sym;
	EDIT_TOKEN*token = 0;
	int index = 0, offset;

	offset = i;
	sym = OS_Malloc(len + 2);

	if (text[i] != '"')
		sym[index++] = '"';

	sym[index++] = text[i++];
	while (i < len) {
		sym[index++] = text[i++];

		if (sym[index - 1] == '\\' && i < len)
			sym[index++] = text[i++];
		else
			if (sym[index - 1] == '"')
				break;
	}

	if (index < 2 || sym[index - 1] != '"') {
		if (sym[index - 1] == '\\') {
			sym[index - 1] = '"';
			token = CreateToken(TOKEN_STRING, lineNo, offset, sym, index);
			token->ident = TOKEN_IDENT_BACKSLASH;
			*pos = 0;
			OS_Free(sym);
			return (token);
		} else
			if (file) {
				CenterBottomBar(1,
				    "[-] Error: Missing closing double quote on line %d:%d [-]",
				    lineNo + 1, offset + 1);

				GotoPosition(file, lineNo + 1, offset + 1);
			}
		OS_Free(sym);
		*pos = -1;
		return (0);
	}

	*pos = i;
	token = CreateToken(TOKEN_STRING, lineNo, offset, sym, index);
	OS_Free(sym);
	return (token);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int TokenCheck(char ch, int ops)
{
	if (ops&TOKEN_SYMBOL) {
		if (ch >= 'a' && ch <= 'z')
			return (1);
		if (ch >= 'A' && ch <= 'Z')
			return (1);
		if (ch == '_')
			return (1);
	}

	if (ops&TOKEN_FLOATING)
		if (ch == '.')
			return (1);

	if (ops&TOKEN_NUMBER) {
		if (ch >= '0' && ch <= '9')
			return (1);
	}

	if (ops&TOKEN_HEX) {
		if (ch == 'x' || ch == 'X')
			return (1);
		if (ch >= '0' && ch <= '9')
			return (1);
		if (ch >= 'a' && ch <= 'f')
			return (1);
		if (ch >= 'A' && ch <= 'F')
			return (1);
	}

	if (ops&TOKEN_WS) {
		if (ch <= 32)
			return (1);
	}

	if (ops&TOKEN_IDENT) {
		if (ch == '(' || ch == ')' || ch == '!' || ch == '~')
			return (1);
		if (ch == '|' || ch == '&' || ch == '+' || ch == '-')
			return (1);
		if (ch == '*' || ch == '/' || ch == '%' || ch == '<')
			return (1);
		if (ch == '>' || ch == '=' || ch == '^')
			return (1);
		if (ch == '?' || ch == ':' || ch == ',')
			return (1);
		if (ch == '[' || ch == ']' || ch == '{' || ch == '}')
			return (1);
		if (ch == ';' || ch == '.' || ch == '\\')
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
static int IsKeyword(char*keyword)
{
	int i;

	for (i = 0; i < (int)NUMBER_KEYWORDS; i++)
		if (!strcmp(keywords[i], keyword))
			return (i + 1);

	return (0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_TOKEN*FindMatchingIdent(EDIT_FILE*file, EDIT_TOKEN*tokens, int concat)
{
	EDIT_TOKEN*base, find;
	int open = 0;
	char*missing;

	base = tokens;

	switch (tokens->ident) {
	case TOKEN_IDENT_OPENP :
		missing = "close parentheses";
		find.type = TOKEN_IDENT;
		find.ident = TOKEN_IDENT_CLOSEP;
		break;

	case TOKEN_IDENT_OPENB :
		missing = "close brace";
		find.type = TOKEN_IDENT;
		find.ident = TOKEN_IDENT_CLOSEB;
		break;

		default :
		CenterBottomBar(1, "Internal Error");
		GotoPosition(file, base->line + 1, base->column + 1);
		return (0);
	}

	tokens = tokens->next;

	while (tokens) {
		if (tokens->type == TOKEN_NEWLINE && concat) {
			tokens = DeleteToken(tokens);
			continue;
		}

		if (tokens->type == base->type && tokens->ident == base->ident)
			open++;

		if (tokens->type == find.type && tokens->ident == find.ident) {
			if (!open)
				break;

			open--;
		}

		tokens = tokens->next;
	}

	if (!tokens) {
		CenterBottomBar(1, "[-] Error: Missing %s %d:%d [-]", missing, base->
		    line + 1, base->column + 1);

		GotoPosition(file, base->line + 1, base->column + 1);
	}

	return (tokens);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_TOKEN*SkipWhitespace(EDIT_FILE*file, EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*lastGood = 0;

	while (tokens) {
		lastGood = tokens;
		if (tokens->type&(TOKEN_COMMENT_BEGIN | TOKEN_COMMENT_LINE |
		    TOKEN_COMMENT_END | TOKEN_WS | TOKEN_NEWLINE | TOKEN_TAB_PLUS |
		    TOKEN_TAB_MINUS | TOKEN_CPP_COMMENT | TOKEN_PREPROC))
			tokens = tokens->next;
		else
			break;
	}

	if (!tokens && lastGood) {
		CenterBottomBar(1, "[-] Syntax Error, end of file reached t"
		    " on line %d:%d [-]", lastGood->line + 1, lastGood->column + 1);
		GotoPosition(file, lastGood->line + 1, lastGood->column + 1);
	}

	return (tokens);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_TOKEN*DeleteToken(EDIT_TOKEN*base)
{
	EDIT_TOKEN*next;

	next = base->next;

	if (base->prev)
		base->prev->next = base->next;

	if (base->next)
		base->next->prev = base->prev;

	OS_Free(base->string);
	OS_Free(base);

	return (next);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_TOKEN*InsertToken(EDIT_TOKEN*base, int type, int line, int column, char*
    string, int len)
{
	EDIT_TOKEN*token;

	token = (EDIT_TOKEN*)OS_Malloc(sizeof(EDIT_TOKEN));

	token->type = type;
	token->next = base;
	token->prev = base->prev;
	token->ident = 0;
	token->line = line;
	token->column = column;
	token->stringLen = len;

	token->string = OS_Malloc(len);
	memcpy(token->string, string, len);

	if (base->prev)
		base->prev->next = token;

	base->prev = token;

	return (token);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_TOKEN*SkipParenSet(EDIT_FILE*file, EDIT_TOKEN*tokens, int concat)
{
	if (tokens->type != TOKEN_IDENT || tokens->ident != TOKEN_IDENT_OPENP) {
		CenterBottomBar(1,
		    "[-] Error: Expected open parentheses on line %d:%d [-]", tokens->
		    line + 1, tokens->column + 1);

		GotoPosition(file, tokens->line + 1, tokens->column + 1);
		return (0);
	}

	return (FindMatchingIdent(file, tokens, concat));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_TOKEN*SkipStatement(EDIT_FILE*file, EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*walk, *closeb, *prev;

	walk = tokens;

	tokens = SkipWhitespace(file, walk);

	if (!tokens)
		return (0);

	if (tokens->type != TOKEN_KEYWORD || (tokens->type == TOKEN_KEYWORD &&
	    tokens->ident == KEYWORD_RETURN) || (tokens->type == TOKEN_KEYWORD &&
	    tokens->ident == KEYWORD_CONTINUE) || (tokens->type == TOKEN_KEYWORD &&
	    tokens->ident == KEYWORD_GOTO) || (tokens->type == TOKEN_KEYWORD &&
	    tokens->ident == KEYWORD_BREAK)) {
		while (tokens) {
			if (tokens->type == TOKEN_IDENT && tokens->ident ==
			    TOKEN_IDENT_SEMI)
				return (tokens->next);

			tokens = tokens->next;
		}
		return (0);
	}

	/* Handle a do-while loop */
	if (tokens->type == TOKEN_KEYWORD && tokens->ident == KEYWORD_DO) {
		walk = SkipWhitespace(file, tokens->next);

		if (!walk)
			return (0);

		if (walk->type == TOKEN_IDENT && walk->ident == TOKEN_IDENT_OPENB) {
			closeb = FindMatchingIdent(file, walk, 0);

			if (!closeb)
				return (0);

			walk = closeb->next;
		} else
			walk = SkipStatement(file, walk);

		if (!walk)
			return (0);

		walk = SkipWhitespace(file, walk);

		if (!walk)
			return (0);

		if (walk->type != TOKEN_KEYWORD || walk->ident != KEYWORD_WHILE) {
			CenterBottomBar(1, "[-] Syntax Error, expecting while keyword"
			    " on line %d:%d [-]", walk->line + 1, walk->column + 1);
			GotoPosition(file, walk->line + 1, walk->column + 1);
			return (0);
		}
		return (SkipStatement(file, walk->next));
	}

	if (tokens->type == TOKEN_KEYWORD) {
		if ((tokens->ident == KEYWORD_IF) || (tokens->ident == KEYWORD_FOR) ||
		    (tokens->ident == KEYWORD_SWITCH) || (tokens->ident ==
		    KEYWORD_WHILE)) {
			walk = SkipWhitespace(file, tokens->next);

			if (!walk)
				return (0);

			walk = SkipParenSet(file, walk, 0);

			if (!walk)
				return (0);

			walk = SkipWhitespace(file, walk->next);

			if (!walk)
				return (0);

			if (walk->type == TOKEN_IDENT && walk->ident == TOKEN_IDENT_OPENB) {
				closeb = FindMatchingIdent(file, walk, 0);

				if (!closeb)
					return (0);

				/* If we're not an IF statement, then we're done. */
				if (tokens->ident != KEYWORD_IF)
					return (closeb->next);

				walk = closeb->next;
			} else {
				walk = SkipStatement(file, walk);
			}

			if (!walk)
				return (0);

			/* If we're not an IF statement, then we're done. */
			if (tokens->ident != KEYWORD_IF)
				return (walk);

			if (!walk)
				return (0);


			prev = walk;

			walk = SkipWhitespace(file, walk);

			if (!walk)
				return (0);

			/* If there is an else, skip over else statement/block. */
			if (walk->type == TOKEN_KEYWORD && walk->ident == KEYWORD_ELSE) {
				walk = SkipWhitespace(file, walk->next);

				if (!walk)
					return (0);

				if (walk->type == TOKEN_IDENT && walk->ident ==
				    TOKEN_IDENT_OPENB) {
					closeb = FindMatchingIdent(file, walk, 0);

					if (!closeb)
						return (0);

					return (closeb->next);
				}
				walk = SkipStatement(file, walk);
			} else
				walk = prev;

			return (walk);
		}
	}
	CenterBottomBar(1, "[-] Syntax Error on line %d:%d [-]", tokens->line + 1,
	    tokens->column + 1);
	GotoPosition(file, tokens->line + 1, tokens->column + 1);
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CheckCaseNewline(EDIT_FILE*file, EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*walk;

	while (tokens) {
		if (tokens->type == TOKEN_IDENT && tokens->ident == TOKEN_IDENT_COLEN) {
			walk = SkipWhitespace(file, tokens->next);

			if (!walk)
				return ;

			if (tokens->line == walk->line)
				InsertToken(walk, TOKEN_NEWLINE, 0, 0, "", 1);

			break;
		}

		tokens = tokens->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void RemoveCaseTab(EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*begin, *end;

	begin = tokens;
	end = tokens;

	while (begin) {
		if (begin->type == TOKEN_NEWLINE) {
			InsertToken(begin->prev, TOKEN_TAB_MINUS, 0, 0, "", 1);
			break;
		}
		begin = begin->prev;
	}


	while (end) {
		if (end->type == TOKEN_NEWLINE) {
			InsertToken(end, TOKEN_TAB_PLUS, 0, 0, "", 1);
			break;
		}
		end = end->next;
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int GetBestBreak(EDIT_TOKEN*tokens, int maxLineLength)
{
	EDIT_TOKEN*walk;
	int min;

	min = tokens->column;

	if (min > maxLineLength)
		return (0);

	/* Try to find the ultimate break point first. */
	walk = tokens;
	while (walk) {
		/* Find first breakable line.. */
		if (((walk->column < maxLineLength) &&
		    ((walk->column + walk->stringLen) <= maxLineLength)) &&
		    (walk->next && walk->next->column >= maxLineLength)) {
			while (walk && walk->next->column > min) {
				if (((walk->type == TOKEN_IDENT) && ((walk->ident ==
				    TOKEN_IDENT_COMMA) || (walk->ident == TOKEN_IDENT_OR) || (
				    walk->ident == TOKEN_IDENT_AND) || (walk->ident ==
				    TOKEN_IDENT_GTEQ) || (walk->ident == TOKEN_IDENT_LTEQ) || (
				    walk->ident == TOKEN_IDENT_EQ) || (walk->ident ==
				    TOKEN_IDENT_NOTEQ))) || (walk->type == TOKEN_COMMENT_END))
					return (walk->next->column);

				walk = walk->prev;
			}
			break;
		}
		walk = walk->next;
	}

	/* Settle for any legal break point. */
	walk = tokens;
	while (walk) {
		/* Find first breakable line.. */
		if (((walk->column < maxLineLength) &&
		    ((walk->column + walk->stringLen) > maxLineLength)) ||
		    (walk->column >= maxLineLength)) {
			while (walk && walk->column > min) {
				if (ValidBreakToken(walk))
					return (walk->column);

				walk = walk->prev;
			}
			break;
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
static int ValidBreakToken(EDIT_TOKEN*token)
{
	if (token->type == TOKEN_COMMENT_LINE)
		return (0);

	if (token->type == TOKEN_COMMENT_END)
		return (0);

	if (token->type == TOKEN_IDENT) {
		if (token->ident == TOKEN_IDENT_CLOSEP || token->ident ==
		    TOKEN_IDENT_BITAND || token->ident == TOKEN_IDENT_COMMA || token->
		    ident == TOKEN_IDENT_PERIOD || token->ident == TOKEN_IDENT_STRUCT ||
		    token->ident == TOKEN_IDENT_POINTER || token->ident ==
		    TOKEN_IDENT_SEMI || token->ident == TOKEN_IDENT_BACKSLASH ||
		    token->ident == TOKEN_IDENT_OPENB || token->ident ==
		    TOKEN_IDENT_CLOSEB)
			return (0);
	}
	return (1);
}




