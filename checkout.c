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


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int CheckoutFile(EDIT_FILE*file, char*pathname)
{
	char path[MAX_FILENAME];
	char filename[MAX_FILENAME];
	char shellPath[MAX_FILENAME];
	char prompt[MAX_FILENAME];
	char command[MAX_FILENAME];
	char tm[OS_MAX_TIMEDATE];
	EDIT_FILE*shell;
	OS_SHELL*os_shell;

	OS_GetFilename(pathname, path, filename);

	sprintf(prompt, "Checkout %s", filename);

	shell = AllocFile(prompt);

	shell->file_flags |= FILE_FLAG_READONLY | FILE_FLAG_NO_COLORIZE;

	InitDisplayFile(shell);
	InitCursorFile(shell);

	DisableUndo(shell);
	CursorEndFile(shell);
	Paint(shell);

	sprintf(command, "wx edit %s", filename);

	OS_ShellGetPath(shellPath);

	OS_ShellSetPath(path);

	for (; ; ) {
		OS_ShellGetPath(path);
		sprintf(prompt, "%s =>", path);
		Input(HISTORY_CHECKOUT_CMD, prompt, command, MAX_FILENAME);

		if (ProcessShellChdir(command)) {
			strcpy(command, "");
			continue;
		}

		if (!strlen(command))
			break;

		os_shell = OS_ShellCommand(command);

		if (!os_shell)
			break;

		strcpy(command, "");

		OS_Time(tm);
		sprintf(prompt, "--- Command issued: %s ---", tm);
		InsertLine(shell, prompt, strlen(prompt), INS_ABOVE_CURSOR, 0);
		CursorDown(shell);

		sprintf(prompt, "%s", os_shell->shell_cmd);
		InsertLine(shell, prompt, strlen(prompt), INS_ABOVE_CURSOR, 0);
		CursorDown(shell);

		CenterBottomBar(0, "[+] Waiting for command to complete... [+]");

		for (; ; ) {
			shell->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

			Paint(shell);

			if (!OS_ShellGetLine(os_shell)) {
				OS_Time(tm);
				sprintf(prompt, "--- Command Completed: %s ---", tm);
				InsertLine(shell, prompt, strlen(prompt), INS_ABOVE_CURSOR, 0);
				CursorDown(shell);
				shell->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
				Paint(shell);
				break;
			}

			InsertLine(shell, os_shell->line, os_shell->lineLen,
			    INS_ABOVE_CURSOR, 0);
			CursorEnd(shell);

			WordWrapLine(shell);

			CursorDown(shell);
			CursorHome(shell);
		}
		OS_ShellClose(os_shell);
	}

	OS_ShellSetPath(shellPath);

	DeallocFile(shell);
	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
	Paint(file);

	return (1);
}




