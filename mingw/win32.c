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

#include "..\osdep.h"
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <stdarg.h>

static int    CheckWildcard(char *filename);

char *SessionFilename(int type);

static HANDLE              fHandle = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATA     finfo;  
static char                curPath[MAX_PATH];
static char                shellPath[MAX_PATH];
static void                ProcessChdirRequest(char *cmd);

#define SPECIAL(finfo) \
   (finfo.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN|| \
   finfo.dwFileAttributes&FILE_ATTRIBUTE_SYSTEM|| \
   finfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)

#define DIR3(finfo) \
   (finfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY&& \
   strcmp(finfo.cFileName,"."))

#define DIR(finfo) \
   (!strcmp(finfo.cFileName,".")||!strcmp(finfo.cFileName,".."))

#define SPECIAL2(finfo) \
   (finfo.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN|| \
   finfo.dwFileAttributes&FILE_ATTRIBUTE_SYSTEM)

#define WIN95_DELAY 500

typedef struct win32_OsShell
   {
   BOOL                windows95;
   BOOL                running;
   DWORD               exitcode;
   OSVERSIONINFO       osv;
   SECURITY_ATTRIBUTES sa;
   STARTUPINFO         si;
   PROCESS_INFORMATION pi;
   DWORD               bytesRead;
   DWORD               bytesAvail;
   DWORD               bytesWrote;
   DWORD               timeDetectedDeath;
   SECURITY_DESCRIPTOR sd;
   HANDLE              hPipeWrite;
   HANDLE              hPipeRead;
   HANDLE              hRead2;
   HANDLE              hWriteSubProcess;
   } WIN32_OS_SHELL;

