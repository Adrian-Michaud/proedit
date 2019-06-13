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

#define _WIN32_WINNT 0x0500

#include "..\osdep.h"
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <stdarg.h>

typedef struct exitMessages_t
   {
   char *string;
   struct exitMessages_t *next;
   struct exitMessages_t *prev;
   } EXIT_MESSAGE;

static void   SetupScreenSize(int xd, int yd);
static void   SaveScreen(void);
static void   RestoreScreen(void);
static int    ProcessKey(INPUT_RECORD *in, int *mode);
static int    ProcessMouse(INPUT_RECORD *in, OS_MOUSE *mouse);
static int    ProcessALTKey(int ch);
static int    ProcessCTRLKey(int ch);
static int    CheckWildcard(char *filename);
static void   OptimizeScreen(char *new, char *prev, int *y1, int *y2);
static void   GetSessionFile(char *filename, int type);
static void   AddMessage(EXIT_MESSAGE *msg);
static void   DisplayMessages(void);

static EXIT_MESSAGE *head;
static EXIT_MESSAGE *tail;
static CLOCK_PFN *clock_pfn;

static CONSOLE_CURSOR_INFO savedCursor;

static char          *screenBuffer;
static char          *original;
static unsigned short cursorX;
static unsigned short cursorY;
static int            screenXD;
static int            screenYD;
static int            screen_x;
static int            screen_y;
static int            screen_setup;

extern int nospawn;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
  switch( fdwCtrlType ) 
  { 
    // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
    // CTRL-CLOSE: confirm that the user wants to exit. 
    case CTRL_CLOSE_EVENT: 
    // Pass other signals to the next handler. 
    case CTRL_BREAK_EVENT: 
    case CTRL_LOGOFF_EVENT: 
    case CTRL_SHUTDOWN_EVENT: 
      return TRUE; 
 
    default: 
      return TRUE; 
  } 
} 

