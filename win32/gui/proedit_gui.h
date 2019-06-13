#ifndef __PROEDIT_GUI_H__
#define __PROEDIT_GUI_H__

#define PE_STATUS_UPDATE   1
#define PE_STATUS_PROMPT   2
#define PE_STATUS_CENTER   3
#define PE_STATUS_INPUT    4
#define PE_STATUS_HISTORY  5
#define PE_STATUS_BROWSE   6

typedef struct proeditPicklist
   {
   char *windowTitle;
   int count;
   int width;
   char *title;
   char **list;
   int start;
   int *retCode;
   HANDLE event;
   } PROEDIT_PICKLIST;

typedef struct proeditInput
   {
   int enabled;
   HWND hWnd;
   HWND hWndStatic;
   HANDLE event;
   int historyIndex;
   char *prompt;
   char *result;
   int max;
   int caretX, caretY;
   int retCode;
   HBRUSH background;
   WNDPROC oldEditWinProc;
   } PROEDIT_INPUT;

typedef struct proeditHistoryWindow
   {
   HWND hWnd;
   } PROEDIT_HISTORY;

typedef struct proeditStatusLine
   {
   int hexMode;
   int line_number;
   int number_lines;
   int column;
   int undos;
   char *buffer;
   int status;
   } PROEDIT_STATUS;

typedef struct inputQueue
   {
   char keyState[256];
   UINT message;
   WPARAM wParam;
   LPARAM lParam;

   struct inputQueue *next;
   struct inputQueue *prev;
   } INPUT_QUEUE;

typedef struct scrollBar_t
   {
   int line;
   int numLines;
   } PE_SCROLL_BAR;

typedef struct proeditWindow_t
   {
   HINSTANCE hInst;								// current instance
   HANDLE event;

   PE_SCROLL_BAR scroll;

   INPUT_QUEUE *head;
   INPUT_QUEUE *tail;

   PROEDIT_INPUT input;

   PROEDIT_STATUS status;

   CRITICAL_SECTION cs;

   PROEDIT_PICKLIST picklist;
   
   PAINTSTRUCT ps;
   int cursor_x;
   int cursor_y;
   int cursor_sizeX;
   int cursor_sizeY;
   int cursor_visible;

   int statusHeight;
   HDC hdc;
   char *buffer;
   int screen_xd;
   int screen_yd;
   HFONT hFont;

   HWND hWndStatus;
   HWND hWndMain;
   HWND hWndDisplay;

   PROEDIT_HISTORY history;

   int lineNumber;
   int numberLines;
   int column;
   int undo;
   } PROEDIT_WINDOW;


void PeAddInputRecord(UINT message, WPARAM wParam, LPARAM lParam, char *keyState);
void PeRegisterStatusClass(HINSTANCE hInstance);

BOOL PeCreateStatusWindow(PROEDIT_WINDOW *pe);
void  PeStatusWindowSize(HWND hWnd, PROEDIT_WINDOW *pe);

void PeRegisterDisplayClass(HINSTANCE hInstance);
BOOL PeCreateDisplayWindow(PROEDIT_WINDOW *pe);

BOOL PeCreateMainWindow(PROEDIT_WINDOW *pe);
void PeRegisterMainClass(HINSTANCE hInstance);

void PeFlushInputQueue(void);
void PeDisableMenu(PROEDIT_WINDOW *pe);
void PeEnableMenu(PROEDIT_WINDOW *pe);

void ProeditPaint(PROEDIT_WINDOW *pe);

#define PE_BASE             (WM_USER)

#define PE_SET_CURSOR_POS   (PE_BASE+0)
#define PE_SET_CURSOR_TYPE  (PE_BASE+1)
#define PE_SET_CURSOR_HIDE  (PE_BASE+2)
#define PE_SET_CURSOR_SHOW  (PE_BASE+3)

#define PE_INPUT            (PE_BASE+4)
#define PE_INPUT_DONE       (PE_BASE+5)
#define PE_INPUT_CANCEL     (PE_BASE+6)
#define PE_PICKLIST         (PE_BASE+9)
#define PE_SHUTDOWN         (PE_BASE+10)

#define PE_EDIT_CONTROL         100

#endif /* __PROEDIT_GUI_H__ */

