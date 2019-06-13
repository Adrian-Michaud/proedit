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

extern int QueryHistoryCount(int index);
extern char *QueryHistory(int index, int number);

#define IDT_TIMER1 1

#define PROEDIT_STATUS_CLASS  _T("ProeditStatusWindow")

INT_PTR CALLBACK HistoryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK PeEditControlSubclass(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ProEditStatusWndProc(HWND, UINT, WPARAM, LPARAM);

static void PeCancelInputControl(HWND hWnd, PROEDIT_WINDOW *pe);
static void PeCreateInputControl(HWND hWnd, PROEDIT_WINDOW *pe);
static void PeProcessInputControl(HWND hWnd, PROEDIT_WINDOW *pe);

static void PePaintStatusUpdate(PROEDIT_WINDOW *pe, HDC hdc, RECT *rect, 
                                int xd, int yd);
static void PePaintStatusCenter(PROEDIT_WINDOW *pe, HDC hdc, RECT *rect,
                                int xd, int yd);
static void PePaintStatusPrompt(PROEDIT_WINDOW *pe, HDC hdc, RECT *rect,
                                int xd, int yd);


static char *historyText[] = {
 "New File History",
 "Search String History",
 "Goto Line History",
 "Replace String History",
 "Goto Address History",
 "Hex Search History",
 "Hex Replace History",
 "",            // Reserved
 "",            // Reserved
 "",            // Reserved
 "",            // Reserved
 "",            // Reserved
 "",            // Reserved
 "",            // Reserved
 "" };


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void  PeStatusWindowSize(HWND hWnd, PROEDIT_WINDOW *pe)
{
HDC hdc;
SIZE size;

   hdc = GetDC(hWnd);
   SelectObject(hdc, pe->hFont);
   GetTextExtentPoint32(hdc, "X", 1, &size);
   ReleaseDC(hWnd, hdc);

   pe->input.caretX = size.cx;
   pe->input.caretY = size.cy;

   pe->statusHeight = size.cy+10;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
BOOL PeCreateStatusWindow(PROEDIT_WINDOW *pe)
{
   pe->hWndStatus=CreateWindowEx(WS_EX_DLGMODALFRAME, 
                PROEDIT_STATUS_CLASS, _T(""), 
                WS_CHILD | WS_VISIBLE,
                0, pe->screen_yd, 
                pe->screen_xd,
                pe->statusHeight,
                pe->hWndMain, 
                0, 
                pe->hInst, 
                0);

   if (!pe->hWndStatus)
      return FALSE;

   SetWindowLong(pe->hWndStatus, 0, (LONG)pe);

   pe->input.background = CreateSolidBrush(RGB(220,220,220));

   return(TRUE);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void PeRegisterStatusClass(HINSTANCE hInstance)
{
        WNDCLASSEX wcex;

        RegisterClassEx(&wcex);

        //Window class for the ProEdit status window
        wcex.cbSize             = sizeof(WNDCLASSEX);                    
        wcex.style              = 0;                                     
        wcex.lpfnWndProc        = ProEditStatusWndProc;                  
        wcex.cbClsExtra         = 0;                                     
        wcex.cbWndExtra         = sizeof(void *);                        
        wcex.hInstance          = hInstance;                             
        wcex.hIcon              = 0;                                     
        wcex.hCursor            = LoadCursor (NULL, IDC_IBEAM);          
        wcex.hbrBackground      = CreateSolidBrush(RGB(220,220,220));    
        wcex.lpszMenuName       = 0;                                     
        wcex.lpszClassName      = PROEDIT_STATUS_CLASS;                  
        wcex.hIconSm            = 0;                                     

        RegisterClassEx(&wcex);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
LRESULT CALLBACK PeEditControlSubclass(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
PROEDIT_WINDOW *pe;
int retCode,len;

    pe = (PROEDIT_WINDOW *)GetWindowLong(GetParent(hWnd), 0);

   switch(message)
      {
      case WM_KEYDOWN:
         {
         if (wParam == VK_INSERT)
            {
            SendMessage(hWnd, WM_PASTE,0,0);
            return(0);
            }

         if (wParam == VK_TAB)
            {
            if ((pe->input.historyIndex == 0) && 
                (pe->status.status == PE_STATUS_INPUT))
               {
               memset(pe->input.result, 0, pe->input.max);
               *(int *)pe->input.result = pe->input.max; 

               len=SendMessage(hWnd, EM_GETLINE, 0, (LPARAM)pe->input.result);
               pe->input.result[len]=0;

               pe->status.status = PE_STATUS_BROWSE;

               retCode=QueryFiles(pe->input.result);

               SendMessage(hWnd, WM_SETTEXT,0,(LPARAM)pe->input.result);
               SendMessage(hWnd, EM_SETSEL,(WPARAM)strlen(pe->input.result),
                         (LPARAM)strlen(pe->input.result));

               pe->status.status = PE_STATUS_INPUT;

               if (retCode)
                  SendMessage(GetParent(hWnd), PE_INPUT_DONE, (WPARAM)0,(LPARAM)0);
               else
                  SetFocus(pe->input.hWnd);
               return(1);
               }
             }
              
                     
         if (wParam == VK_UP)
            {
            if ((pe->input.historyIndex != -1) && 
                (pe->status.status == PE_STATUS_INPUT))
               {
               if (QueryHistoryCount(pe->input.historyIndex))
                  {
                  pe->status.status = PE_STATUS_HISTORY;
                  DialogBox(pe->hInst, MAKEINTRESOURCE(IDD_HISTORY),
                       hWnd, HistoryDlgProc);
                  }
               }
            return(0);
            }
         }

      case WM_CHAR:
         {
         if (wParam == VK_TAB && pe->input.historyIndex == 0)
            return(0);

         if (wParam == VK_RETURN)
            SendMessage(GetParent(hWnd), PE_INPUT_DONE, (WPARAM)0,(LPARAM)0);

         if (wParam == VK_ESCAPE)
            SendMessage(GetParent(hWnd), PE_INPUT_CANCEL, (WPARAM)0,(LPARAM)0);

         break;
         }
      }

   return(pe->input.oldEditWinProc(hWnd,message,wParam,lParam));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
LRESULT CALLBACK ProEditStatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
HDC hdc;
PAINTSTRUCT ps;
PROEDIT_WINDOW *pe;
RECT rect;
int xd,yd;

        pe = (PROEDIT_WINDOW *)GetWindowLong(hWnd, 0);

        switch(message)
        {
        case WM_NCCREATE:
                SetTimer(hWnd, IDT_TIMER1, 1000, NULL);
                return TRUE;

        case WM_NCDESTROY:
                KillTimer(hWnd, IDT_TIMER1);
                return 0;

        case WM_TIMER:
               switch(wParam)
                  {
                  case IDT_TIMER1:
                       if (pe->status.status == PE_STATUS_UPDATE)
                          InvalidateRect(hWnd, NULL, FALSE);
                       break;
                  }
               return(0);

        case WM_CTLCOLOREDIT:
             if (lParam == (LPARAM)pe->input.hWnd)
                {
                SetTextColor((HDC)wParam, RGB(0,0,0));
                SetBkColor((HDC)wParam, RGB(220,220,220));
                return((LRESULT)pe->input.background);
                }
              break;

        case PE_INPUT:
             {
             pe->status.status = PE_STATUS_INPUT;

             PeCreateInputControl(hWnd, pe);
             return(0);
             }

        case PE_INPUT_CANCEL:
             {
             PeCancelInputControl(hWnd, pe);
             return(0);
             }

        case PE_INPUT_DONE:
             {
             PeProcessInputControl(hWnd, pe);
             return(0);
             }

        case WM_COMMAND:
             {
             if ((LOWORD(wParam) == PE_EDIT_CONTROL) && 
                (HIWORD(wParam) == EN_KILLFOCUS))
                   {
                   if (pe->status.status == PE_STATUS_INPUT)
                      SendMessage(hWnd, PE_INPUT_CANCEL, (WPARAM)0,(LPARAM)0);
                   }

             if ((LOWORD(wParam) == PE_EDIT_CONTROL) &&
                (HIWORD(wParam) == EN_UPDATE))
                   {
                   if (pe->status.status == PE_STATUS_INPUT)
                      {
                      if (pe->input.max == 1)
                         SendMessage(hWnd, PE_INPUT_DONE, (WPARAM)0,(LPARAM)0);
                      }
                   }
             break;
             }

        case WM_PAINT:
               hdc = BeginPaint(hWnd, &ps);

               SetTextColor(hdc, RGB(0,0,0));
               SetBkColor(hdc, RGB(220,220,220));

               SelectObject(hdc, pe->hFont);
               GetClientRect(hWnd, &rect);

               yd = (rect.bottom-rect.top)+1;
               xd = (rect.right-rect.left)+1;

               switch(pe->status.status)
                  {
                  case PE_STATUS_UPDATE:
                     PePaintStatusUpdate(pe, hdc, &rect, xd, yd);
                     break;

                  case PE_STATUS_CENTER:
                     PePaintStatusCenter(pe, hdc, &rect, xd, yd);
                     break;

                  case PE_STATUS_PROMPT:
                     PePaintStatusPrompt(pe, hdc, &rect, xd, yd);
                  }

                      EndPaint(hWnd, &ps);
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
static void PePaintStatusUpdate(PROEDIT_WINDOW *pe, HDC hdc, RECT *rect, 
                              int xd, int yd)
{
char lineText[50];
char columnText[50];
char undoText[50];
char timeText[50];
SIZE size;
int i,offset;
char *strings[5];

   strings[0] = "ProEdit MP 2.0";
   strings[1] = lineText;
   strings[2] = columnText;
   strings[3] = undoText;
   strings[4] = timeText;
   
   if (pe->status.hexMode)
      {
      sprintf(lineText, "Address: 0x%x", pe->status.line_number);
      sprintf(columnText, "Size: 0x%x", pe->status.number_lines);
      }
   else
      {
      sprintf(lineText, "Line: %d/%d", pe->status.line_number+1,
         pe->status.number_lines);
      sprintf(columnText, "Col: %d", pe->status.column+1);
      }
   
   sprintf(undoText, "Undo: %d", pe->status.undos);
   
   OS_Time(timeText);
   
   
   for (i=0; i<5; i++)
      {
      GetTextExtentPoint32(hdc, strings[i], strlen(strings[i]), &size);
   
      rect->left  = i*xd/5;
      rect->right = ((i+1)*xd/5);
      offset = (xd/5)/2-size.cx/2;
   
      ExtTextOut(hdc,rect->left+offset, yd/2-size.cy/2, 
         ETO_OPAQUE, rect, strings[i], strlen(strings[i]), NULL);
      }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void PePaintStatusCenter(PROEDIT_WINDOW *pe, HDC hdc, RECT *rect, 
                              int xd, int yd)
{
SIZE size;

   if (pe->status.buffer)
      {
      GetTextExtentPoint32(hdc, pe->status.buffer, 
         strlen(pe->status.buffer), &size);
   
      ExtTextOut(hdc,xd/2-size.cx/2, yd/2-size.cy/2, 
         ETO_OPAQUE, rect, pe->status.buffer, 
            strlen(pe->status.buffer), NULL);
      }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void PePaintStatusPrompt(PROEDIT_WINDOW *pe, HDC hdc, RECT *rect, 
                              int xd, int yd)
{
SIZE size;

   if (pe->status.buffer)
      {
      GetTextExtentPoint32(hdc, pe->status.buffer, 
         strlen(pe->status.buffer), &size);
  
      ExtTextOut(hdc,0, yd/2-size.cy/2, 
         ETO_OPAQUE, rect, pe->status.buffer, strlen(pe->status.buffer), NULL);
      }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void PeProcessInputControl(HWND hWnd, PROEDIT_WINDOW *pe)
{
   int len;
   if (pe->input.enabled)
      {
      pe->input.enabled=0;

      memset(pe->input.result, 0, pe->input.max);

      *(int *)pe->input.result = pe->input.max; 

      len=SendMessage(pe->input.hWnd, EM_GETLINE,
         0, (LPARAM)pe->input.result);

      pe->input.result[len]=0;
      pe->input.retCode=1;
      SetFocus(pe->hWndDisplay);
      DestroyWindow(pe->input.hWnd);
      DestroyWindow(pe->input.hWndStatic);
      SetEvent(pe->input.event);
      }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void PeCancelInputControl(HWND hWnd, PROEDIT_WINDOW *pe)
{
    if (pe->input.enabled)
       {
       pe->input.enabled=0;
       pe->input.result[0]=0;
       pe->input.retCode=0;
       SetFocus(pe->hWndDisplay);
       DestroyWindow(pe->input.hWnd);
       DestroyWindow(pe->input.hWndStatic);
       SetEvent(pe->input.event);
       }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void PeCreateInputControl(HWND hWnd, PROEDIT_WINDOW *pe)
{
HDC hdc;
SIZE size;
RECT rect;
int xd,yd;

   GetClientRect(hWnd, &rect);

   yd = (rect.bottom-rect.top)+1;
   xd = (rect.right-rect.left)+1;

   hdc = GetDC(hWnd);
   SelectObject(hdc, pe->hFont);
   GetTextExtentPoint32(hdc, pe->input.prompt, strlen(pe->input.prompt), &size);
   ReleaseDC(hWnd, hdc);
 
   pe->input.hWndStatic=CreateWindow("STATIC", NULL, SS_SUNKEN|SS_CENTER|WS_VISIBLE|WS_CHILD,
      0, 0, size.cx + 12, pe->statusHeight, hWnd, (HMENU)0, 
      pe->hInst, 0);

   SendMessage(pe->input.hWndStatic, WM_SETFONT, (WPARAM)pe->hFont,(LPARAM)TRUE);
   SendMessage(pe->input.hWndStatic, WM_SETTEXT,0,(LPARAM)pe->input.prompt);

   pe->input.hWnd=CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, 
      ES_AUTOHSCROLL|ES_LEFT|WS_VISIBLE|WS_CHILD,
      size.cx+12, 0, xd-(size.cx+12), pe->statusHeight, hWnd, (HMENU)PE_EDIT_CONTROL, 
      pe->hInst, 0);

   SendMessage(pe->input.hWnd, WM_SETTEXT,0,(LPARAM)pe->input.result);

   SendMessage(pe->input.hWnd, EM_SETTABSTOPS, 1, 16);
   SendMessage(pe->input.hWnd, EM_SETLIMITTEXT,(WPARAM)pe->input.max, 0);
   
   SendMessage(pe->input.hWnd, EM_SETSEL, (WPARAM)strlen(pe->input.result),
      (LPARAM)strlen(pe->input.result));
   SendMessage(pe->input.hWnd, WM_SETFONT, (WPARAM)pe->hFont,(LPARAM)TRUE);

   SetFocus(pe->input.hWnd);

   pe->input.oldEditWinProc = (WNDPROC)SetWindowLongPtr(pe->input.hWnd, GWLP_WNDPROC, 
        (LONG_PTR)PeEditControlSubclass);

   pe->input.enabled=1;
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
INT_PTR CALLBACK HistoryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
PROEDIT_WINDOW *pe;
int len, i, pos;
HWND hWndList;
RECT rc, rcDlg, rcOwner;

        pe = (PROEDIT_WINDOW *)GetWindowLong(GetParent(hDlg), 0);

        hWndList = GetDlgItem(hDlg, IDC_HISTORY_LIST1);

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
                 rcOwner.left + (rc.right / 2), 
                 rcOwner.top + (rc.bottom / 2), 
                 0, 0,          // ignores size arguments 
                 SWP_NOSIZE); 

#if 0
             SendMessage(hWndList, WM_SETFONT, (WPARAM)pe->hFont,(LPARAM)TRUE);
#endif
 
             len =  QueryHistoryCount(pe->input.historyIndex);

             for (i=0; i<len; i++)
                { 
                pos=SendMessage(hWndList, LB_ADDSTRING, 0, 
                    (LPARAM)QueryHistory(pe->input.historyIndex,i)); 
                SendMessage(hWndList, LB_SETITEMDATA, pos, (LPARAM) i); 
                } 

             SendMessage(hWndList, LB_SETCURSEL, len-1,0);
             SetWindowText(hDlg, historyText[pe->input.historyIndex]);
             SetFocus(hWndList); 

             return (INT_PTR)TRUE;
             }

        case WM_COMMAND:
             {
             switch(LOWORD(wParam)) 
                {
                case IDCANCEL:
                   {
                   EndDialog(hDlg, LOWORD(wParam));
                   pe->status.status = PE_STATUS_INPUT;
                   SetFocus(pe->input.hWnd);
                   return (INT_PTR)TRUE;
                   }

                case IDOK:
                   {
                   pos=SendMessage(hWndList, LB_GETCURSEL,0,0);
                  
                   if (pos != LB_ERR)
                      {
                      i=SendMessage(hWndList, LB_GETITEMDATA, pos, 0);
                  
                      SendMessage(pe->input.hWnd, WM_SETTEXT,0,
                         (LPARAM)QueryHistory(pe->input.historyIndex,i));
                  
                      SendMessage(pe->input.hWnd, EM_SETSEL, 
                         (WPARAM)strlen(QueryHistory(pe->input.historyIndex,i)),
                         (LPARAM)strlen(QueryHistory(pe->input.historyIndex,i)));
                      }

                   EndDialog(hDlg, LOWORD(wParam));
                   pe->status.status = PE_STATUS_INPUT;
                   SetFocus(pe->input.hWnd);
                   return (INT_PTR)TRUE;
                   }
                }
             }
        }
        return (INT_PTR)FALSE;
}
