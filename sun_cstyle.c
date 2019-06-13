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

#define MAX_LINE_LENGTH 80

#if 0
#define DEBUG_RETURN(x) { OS_Printf("failed on line %d\n", __LINE__); getch(); return(x); }
#else
#define DEBUG_RETURN(x) return(x)
#endif

static int IdentSpacing(int ident);
static EDIT_TOKEN*ProcessStatements(EDIT_FILE*file, EDIT_TOKEN*tokens);
static EDIT_TOKEN*ProcessPreprocessor(EDIT_TOKEN*tokens);
static EDIT_TOKEN*ProcessKeywordBlock(EDIT_FILE*file, EDIT_TOKEN*tokens);
static EDIT_TOKEN*ProcessDoKeyword(EDIT_FILE*file, EDIT_TOKEN*tokens);
static EDIT_TOKEN*MoveToken(EDIT_TOKEN*openb, EDIT_TOKEN*pos);
static EDIT_TOKEN*ProcessIfElse(EDIT_FILE*file, EDIT_TOKEN*tokens);
static void StripTrailingWS(char*line, int*len, int linetabs);
static EDIT_TOKEN*ProcessSwitch(EDIT_FILE*file, EDIT_TOKEN*tokens);
static char*CheckWhitespace(char*line, int*len, int*allocSize);
static EDIT_TOKEN*ProcessBlockList(EDIT_FILE*file, EDIT_TOKEN*tokens);
static char*LineAlloc(char*line, int newLen, int*alloc);
static EDIT_TOKEN*ProcessElseBlock(EDIT_FILE*file, EDIT_TOKEN*tokens);
static void AddLine(EDIT_CLIPBOARD*clipboard, char*line, int len);
static char*LineStrcpy(char*line, char*str, int strLen, int*len, int*alloc);
static char*LineStrcat(char*line, char*str, int strLen, int*len, int*alloc);
static void ProcessLine(EDIT_FILE*file, EDIT_CLIPBOARD*clipboard, char*line,
    int lineLen, int tabs, int nest);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SunStyle(EDIT_FILE*file, EDIT_TOKEN*tokens, EDIT_CLIPBOARD*clipboard)
{
	int i, linetabs = 0;
	EDIT_TOKEN*prev = 0;
	int tabs = 0;
	int linePreserve = 0;
	char*line = 0;
	int len = 0;
	int allocSize = 0;
	int inlineBrackets = 0;
	int commentBegin = 0;

	line = LineStrcpy(line, "", 0, &len, &allocSize);

	if (!ProcessStatements(file, tokens)) {
		if (line)
			OS_Free(line);
		DEBUG_RETURN(0);
	}

	if (!ProcessPreprocessor(tokens)) {
		if (line)
			OS_Free(line);
		DEBUG_RETURN(0);
	}

	while (tokens) {
		if (tokens->type == TOKEN_IDENT && tokens->ident ==
		    TOKEN_IDENT_CLOSEB) {
			if (!tabs) {
				CenterBottomBar(1,
				    "[-] Error: Unexpected '}' on line %d:%d [-]",
				    tokens->line + 1, tokens->column + 1);

				GotoPosition(file, tokens->line + 1, tokens->column + 1);

				if (line)
					OS_Free(line);

				return (0);
			}

			if (inlineBrackets)
				inlineBrackets--;
			else
				if (linetabs) {
					for (i = 0; i < len; i++)
						line[i] = line[i + 1];
					len--;
					linetabs--;
				}
			tabs--;
		}

		if (tokens->type == TOKEN_IDENT && tokens->ident == TOKEN_IDENT_OPENB) {
			inlineBrackets++;
			tabs++;
		}

		if (tokens->type == TOKEN_TAB_PLUS)
			tabs++;

		if (tokens->type == TOKEN_TAB_MINUS)
			tabs--;

		if (tokens->type == TOKEN_NEWLINE) {
			if (linePreserve || commentBegin)
				AddLine(clipboard, line, len);
			else
				ProcessLine(file, clipboard, line, len, linetabs, 0);

			inlineBrackets = 0;
			linePreserve = 0;

			linetabs = tabs;

			line = LineStrcpy(line, "", 0, &len, &allocSize);

			for (i = 0; i < tabs; i++)
				line = LineStrcat(line, "\t", 1, &len, &allocSize);
		}

		/* Remove any tabs/idents for labels */
		if (tokens->type == TOKEN_SYMBOL && tokens->prev && tokens->next) {
			if (tokens->prev->type == TOKEN_NEWLINE && (tokens->next->type ==
			    TOKEN_IDENT && tokens->next->ident == TOKEN_IDENT_COLEN)) {
				/* write out label */
				line = LineStrcpy(line, tokens->string, tokens->stringLen, &len,
				    &allocSize);

				/* write out colen */
				tokens = tokens->next;
				line = LineStrcat(line, tokens->string, tokens->stringLen, &len,
				    &allocSize);

				if (tokens->next)
					if (tokens->next->type != TOKEN_NEWLINE)
						InsertToken(tokens->next, TOKEN_NEWLINE, 0, 0, "", 1);

				prev = tokens;
				tokens = tokens->next;
				continue;
			}
		}

		switch (tokens->type) {
		case TOKEN_IDENT :
			if (!len || !TokenCheck(line[len - 1], TOKEN_WS)) {
				if (IdentSpacing(tokens->ident))
					line = LineStrcat(line, " ", 1, &len, &allocSize);

				if (prev && prev->type == TOKEN_KEYWORD)
					line = LineStrcat(line, " ", 1, &len, &allocSize);
			}

			if (tokens->ident == TOKEN_IDENT_COMMA || tokens->ident ==
			    TOKEN_IDENT_SEMI || tokens->ident == TOKEN_IDENT_CLOSEP)
				StripTrailingWS(line, &len, linetabs);

			line = LineStrcat(line, tokens->string, tokens->stringLen, &len, &
			    allocSize);
			if (((tokens->ident == TOKEN_IDENT_COMMA) || (tokens->ident ==
			    TOKEN_IDENT_SEMI)) && tokens->next && tokens->next->type !=
			    TOKEN_NEWLINE) {
				if (!(tokens->next->type == TOKEN_IDENT &&
				    IdentSpacing(tokens->next->ident)))
					line = LineStrcat(line, " ", 1, &len, &allocSize);
			} else
				if (IdentSpacing(tokens->ident))
					line = LineStrcat(line, " ", 1, &len, &allocSize);
			break;

		case TOKEN_SYMBOL :
		case TOKEN_KEYWORD :
			if (prev) {
				if ((prev->type == TOKEN_SYMBOL || prev->type == TOKEN_KEYWORD
				    || prev->type == TOKEN_COMMENT_END))
					line = LineStrcat(line, " ", 1, &len, &allocSize);
			}

			line = LineStrcat(line, tokens->string, tokens->stringLen, &len, &
			    allocSize);

			if (tokens->type == TOKEN_KEYWORD && ((tokens->ident ==
			    KEYWORD_IF) || (tokens->ident == KEYWORD_FOR) ||
			    (tokens->ident == KEYWORD_WHILE) || (tokens->ident ==
			    KEYWORD_RETURN) || (tokens->ident == KEYWORD_SWITCH) || (
			    tokens->ident == KEYWORD_DO)))
				line = LineStrcat(line, " ", 1, &len, &allocSize);
			break;

		case TOKEN_STRING :
		case TOKEN_NUMBER :
			if (prev) {
				if ((prev->type == TOKEN_SYMBOL || prev->type == TOKEN_KEYWORD))
					line = LineStrcat(line, " ", 1, &len, &allocSize);
			}

			line = LineStrcat(line, tokens->string, tokens->stringLen, &len, &
			    allocSize);
			break;


		case TOKEN_PREPROC :
			line = LineStrcat(line, tokens->string, tokens->stringLen, &len, &
			    allocSize);
			linePreserve = 1;
			break;

		case TOKEN_CPP_COMMENT :
			line = CheckWhitespace(line, &len, &allocSize);
			line = LineStrcat(line, tokens->string, tokens->stringLen, &len, &
			    allocSize);
			break;

		case TOKEN_COMMENT_BEGIN :
			commentBegin++;
			line = CheckWhitespace(line, &len, &allocSize);
			line = LineStrcat(line, tokens->string, tokens->stringLen, &len, &
			    allocSize);
			break;

		case TOKEN_COMMENT_END :
			if (commentBegin)
				commentBegin--;
			line = LineStrcat(line, tokens->string, tokens->stringLen, &len, &
			    allocSize);
			break;

		case TOKEN_COMMENT_LINE :
			{
				#if 0
				/* SUN C-Style: Fix "missing blank after open comment" */
				if (!TokenCheck(tokens->string[0], TOKEN_WS))
					line = LineStrcat(line, " ", 1, &len, &allocSize);
				#endif
				line = LineStrcat(line, tokens->string, tokens->stringLen, &len,
				    &allocSize);
				#if 0

				/* SUN C-Style: Fix "missing blank before close comment" */
				if (!tokens->stringLen || !TokenCheck(tokens->string[tokens->
				    stringLen - 1], TOKEN_WS))
					line = LineStrcat(line, " ", 1, &len, &allocSize);
				#endif
				break;
			}

		case TOKEN_WS :
			line = LineStrcat(line, tokens->string, tokens->stringLen, &len, &
			    allocSize);
			break;
		}
		prev = tokens;
		tokens = tokens->next;
	}

	if (line) {
		if (linePreserve || commentBegin)
			AddLine(clipboard, line, len);
		else
			ProcessLine(file, clipboard, line, len, linetabs, 0);

		OS_Free(line);
	}

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char*CheckWhitespace(char*line, int*len, int*allocSize)
{
	if (*len) {
		if (line[(*len) - 1] > 32)
			line = LineStrcat(line, " ", 1, len, allocSize);
	}
	return (line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void StripTrailingWS(char*line, int*len, int linetabs)
{
	while (*len && (*len >= linetabs) && TokenCheck(line[*len - 1], TOKEN_WS))
		(*len)--;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*LineStrcat(char*line, char*str, int strLen, int*len, int*alloc)
{
	line = LineAlloc(line, *len + strLen, alloc);

	memcpy(&line[*len], str, strLen);

	*len += strLen;

	return (line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*LineStrcpy(char*line, char*str, int strLen, int*len, int*alloc)
{
	line = LineAlloc(line, strLen, alloc);

	memcpy(line, str, strLen);

	*len = strLen;

	return (line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*LineAlloc(char*line, int newLen, int*alloc)
{
	char*newLine;

	if (newLen >= *alloc) {
		newLine = OS_Malloc(newLen + 256);
		if (line) {
			memcpy(newLine, line, *alloc);
			OS_Free(line);
		}
		*alloc = newLen + 256;
		line = newLine;
	}

	return (line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ProcessLine(EDIT_FILE*file, EDIT_CLIPBOARD*clipboard, char*line,
    int lineLen, int tabs, int nest)
{
	EDIT_LINE lines;
	EDIT_TOKEN*tokens;
	char*newLine, *tabLine;
	int i, index;
	int len;

	tabLine = TabulateString(line, lineLen, &len);

	if (len < MAX_LINE_LENGTH) {
		AddLine(clipboard, line, lineLen);
		OS_Free(tabLine);
		return ;
	}

	lines.len = len;
	lines.line = tabLine;
	lines.prev = 0;
	lines.next = 0;

	tokens = TokenizeLines(0, &lines);

	if (tokens) {
		index = GetBestBreak(tokens, MAX_LINE_LENGTH);

		if (index) {
			AddLine(clipboard, tabLine, index);

			newLine = OS_Malloc(len + 10);

			strcpy(newLine, "");

			for (i = 0; i < tabs; i++)
				strcat(newLine, "\t");

			strcat(newLine, "    ");
			i = strlen(newLine);

			memcpy(&newLine[i], &tabLine[index], len - index);

			ProcessLine(file, clipboard, newLine, i + (len - index), tabs,
			    nest + 1);
			DeallocTokens(tokens);
			OS_Free(newLine);
			OS_Free(tabLine);
			return ;
		}
	}

	OS_Free(tabLine);

	if (tokens)
		DeallocTokens(tokens);

	AddLine(clipboard, line, lineLen);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void AddLine(EDIT_CLIPBOARD*clipboard, char*line, int len)
{
	char*str;
	int newLen;

	str = TabulateString(line, len, &newLen);

	if (str) {
		/* Strip trailing whitespace.. */
		while (newLen && TokenCheck(str[newLen - 1], TOKEN_WS))
			newLen--;

		AddClipboardLine(clipboard, str, newLen, COPY_LINE);

		OS_Free(str);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int IdentSpacing(int ident)
{
	switch (ident) {
	case TOKEN_IDENT_OR :
	case TOKEN_IDENT_AND :
	case TOKEN_IDENT_GT :
	case TOKEN_IDENT_LT :
	case TOKEN_IDENT_GTEQ :
	case TOKEN_IDENT_LTEQ :
	case TOKEN_IDENT_EQ :
	case TOKEN_IDENT_NOTEQ :
	case TOKEN_IDENT_BITOR_EQ :
	case TOKEN_IDENT_BITAND_EQ :
	case TOKEN_IDENT_PLUS_EQ :
	case TOKEN_IDENT_MINUS_EQ :
	case TOKEN_IDENT_TIMES_EQ :
	case TOKEN_IDENT_DIV_EQ :
	case TOKEN_IDENT_ASSIGN :
	case TOKEN_IDENT_DIV :
	case TOKEN_IDENT_PLUS :
	case TOKEN_IDENT_MINUS :
	case TOKEN_IDENT_BITOR :
	case TOKEN_IDENT_SHFL :
	case TOKEN_IDENT_SHFR :
	case TOKEN_IDENT_QUESTION :
	case TOKEN_IDENT_COLEN :
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
static EDIT_TOKEN*ProcessPreprocessor(EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*base;
	EDIT_TOKEN*preproc = 0;
	int nonws = 0;

	base = tokens;

	while (base) {
		if (base->type == TOKEN_IDENT || base->type == TOKEN_STRING || base->
		    type == TOKEN_NUMBER || base->type == TOKEN_KEYWORD || base->type ==
		    TOKEN_SYMBOL || base->type == TOKEN_HEX || base->type ==
		    TOKEN_FLOATING) {
			if (preproc) {
				InsertToken(preproc->next, TOKEN_NEWLINE, 0, 0, "", 1);
				preproc = 0;
			}
			nonws++;
		}

		if (base->type == TOKEN_NEWLINE) {
			if (nonws && preproc)
				InsertToken(preproc->next, TOKEN_NEWLINE, 0, 0, "", 1);

			nonws = 0;
			preproc = 0;
		}

		if (base->type == TOKEN_PREPROC) {
			preproc = base;

			if (nonws)
				InsertToken(base, TOKEN_NEWLINE, 0, 0, "", 1);

			nonws = 0;
		}

		base = base->next;
	}
	return (tokens);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*ProcessStatements(EDIT_FILE*file, EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*base;

	base = tokens;

	while (base) {
		if (base->type == TOKEN_SYMBOL && base->next) {
			if (base->next->type == TOKEN_IDENT && base->next->ident ==
			    TOKEN_IDENT_OPENP) {
				base = SkipParenSet(file, base->next, 1);

				if (!base)
					return (0);
			}
		}

		if ((base->type == TOKEN_IDENT && base->ident == TOKEN_IDENT_ASSIGN) &&
		    base->next && (base->next->type == TOKEN_IDENT &&
		    base->next->ident == TOKEN_IDENT_OPENB)) {
			base = ProcessBlockList(file, base->next);
			if (!base)
				return (0);
		}

		if (base->type == TOKEN_KEYWORD && base->ident == KEYWORD_DO) {
			if (!ProcessDoKeyword(file, base))
				DEBUG_RETURN(0);
		}

		if (base->type == TOKEN_KEYWORD) {
			if ((base->ident == KEYWORD_IF) || (base->ident == KEYWORD_FOR) ||
			    (base->ident == KEYWORD_SWITCH) || (base->ident ==
			    KEYWORD_WHILE))
				if (!ProcessKeywordBlock(file, base))
					DEBUG_RETURN(0);
		}

		base = base->next;
	}


	return (tokens);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*ProcessBlockList(EDIT_FILE*file, EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*closeb;

	closeb = FindMatchingIdent(file, tokens, 0);

	if (closeb) {
		if (tokens->line != closeb->line)
			if (closeb->prev->type != TOKEN_NEWLINE)
				InsertToken(closeb, TOKEN_NEWLINE, 0, 0, "", 1);
	}

	return (closeb);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*ProcessKeywordBlock(EDIT_FILE*file, EDIT_TOKEN*keyword)
{
	EDIT_TOKEN*base, *walk, *tokens, *statement;

	tokens = keyword->next;

	tokens = SkipWhitespace(file, tokens);

	if (!tokens)
		DEBUG_RETURN(0);

	base = SkipParenSet(file, tokens, 1);

	if (!base)
		DEBUG_RETURN(0);

	walk = SkipWhitespace(file, base->next);

	if (!walk)
		return (tokens);

	/* Empty For/While loop? */
	if (keyword->ident == KEYWORD_WHILE || keyword->ident == KEYWORD_FOR) {
		if (walk->type == TOKEN_IDENT && walk->ident == TOKEN_IDENT_SEMI)
			return (walk->next);
	}

	if (walk->type == TOKEN_IDENT && walk->ident == TOKEN_IDENT_OPENB) {
		walk = MoveToken(walk, base);

		statement = SkipWhitespace(file, walk->next);

		if (!statement)
			DEBUG_RETURN(0);

		if (statement->prev->type != TOKEN_NEWLINE)
			InsertToken(statement, TOKEN_NEWLINE, 0, 0, "", 1);

		if (keyword->ident == KEYWORD_IF)
			return (ProcessIfElse(file, walk));

		if (keyword->ident == KEYWORD_SWITCH)
			return (ProcessSwitch(file, walk));
	} else {
		InsertToken(base->next, TOKEN_TAB_PLUS, 0, 0, "", 1);

		if (walk->prev->type != TOKEN_NEWLINE)
			InsertToken(walk, TOKEN_NEWLINE, 0, 0, "", 1);

		walk = SkipStatement(file, walk);

		if (!walk)
			DEBUG_RETURN(0);

		InsertToken(walk, TOKEN_TAB_MINUS, 0, 0, "", 1);

		walk = SkipWhitespace(file, walk);

		if (!walk)
			DEBUG_RETURN(0);

		if (keyword->ident == KEYWORD_IF) {
			if (walk->type == TOKEN_KEYWORD && walk->ident == KEYWORD_ELSE) {
				if (walk->prev->type != TOKEN_NEWLINE)
					InsertToken(walk, TOKEN_NEWLINE, 0, 0, "", 1);

				return (ProcessElseBlock(file, walk));
			}
		}
	}


	return (tokens);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*ProcessDoKeyword(EDIT_FILE*file, EDIT_TOKEN*keyword)
{
	EDIT_TOKEN*base, *walk, *statement, *closeb;

	base = keyword;

	walk = SkipWhitespace(file, keyword->next);

	if (!walk)
		return (0);

	if (walk->type == TOKEN_IDENT && walk->ident == TOKEN_IDENT_OPENB) {
		walk = MoveToken(walk, base);

		statement = SkipWhitespace(file, walk->next);

		if (!statement)
			DEBUG_RETURN(0);

		if (statement->prev->type != TOKEN_NEWLINE)
			InsertToken(statement, TOKEN_NEWLINE, 0, 0, "", 1);

		closeb = FindMatchingIdent(file, walk, 0);

		if (!closeb)
			DEBUG_RETURN(0);

		if (closeb->prev->type != TOKEN_NEWLINE)
			InsertToken(closeb, TOKEN_NEWLINE, 0, 0, "", 1);

		walk = SkipWhitespace(file, closeb->next);

		if (!walk)
			DEBUG_RETURN(0);
	} else {
		InsertToken(base->next, TOKEN_TAB_PLUS, 0, 0, "", 1);

		if (walk->prev->type != TOKEN_NEWLINE)
			InsertToken(walk, TOKEN_NEWLINE, 0, 0, "", 1);

		walk = SkipStatement(file, walk);

		if (!walk)
			DEBUG_RETURN(0);

		InsertToken(walk, TOKEN_TAB_MINUS, 0, 0, "", 1);

		walk = SkipWhitespace(file, walk);

		if (!walk)
			DEBUG_RETURN(0);

		if (walk->prev->type != TOKEN_NEWLINE)
			InsertToken(walk, TOKEN_NEWLINE, 0, 0, "", 1);
	}

	if (walk->type != TOKEN_KEYWORD || walk->ident != KEYWORD_WHILE) {
		CenterBottomBar(1,
		    "[-] Error: Expected while statement on line %d:%d [-]",
		    walk->line + 1, walk->column + 1);

		GotoPosition(file, walk->line + 1, walk->column + 1);
		return (0);
	}

	return (walk);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*MoveToken(EDIT_TOKEN*openb, EDIT_TOKEN*pos)
{
	EDIT_TOKEN*walk;
	int type, ident, len;
	char*string;

	if ((openb->prev && openb->prev->type == TOKEN_NEWLINE) && (openb->next &&
	    openb->next->type == TOKEN_NEWLINE))
		DeleteToken(openb->next);

	type = openb->type;
	ident = openb->ident;
	len = openb->stringLen;
	string = OS_Malloc(openb->stringLen);
	memcpy(string, openb->string, len);

	DeleteToken(openb);

	walk = InsertToken(pos->next, TOKEN_WS, 0, 0, " ", 1);

	walk = InsertToken(walk->next, type, pos->line, pos->column + 2, string,
	    len);

	walk->ident = ident;

	OS_Free(string);

	return (walk);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*ProcessIfElse(EDIT_FILE*file, EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*elseToken, *closeb;

	if (tokens->type == TOKEN_IDENT && tokens->ident == TOKEN_IDENT_OPENB) {
		closeb = FindMatchingIdent(file, tokens, 0);

		if (!closeb)
			DEBUG_RETURN(0);

		if (closeb->prev->type != TOKEN_NEWLINE)
			InsertToken(closeb, TOKEN_NEWLINE, 0, 0, "", 1);

		elseToken = SkipWhitespace(file, closeb->next);

		if (!elseToken)
			return (tokens);

		if (elseToken->type == TOKEN_KEYWORD && elseToken->ident ==
		    KEYWORD_ELSE) {
			elseToken = MoveToken(elseToken, closeb);

			return (ProcessElseBlock(file, elseToken));
		}
	}

	return (tokens);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*ProcessSwitch(EDIT_FILE*file, EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*closeb;

	if (tokens->type != TOKEN_IDENT || tokens->ident != TOKEN_IDENT_OPENB)
		return (tokens);

	closeb = FindMatchingIdent(file, tokens, 0);

	if (!closeb)
		DEBUG_RETURN(0);

	if (closeb->prev->type != TOKEN_NEWLINE)
		InsertToken(closeb, TOKEN_NEWLINE, 0, 0, "", 1);

	tokens = tokens->next;

	for (; ; ) {
		tokens = SkipWhitespace(file, tokens);

		if (!tokens)
			break;

		if (tokens == closeb)
			break;

		if (tokens->type == TOKEN_KEYWORD && tokens->ident == KEYWORD_CASE) {
			CheckCaseNewline(file, tokens);
			RemoveCaseTab(tokens);
		}

		/* Skip over block */
		if (tokens->type == TOKEN_IDENT && tokens->ident == TOKEN_IDENT_OPENB) {
			tokens = FindMatchingIdent(file, tokens, 0);

			if (!tokens)
				return (0);
		}

		tokens = tokens->next;
	}
	return (tokens);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_TOKEN*ProcessElseBlock(EDIT_FILE*file, EDIT_TOKEN*tokens)
{
	EDIT_TOKEN*walk;
	EDIT_TOKEN*closeb;
	EDIT_TOKEN*statement;

	walk = SkipWhitespace(file, tokens->next);

	if (!walk)
		return (tokens);

	if (walk->type == TOKEN_IDENT && walk->ident == TOKEN_IDENT_OPENB) {
		walk = MoveToken(walk, tokens);

		statement = SkipWhitespace(file, walk->next);

		if (!statement)
			DEBUG_RETURN(0);

		if (statement->prev->type != TOKEN_NEWLINE)
			InsertToken(statement, TOKEN_NEWLINE, 0, 0, "", 1);

		closeb = FindMatchingIdent(file, walk, 0);

		if (!closeb)
			DEBUG_RETURN(0);

		if (closeb->prev->type != TOKEN_NEWLINE)
			InsertToken(closeb, TOKEN_NEWLINE, 0, 0, "", 1);

		return (closeb);
	} else {
		if (walk->prev->type != TOKEN_NEWLINE)
			InsertToken(walk, TOKEN_NEWLINE, 0, 0, "", 1);

		InsertToken(tokens->next, TOKEN_TAB_PLUS, 0, 0, "", 1);

		walk = SkipStatement(file, walk);

		if (!walk)
			DEBUG_RETURN(0);

		InsertToken(walk, TOKEN_TAB_MINUS, 0, 0, "", 1);
	}

	return (tokens);
}


