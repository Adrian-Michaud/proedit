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

static int calculatorError;
static int Calculate(char*tokens, unsigned long long*data, int*length);
static int GetLeft(char*tokens, unsigned long long*data, int index, char*
    newTokens, unsigned long long*newData, int*newLen);
static int GetRight(char*tokens, unsigned long long*data, int length, int index,
    char*newTokens, unsigned long long*newData, int*newLen);

static int EvaluateExpression(EDIT_FILE*file, char*exp, int length);

static void DeleteTokens(char*tokens, unsigned long long*data, int*length, int
    from, int to);
static int FindToken(char*tokens, int length, char ch);
static EDIT_FILE*CalculatorInput(EDIT_FILE*file, int mode, int ch);
static EDIT_FILE*CalculatorWindow(EDIT_FILE*file);

static EDIT_FILE*global_calc = 0;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void Calculator(EDIT_FILE*file)
{
	char*scrBuff;
	EDIT_FILE*calc;

	file->paint_flags |= CURSOR_FLAG;

	scrBuff = SaveScreen();

	calc = CalculatorWindow(file);

	for (; ; ) {
		Paint(calc);

		UpdateStatusBar(calc);

		if (!ProcessUserInput(calc, CalculatorInput))
			break;
	}

	RestoreScreen(scrBuff);
}

