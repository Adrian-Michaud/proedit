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

static char last_build[MAX_BUILD];
static char force_build[MAX_BUILD];

static int forceBuild = 0;

extern int auto_build_saveall;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*Build(EDIT_FILE*file)
{
	if (!strlen(force_build))
		return (BuildShell(file));

	strcpy(last_build, force_build);

	forceBuild = 1;

	file = ClearErrorWarnings(file);

	file = BuildShell(file);

	forceBuild = 0;

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*BuildShell(EDIT_FILE*file)
{
	EDIT_FILE*shell, *errorFile;
	OS_SHELL*os_shell;
	char temp[MAX_BUILD];
	char shellDir[MAX_FILENAME];
	int errors, warnings, quit;
	SHELL_ERROR*error;
	char curDir[MAX_FILENAME];
	int ch;

	if (NumberOfErrors()) {
		for (; ; ) {
			ch = Question(
			    "\"%s\" has Warnings/Errors, [C]lear, [V]iew, [Q]uit:",
			    last_build);

			if (ch == 'C' || ch == 'c') {
				file = ClearErrorWarnings(file);
				break;
			}

			if (ch == 'V' || ch == 'v') {
				errorFile = CreateErrorPicklist(file);
				if (errorFile) {
					errorFile->paint_flags |= CONTENT_FLAG | CURSOR_FLAG |
					    FRAME_FLAG;
					return (errorFile);
				}
				file->paint_flags |= CURSOR_FLAG;
				return (file);
			}

			if (ch == 'Q' || ch == 'q' || ch == ED_KEY_ESC) {
				file->paint_flags |= CURSOR_FLAG;
				return (file);
			}
		}
	}

	if (auto_build_saveall) {
		if (!ForceSaveAllModified(file))
			return (file);
	} else {
		if (!SaveAllModified(file))
			return (file);
	}

	for (; ; ) {
		OS_ShellGetPath(shellDir);

		if (!forceBuild) {
			sprintf(temp, "%s =>", shellDir);

			strcpy(last_build, "");

			Input(HISTORY_BUILD_CMD, temp, last_build, MAX_BUILD);

			if (ProcessShellChdir(last_build))
				continue;

			strcpy(force_build, last_build);
		}

		file->paint_flags |= CURSOR_FLAG;

		if (!strlen(last_build)) {
			file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
			return (file);
		}

		OS_GetCurrentDir(curDir);
		OS_SetCurrentDir(shellDir);

		os_shell = OS_ShellCommand(last_build);

		if (!os_shell) {
			CenterBottomBar(1, "[-] The shell failed to start. [-]");
			file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
			OS_SetCurrentDir(curDir);
			return (file);
		}

		LoadFindState(os_shell->cwd, os_shell->command);

		shell = FileAlreadyLoaded("Build Console");

		if (!shell) {
			shell = AllocFile("Build Console");

			shell->file_flags |= FILE_FLAG_READONLY | FILE_FLAG_NO_COLORIZE;

			InitDisplayFile(shell);
			InitCursorFile(shell);

			AddFile(shell, ADD_FILE_SORTED);

			DisableUndo(shell);
		}

		CursorEndFile(shell);

		CenterBottomBar(0, "[+] Running Shell, Press [ESC] to Abort [+]");

		InsertLine(shell, os_shell->shell_cmd, strlen(os_shell->shell_cmd),
		    INS_ABOVE_CURSOR, 0);
		CursorDown(shell);

		errors = 0;
		warnings = 0;

		shell = ClearErrorWarnings(shell);

		shell->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

		file = shell;

		quit = 0;
		for (; ; ) {
			Paint(shell);

			if (!OS_ShellGetLine(os_shell))
				break;

			InsertLine(shell, os_shell->line, os_shell->lineLen,
			    INS_ABOVE_CURSOR, 0);
			CursorEnd(shell);

			error = ProcessErrorLine(os_shell->line, os_shell->lineLen);

			if (error) {
				if (strlen(error->error))
					errors++;
				if (strlen(error->warning))
					warnings++;
				HighlightLine(shell, shell->cursor.line, 1);
				if (!quit)
					quit = AddErrorWarning(os_shell->cwd, error);
				shell->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

				OS_Free(error);
			}

			WordWrapLine(shell);

			CursorDown(shell);
			CursorHome(shell);

			shell->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
		}

		UpdateStatusBar(shell);

		SaveFindState(os_shell->cwd, os_shell->command);

		OS_ShellClose(os_shell);

		OS_SetCurrentDir(curDir);

		if (warnings || errors) {
			InsertLine(shell, 0, 0, INS_ABOVE_CURSOR, 0);
			CursorDown(shell);
			sprintf(temp, "--- There were (%d) Warnings, and (%d) Errors ---",
			    warnings, errors);
			InsertLine(shell, temp, strlen(temp), INS_ABOVE_CURSOR, 0);
			CursorDown(shell);
		}

		InsertLine(shell, 0, 0, INS_ABOVE_CURSOR, 0);
		CursorDown(shell);

		shell->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
		Paint(shell);

		if (warnings || errors || forceBuild)
			break;
	}

	if (NumberOfErrors()) {
		errorFile = CreateErrorPicklist(shell);
		if (errorFile) {
			errorFile->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
			return (errorFile);
		}
		shell->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
	}

	return (shell);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessShellChdir(char*cmd)
{
	char curDir[MAX_FILENAME];
	char shellDir[MAX_FILENAME];

	while (*cmd && (*cmd <= 32))
		cmd++;

	OS_ShellGetPath(shellDir);

	if ((cmd[0] == 'c' || cmd[0] == 'C') && (cmd[1] == 'd' || cmd[1] == 'D') &&
	    (cmd[2] == 32)) {
		OS_GetCurrentDir(curDir);
		OS_SetCurrentDir(shellDir);
		OS_SetCurrentDir(&cmd[3]);
		OS_GetCurrentDir(shellDir);
		OS_ShellSetPath(shellDir);
		OS_SetCurrentDir(curDir);
		return (1);
	}
	return (0);
}


