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

#include <windows.h>

#define WIN95_DELAY 500

typedef struct osShell{
	BOOL windows95;
	BOOL running;
	DWORD exitcode;
	OSVERSIONINFO osv;
	SECURITY_ATTRIBUTES sa;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD bytesRead;
	DWORD bytesAvail;
	DWORD bytesWrote;
	DWORD timeDetectedDeath;
	SECURITY_DESCRIPTOR sd;
	HANDLE hPipeWrite;
	HANDLE hPipeRead;
	HANDLE hRead2;
	HANDLE hWriteSubProcess;
}
OS_SHELL;

OS_SHELL*OS_ShellCreate(char*cmd);
int OS_ShellClose(OS_SHELL*shell);
int OS_ShellKey(OS_SHELL*shell, int*mode);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void main(int argc, char**argv){
	OS_SHELL*os_shell;
	int mode, ch;

	os_shell = OS_ShellCreate("cmd");

	if (os_shell) {
		for (; ; ) {
			ch = OS_ShellKey(os_shell, &mode);

			if (mode) {
				printf("\nGot aboirt!\n");
				break;
			}
			printf("%c", ch);
		}

		printf("exitcode=%d\n", OS_ShellClose(os_shell));
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_ShellKey(OS_SHELL*shell, int*mode){
	int i;
	char ch;
	int bTest;

	shell->timeDetectedDeath = 0;

	while (shell->running) {
		shell->bytesRead = 0;
		shell->bytesAvail = 0;

		if (!PeekNamedPipe(shell->hPipeRead, &ch, 1, &shell->bytesRead,
		    &shell->bytesAvail, NULL)) {
			shell->bytesAvail = 0;
		}

		if (shell->bytesAvail > 0) {
			bTest = ReadFile(shell->hPipeRead, &ch, 1, &shell->bytesRead, NULL);

			if (bTest && shell->bytesRead) {
				*mode = 0;
				return (ch);
			} else
				shell->running = FALSE;
		} else {
			if (kbhit()) {
				ch = (char)getch();
				if (ch == 13)
					ch = 10;
				WriteFile(shell->hWriteSubProcess, &ch, 1, &shell->bytesWrote,
				    NULL);
				*mode = 0;
				return (ch);
			}

			if (GetExitCodeProcess(shell->pi.hProcess, &shell->exitcode)) {
				if (STILL_ACTIVE != shell->exitcode) {
					if (shell->windows95) {
						// Process is dead, but wait a second in case there is some output in transit
						if (shell->timeDetectedDeath == 0) {
							shell->timeDetectedDeath = GetTickCount();
						} else {
							if ((GetTickCount() - shell->timeDetectedDeath) >
							    WIN95_DELAY) {
								shell->running = FALSE; // It's a dead process
							}
						}
					} else { // NT, so dead already
						shell->running = FALSE;
					}
				}
			}
		}
	}
	*mode = 1;
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_ShellClose(OS_SHELL*shell){
	int exitcode;

	if (shell->running) {
		if (WAIT_OBJECT_0 != WaitForSingleObject(shell->pi.hProcess, 500))
			TerminateProcess(shell->pi.hProcess, 1);

		if (WAIT_OBJECT_0 != WaitForSingleObject(shell->pi.hProcess, 1000))
			TerminateProcess(shell->pi.hProcess, 2);

		GetExitCodeProcess(shell->pi.hProcess, &shell->exitcode);

		CloseHandle(shell->pi.hProcess);
		CloseHandle(shell->pi.hThread);
	}

	CloseHandle(shell->hPipeRead);
	CloseHandle(shell->hPipeWrite);
	CloseHandle(shell->hRead2);
	CloseHandle(shell->hWriteSubProcess);

	exitcode = shell->exitcode;

	free(shell);

	return (exitcode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
OS_SHELL*OS_ShellCreate(char*cmd){
	OS_SHELL*shell;

	shell = (OS_SHELL*)malloc(sizeof(OS_SHELL));
	memset(shell, 0, sizeof(OS_SHELL));

	shell->osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	strcpy(shell->osv.szCSDVersion, "");

	shell->sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	shell->si.cb = sizeof(STARTUPINFO);

	GetVersionEx(&shell->osv);

	shell->windows95 = (shell->osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

	shell->sa.bInheritHandle = TRUE;
	shell->sa.lpSecurityDescriptor = NULL;

	if (!shell->windows95) {
		InitializeSecurityDescriptor(&shell->sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&shell->sd, TRUE, NULL, FALSE);
		shell->sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		shell->sa.lpSecurityDescriptor = &shell->sd;
	}

	CreatePipe(&shell->hPipeRead, &shell->hPipeWrite, &shell->sa, 0);

	shell->hWriteSubProcess = NULL;

	CreatePipe(&shell->hRead2, &shell->hWriteSubProcess, &shell->sa, 0);

	SetHandleInformation(shell->hPipeRead, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(shell->hWriteSubProcess, HANDLE_FLAG_INHERIT, 0);

	shell->si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	//si.wShowWindow = SW_HIDE;

	shell->si.wShowWindow = SW_SHOW;

	shell->si.hStdInput = shell->hRead2;
	shell->si.hStdOutput = shell->hPipeWrite;
	shell->si.hStdError = shell->hPipeWrite;

	shell->running = CreateProcess(NULL, cmd, NULL, NULL, TRUE,
	    CREATE_NEW_PROCESS_GROUP, NULL, NULL,
	    // current directory.&shell->si, &shell->pi);

	if (!shell->running) {
		OS_ShellClose(shell);
		return (0);
	}

	return (shell);
}