#ifdef OS_DAEMONIZE
char cmd_line[4096];
char cmd_item[4096];
#endif

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int OS_LoadScreenPos(int *xp, int *yp)
{
char pathname[MAX_PATH];
FILE_HANDLE *fp;

	*xp = 0;
	*yp = 0;

	OS_GetSessionFile(pathname, 6, 0);

	if (!strlen(pathname))
		return(0);

	fp = OS_Open(pathname,"rb");

	if (fp) {
		OS_Read(xp, sizeof(int), 1, fp);
		OS_Read(yp, sizeof(int), 1, fp);
		OS_Close(fp);
	}
   return(0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int OS_SaveScreenPos(int xp, int yp)
{
char pathname[MAX_PATH];
FILE_HANDLE *fp;

	OS_GetSessionFile(pathname, 6, 0);

	if (!strlen(pathname))
		return(0);

	fp = OS_Open(pathname,"wb");

	if (fp) {
		OS_Write(&xp, sizeof(int), 1, fp);
		OS_Write(&yp, sizeof(int), 1, fp);
		OS_Close(fp);
	}
   return(0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int main(int argc, char **argv)
{
#ifdef OS_DAEMONIZE
int i;
STARTUPINFO si;
PROCESS_INFORMATION pi;

	if (!ProcessCmdLine(argc,argv,1)) {
	   OS_Exit();
	   return(0);
	}

	if (nospawn) {
	   if (EditorInit(argc,argv))
    	  EditorMain(argc,argv);
	   OS_Exit();
	   return(0);
	}

	sprintf(cmd_line, "\"%s\"", argv[0]);
	strcat(cmd_line, " -nospawn");

	for (i=1; i<argc; i++) {
		sprintf(cmd_item, " \"%s\"", argv[i]);
		strcat(cmd_line, cmd_item);
	}

	OS_LoadScreenPos(&screen_x, &screen_y);
	OS_SaveScreenPos(screen_x+20, screen_y+20);

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);

	if (screen_x || screen_y) {
		si.dwFlags = STARTF_USEPOSITION;
		si.dwX = screen_x;
		si.dwY = screen_y;
	}

	memset(&pi, 0, sizeof(pi));


	CreateProcess(NULL,
                 cmd_line,
                 NULL, 
                 NULL,
                 FALSE,
                 CREATE_NEW_PROCESS_GROUP|CREATE_NEW_CONSOLE,
                 NULL,
                 NULL, // current directory.
                 &si,
                 &pi);

#else
	   if (EditorInit(argc,argv))
    	  EditorMain(argc,argv);
#endif

	   OS_Exit();
	   return(0);
}

         
/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void  OS_Printf(char *fmt, ...)
{
EXIT_MESSAGE *msg;

va_list args;
char *buffer;

   buffer = OS_Malloc(1024);
   va_start(args,fmt);
   vsprintf(buffer, fmt, args);
   va_end(args);

   msg = (EXIT_MESSAGE *)OS_Malloc(sizeof(EXIT_MESSAGE));
   msg->next=0;
   msg->prev=0;
   msg->string = OS_Malloc(strlen(buffer)+1);
   strcpy(msg->string, buffer);

   OS_Free(buffer);

   AddMessage(msg);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void AddMessage(EXIT_MESSAGE *msg)
{
   if (!head)
      {
      head=msg;
      tail=msg;
      }
   else
      {
      if (tail)
         tail->next = msg;
 
      msg->prev=tail;
      tail=msg;
      }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DisplayMessages(void)
{
EXIT_MESSAGE *msg;

   msg = head;

   while(msg)
      {
      printf("%s", msg->string);
      msg=msg->next;
      }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetTextPosition(int x_position,int y_position)
{                   
COORD xy;

xy.X=x_position-1;
xy.Y=y_position-1;
SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),xy);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetCursorType(int overstrike, int visible)
{
int update=0;

CONSOLE_CURSOR_INFO cursor;

   GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor);

   if (overstrike)
      {
      if (cursor.dwSize != 100)
         {
         update=1;
         cursor.dwSize=100;
         }
      }
   else
      {
      if (cursor.dwSize != 50)
         {
         update=1;
         cursor.dwSize=50;
         }
      }

   if (visible)
      {
      if (!cursor.bVisible)
         {
         update=1;
         cursor.bVisible = 1;
         }
      }
   else
      {
      if (cursor.bVisible)
         {
         update=1;
         cursor.bVisible = 0;
         }
      }

   if (update)
      SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_PaintScreen(char *buffer)
{
COORD dim,pos;
SMALL_RECT rcl;
int newYPos = 0;
int newBottom = 0;

   dim.X=screenXD;
   dim.Y=screenYD;

   OptimizeScreen(buffer, original, &newYPos, &newBottom);

   pos.X=0;
   pos.Y=newYPos;

   rcl.Left  =0;
   rcl.Top   =newYPos;
   rcl.Right =screenXD-1;
   rcl.Bottom=screenYD-2-newBottom;

   WriteConsoleOutput(GetStdHandle(STD_OUTPUT_HANDLE),
      (PCHAR_INFO)buffer,dim,pos, &rcl);

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OptimizeScreen(char *new, char *prev, int *y1, int *y2)
{
int index,stop,stop2;
unsigned long *cmp1, *cmp2;

   cmp1 = (unsigned long*)new;
   cmp2 = (unsigned long*)prev;
  
   stop = screenXD * (screenYD-2);
   
   for (index=0; index <= stop; index++)
      if (*(cmp1+index) != *(cmp2+index))
         {
         *y1 = index / screenXD;
         break;
         }
   if (index > stop) return;
  
   stop2 = *y1 * screenXD;
  
   for (index= stop-1; index >= stop2; index--)
      if (*(cmp1+index) != *(cmp2+index))
         {
         *y2 = ( (stop-1) - index) / screenXD;
         break;
         }            
  
   memcpy(&prev[*y1 * screenXD*SCR_PTR_SIZE], 
          &new [*y1 * screenXD*SCR_PTR_SIZE], 
          (screenYD-2 - (*y1+*y2) ) * screenXD*SCR_PTR_SIZE);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_PaintStatus(char *buffer)
{
COORD dim,pos;
SMALL_RECT rcl;

   dim.X=screenXD;
   dim.Y=screenYD;
  
   pos.X=0;
   pos.Y=screenYD-1;
  
   rcl.Left  =0;
   rcl.Right =screenXD-1; 
  
   rcl.Top   =screenYD-1;
   rcl.Bottom=screenYD-1;
  
   WriteConsoleOutput(GetStdHandle(STD_OUTPUT_HANDLE),
      (PCHAR_INFO)buffer,dim,pos, &rcl);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_ScreenSize(int *xd, int *yd)
{
CONSOLE_SCREEN_BUFFER_INFO info;


   if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&info))
      return(0);

   *xd = screenXD = info.dwSize.X;
   *yd = screenYD = info.dwSize.Y;

   original = OS_Malloc(screenXD * screenYD * SCR_PTR_SIZE); 
   memset(original, 0, screenXD * screenYD * SCR_PTR_SIZE);

   return(1);
}


#if 0
/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SaveScreen(void)
{     
char *ptr;
COORD dim,pos;
SMALL_RECT rcl;
CONSOLE_SCREEN_BUFFER_INFO info;

   if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&info))
      {
      OS_Printf("ProEdit MP: Failed to acquire screen buffer info\n");
      OS_Exit();
      }

   screenBuffer = OS_Malloc(info.dwSize.X*info.dwSize.Y*2*2);

   dim.X=info.dwSize.X;
   dim.Y=info.dwSize.Y;

   pos.X=0;
   pos.Y=0;

   rcl.Left  =0;
   rcl.Top   =0;
   rcl.Right =dim.X-1;
   rcl.Bottom=dim.Y-1;

   ReadConsoleOutput(GetStdHandle(STD_OUTPUT_HANDLE),
       (PCHAR_INFO)screenBuffer,dim,pos, &rcl);

   cursorX=info.dwCursorPosition.X;
   cursorY=info.dwCursorPosition.Y;

   GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &savedCursor);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void RestoreScreen(void)
{
COORD xy;

   if (screenBuffer)
      {
      xy.X=cursorX;
      xy.Y=cursorY;

      SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),xy);

      OS_PaintScreen(screenBuffer);
      OS_PaintStatus(screenBuffer); 

      SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &savedCursor);
      }
}
#endif

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_Getch(void)
{
   fflush(stdin);
   return(getch());
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_PeekKey(int *mode, OS_MOUSE *mouse)
{
int ch;
INPUT_RECORD in;
DWORD cnt;

   *mode=0;

   if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &in, 1,&cnt) && cnt)
      {
      if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &in, 1,&cnt) && cnt)
         {
         if (in.EventType == KEY_EVENT)
            {
            *mode=0;
            ch = ProcessKey(&in, mode);
            return(ch);
            }

         if (in.EventType == MOUSE_EVENT)
            {
            if (ProcessMouse(&in, mouse))
               {
               *mode=1;
               return(ED_KEY_MOUSE);
               }    
            }
         }
      }

   return(0);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_GetKey(int *mode, OS_MOUSE *mouse)
{
INPUT_RECORD in;
DWORD cnt;
int ch;

   for ( ; ; )
   {
   if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &in, 1,&cnt) && !cnt)
      {
      if (WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), 1000) !=
         WAIT_OBJECT_0)
         {
         if (clock_pfn)
            clock_pfn();
         continue;
         }
      }

   if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &in, 1,&cnt) && cnt)
      {
      if (in.EventType == KEY_EVENT)
         {
         *mode=0;
         ch=ProcessKey(&in, mode);
         if (ch)
           return(ch);
         }

      if (in.EventType == MOUSE_EVENT)
         {
         if (ProcessMouse(&in, mouse))
            {
            *mode=1;
            return(ED_KEY_MOUSE);
            }    
         }
      }
   else
      return(0);
   }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessMouse(INPUT_RECORD *in, OS_MOUSE *mouse)
{

   mouse->wheel        = 0;
   mouse->buttonStatus = 0;
   mouse->moveStatus   = 0;

   if (in->Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
      {
      mouse->moveStatus = 1;
      mouse->xpos       = in->Event.MouseEvent.dwMousePosition.X;
      mouse->ypos       = in->Event.MouseEvent.dwMousePosition.Y;
      return(1);
      }

   if (in->Event.MouseEvent.dwEventFlags == 0)
      {
      mouse->buttonStatus = 1;

      if (in->Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
         mouse->button1 = OS_BUTTON_PRESSED;
      else
         mouse->button1 = OS_BUTTON_RELEASED;

      if (in->Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
         mouse->button2 = OS_BUTTON_PRESSED;
      else
         mouse->button2 = OS_BUTTON_RELEASED;

      if (in->Event.MouseEvent.dwButtonState & 4)
         mouse->button3 = OS_BUTTON_PRESSED;
      else
         mouse->button3 = OS_BUTTON_RELEASED;

      return(1);
      }

   if (in->Event.MouseEvent.dwEventFlags == MOUSE_WHEELED)
      {
      if (in->Event.MouseEvent.dwButtonState & 0xFF000000)
         mouse->wheel = OS_WHEEL_DOWN;
      else
         mouse->wheel = OS_WHEEL_UP;
      return(1);
      }

   return(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessKey(INPUT_RECORD *in, int *mode)
{
int ch;
static int alt=0,ctrl=0;

   if (in->Event.KeyEvent.bKeyDown == 0) /* Key being released */
      {
      if (in->Event.KeyEvent.wVirtualScanCode == 56) 
         alt=0;

      if (in->Event.KeyEvent.wVirtualScanCode == 29)
         ctrl=0;

      if (in->Event.KeyEvent.wVirtualScanCode == 42)
         {
         *mode=1;
         return(ED_KEY_SHIFT_UP);
         }
      }

if (in->Event.KeyEvent.bKeyDown == 1) /* Key being pressed */
   {
   if (in->Event.KeyEvent.wVirtualScanCode == 42)
      {
      *mode=1;
      return(ED_KEY_SHIFT_DOWN);
      }

   if (in->Event.KeyEvent.wVirtualScanCode == 29) ctrl=1;

   if (in->Event.KeyEvent.wVirtualScanCode == 56) alt=1;

   if (in->Event.KeyEvent.uChar.UnicodeChar) /* Special Key */
      {
      ch = in->Event.KeyEvent.uChar.UnicodeChar;

      if (alt)
         {
         *mode=1;
         return(ProcessALTKey(ch));
         }

      if (ctrl)
         {
         *mode=1;
         return(ProcessCTRLKey(ch));
         }

      return(ch);
      }
   *mode=1;
   switch(in->Event.KeyEvent.wVirtualScanCode)
        {
        case 9:
             if (ctrl) return(ED_ALT_TAB);
             break;
        case 72:
            if (alt)  return(ED_KEY_ALT_UP);
            if (ctrl)  return(ED_KEY_CTRL_UP);
            return(ED_KEY_UP);
        case 77:
            if (alt)  return(ED_KEY_ALT_RIGHT);
            if (ctrl)  return(ED_KEY_CTRL_RIGHT);
            return(ED_KEY_RIGHT);
        case 80:
            if (alt)  return(ED_KEY_ALT_DOWN);
            if (ctrl)  return(ED_KEY_CTRL_DOWN);
            return(ED_KEY_DOWN);
        case 75:
            if (alt)  return(ED_KEY_ALT_LEFT);
            if (ctrl)  return(ED_KEY_CTRL_LEFT);
            return(ED_KEY_LEFT);
        case 82:               
            if (alt)  return(ED_KEY_ALT_INSERT);
            if (ctrl) return(ED_KEY_CTRL_INSERT);
            return(ED_KEY_INSERT);
        case 83:
            return(ED_KEY_DELETE);
        case 71:
            return(ED_KEY_HOME);
        case 79:
            return(ED_KEY_END);
        case 73:
            return(ED_KEY_PGUP);
        case 81:
            return(ED_KEY_PGDN);

        case 59:
            return(ED_F1);
        case 60:
            return(ED_F2);
        case 61:
            return(ED_F3);
        case 62:
            if (alt) return(ED_ALT_F4);
            return(ED_F4);

        case 63:
            return(ED_F5);
        case 64:
            return(ED_F6);
        case 65:
            return(ED_F7);
        case 66:

            return(ED_F8);
        case 67:
            return(ED_F9);
        case 68:

            return(ED_F10);
        case 87:
            return(ED_F11);
        case 88:
            return(ED_F12);
        }
   }
return(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessALTKey(int ch)
{
   if (ch >= 'a' && ch <='z') ch-=32;  

   switch(ch)
      {
      case 'A':return(ED_ALT_A); 
      case 'B':return(ED_ALT_B); 
      case 'C':return(ED_ALT_C); 
      case 'D':return(ED_ALT_D); 
      case 'E':return(ED_ALT_E); 
      case 'F':return(ED_ALT_F); 
      case 'G':return(ED_ALT_G); 
      case 'H':return(ED_ALT_H); 
      case 'I':return(ED_ALT_I); 
      case 'J':return(ED_ALT_J); 
      case 'K': return(ED_ALT_K); 
      case 'L': return(ED_ALT_L); 
      case 'M': return(ED_ALT_M); 
      case 'N': return(ED_ALT_N); 
      case 'O': return(ED_ALT_O); 
      case 'P': return(ED_ALT_P); 
      case 'Q': return(ED_ALT_Q); 
      case 'R': return(ED_ALT_R); 
      case 'S': return(ED_ALT_S);  
      case 'T': return(ED_ALT_T); 
      case 'U': return(ED_ALT_U); 
      case 'V': return(ED_ALT_V); 
      case 'W': return(ED_ALT_W); 
      case 'X': return(ED_ALT_X); 
      case 'Y': return(ED_ALT_Y); 
      case 'Z': return(ED_ALT_Z); 
      }

   return(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessCTRLKey(int ch)
{
   if (ch >= 'a' && ch <='z') ch-=32;  

   switch(ch)
      {
      case 1:  return(ED_CTRL_A); 
      case 2:  return(ED_CTRL_B); 
      case 3:  return(ED_CTRL_C); 
      case 4:  return(ED_CTRL_D); 
      case 5:  return(ED_CTRL_E); 
      case 6:  return(ED_CTRL_F); 
      case 7:  return(ED_CTRL_G); 
      case 8:  return(ED_CTRL_H); 
      case 9:  return(ED_CTRL_I); 
      case 10: return(ED_CTRL_J); 
      case 11: return(ED_CTRL_K); 
      case 12: return(ED_CTRL_L); 
      case 13: return(ED_CTRL_M); 
      case 14: return(ED_CTRL_N); 
      case 15: return(ED_CTRL_O); 
      case 16: return(ED_CTRL_P); 
      case 17: return(ED_CTRL_Q); 
      case 18: return(ED_CTRL_R); 
      case 19: return(ED_CTRL_S);  
      case 20: return(ED_CTRL_T); 
      case 21: return(ED_CTRL_U); 
      case 22: return(ED_CTRL_V); 
      case 23: return(ED_CTRL_W); 
      case 24: return(ED_CTRL_X); 
      case 25: return(ED_CTRL_Y); 
      case 26: return(ED_CTRL_Z); 
      }

   return(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Exit()
{
RECT rect;
HWND hwnd;

    if (screen_setup)
    {
       hwnd = GetConsoleWindow();
       if (hwnd) {
          GetWindowRect(hwnd, &rect);
          OS_SaveScreenPos(rect.left, rect.top);
       }
    }
   DisplayMessages();
   exit(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_InitScreen(int xd, int yd)
{
   SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
      ENABLE_WINDOW_INPUT|ENABLE_MOUSE_INPUT);

   SetupScreenSize(xd,yd);

   screen_setup = 1;

   SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

   return(1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetupScreenSize(int xd, int yd)
{
SMALL_RECT rect;
COORD dwSize;
CONSOLE_SCREEN_BUFFER_INFO info;
#if 0
   FreeConsole();

   AllocConsole();
#endif
   rect.Left   = 0;
   rect.Top    = 0;
   rect.Right  = xd-1;
   rect.Bottom = yd-1;

   SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

   dwSize.X=xd;
   dwSize.Y=yd;

   SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), dwSize);

   dwSize = GetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE));

   if (yd >= dwSize.Y)
      yd = dwSize.Y-1;

   if (xd >= dwSize.X)
      xd = dwSize.X-1;

   rect.Left   = 0;
   rect.Top    = 0;
   rect.Right  = xd-1;
   rect.Bottom = yd-1;

   SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

   dwSize.X=xd;
   dwSize.Y=yd;

   SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), dwSize);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetWindowTitle(char *pathname)
{
char filename[MAX_PATH];

   OS_GetFilename(pathname, 0, filename);

   SetConsoleTitle(filename);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char  OS_Frame(int index)
{
static unsigned char titles[] =
 { 0xc9, 0xcd, 0xbb, 0xc8, 0xbc, 0xba, 0xb0, 0xb2, 0x18, 
   0x19, 0xfe, 0xbf, 0x07 };

   return(titles[index]);
}
                                 

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetClockPfn(CLOCK_PFN *clockPfn)
{
   clock_pfn = clockPfn;
}
