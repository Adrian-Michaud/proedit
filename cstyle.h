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


#ifndef __CSTYLE_H__
#define __CSTYLE_H__

#define TOKEN_SYMBOL        0x0001
#define TOKEN_WS            0x0002
#define TOKEN_IDENT         0x0004
#define TOKEN_NUMBER        0x0008
#define TOKEN_HEX           0x0010
#define TOKEN_STRING        0x0020
#define TOKEN_COMMENT_BEGIN 0x0040
#define TOKEN_COMMENT_LINE  0x0080
#define TOKEN_COMMENT_END   0x0100
#define TOKEN_PREPROC       0x0200
#define TOKEN_FLOATING      0x0400
#define TOKEN_KEYWORD       0x0800
#define TOKEN_TAB_PLUS      0x1000
#define TOKEN_TAB_MINUS     0x2000
#define TOKEN_NEWLINE       0x4000
#define TOKEN_CPP_COMMENT   0x8000

#define KEYWORD_INT       0
#define KEYWORD_DOUBLE    1
#define KEYWORD_FLOAT     2
#define KEYWORD_LONG      3
#define KEYWORD_CHAR      4
#define KEYWORD_SHORT     5
#define KEYWORD_VOID      6
#define KEYWORD_STRUCT    7
#define KEYWORD_SIGNED    8
#define KEYWORD_UNSIGNED  9
#define KEYWORD_VOLATILE  10
#define KEYWORD_CONST     11
#define KEYWORD_EXTERN    12
#define KEYWORD_STATIC    13
#define KEYWORD_FOR       14
#define KEYWORD_SIZEOF    15
#define KEYWORD_BREAK     16
#define KEYWORD_CASE      17
#define KEYWORD_GOTO      18
#define KEYWORD_IF        19
#define KEYWORD_SWITCH    20
#define KEYWORD_CONTINUE  21
#define KEYWORD_DEFAULT   22
#define KEYWORD_TYPEDEF   23
#define KEYWORD_UNION     24
#define KEYWORD_DO        25
#define KEYWORD_ELSE      26
#define KEYWORD_REGISTER  27
#define KEYWORD_RETURN    28
#define KEYWORD_WHILE     29

