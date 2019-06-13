/*###########################################################################*/
/*#                                                                         #*/
/*#              ProEdit MP Multi-platform Programming Editor               #*/
/*#                                                                         #*/
/*#           Designed/Developed/Produced by Adrian J. Michaud              #*/
/*#                                                                         #*/
/*#        (C) 2006-2007 by Adrian J. Michaud. All Rights Reserved.         #*/
/*#                                                                         #*/
/*#      Unpublished - rights reserved under the Copyright Laws of the      #*/
/*#      United States.  Use, duplication, or disclosure by the             #*/
/*#      Government is subject to restrictions as set forth in              #*/
/*#      subparagraph (c)(1)(ii) of the Rights in Technical Data and        #*/
/*#      Computer Software clause at 252.227-7013.                          #*/
/*#                                                                         #*/
/*#      This software contains information of a proprietary nature         #*/
/*#      and is classified confidential.                                    #*/
/*#                                                                         #*/
/*#      ALL INFORMATION CONTAINED HEREIN SHALL BE KEPT IN CONFIDENCE.      #*/
/*#                                                                         #*/
/*###########################################################################*/

#include "resource.h"

#include <windows.h>
#include "..\..\osdep.h" /* Platform dependant interface */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "proedit_gui.h"

#define QUESTION1 1
#define QUESTION2 2
#define QUESTION3 3

#define HISTORY_NEWFILE     0
#define HISTORY_SEARCH      1
#define HISTORY_GOTOLINE    2
#define HISTORY_REPLACE     3
#define HISTORY_GOTOADDRESS 4
#define HISTORY_HEX_SEARCH  5
#define HISTORY_HEX_REPLACE 6
#define HISTORY_HEX_ENTRY   7
#define HISTORY_CALCULATOR  8
#define HISTORY_CB_PATH     9
#define HISTORY_TAB_COUNT   10

extern PROEDIT_WINDOW proeditWindow;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_UpdateStatusBar(int hexMode, int line_number, int number_lines,
                       int column, int undos)
{
   proeditWindow.status.hexMode      = hexMode;
   proeditWindow.status.line_number  = line_number;
   proeditWindow.status.number_lines = number_lines;
   proeditWindow.status.column       = column;
   proeditWindow.status.undos        = undos;
   proeditWindow.status.status       = PE_STATUS_UPDATE;

   InvalidateRect(proeditWindow.hWndStatus, NULL, FALSE);

   return(1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_CenterBottomBar(char *string)
{
   if (proeditWindow.status.buffer)
      free(proeditWindow.status.buffer);

   proeditWindow.status.buffer=_strdup(string);

   proeditWindow.status.status = PE_STATUS_CENTER;

   InvalidateRect(proeditWindow.hWndStatus, NULL, FALSE);

   return(1);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_DisplayBottomBar(char *buffer)
{
   if (proeditWindow.status.buffer)
      free(proeditWindow.status.buffer);

   proeditWindow.status.buffer=_strdup(buffer);

   proeditWindow.status.status = PE_STATUS_PROMPT;

   InvalidateRect(proeditWindow.hWndStatus, NULL, FALSE);

   return(1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_Input(int historyIndex, char *prompt, char *result, int max, 
             int *retCode)
{
   proeditWindow.input.historyIndex = historyIndex;
   proeditWindow.input.prompt       = prompt;
   proeditWindow.input.result       = result;
   proeditWindow.input.max          = max;

   PeDisableMenu(&proeditWindow);

   SendMessage(proeditWindow.hWndStatus, PE_INPUT, (WPARAM)0,(LPARAM)0);
   
   WaitForSingleObject(proeditWindow.input.event, INFINITE);
   
   *retCode = proeditWindow.input.retCode;

   if (*retCode)
      AddToHistory(historyIndex, result);

   PeFlushInputQueue();

   PeEnableMenu(&proeditWindow);

   return(1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_DisplayHelp(void)
{
   return(0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_Question(int id, char *string, char *file, int *ch)
{
char result[32];

   strcpy(result, "");

   proeditWindow.input.historyIndex = -1;
   proeditWindow.input.prompt       = string;
   proeditWindow.input.result       = result;
   proeditWindow.input.max          = 1;

   PeDisableMenu(&proeditWindow);

   SendMessage(proeditWindow.hWndStatus, PE_INPUT, (WPARAM)0,(LPARAM)0);

   WaitForSingleObject(proeditWindow.input.event, INFINITE);

   if (proeditWindow.input.retCode)
      *ch = result[0];
   else
      *ch = 27;

   PeFlushInputQueue();

   PeEnableMenu(&proeditWindow);

   return(1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_PickList(char *windowTitle, int count, int width, char *title, 
                char *list[], int start, int *retCode)
{
char *newTitle, *ptr;

   newTitle=_strdup(title);

   while(ptr=strstr(newTitle, "@")) *ptr=' ';

   proeditWindow.picklist.windowTitle = windowTitle;
   proeditWindow.picklist.count       = count;
   proeditWindow.picklist.width       = width;
   proeditWindow.picklist.list        = list;
   proeditWindow.picklist.start       = start;
   proeditWindow.picklist.retCode     = retCode;

   if (windowTitle == title)
      proeditWindow.picklist.title = "Select";
   else
      proeditWindow.picklist.title = newTitle;

   SendMessage(proeditWindow.hWndDisplay, PE_PICKLIST, (WPARAM)0,(LPARAM)0);
   
   WaitForSingleObject(proeditWindow.picklist.event, INFINITE);

   free(newTitle);

   return(1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_PaintFrame(char *title, int line, int numLines)
{
#if 0
   if (proeditWindow.scroll.numLines != numLines)
      {
      proeditWindow.scroll.numLines = numLines;
      proeditWindow.scroll.line     = line;
      SetScrollRange(proeditWindow.hWndDisplay, SB_VERT, 0, numLines, TRUE);
      SetScrollPos(proeditWindow.hWndDisplay, SB_VERT, line, TRUE);
      }

   if (proeditWindow.scroll.line != line) 
      {
      proeditWindow.scroll.line     = line;
      SetScrollPos(proeditWindow.hWndDisplay, SB_VERT, line, TRUE);
      }
   return(1);
#else
   return(0);
#endif
}

