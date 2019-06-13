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

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE

#include "resource.h"

#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <mbstring.h>
#include <commctrl.h>

#include "../osdep.h"
#include "proedit_gui.h"

static void ProeditThread(char*cmdLine);

extern PROEDIT_WINDOW proeditWindow;

static int nArgs;
static char*argv[255];
static void SetupCommandLine(void);
static LPWSTR*szArgList;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR
    lpCmdLine, int nCmdShow)
{
	MSG msg;

	UNREFERENCED_PARAMETER(hPrevInstance);

	PeRegisterMainClass(hInstance);
	PeRegisterDisplayClass(hInstance);

	SetupCommandLine();

	// Perform application initialization:
	proeditWindow.hInst = hInstance;
	proeditWindow.event = CreateEvent(NULL, FALSE, FALSE, NULL);

	InitializeCriticalSection(&proeditWindow.cs);

	if (!EditorInit(nArgs, argv))
		return (0);

	PeCreateMainWindow(&proeditWindow);
	PeCreateDisplayWindow(&proeditWindow);

	ShowWindow(proeditWindow.hWndMain, nCmdShow);
	UpdateWindow(proeditWindow.hWndMain);

	SetFocus(proeditWindow.hWndDisplay);

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProeditThread, lpCmdLine, 0,
	    0);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
		DispatchMessage(&msg);

	return (msg.wParam);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ProeditThread(char*cmdLine)
{
	UNREFERENCED_PARAMETER(cmdLine);

	EditorMain(nArgs, argv);

	LocalFree(szArgList);

	PostMessage(proeditWindow.hWndMain, PE_SHUTDOWN, 0, 0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SetupCommandLine(void)
{
	int len;
	char*error;
	int i;

	szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	if (szArgList != NULL) {
		if (nArgs > 255)
			nArgs = 255;

		for (i = 0; i < nArgs; i++) {
			len = wcslen(szArgList[i]);

			argv[i] = malloc(len + 1);
			memset(argv[i], 0, len + 1);

			if (!WideCharToMultiByte(CP_ACP, 0, szArgList[i], len, argv[i],
			    len + 1, NULL, NULL)) {
				error = malloc(len + 255);

				sprintf(error, "Error %d, Failed to process command line "
				    "argument :\n\"%ws\"", GetLastError(), szArgList[i]);

				MessageBox(proeditWindow.hWndMain, error,
				    "ProEdit MP 2.0 Error", MB_OK);

				free(error);

				LocalFree(szArgList);

				PostMessage(proeditWindow.hWndMain, WM_CLOSE, 0, 0);
				return ;
			}
		}
	}
}


