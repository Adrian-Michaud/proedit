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

#define PROEDIT_DISPLAY_CLASS _T("ProeditDisplayWindow")

#define MAX(a,b) (((a)>(b))?(a):(b))

LRESULT CALLBACK ProEditDisplayWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK PePicklistDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
BOOL PeCreateDisplayWindow(PROEDIT_WINDOW *pe)
{
   pe->hWndDisplay=CreateWindow(PROEDIT_DISPLAY_CLASS, _T(""), 
		WS_CHILD|WS_VISIBLE,
		0, 0, 
                pe->screen_xd,
                pe->screen_yd,
		pe->hWndMain, 
		0, 
		pe->hInst, 
		0);

   if (!pe->hWndDisplay)
      return FALSE;

   SetWindowLong(pe->hWndDisplay, 0, (LONG)pe);

   return TRUE;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeRegisterDisplayClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	//Window class for the ProEdit display window
	wcex.cbSize             = sizeof(WNDCLASSEX);
	wcex.style		= 0;
	wcex.lpfnWndProc	= ProEditDisplayWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(void *);
	wcex.hInstance		= hInstance;
	wcex.hIcon		= 0;
	wcex.hCursor		= LoadCursor (NULL, IDC_IBEAM);
	wcex.hbrBackground	= (HBRUSH)0;
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= PROEDIT_DISPLAY_CLASS;	
	wcex.hIconSm		= 0;

	RegisterClassEx(&wcex);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
LRESULT CALLBACK ProEditDisplayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        char keyState[256];
	PROEDIT_WINDOW *pe;

	pe = (PROEDIT_WINDOW *)GetWindowLong(hWnd, 0);

	switch(message)
	{
	case WM_NCCREATE:
		return TRUE;

	case WM_NCDESTROY:
		return 0;

        #if 0
        case WM_VSCROLL:
                switch(LOWORD(wParam))
                   {
                   case SB_THUMBPOSITION:
                   case SB_THUMBTRACK:
                        break;

                   case SB_LINEDOWN:
                        memset(keyState,0,sizeof(keyState));
                        keyState[VK_MENU] = 0x80;
                        PeAddInputRecord(WM_KEYDOWN, VK_DOWN, 0, keyState);
                        return(0);

                   case SB_LINEUP:
                        memset(keyState,0,sizeof(keyState));
                        keyState[VK_MENU] = 0x80;
                        PeAddInputRecord(WM_KEYDOWN, VK_UP, 0, keyState);
                        return(0);

                   case SB_PAGEDOWN:
                        memset(keyState,0,sizeof(keyState));
                        PeAddInputRecord(WM_KEYDOWN, VK_NEXT, 0, keyState);
                        return(0);

                   case SB_PAGEUP:
                        memset(keyState,0,sizeof(keyState));
                        PeAddInputRecord(WM_KEYDOWN, VK_PRIOR, 0, keyState);
                        return(0);
                   }
                break;
        #endif

	case WM_SETFOCUS:
                CreateCaret(hWnd, NULL, pe->cursor_sizeX, pe->cursor_sizeY);
                SetCaretPos(pe->cursor_x, pe->cursor_y);
                ShowCaret(hWnd);
                break;

        case PE_PICKLIST:
                DialogBox(pe->hInst, MAKEINTRESOURCE(IDD_PICKLIST), hWnd, 
                     PePicklistDlgProc);
                return(0);

        case PE_SET_CURSOR_POS:
                SetCaretPos(pe->cursor_x, pe->cursor_y);
                return(0);

        case PE_SET_CURSOR_SHOW:
                ShowCaret(hWnd);
                return(0);

        case PE_SET_CURSOR_HIDE:
                HideCaret(hWnd);
                return(0);

        case PE_SET_CURSOR_TYPE:
                DestroyCaret();
                CreateCaret(hWnd, NULL, pe->cursor_sizeX, pe->cursor_sizeY);
                SetCaretPos(pe->cursor_x, pe->cursor_y);
                ShowCaret(hWnd);
                return(0);

        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
                GetKeyboardState((PBYTE)keyState);
                PeAddInputRecord(message, wParam, lParam, keyState);
                return(0);

        case WM_KILLFOCUS:
                DestroyCaret();
                break;

	case WM_PAINT:
		pe->hdc = BeginPaint(hWnd, &pe->ps);
                SelectObject(pe->hdc, pe->hFont);
                ProeditPaint(pe);
		EndPaint(hWnd, &pe->ps);
		return(0);
	
	default:
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
INT_PTR CALLBACK PePicklistDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
PROEDIT_WINDOW *pe;
RECT rc, rcDlg, rcOwner;
HWND hWndList;
int i,pos;

        hWndList = GetDlgItem(hDlg, IDC_PICKLIST_LIST1);

        pe = (PROEDIT_WINDOW *)GetWindowLong(GetParent(hDlg), 0);

	switch (message)
	{
	case WM_INITDIALOG:
             {
             GetWindowRect(GetParent(hDlg), &rcOwner); 
             GetWindowRect(hDlg, &rcDlg); 
             CopyRect(&rc, &rcOwner); 
 
             // Offset the owner and dialog box rectangles so that 
             // right and bottom values represent the width and 
             // height, and then offset the owner again to discard 
             // space taken up by the dialog box. 
 
             OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
             OffsetRect(&rc, -rc.left, -rc.top); 
             OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 
 
             // The new position is the sum of half the remaining 
             // space and the owner's original position. 
 
             SetWindowPos(hDlg, 
                 HWND_TOP, 
                 MAX(0,rcOwner.left + (rc.right / 2)), 
                 MAX(0,rcOwner.top + (rc.bottom / 2)), 
                 0, 0,          // ignores size arguments 
                 SWP_NOSIZE); 

#if 0
             SendMessage(hWndList, WM_SETFONT, (WPARAM)pe->hFont,(LPARAM)TRUE);
#endif
             for (i=0; i<pe->picklist.count; i++)
                { 
                pos=SendMessage(hWndList, LB_ADDSTRING, 0, 
                    (LPARAM)pe->picklist.list[i]);
 
                SendMessage(hWndList, LB_SETITEMDATA, pos, (LPARAM) i); 
                } 
             SendMessage(hWndList, LB_SETCURSEL, pe->picklist.start-1,0);

             SetWindowText(GetDlgItem(hDlg, IDC_PICKLIST_GROUPBOX), pe->picklist.title);
             SetWindowText(hDlg, pe->picklist.windowTitle);
             SetFocus(hWndList);
             return (INT_PTR)TRUE;
             }

	case WM_COMMAND:
             switch(LOWORD(wParam)) 
                {
                case IDCANCEL:
                   {
                   EndDialog(hDlg, LOWORD(wParam));
                   *pe->picklist.retCode = 0;
                   SetFocus(pe->hWndDisplay);
                   SetEvent(pe->picklist.event);
                   return (INT_PTR)TRUE;
                   }

                case IDOK:
                   {
                   pos=SendMessage(hWndList, LB_GETCURSEL,0,0);
                  
                   if (pos != LB_ERR)
                      {
                      i=SendMessage(hWndList, LB_GETITEMDATA, pos, 0);
                  
                      *pe->picklist.retCode = i+1;
                      }
                   else
                      *pe->picklist.retCode = 0;

                   EndDialog(hDlg, LOWORD(wParam));
                   SetFocus(pe->hWndDisplay);
                   SetEvent(pe->picklist.event);
                   return (INT_PTR)TRUE;
                   }
                }
		break;
	}
	return (INT_PTR)FALSE;
}