#define MAX_SHELL_LINE_LEN 4096

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_GetBackupPath(char *path, char *timedate)
{
char pathname[MAX_PATH];
static int counter=0;
char string[128];
int hour,min,sec, month,day,year;

   OS_GetSessionFile(pathname, SESSION_BACKUPS, 0);

   mkdir(pathname);

   sprintf(path, "%s\\", pathname);

   _strtime(string);
   sscanf(string, "%02d:%02d:%02d", &hour,&min,&sec);

   _strdate(string);
   sscanf(string, "%02d/%02d/%04d", &month,&day,&year);

   sprintf(timedate, "-%04d%02d%02d-%02d%02d%02d-%d",
      year+2000,
      month,
      day,
      hour,
      min,
      sec, 
      counter);

   counter++;
   return(1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_GetSessionFile(char *filename, int type, int instance)
{
char *homedir=0;
char baseDir[MAX_PATH];
char baseDrive[MAX_PATH];
char newPath[MAX_PATH];
char name[MAX_PATH];

   if (GetEnvironmentVariable("PROEDIT_DIR", baseDir, sizeof(baseDir)-1))
      homedir = baseDir;

   if (!homedir)
      {
      if (GetEnvironmentVariable("HOMEPATH", baseDir, sizeof(baseDir)-1))
         {
         if (baseDir[1] != ':')
            {
            if (GetEnvironmentVariable("HOMEDRIVE", baseDrive, sizeof(baseDrive)-1))
               {
               if (baseDrive[strlen(baseDrive)-1] == '\\' ||
                   baseDir[0] == '\\')
                  sprintf(newPath, "%s%s", baseDrive, baseDir);
               else
                  sprintf(newPath, "%s\\%s", baseDrive, baseDir);
               homedir = newPath;
               }
            }
         else
         homedir = baseDir;
         }
      }

   if (!homedir)
      {
      if (GetWindowsDirectory(baseDir, sizeof(baseDir)-1))
         homedir = baseDir;
      }

   if (!homedir)
      homedir = "c:";


   if (homedir[strlen(homedir)-1] != '\\')
      sprintf(filename, "%s\\proedit", homedir);
   else
      sprintf(filename, "%sproedit", homedir);

   /* Make sure the Proedit directory exists. */
   mkdir(filename);

   if (instance)
      sprintf(name, "%s%d", SessionFilename(type),instance);
   else
      strcpy(name, SessionFilename(type));

   if (homedir[strlen(homedir)-1] != '\\')
      sprintf(filename, "%s\\proedit\\%s", homedir, name);
   else
      sprintf(filename, "%sproedit\\%s", homedir, name);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void *OS_LoadSession(int *len, int type, int instance)
{
FILE_HANDLE *fp;
char *session;
char filename[MAX_PATH];

   OS_GetSessionFile(filename, type, instance);

   if (!strlen(filename))
      return(0);

   fp = OS_Open(filename,"rb");

   if (fp)
      {
      *len = OS_Filesize(fp);
      session = OS_Malloc(*len);
      if (!OS_Read(session, *len, 1, fp))
         {
         OS_Close(fp);
         OS_Free(session);
         return(0);
         }

      OS_Close(fp);
      return(session);
      }
   return(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void  OS_FreeSession(void *session)
{
   OS_Free(session);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_SaveSession(void *session, int len, int type, int instance)
{
FILE_HANDLE *fp;
char filename[MAX_PATH];

   OS_GetSessionFile(filename, type, instance);

   if (!strlen(filename))
      return(0);

   fp = OS_Open(filename,"wb");

   if (fp)
      {
      if (!OS_Write(session, len, 1, fp))
         {
         OS_Close(fp);
         return(0);
         }

      OS_Close(fp);
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
int OS_GetFileInfo(char *path, OS_FILE_INFO *info)
{                             
WIN32_FILE_ATTRIBUTE_DATA data;
SYSTEMTIME systime;

if (GetFileAttributesEx(path, GetFileExInfoStandard, &data))
   {
   info->fileSize = data.nFileSizeLow;

   FileTimeToSystemTime(&data.ftLastWriteTime, &systime);
 
   sprintf(info->asciidate, "%02d/%02d/%04d %02d:%02d",
       systime.wMonth,
       systime.wDay,
       systime.wYear,
       systime.wHour,
       systime.wMinute);

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
void OS_Time(char *string)
{
char ampm[3];

int hour,min,sec;

_strtime(string);
sscanf(string, "%02d:%02d:%02d", &hour,&min,&sec);

if (hour > 12)
   {
   hour-=12;
   strcpy(ampm, "PM");
   }
else
   strcpy(ampm, "AM");
sprintf(string, "%02d:%02d:%02d %s",hour,min,sec,ampm);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_DeallocDirs(char **dirs, int numDirs)
{
int i;

   for (i=0; i<numDirs; i++)
      OS_Free(dirs[i]);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Bell(void)
{
MessageBeep(-1);
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_ReadDirectory(char *path, char *mask, char **dirs, int max, int opt)
{
char curDir[MAX_PATH];
char newDir[MAX_PATH];
char filename[MAX_PATH];
int total=0;
HANDLE handle;
WIN32_FIND_DATA info;  

   GetCurrentDirectory(MAX_PATH-1, curDir);

   /* Did we fail to change to the directory? */
   if (SetCurrentDirectory(path) == FALSE) 
      return(0);

   GetCurrentDirectory(MAX_PATH-1, newDir);

   handle=FindFirstFile(mask, &info); 

   if(handle != INVALID_HANDLE_VALUE)
      {
      while(1)
         {
         if (DIR3(info) && !SPECIAL2(info)) 
            {
            if (total < max)
               {
               if (!(!strcmp(info.cFileName, "..") && (opt & OS_NO_SPECIAL_DIR)))
                  {
                  sprintf(filename, "%s\\%s", newDir, info.cFileName);
                  dirs[total] = OS_Malloc(strlen(filename)+1);
                  strcpy(dirs[total], filename);
                  total++;
                  }
               }
            }
         if (FindNextFile(handle, &info) == FALSE)
            break;
         }
      }

   SetCurrentDirectory(curDir);

   if(handle != INVALID_HANDLE_VALUE)  
      FindClose(handle);  

   return(total);
}                


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_JoinPath(char *pathname, char *dir, char *filename)
{
   sprintf(pathname, "%s\\%s", dir, filename);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_DosFindFirst(char *mask, char *filename) 
{
int retcode=0, wildcard;
char tempPath[MAX_PATH];
char searchName[MAX_PATH];

   strcpy(curPath, "");

   OS_GetFilename(mask, curPath, searchName);

   if (!strlen(searchName))
      {
      strcpy(filename, "");
      return(0);
      } 

   wildcard=CheckWildcard(searchName);

   if (!strlen(curPath))
     OS_GetCurrentDir(curPath);

   /* Fixup and test path.. */
   OS_GetCurrentDir(tempPath);

   if (SetCurrentDirectory(curPath) == FALSE)
      {
      OS_SetCurrentDir(tempPath);
      strcpy(filename, "");
      return(0);
      }

   OS_SetCurrentDir(tempPath);

   fHandle=FindFirstFile(mask, &finfo); 

   if(fHandle != INVALID_HANDLE_VALUE) 
      {
      if (DIR(finfo) || SPECIAL(finfo))
         return(OS_DosFindNext(filename));

      sprintf(filename, "%s%s", curPath, finfo.cFileName);
      retcode=1;
      }
   else
      {
      if (wildcard)
         strcpy(filename, "");
      else
         if (mask[1] != ':')
            sprintf(filename, "%s%s", curPath, mask);
         else
            sprintf(filename, "%s", mask);

      retcode=0;
      }

return(retcode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int CheckWildcard(char *filename) 
{
int i;

   for (i=0; i<(int)strlen(filename); i++)
      if (filename[i] == '?' || filename[i] == '*')
         return(1);

   return(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_DosFindNext(char *filename) 
{
int retcode=0;

   if(fHandle == INVALID_HANDLE_VALUE)   
      {
      strcpy(filename, ""); 
      return(0);
      }

   if (FindNextFile(fHandle, &finfo) == FALSE)
      {
      strcpy(filename, "");
      retcode=0;
      }
   else
      {
      if (DIR(finfo) || SPECIAL(finfo))
         return(OS_DosFindNext(filename));

      retcode=1;
      sprintf(filename, "%s%s", curPath, finfo.cFileName);
      }            

   return(retcode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_DosFindEnd(void)
{
   if(fHandle != INVALID_HANDLE_VALUE)  
      {
      FindClose(fHandle);
      fHandle = INVALID_HANDLE_VALUE;
      }
}



/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void *OS_Malloc (long size)
{
void *ptr;

   ptr = malloc(size);

   if (!ptr)
      {
      OS_Printf("Error allocating %ld bytes.\n", size);
      OS_Exit();
      }

   return(ptr);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Free (void *data)
{
   free(data);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SaveScreen(void)
{     
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void RestoreScreen(void)
{
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void  OS_Close (FILE_HANDLE *fp)
{
fclose((FILE *)fp);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
long OS_Write(void *buffer, long blkSize,long numBlks, FILE_HANDLE *fp)
{
   return(fwrite(buffer,blkSize,numBlks,(FILE*)fp));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
long OS_Read(void *buffer, long blkSize,long numBlks, FILE_HANDLE *fp)
{
   return(fread(buffer,blkSize,numBlks,(FILE*)fp));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
long OS_ReadLine(char *buffer, int len, FILE_HANDLE *fp)
{
   if (fgets(buffer,len,fp))
      {
      len = strlen(buffer);
      while(len && (buffer[len-1] == 10 || buffer[len-1] == 13))
         len--;
      buffer[len]=0;
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
long OS_Filesize(FILE_HANDLE *fp)
{
long size;

   fseek((FILE *)fp, 0, SEEK_END);
   size = ftell((FILE *)fp);
   fseek((FILE *)fp, 0, SEEK_SET);
   return(size);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Delete(char *filename)
{
   remove(filename);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
FILE_HANDLE *OS_Open(char *filename, char *mode)
{
FILE *fp;

   fp = fopen(filename, mode);

   return((FILE_HANDLE*)fp);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_PutByte(char ch, FILE_HANDLE *fp)
{
   fputc(ch, (FILE *)fp);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_GetCurrentDir (char *curDir)
{
   GetCurrentDirectory(MAX_PATH-1, curDir); 

   if (curDir[strlen(curDir)-1] != '\\')
      strcat(curDir, "\\");
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetCurrentDir (char *curDir)
{
   SetCurrentDirectory(curDir); 
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_Strcasecmp(char *str1, char *str2)
{
return(_stricmp(str1,str2));
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
int OS_GetFullPathname(char *filename, char *pathname, int max)
{
char *filepart;

   if (!GetFullPathName(filename, max, pathname, &filepart))
      {
      strcpy(pathname, filename);
      return(0);
      }

   return(1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_PathDepth(char *filename)
{
int i, depth=0;

   for (i=0; i<(int)strlen(filename); i++)
      {
      if (filename[i] == '\\')
         depth++;
      }
   return(depth);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_SingleFile(char *dir)
{
char path[MAX_PATH];
char filename[MAX_PATH];
DWORD type;

   type = GetFileAttributes(dir);

   if (type != INVALID_FILE_ATTRIBUTES)
      {
      if (type & FILE_ATTRIBUTE_DIRECTORY)
         return(0);
     
      if (type & FILE_ATTRIBUTE_NORMAL)
         return(1);
      }

   OS_GetFilename(dir, path, filename);

   if (strlen(path) && strlen(filename))
      if (!CheckWildcard(filename))
         return(1);

   return(0);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_GetFilename(char *pathname, char *path, char *file)
{
   char newpath[MAX_PATH];
   char *ptr;
   DWORD type;

   GetFullPathName(pathname, sizeof(newpath), newpath, &ptr);

   type = GetFileAttributes(newpath);

   if (type != INVALID_FILE_ATTRIBUTES)
      {
      if ((type & FILE_ATTRIBUTE_DIRECTORY) && path && file)
         {
         strcpy(path, newpath);
         if (path[strlen(path)-1] != '\\')
            strcat(path, "\\");
         strcpy(file, "");
         return;
         }
      }

   if (file)
      {
      if (ptr)
         strcpy(file, ptr);
      else
         strcpy(file, "");
      }
   
   if (ptr)
      *ptr = 0;
   
   if (path)
      strcpy(path, newpath);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_FreeClipboard(char *clip)
{
   OS_Free(clip);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
char *OS_GetClipboard(void)
{
HANDLE clipData;
char *data=0, *walk;
int i, len, index=0;

if (OpenClipboard(NULL))
   {
   clipData = GetClipboardData(CF_OEMTEXT);

   walk = (char *)clipData;

   if (walk)
      {
      len = strlen(walk);

      data = OS_Malloc(len+1);

      for (i=0; i<len; i++)
         {
         if (walk[i] == ED_KEY_LF)
            continue;
         data[index++] = walk[i];
         }

      data[index]=0;
      }

   CloseClipboard();
   }

   return(data);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_SetClipboard(char *string, int len)
{
HGLOBAL data;
char *ptr;

   if (OpenClipboard(NULL))
      {
      data = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, len);
      if (data)
         {
         ptr=GlobalLock(data);

         if (ptr)
            {
            memcpy(ptr, string, len);
            GlobalUnlock(data);
            EmptyClipboard();
            SetClipboardData(CF_OEMTEXT, data);
            }
         }
      CloseClipboard();
      }
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_ShellGetLine(OS_SHELL *user)
{
   char ch;
   int bTest;
   int mode;
   WIN32_OS_SHELL *shell;
   OS_MOUSE mouse;

   if (user->lineLen)
      {
      user->lineLen=0;
      OS_Free(user->line);
      }

   user->line = OS_Malloc(MAX_SHELL_LINE_LEN);

   shell = (WIN32_OS_SHELL *)user->shellData;

   shell->timeDetectedDeath = 0;

   while (shell->running) 
      {
      shell->bytesRead  = 0;
      shell->bytesAvail = 0;

      WaitForSingleObject(shell->hPipeRead, 1000);

      /* Try to terminate the process? */
      if (OS_PeekKey(&mode,&mouse) == ED_KEY_ESC && mode==0)
         {
         if (WAIT_OBJECT_0 != WaitForSingleObject(shell->pi.hProcess, 500)) 
            TerminateProcess(shell->pi.hProcess, 1);

         shell->running = FALSE;
         sprintf(user->line, "--- Process has been Terminated. ---");
         user->lineLen = strlen(user->line);
         return(1);
         }

      if (!PeekNamedPipe(shell->hPipeRead, &ch, 1, &shell->bytesRead, 
         &shell->bytesAvail, NULL))
          shell->bytesAvail = 0;

      if (shell->bytesAvail > 0) 
         {
         bTest = ReadFile(shell->hPipeRead, &ch, 1, &shell->bytesRead, NULL);

         if (bTest && shell->bytesRead == 1) 
            {
            if (ch == 10)
               return(1);

            if (user->lineLen < MAX_SHELL_LINE_LEN)
               {
               if (ch >= 32)
                  user->line[user->lineLen++] = ch;
               }
            }
         else
            shell->running = FALSE;
         }
      else
         {
         if (GetExitCodeProcess(shell->pi.hProcess, &shell->exitcode)) 
            {
            if (shell->exitcode != STILL_ACTIVE) 
               {
               if (shell->windows95) 
                  {
                  // Process is dead, but wait a second in case there is some output in transit
                  if (shell->timeDetectedDeath == 0) 
                     {
                     shell->timeDetectedDeath = GetTickCount();
                     } 
                  else
                     {
                     if ((GetTickCount() - shell->timeDetectedDeath) > WIN95_DELAY) 
                        {
                        shell->running = FALSE;    // It's a dead process
                        }
                     }
                  } 
               else
                  { // NT, so dead already
                  shell->running = FALSE;
                  }
               }
            }
         }
      }
   return(user->lineLen);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_ShellClose(OS_SHELL *user)
{
int exitcode;
WIN32_OS_SHELL *shell;

   shell = (WIN32_OS_SHELL *)user->shellData;

   if (shell->running)
      {
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

 OS_Free(shell);
 OS_Free(user->cwd);
 OS_Free(user->shell_cmd);
 OS_Free(user->command);
 OS_Free(user);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
OS_SHELL *OS_ShellCommand(char *cmd)
{
   WIN32_OS_SHELL *shell;
   OS_SHELL *user;
   char command[MAX_PATH];

   ProcessChdirRequest(cmd);

   user = (OS_SHELL*)OS_Malloc(sizeof(OS_SHELL));
   memset(user, 0, sizeof(OS_SHELL));

   shell = (WIN32_OS_SHELL *)OS_Malloc(sizeof(WIN32_OS_SHELL));
   memset(shell, 0, sizeof(WIN32_OS_SHELL));

   user->shellData = shell;

   shell->osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   strcpy(shell->osv.szCSDVersion, "");
   
   shell->sa.nLength = sizeof(SECURITY_ATTRIBUTES);
   shell->si.cb      = sizeof(STARTUPINFO);

   GetVersionEx(&shell->osv);
 
   shell->windows95 = (shell->osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

   shell->sa.bInheritHandle       = TRUE;
   shell->sa.lpSecurityDescriptor = NULL;

   if (!shell->windows95) 
      {
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

   shell->si.wShowWindow = SW_HIDE;

   shell->si.hStdInput  = shell->hRead2;
   shell->si.hStdOutput = shell->hPipeWrite;
   shell->si.hStdError  = shell->hPipeWrite;

   sprintf(command, "cmd /C %s", cmd);

   shell->running = CreateProcess(NULL,
                 command,
                 NULL, 
                 NULL,
                 TRUE,
                 CREATE_NEW_PROCESS_GROUP,
                 NULL,
                 shellPath, // current directory.
                 &shell->si,
                 &shell->pi);

   if (!shell->running)
      {
      OS_ShellClose(user);
      return(0);
      }

   user->command   = OS_Malloc(strlen(cmd)      +1);
   user->cwd       = OS_Malloc(strlen(shellPath)+1);
   user->shell_cmd = OS_Malloc(strlen(command)  +1);

   strcpy(user->shell_cmd , command);
   strcpy(user->command   , cmd);
   strcpy(user->cwd       , shellPath);

   return(user);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_ShellSetPath(char *path)
{
   strcpy(shellPath, path);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_ShellGetPath(char *path)
{
   if (!strlen(shellPath))
     OS_GetCurrentDir(shellPath);

   strcpy(path, shellPath);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ProcessChdirRequest(char *cmd)
{
char temp[MAX_PATH];

   while(*cmd && (*cmd <= 32))
      cmd++;

   if ((cmd[0] == 'c' || cmd[0] == 'C') &&
       (cmd[1] == 'd' || cmd[1] == 'D') &&
       (cmd[2] == 32))
       {
       OS_GetCurrentDir(temp);

       SetCurrentDirectory(shellPath);

       if (SetCurrentDirectory(&cmd[3]))
          OS_GetCurrentDir(shellPath);

       OS_SetCurrentDir(temp);
       }
}

/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
int OS_ValidPath(char *path)
{
DWORD type;

   type = GetFileAttributes(path);

   if (type != INVALID_FILE_ATTRIBUTES && (type & FILE_ATTRIBUTE_DIRECTORY))
         return(1);

   return(0);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
int OS_CreatePath(char *path)
{
int retCode;
char subpath[MAX_PATH];
char newpath[MAX_PATH];

   strcpy(newpath, path);

   if (newpath[strlen(newpath)-1] == '\\')
      newpath[strlen(newpath)-1] = 0;

   retCode =CreateDirectory(newpath, 0);

   if (retCode)
      return(1);

   if (GetLastError() == ERROR_PATH_NOT_FOUND)
      {
      OS_GetFilename(newpath, subpath, 0);
      if (OS_CreatePath(subpath))
         return(OS_CreatePath(newpath));
      }

   return(0);
}

