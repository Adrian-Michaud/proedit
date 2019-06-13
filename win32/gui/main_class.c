#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE

#include "resource.h"

#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "../../osdep.h"
#include "proedit_gui.h"

#define PROEDIT_MAIN_CLASS _T("ProeditMainWindow")
#define PROEDIT_MAIN_TITLE _T("Proedit MP v2.0")

#define PROEDIT_STYLE WS_POPUPWINDOW|WS_CAPTION|WS_CLIPCHILDREN

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
BOOL PeCreateMainWindow(PROEDIT_WINDOW *pe)
{
   RECT rect;

   rect.top    = 0;
   rect.left   = 0;
   rect.right  = (pe->screen_xd-1);
   rect.bottom = (pe->screen_yd-1);

   AdjustWindowRect(&rect, PROEDIT_STYLE, TRUE);

   PeStatusWindowSize(0, pe);

   pe->hWndMain = CreateWindow(PROEDIT_MAIN_CLASS,
                       PROEDIT_MAIN_TITLE,
                       PROEDIT_STYLE,
                       CW_USEDEFAULT, 
                       CW_USEDEFAULT, 
                       (rect.right - rect.left),
                       (rect.bottom - rect.top)+pe->statusHeight,
                       NULL, NULL, pe->hInst, NULL);

   if (!pe->hWndMain)
      return FALSE;

   SetWindowLong(pe->hWndMain, 0, (LONG)pe);

   return TRUE;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeRegisterMainClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	//Window class for the main application window
	wcex.cbSize             = sizeof(WNDCLASSEX);
	wcex.style		= 0;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(void *);
	wcex.hInstance		= hInstance;
	wcex.hIcon		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROEDIT_GUI));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_PROEDIT_GUI);
	wcex.lpszClassName	= PROEDIT_MAIN_CLASS;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassEx(&wcex);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PROEDIT_WINDOW *pe;
        char keyState[256];

	pe = (PROEDIT_WINDOW *)GetWindowLong(hWnd, 0);

	switch (message)
	{
	case WM_COMMAND:
             {
             wmId    = LOWORD(wParam);
             wmEvent = HIWORD(wParam);
             // Parse the menu selections:
             switch (wmId)
                {
                case IDM_ABOUT:
                     DialogBox(pe->hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
	             return(0);

                case IDM_EXIT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'X', 0, keyState);
	             return(0);

                case ID_FILE_OPEN40002:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'E', 0, keyState);
	             return(0);

                case ID_FILE_CLOSE40003:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'Q', 0, keyState);
	             return(0);

                case ID_FILE_SAVE40004:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'W', 0, keyState);
	             return(0);

                case ID_FILE_RENAME:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'T', 0, keyState);
	             return(0);

                case ID_EDIT_UNDO40008:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'U', 0, keyState);
	             return(0);

                case ID_EDIT_CUT40009:
                     memset(keyState,0,sizeof(keyState));
                     PeAddInputRecord(WM_KEYDOWN, VK_DELETE, 0, keyState);
	             return(0);

                case ID_EDIT_COPY40010:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_CONTROL] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'C', 0, keyState);
	             return(0);

                case ID_EDIT_PASTE40011:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_CONTROL] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'V', 0, keyState);
	             return(0);

                case ID_EDIT_BLOCKSELECTALT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'V', 0, keyState);
	             return(0);

                case ID_EDIT_LINESELECTALT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'C', 0, keyState);
	             return(0);

                case ID_EDIT_DELETELINEALT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'D', 0, keyState);
	             return(0);

                case ID_EDIT_ERASETOEOLALT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'K', 0, keyState);
	             return(0);

                case ID_SEARCH_FINDALT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'S', 0, keyState);
	             return(0);

                case ID_SEARCH_FINDAGAINALT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'L', 0, keyState);
	             return(0);

                case ID_SEARCH_REPLACEALT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'R', 0, keyState);
	             return(0);

                case ID_SEARCH_IDENT:
                     memset(keyState,0,sizeof(keyState));
                     keyState[VK_MENU] = 0x80;
                     PeAddInputRecord(WM_KEYDOWN, 'M', 0, keyState);
	             return(0);

                case ID_CASESENSITIVITYF5_CASESENSITIVESEARCH:
                     return(0);

                case ID_CASESENSITIVITYF5_NON:
                     return(0);

                case ID_FILESEARCHINGF5_CURRENTFILEONLY:
                     return(0);

                case ID_FILESEARCHINGF5_SEARCHALLLOADEDFILES:
                     return(0);
                }
             break;
             }


        case WM_SETFOCUS:
             if (pe)
                SetFocus(pe->hWndDisplay);
             break;

        case PE_SHUTDOWN:
             DestroyWindow(hWnd);
             return(0);
               
        case WM_CLOSE:
             memset(keyState,0,sizeof(keyState));
             keyState[VK_MENU] = 0x80;
             PeAddInputRecord(WM_KEYDOWN, 'X', 0, keyState);
             return(0);

	case WM_DESTROY:
             PostQuitMessage(0);
             break;
	}

   return DefWindowProc(hWnd, message, wParam, lParam);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeDisableMenu(PROEDIT_WINDOW *pe)
{
HMENU menu;

   menu = GetMenu(pe->hWndMain);

   EnableMenuItem(menu, ID_FILE_OPEN40002                         , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_FILE_CLOSE40003                        , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_FILE_SAVE40004                         , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_FILE_RENAME                            , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_EDIT_UNDO40008                         , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_EDIT_CUT40009                          , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_EDIT_COPY40010                         , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_EDIT_PASTE40011                        , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_EDIT_BLOCKSELECTALT                    , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_EDIT_LINESELECTALT                     , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_EDIT_DELETELINEALT                     , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_EDIT_ERASETOEOLALT                     , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_SEARCH_FINDALT                         , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_SEARCH_FINDAGAINALT                    , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_SEARCH_REPLACEALT                      , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_SEARCH_IDENT                           , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_CASESENSITIVITYF5_CASESENSITIVESEARCH  , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_CASESENSITIVITYF5_NON                  , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_FILESEARCHINGF5_CURRENTFILEONLY        , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
   EnableMenuItem(menu, ID_FILESEARCHINGF5_SEARCHALLLOADEDFILES   , MF_GRAYED|MF_BYCOMMAND|MF_DISABLED);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeEnableMenu(PROEDIT_WINDOW *pe)
{
HMENU menu;

   menu = GetMenu(pe->hWndMain);

   EnableMenuItem(menu, ID_FILE_OPEN40002                         , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_FILE_CLOSE40003                        , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_FILE_SAVE40004                         , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_FILE_RENAME                            , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_EDIT_UNDO40008                         , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_EDIT_CUT40009                          , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_EDIT_COPY40010                         , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_EDIT_PASTE40011                        , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_EDIT_BLOCKSELECTALT                    , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_EDIT_LINESELECTALT                     , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_EDIT_DELETELINEALT                     , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_EDIT_ERASETOEOLALT                     , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_SEARCH_FINDALT                         , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_SEARCH_FINDAGAINALT                    , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_SEARCH_REPLACEALT                      , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_SEARCH_IDENT                           , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_CASESENSITIVITYF5_CASESENSITIVESEARCH  , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_CASESENSITIVITYF5_NON                  , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_FILESEARCHINGF5_CURRENTFILEONLY        , MF_BYCOMMAND|MF_ENABLED);
   EnableMenuItem(menu, ID_FILESEARCHINGF5_SEARCHALLLOADEDFILES   , MF_BYCOMMAND|MF_ENABLED);
}

