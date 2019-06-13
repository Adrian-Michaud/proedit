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

#include "../../osdep.h"
#include "proedit_gui.h"

void ProeditThread(void *na);

extern PROEDIT_WINDOW proeditWindow;

static int nArgs;
static char *argv[255];
static void SetupCommandLine(void);
static LPWSTR *szArgList;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine, int nCmdShow)
{
   MSG msg;

   UNREFERENCED_PARAMETER(hPrevInstance);

   PeRegisterMainClass(hInstance);
   PeRegisterDisplayClass(hInstance);
   PeRegisterStatusClass(hInstance);

   SetupCommandLine();

   if (!EditorInit(nArgs, argv))
      return(0);

   // Perform application initialization:
   proeditWindow.hInst = hInstance;

   PeCreateMainWindow(&proeditWindow);
   PeCreateDisplayWindow(&proeditWindow);
   PeCreateStatusWindow(&proeditWindow);

   ShowWindow(proeditWindow.hWndMain, nCmdShow);
   UpdateWindow(proeditWindow.hWndMain);

   SetFocus(proeditWindow.hWndDisplay);

   CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProeditThread, lpCmdLine, 0, 0);

   // Main message loop:
   while (GetMessage(&msg, NULL, 0, 0))
      {
      if (proeditWindow.input.enabled)
         TranslateMessage(&msg);

      DispatchMessage(&msg);
      }

   return(msg.wParam);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ProeditThread(char *cmdLine)
{
   UNREFERENCED_PARAMETER(cmdLine);

   proeditWindow.event = CreateEvent(NULL, FALSE, FALSE, NULL);

   InitializeCriticalSection(&proeditWindow.cs);

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
char *error;
int i;

   szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);

   if (szArgList != NULL)
      {
      if (nArgs > 255)
         nArgs = 255;

      for (i=0; i<nArgs; i++)
         {
         len = wcslen(szArgList[i]);

         argv[i] = malloc(len+1);
         memset(argv[i], 0, len+1);

         if (!WideCharToMultiByte(CP_ACP, 0, szArgList[i], 
                len, argv[i], len+1, NULL, NULL))
            {
            error = malloc(len+255);

            sprintf(error, "Error %d, Failed to process command line "
                  "argument :\n\"%ws\"", GetLastError(), szArgList[i]);

            MessageBox(proeditWindow.hWndMain, error, 
               "ProEdit MP 2.0 Error", MB_OK);

            free(error);

            LocalFree(szArgList);

            PostMessage(proeditWindow.hWndMain, WM_CLOSE, 0, 0);
            return;
            }
         }
      }
}