#define TOKEN_IDENT_OPENP     (1 ) /* (  Open paren            */
#define TOKEN_IDENT_CLOSEP    (2 ) /* )  Close paren           */
#define TOKEN_IDENT_BITOR     (3 ) /* |  Bitwise OR            */
#define TOKEN_IDENT_BITAND    (4 ) /* &  Bitwise AND           */
#define TOKEN_IDENT_OR        (5 ) /* || Logical OR            */
#define TOKEN_IDENT_AND       (6 ) /* && Logical AND           */
#define TOKEN_IDENT_PLUS      (7 ) /* +  Add                   */
#define TOKEN_IDENT_MINUS     (8 ) /* -  Subtract              */
#define TOKEN_IDENT_TIMES     (9 ) /* *  Multiply              */
#define TOKEN_IDENT_DIV       (10) /* /  Divide                */
#define TOKEN_IDENT_GT        (11) /* >  Greater-than          */
#define TOKEN_IDENT_LT        (12) /* <  Less-than             */
#define TOKEN_IDENT_GTEQ      (13) /* >= Greater-than or Equal */
#define TOKEN_IDENT_LTEQ      (14) /* <= Less-than or Equal    */
#define TOKEN_IDENT_EQ        (15) /* == Equal                 */
#define TOKEN_IDENT_NOTEQ     (16) /* != Not Equal             */
#define TOKEN_IDENT_INV       (17) /* ~  Bitwise NOT           */
#define TOKEN_IDENT_NOT       (18) /* !  Logical NOT           */
#define TOKEN_IDENT_REM       (19) /* %  Remainder             */
#define TOKEN_IDENT_EOR       (20) /* ^  Bitwise Exclusive OR  */
#define TOKEN_IDENT_SHFL      (22) /* << Shift Left            */
#define TOKEN_IDENT_SHFR      (23) /* >> Shift Right           */
#define TOKEN_IDENT_UPLUS     (24) /* +  Unary plus            */
#define TOKEN_IDENT_UMINUS    (25) /* -  Unary minus           */
#define TOKEN_IDENT_DEFINED   (26) /* defined                  */
#define TOKEN_IDENT_QUESTION  (27) /* ?                        */
#define TOKEN_IDENT_COLEN     (28) /* :                        */
#define TOKEN_IDENT_COMMA     (29) /* ,                        */
#define TOKEN_IDENT_OPENI     (30) /* [                        */
#define TOKEN_IDENT_CLOSEI    (31) /* ]                        */
#define TOKEN_IDENT_OPENB     (32) /* {                        */
#define TOKEN_IDENT_CLOSEB    (33) /* }                        */
#define TOKEN_IDENT_SEMI      (35) /* ;                        */
#define TOKEN_IDENT_PERIOD    (36) /* .                        */
#define TOKEN_IDENT_ASSIGN    (37) /* =  Assign Operator       */
#define TOKEN_IDENT_INC       (38) /* ++ Increase              */
#define TOKEN_IDENT_DEC       (39) /* -- Decrease              */
#define TOKEN_IDENT_POINTER   (40) /* -> Pointer               */
#define TOKEN_IDENT_BITOR_EQ  (41) /* |= OR EQ                 */
#define TOKEN_IDENT_BITAND_EQ (42) /* &= OR EQ                 */
#define TOKEN_IDENT_PLUS_EQ   (43) /* += OR EQ                 */
#define TOKEN_IDENT_MINUS_EQ  (44) /* -= OR EQ                 */
#define TOKEN_IDENT_TIMES_EQ  (46) /* *= OR EQ                 */
#define TOKEN_IDENT_DIV_EQ    (47) /* /= OR EQ                 */
#define TOKEN_IDENT_STRUCT    (48) /* .                        */
#define TOKEN_IDENT_BACKSLASH (49) /* \ backslash              */


typedef struct tokenize
{
	int type;
	char*string;
	int stringLen;
	int ident;
	int line;
	int column;
	struct tokenize*next;
	struct tokenize*prev;
}EDIT_TOKEN;

int SunStyle(EDIT_FILE*file, EDIT_TOKEN*tokens, EDIT_CLIPBOARD*clipboard);
int BsdStyle(EDIT_FILE*file, EDIT_TOKEN*tokens, EDIT_CLIPBOARD*clipboard);
int AdrianStyle(EDIT_FILE*file, EDIT_TOKEN*tokens, EDIT_CLIPBOARD*clipboard);
EDIT_TOKEN*TokenizeLines(EDIT_FILE*file, EDIT_LINE*lines);
void DeallocTokens(EDIT_TOKEN*tokens);
void DumpTokens(EDIT_TOKEN*token);
void DisplayToken(EDIT_TOKEN*token);
int TokenCheck(char ch, int ops);
EDIT_TOKEN*DeleteToken(EDIT_TOKEN*base);
EDIT_TOKEN*SkipWhitespace(EDIT_FILE*file, EDIT_TOKEN*tokens);
EDIT_TOKEN*RemoveNewlines(EDIT_TOKEN*from, EDIT_TOKEN*to);
EDIT_TOKEN*SkipStatement(EDIT_FILE*file, EDIT_TOKEN*tokens);
void RemoveCaseTab(EDIT_TOKEN*tokens);
void CheckCaseNewline(EDIT_FILE*file, EDIT_TOKEN*tokens);
EDIT_TOKEN*SkipParenSet(EDIT_FILE*file, EDIT_TOKEN*tokens, int concat);
int GetBestBreak(EDIT_TOKEN*tokens, int maxLineLength);
EDIT_TOKEN*FindMatchingIdent(EDIT_FILE*file, EDIT_TOKEN*tokens, int concat);
EDIT_TOKEN*InsertToken(EDIT_TOKEN*base, int type, int line, int column, char*
    string, int len);


#endif /* __CSTYLE_H__ */