#ifdef DEBUG_MEMORY
/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CloseCalculator(void)
{
	if (global_calc) {
		DeallocFile(global_calc);
		global_calc = 0;
	}
}
#endif

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*CalculatorWindow(EDIT_FILE*file)
{
	int x, y, xd, yd;

	if (!global_calc) {
		xd = file->display.xd - 2;
		yd = file->display.yd / 2;

		x = file->display.xd / 2 - xd / 2;
		y = (file->display.yd - 1) / 2 - yd / 2;

		global_calc = AllocFile("Calculator");

		InitDisplayFile(global_calc);

		global_calc->display.xpos = x;
		global_calc->display.ypos = y;
		global_calc->display.xd = xd;
		global_calc->display.yd = yd;
		global_calc->display.columns = xd - 2;
		global_calc->display.rows = yd - 3;

		global_calc->client = 2;
		global_calc->border = 2;

		global_calc->file_flags |= FILE_FLAG_NONFILE;

		InitCursorFile(global_calc);

		CursorTopFile(global_calc);

		GotoPosition(global_calc, 1, 1);
	}

	global_calc->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	return (global_calc);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*CalculatorInput(EDIT_FILE*file, int mode, int ch)
{
	if (!mode) {
		if (ch == ED_KEY_CR) {
			if (file->cursor.offset && file->cursor.line->line) {
				EvaluateExpression(file, file->cursor.line->line, file->cursor.
				    offset);
				return (file);
			}
		}

		if (ch == ED_KEY_ESC)
			return (0);

		ProcessNormal(file, ch);
	} else {
		switch (ch) {
		case ED_ALT_A : /* Ignore Save As        */
		case ED_ALT_N : /* Ignore Next File      */
		case ED_ALT_Y : /* Ignore Merge          */
		case ED_ALT_F : /* Ignore File Select    */
		case ED_ALT_W : /* Ignore Write          */
		case ED_ALT_E : /* Ignore Newfile        */
		case ED_ALT_T : /* Ignore Title change   */
		case ED_ALT_H : /* Ignore Help           */
		case ED_ALT_O : /* Ignore Operations     */
		case ED_ALT_B : /* Ignore Build          */
		case ED_ALT_Z : /* Ignore Shell          */
		case ED_F3 : /* File Backups          */
		case ED_F5 : /* Ignore Toggle Case    */
		case ED_F6 : /* Ignore Toggle Global  */
		case ED_F9 : /* Ignore Utils          */
			break;

		case ED_ALT_Q : /* Exit Calculator       */
		case ED_ALT_X : /* Exit Calculator       */
			return (0);

			default :
			file = ProcessSpecial(file, ch);
		}
	}

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int EvaluateExpression(EDIT_FILE*file, char*exp, int length)
{
	char bin[128];
	int i, total = 0;
	char*tokens;
	unsigned long long*data;
	char results[512];
	int bits_ul = sizeof(unsigned long long)*8;

	calculatorError = 0;

	tokens = OS_Malloc(length);
	data = (unsigned long long*)OS_Malloc(length*sizeof(unsigned long long));

	/* Tokenize Calculator Input */
	for (i = 0; i < length; i++) {
		if (exp[i] <= 32)
			continue;

		if (exp[i] == '(')
			tokens[total++] = '('; /* TOKEN_OPEN_PAREN */
		if (exp[i] == ')')
			tokens[total++] = ')'; /* TOKEN_CLOSE_PAREN */
		if (exp[i] == '+')
			tokens[total++] = '+'; /* TOKEN_PLUS */
		if (exp[i] == '-')
			tokens[total++] = '-'; /* TOKEN_MINUS */
		if (exp[i] == '*')
			tokens[total++] = '*'; /* TOKEN_TIMES */
		if (exp[i] == '/')
			tokens[total++] = '/'; /* TOKEN_DIVIDE */

		if (exp[i] == '%') {
			char num[256];
			int len = 0, j, k;
			tokens[total] = 'n'; /* TOKEN_NUMBER */

			for (j = i + 1; j < length; j++) {
				if ((exp[j] == '0' || exp[j] == '1')) {
					num[len++] = exp[j];
				} else
					break;
			}
			num[len] = 0;
			data[total] = 0;
			for (k = strlen(num) - 1; k >= 0; k--)
				if (num[k] == '1')
					data[total] |= (1 << (strlen(num) - (k + 1)));
			i = j - 1;
			total++;
		}

		if (exp[i] >= '0' && exp[i] <= '9') {
			char num[256];
			int len = 0, j;
			tokens[total] = 'n'; /* TOKEN_NUMBER */
			if (exp[i + 1] == 'x' || exp[i + 1] == 'X') {
				for (j = i + 2; j < length; j++) {
					if ((exp[j] >= '0' && exp[j] <= '9') || (exp[j] >= 'a' &&
					    exp[j] <= 'f') || (exp[j] >= 'A' && exp[j] <= 'F')) {
						num[len++] = exp[j];
					} else
						break;
				}
				num[len] = 0;
				data[total] = strtoull(num, 0, 16);
				//	    sscanf(num, "%lx", (unsigned long *)&data[total]);
			} else {
				for (j = i; j < length; j++) {
					if (exp[j] < '0' || exp[j] > '9')
						break;
					num[len++] = exp[j];
				}
				num[len] = 0;
				data[total] = strtoull(num, 0, 10);
			}
			i = j - 1;
			total++;
		}
	}

	/* Fix Signed Numbers */
	for (i = 0; i < total; i++) {
		if (tokens[i] == '-' && (i + 1 < total) && tokens[i + 1] == 'n') {
			if (i) {
				if (tokens[i - 1] == ')')
					continue;
				if (tokens[i - 1] == 'n')
					continue;
			}

			data[i + 1] = data[i + 1]*-1; /* Negate Data */
			DeleteTokens(tokens, data, &total, i, i);
		}
	}

	while (Calculate(tokens, data, &total));

	if (calculatorError) {
		OS_Free(tokens);
		OS_Free(data);
		return (0);
	}

	if (total != 1) {
		CenterBottomBar(1, "[-] Syntax Error In Expression [-]");
		OS_Free(tokens);
		OS_Free(data);
		return (0);
	}

	/* Convert number to binary */
	strcpy(bin, "");
	for (i = 0; i < bits_ul; i++) {
		if (data[0]&((unsigned long long)1 << ((bits_ul - 1) - i)))
			bin[i] = '1';
		else
			bin[i] = '0';
	}
	bin[i] = 0;

	UndoBegin(file);
	CarrageReturn(file, NO_INDENT);

	sprintf(results, "=%llu (0x%0llx) (b%s)", data[0], data[0], bin);
	InsertText(file, results, strlen(results), 0);
	CursorEnd(file);
	CarrageReturn(file, NO_INDENT);

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	OS_Free(tokens);
	OS_Free(data);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int Calculate(char*tokens, unsigned long long*data, int*length)
{
	int index, op = 0, maxLength;
	char*newTokens;
	unsigned long long*newData;
	int newLength;
	unsigned long long lvalue, rvalue;
	int lpos, rpos;

	maxLength = *length;

	if (calculatorError)
		return (0);

	index = FindToken(tokens, *length, '*');

	if (index != -1)
		op = '*';
	else {
		index = FindToken(tokens, *length, '/');

		if (index != -1)
			op = '/';
		else {
			index = FindToken(tokens, *length, '+');

			if (index != -1)
				op = '+';
			else {
				index = FindToken(tokens, *length, '-');

				if (index != -1)
					op = '-';
			}
		}
	}

	if (!op) {
		while (*length > 1) {
			if (tokens[0] == '(' && tokens[*length - 1] == ')') {
				DeleteTokens(tokens, data, length, 0, 0);
				DeleteTokens(tokens, data, length, *length - 1, *length - 1);
			} else {
				CenterBottomBar(1, "[-] Syntax Error [-]");
				calculatorError = 1;
				return (0);
			}
		}
	}

	newTokens = (char*)OS_Malloc(maxLength);
	newData = (unsigned long long*)OS_Malloc(maxLength*sizeof(unsigned long
	    long));

	lpos = GetLeft(tokens, data, index, newTokens, newData, &newLength);

	if (lpos == -1) {
		OS_Free(newTokens);
		OS_Free(newData);
		return (0);
	}

	while (Calculate(newTokens, newData, &newLength));

	if (newLength != 1 || newTokens[0] != 'n') {
		CenterBottomBar(1, "[-] Syntax Error [-]");
		OS_Free(newTokens);
		OS_Free(newData);
		calculatorError = 1;
		return (0);
	}

	lvalue = newData[0];

	rpos = GetRight(tokens, data, *length, index, newTokens, newData,
	    &newLength);

	if (rpos == -1) {
		OS_Free(newTokens);
		OS_Free(newData);
		return (0);
	}

	while (Calculate(newTokens, newData, &newLength));

	if (newLength != 1 || newTokens[0] != 'n') {
		CenterBottomBar(1, "[-] Syntax Error [-]");
		OS_Free(newTokens);
		OS_Free(newData);
		calculatorError = 1;
		return (0);
	}

	rvalue = newData[0];

	OS_Free(newTokens);
	OS_Free(newData);

	switch (op) {
	case '*' :
		lvalue = lvalue*rvalue;
		break;

	case '/' :
		if (rvalue == 0) {
			CenterBottomBar(1, "[-] Division By 0 Error [-]");
			calculatorError = 1;
			return (0);
		}
		lvalue = lvalue / rvalue;
		break;

	case '+' :
		lvalue = lvalue + rvalue;
		break;

	case '-' :
		lvalue = lvalue - rvalue;
		break;

	}

	DeleteTokens(tokens, data, length, lpos + 1, rpos);
	tokens[lpos] = 'n';
	data[lpos] = lvalue;

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GetLeft(char*tokens, unsigned long long*data, int index, char*
    newTokens, unsigned long long*newData, int*newLen)
{
	int i, j, len = 0, openP = 0;

	for (i = index - 1; i >= 0; i--) {
		if (tokens[i] == ')')
			openP++;
		if (!openP && tokens[i] != 'n')
			break;
		if (tokens[i] == '(')
			openP--;
	}

	i++;

	if (!i && openP) {
		CenterBottomBar(1, "[-] Syntax Error [-]");
		calculatorError = 1;
		return (-1);
	}

	for (j = i; j < index; j++) {
		newTokens[len] = tokens[j];
		newData[len] = data[j];
		len++;
	}

	*newLen = len;

	return (i);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GetRight(char*tokens, unsigned long long*data, int length, int index,
    char*newTokens, unsigned long long*newData, int*newLen)
{
	int i, j, len = 0, openP = 0;

	for (i = index + 1; i < length; i++) {
		if (tokens[i] == '(')
			openP++;
		if (!openP && tokens[i] != 'n')
			break;
		if (tokens[i] == ')')
			openP--;
	}

	if (i == length && openP) {
		CenterBottomBar(1, "[-] Syntax Error [-]");
		calculatorError = 1;
		return (-1);
	}

	for (j = index + 1; j < i; j++) {
		newTokens[len] = tokens[j];
		newData[len] = data[j];
		len++;
	}

	*newLen = len;
	return (j - 1);
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
static int FindToken(char*tokens, int length, char ch)
{
	int i;

	for (i = 0; i < length; i++)
		if (tokens[i] == ch)
			return (i);

	return (-1);
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
static void DeleteTokens(char*tokens, unsigned long long*data, int*length, int
    from, int to)
{
	int i, len, index;

	len = *length;

	index = from;

	for (i = to + 1; i < len; i++) {
		tokens[index] = tokens[i];
		data[index] = data[i];
		index++;
	}

	len -= ((to - from) + 1);

	if (len < 0)
		len = 0;

	*length = len;
}


