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

#define _GNU_SOURCE
#include <sys/types.h>
#include "../osdep.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>
#include <fnmatch.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include "xinit.h"
#include "../types.h"
#include "../proedit.h"

extern int nospawn;

#define CURSOR_FLASH_PER_SEC 3

typedef struct exitMessages_t
{
	char*string;
	struct exitMessages_t*next;
	struct exitMessages_t*prev;
}EXIT_MESSAGE;

extern void InitX11Clipboard(void);
extern void SetupClipboard(XSelectionRequestEvent*reqevent);

static int GetCursorBlink(void);
static void SetCursorBlink(int blink);
static void BlinkCursor(void);

static int screenXD, screenYD;
static void OS_Paint_Line(char*buffer, int line);
static int ProcessEvent(int*mode, XEvent*ev, OS_MOUSE*mouse);
static int WildcardMatch(char*mask, char*filename);
static void DrawFontString(int x, int y, char*str, int len);
static int CheckWildcard(char*filename);
static void DisplayMessages(void);
static void AddMessage(EXIT_MESSAGE*msg);
static int X11ErrorHandler(Display*display, XErrorEvent*error);
static int XNextEventTimeout(Display*display, XEvent*ev, struct timeval*tm);
static int ValidFile(char*pathname);

static int OS_SaveScreenPos(int xp, int yp);
static int OS_LoadScreenPos(int*xp, int*yp);

static EXIT_MESSAGE*head;
static EXIT_MESSAGE*tail;
static CLOCK_PFN*clock_pfn;

static int OptimizeScreen(char*new, char*prev, int*y1, int*y2);
static char*original;
static int lastError;

static void DrawSpecialChar(char ch, int x, int y);

#define UP_ARROW   0x10
#define DOWN_ARROW 0x14
#define MAX_PATH   512

#define DEFAULT_CURSOR_HEIGHT 4

static int cursor_visible = 0;
static int cursor_enabled = 1;
static int cursor_height = DEFAULT_CURSOR_HEIGHT;

static char line_text[1024];
static char*old_buffer = 0;

#define FONT_STRING "-misc-fixed-bold-r-normal--15-120-100-100-c-90-iso8859-1"

static int cursor_x, cursor_y;
static int screen_xd = 0;
static int screen_yd = 0;

static int screen_x = 0;
static int screen_y = 0;
static int need_redraw = 0;
static void show_cursor(void);
static void hide_cursor(void);

static int*OS_Build_Palette(void);

static void change_font_window(char*fontName);

static void OnBreak(int Arg);

static int COLS, LINES;
static int font_size_y, font_size_x, font_descent;

static int*MyPalette;
static char shellPath[MAX_PATH];

XWindow*xw = (XWindow*)0;

static void OnCtrlC(int Arg);


static int ProcessALTKey(int ch);
static int ProcessCTRLKey(int ch);
static int ProcessSHIFTKey(int ch);

#define FG_COLOR(color) ((color) & 0x0f)
#define BG_COLOR(color) (((color) & 0xf0) >> 4)

#define BACKGROUND_COLOR BG_COLOR(COLOR1)
#define FOREGROUND_COLOR FG_COLOR(COLOR1)


static DIR*fHandle;
static char curPath[MAX_PATH];
static char findFileMask[MAX_PATH];

typedef struct fileInfoStruct
{
	char cFileName[MAX_PATH];
	int dwFileAttributes;
}FINFO;

static FINFO finfo;

#define FILE_ATTRIBUTE_HIDDEN       1
#define FILE_ATTRIBUTE_SYSTEM       2
#define FILE_ATTRIBUTE_DIRECTORY    4
#define FILE_ATTRIBUTE_SPECIAL      8
#define FILE_ATTRIBUTE_NORMAL       16

#define ISDIR(finfo)          (!strcmp(finfo.cFileName,".")||!strcmp(finfo.cFileName,".."))
#define ISFILE(finfo)         (finfo.dwFileAttributes&FILE_ATTRIBUTE_NORMAL)
#define SPECIAL(finfo)  (finfo.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN|| finfo.dwFileAttributes&FILE_ATTRIBUTE_SYSTEM|| finfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY|| finfo.dwFileAttributes&FILE_ATTRIBUTE_SPECIAL)
#define DIR2(finfo) (finfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY&&strcmp(finfo.cFileName,".")&&strcmp(finfo.cFileName,".."))
#define SPECIAL2(finfo) (finfo.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN||finfo.dwFileAttributes&FILE_ATTRIBUTE_SYSTEM)

static DIR*UnixFindFirstFile(char*mask, FINFO*fileInfo);
static int UnixFindNextFile(DIR*dirp, char*mask, FINFO*fileInfo);

static void GetFontSize(int*xd, int*yd, char*font_name);

typedef struct unix_OsShell
{
	int fh, fd, fifo;
	int pid;
	int stopped;
	char tempFilename[MAX_PATH];
}UNIX_OS_SHELL;

#define MAX_SHELL_LINE_LEN 4096

#ifdef OS_DAEMONIZE
static void OS_Daemonize(void)
{
	umask(0);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	fcloseall();

	if (fork())
		exit(0);

	//	chdir("/");

	setpgrp();

	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);

	errno = 0;
}
#endif

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int main(int argc, char**argv)
{
	signal(SIGINT, OnCtrlC);

	#ifdef OS_DAEMONIZE

	if (!ProcessCmdLine(argc, argv, 1)) {
		OS_Exit();
		return (0);
	}

	if (!nospawn)
		OS_Daemonize();
	#endif

	if (EditorInit(argc, argv))
		EditorMain(argc, argv);

	OS_Exit();
	return (0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int OS_LoadScreenPos(int*xp, int*yp)
{
	char pathname[MAX_PATH];
	FILE_HANDLE*fp;

	*xp = 0;
	*yp = 0;

	OS_GetSessionFile(pathname, SESSION_SCREEN, 0);

	if (!strlen(pathname))
		return (0);

	fp = OS_Open(pathname, "rb");

	if (fp) {
		OS_Read(xp, sizeof(int), 1, fp);
		OS_Read(yp, sizeof(int), 1, fp);
		OS_Close(fp);
	}
	return (0);
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
	FILE_HANDLE*fp;

	OS_GetSessionFile(pathname, SESSION_SCREEN, 0);

	if (!strlen(pathname))
		return (0);

	fp = OS_Open(pathname, "wb");

	if (fp) {
		OS_Write(&xp, sizeof(int), 1, fp);
		OS_Write(&yp, sizeof(int), 1, fp);
		OS_Close(fp);
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_GetBackupPath(char*path, char*timedate)
{
	char pathname[MAX_PATH];
	time_t tmer;
	struct tm*tm;
	static int counter = 0;


	OS_GetSessionFile(pathname, SESSION_BACKUPS, 0);

	if (strlen(pathname)) {
		mkdir(pathname, S_IRUSR | S_IXUSR | S_IWUSR);

		if (access(pathname, R_OK) == 0) {
			tmer = time(0);
			tm = localtime(&tmer);

			sprintf(path, "%s/", pathname);

			if (tm)
				sprintf(timedate, "-%04d%02d%02d-%02d%02d%02d-%d",
				    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
				    tm->tm_hour, tm->tm_min, tm->tm_sec, counter);
			else
				sprintf(timedate, "-%d", counter);

			counter++;
			return (1);
		}
	}

	return (0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_InitScreen(int xd, int yd)
{
	int err;

	XSetWindowAttributes xsw;

	GetFontSize(&font_size_x, &font_size_y, FONT_STRING);

	screen_xd = xd*font_size_x;
	screen_yd = yd*font_size_y;

	COLS = xd;
	LINES = yd;

	MyPalette = OS_Build_Palette();

	/* Opening a window with a palette and no virtual screen */
	xw = (XWindow*)OS_Malloc(sizeof(XWindow));

	if (!xw)
		return (0);

	memset(xw, 0, sizeof(XWindow));

	OS_LoadScreenPos(&screen_x, &screen_y);
	OS_SaveScreenPos(screen_x + 20, screen_y + 20);

	if ((err = OpenXWindow(xw, screen_x, screen_y, screen_xd, screen_yd, -1,
	    "ProEdit MP 2.0", MyPalette, FALSE, TRUE, TRUE, 0))) {
		OS_Free(xw);
		xw = 0;
		OS_Printf(
		    "\nProEdit MP: Unable to open an X window. err=%d, Terminating\n",
		    err);
		return (0);
	}

	/* Setting specifics properties for a window */
	xsw.border_pixel = WhitePixel(xw->display, xw->screennum);
	xsw.background_pixel = BlackPixel(xw->display, xw->screennum);
	xsw.win_gravity = SouthEastGravity;
	XChangeWindowAttributes(xw->display, xw->window,
	    CWBackPixel | CWBorderPixel | CWWinGravity, &xsw);
	XClearWindow(xw->display, xw->window);

	/* Disable the WM_DELETE_WINDOW protocol. We don't want to be shutdown */
	xw->protocols = XInternAtom(xw->display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(xw->display, xw->window, &xw->protocols, True);

	/* draw_content(xw); */

	XSetForeground(xw->display, xw->gc, xw->palette[FOREGROUND_COLOR]);
	XSetBackground(xw->display, xw->gc, xw->palette[BACKGROUND_COLOR]);

	change_font_window(FONT_STRING);

	InitX11Clipboard();

	/* Clean exiting on severals breaks */
	signal(SIGHUP, OnBreak);
	signal(SIGINT, OnCtrlC);
	signal(SIGQUIT, OnBreak);
	signal(SIGTERM, OnBreak);
	signal(SIGTSTP, OnBreak);

	XSetErrorHandler(X11ErrorHandler);

	return (1);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_GetSessionFile(char*filename, int type, int instance)
{
	char*homedir;

	homedir = getenv("HOME");

	if (!homedir)
		homedir = getenv("PROEDIT_DIR");

	if (homedir) {
		sprintf(filename, "%s/proedit", homedir);

		mkdir(filename, S_IRUSR | S_IXUSR | S_IWUSR);

		if (instance)
			sprintf(filename, "%s/proedit/%s%d", homedir, SessionFilename(type),
			    instance);
		else
			sprintf(filename, "%s/proedit/%s", homedir, SessionFilename(type));
	} else
		strcpy(filename, "");
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetWindowTitle(char*pathname)
{
	char filename[MAX_PATH];

	OS_GetFilename(pathname, 0, filename);

	XStoreName(xw->display, xw->window, filename);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void*OS_LoadSession(int*len, int type, int instance)
{
	FILE_HANDLE*fp;
	char*session;
	char filename[MAX_PATH];

	OS_GetSessionFile(filename, type, instance);

	if (!strlen(filename))
		return (0);

	fp = OS_Open(filename, "rb");

	if (fp) {
		*len = OS_Filesize(fp);
		session = OS_Malloc(*len);
		if (!OS_Read(session, *len, 1, fp)) {
			OS_Close(fp);
			OS_Free(session);
			return (0);
		}

		OS_Close(fp);
		return (session);
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_FreeSession(void*session)
{
	OS_Free(session);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_SaveSession(void*session, int len, int type, int instance)
{
	FILE_HANDLE*fp;
	char filename[MAX_PATH];

	OS_GetSessionFile(filename, type, instance);

	if (!strlen(filename))
		return (0);

	fp = OS_Open(filename, "wb");

	if (fp) {
		if (!OS_Write(session, len, 1, fp)) {
			OS_Close(fp);
			return (0);
		}

		OS_Close(fp);
		return (1);
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetTextPosition(int x_position, int y_position)
{
	if (cursor_visible)
		hide_cursor();

	cursor_x = x_position;
	cursor_y = y_position;

	show_cursor();
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Time(char*string)
{
	time_t tmer;
	char*ptr;
	char ampm[3];
	int hour, min, sec;

	tmer = time(0);
	ptr = ctime(&tmer);

	if (ptr)
		sscanf(&ptr[11], "%02d:%02d:%02d", &hour, &min, &sec);
	else {
		hour = 0;
		min = 0;
		sec = 0;
	}

	if (hour > 12) {
		hour -= 12;
		strcpy(ampm, "PM");
	} else
		strcpy(ampm, "AM");
	sprintf(string, "%02d:%02d:%02d %s", hour, min, sec, ampm);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_GetFileInfo(char*path, OS_FILE_INFO*info)
{
	struct stat filestat;
	char*ptr;

	if (!lstat(path, &filestat)) {
		if (S_ISDIR(filestat.st_mode))
			info->file_type = FILE_TYPE_DIR;
		else
			if (S_ISREG(filestat.st_mode))
				info->file_type = FILE_TYPE_FILE;
			else
				return (0);

		info->fileSize = filestat.st_size;

		ptr = ctime(&filestat.st_ctime);

		if (ptr)
			strcpy(info->asciidate, ptr);
		else
			strcpy(info->asciidate, "");

		if (strlen(info->asciidate))
			info->asciidate[strlen(info->asciidate) - 1] = 0;

		return (1);
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_DeallocDirs(char**dirs, int numDirs)
{
	int i;

	for (i = 0; i < numDirs; i++)
		OS_Free(dirs[i]);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_JoinPath(char*pathname, char*dir, char*filename)
{
	if (filename[0] == '/')
		sprintf(pathname, "%s%s", dir, filename);
	else
		sprintf(pathname, "%s/%s", dir, filename);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_ReadDirectory(char*path, char*mask, char**dirs, int max, int opt)
{
	char curDir[MAX_PATH];
	char newDir[MAX_PATH];
	char filename[MAX_PATH];
	int total = 0;
	DIR*handle;
	struct dirent*dp;
	struct stat filestat;

	getcwd(curDir, MAX_PATH - 1);

	/* Did we fail to change to the directory? */
	if (chdir(path))
		return (0);

	getcwd(newDir, MAX_PATH - 1);

	handle = opendir(newDir);

	if (handle) {
		for (; ; ) {
			dp = readdir(handle);

			if (!dp)
				break;

			if (!lstat(dp->d_name, &filestat)) {
				if (ValidFile(dp->d_name) && (S_ISDIR(filestat.st_mode))) {
					if (total < max) {
						if (strcmp(dp->d_name, ".")) {
							if (WildcardMatch(mask, dp->d_name)) {
								if (!(!strcmp(dp->d_name, "..") && (opt&
								    OS_NO_SPECIAL_DIR))) {
									if (strlen(newDir) &&
									    newDir[strlen(newDir) - 1] == '/')
										sprintf(filename, "%s%s", newDir, dp->
										    d_name);
									else
										sprintf(filename, "%s/%s", newDir, dp->
										    d_name);

									dirs[total] = OS_Malloc(strlen(filename) +
									    1);
									strcpy(dirs[total], filename);
									total++;
								}
							}
						}
					}
				}
			}
		}
	}

	chdir(curDir);

	if (handle)
		closedir(handle);

	return (total);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_DosFindFirst(char*mask, char*filename)
{
	int retcode = 0, wildcard;
	char tempPath[MAX_PATH];

	strcpy(curPath, "");
	strcpy(findFileMask, "");

	OS_GetFilename(mask, curPath, findFileMask);

	if (!strlen(findFileMask)) {
		strcpy(filename, "");
		return (0);
	}

	if (!strlen(curPath))
		OS_GetCurrentDir(curPath);

	/* Fixup and test path.. */
	OS_GetCurrentDir(tempPath);

	if (chdir(curPath)) {
		OS_SetCurrentDir(tempPath);
		strcpy(filename, "");
		return (0);
	}

	OS_GetCurrentDir(curPath);

	OS_SetCurrentDir(tempPath);

	wildcard = CheckWildcard(findFileMask);

	fHandle = UnixFindFirstFile(findFileMask, &finfo);

	if (fHandle) {
		//	  if (ISDIR(finfo) || SPECIAL(finfo))
		if (!ISFILE(finfo))
			return (OS_DosFindNext(filename));

		sprintf(filename, "%s%s", curPath, finfo.cFileName);

		retcode = 1;
	} else {
		if (wildcard)
			strcpy(filename, "");
		else
			if (mask[0] == '/')
				strcpy(filename, mask);
			else
				sprintf(filename, "%s%s", curPath, mask);

		retcode = 0;
	}

	return (retcode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int CheckWildcard(char*filename)
{
	int i;

	for (i = 0; i < (int)strlen(filename); i++)
		if (filename[i] == '?' || filename[i] == '*')
			return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_DosFindNext(char*filename)
{
	if (!fHandle || !UnixFindNextFile(fHandle, findFileMask, &finfo)) {
		strcpy(filename, "");
		return (0);
	}

	//   if (ISDIR(finfo) || SPECIAL(finfo))
	if (!ISFILE(finfo))
		return (OS_DosFindNext(filename));

	sprintf(filename, "%s%s", curPath, finfo.cFileName);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static DIR*UnixFindFirstFile(char*mask, FINFO*fileInfo)
{
	struct dirent*dp;
	struct stat filestat;
	char fullpath[MAX_PATH];

	fHandle = opendir(curPath);

	if (!fHandle)
		return (0);

	for (; ; ) {
		dp = readdir(fHandle);

		if (!dp)
			break;

		sprintf(fullpath, "%s%s", curPath, dp->d_name);

		if (WildcardMatch(mask, dp->d_name) && ValidFile(fullpath)) {
			strcpy(fileInfo->cFileName, dp->d_name);

			fileInfo->dwFileAttributes = 0;

			if (!lstat(fullpath, &filestat)) {
				if (S_ISDIR(filestat.st_mode))
					fileInfo->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
				if (S_ISREG(filestat.st_mode))
					fileInfo->dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
			}

			return (fHandle);
		}
	}

	closedir(fHandle);
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int WildcardMatch(char*mask, char*filename)
{
	if (!fnmatch(mask, filename, FNM_PATHNAME))
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int UnixFindNextFile(DIR*dirp, char*mask, FINFO*fileInfo)
{
	struct dirent*dp;
	struct stat filestat;
	char fullpath[MAX_PATH];

	for (; ; ) {
		dp = readdir(dirp);

		if (!dp)
			break;

		sprintf(fullpath, "%s%s", curPath, dp->d_name);

		if (WildcardMatch(mask, dp->d_name) && ValidFile(fullpath)) {
			strcpy(fileInfo->cFileName, dp->d_name);

			fileInfo->dwFileAttributes = 0;

			if (!lstat(fullpath, &filestat)) {
				if (S_ISDIR(filestat.st_mode))
					fileInfo->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
				if (S_ISREG(filestat.st_mode))
					fileInfo->dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
			}

			return (1);
		}
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_DosFindEnd(void)
{
	if (fHandle) {
		closedir(fHandle);
		fHandle = 0;
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_PaintScreen(char*buffer)
{
	int y, draw = 0, newYPos = 12345, newBottom = 56789;

	old_buffer = buffer;

	if (OptimizeScreen(buffer, original, &newYPos, &newBottom)) {
		if (cursor_visible) {
			hide_cursor();
			draw = 1;
		}

		for (y = newYPos; y <= newBottom; y++)
			OS_Paint_Line(buffer, y);

		if (draw)
			show_cursor();
	}

	XFlush(xw->display);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_PaintStatus(char*buffer)
{
	int draw = 0;

	if (cursor_visible) {
		hide_cursor();
		draw = 1;
	}

	OS_Paint_Line(buffer, screenYD - 1);

	if (draw)
		show_cursor();

	XFlush(xw->display);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void OS_Paint_Line(char*buffer, int line)
{
	unsigned char attr, prev;
	int x, counter, xPos, lineAddr;
	int draw_x, draw_y, optimizedX1, optimizedX2;

	lineAddr = line*screenXD*SCR_PTR_SIZE;

	for (optimizedX1 = 0; optimizedX1 < screenXD; optimizedX1++) {
		if ((original[lineAddr] != buffer[lineAddr]) ||
		    (original[lineAddr + 1] != buffer[lineAddr + 1]))
			break;

		lineAddr += SCR_PTR_SIZE;
	}

	lineAddr = line*screenXD*SCR_PTR_SIZE + (screenXD - 1)*SCR_PTR_SIZE;

	for (optimizedX2 = screenXD - 1; optimizedX2 > optimizedX1; optimizedX2--) {
		if ((original[lineAddr] != buffer[lineAddr]) ||
		    (original[lineAddr + 1] != buffer[lineAddr + 1]))
			break;

		lineAddr -= SCR_PTR_SIZE;
	}

	/* Update original buffer line for ScreenOptimize routine */
	lineAddr = line*screenXD*SCR_PTR_SIZE + optimizedX1*SCR_PTR_SIZE;

	memcpy(&original[lineAddr], &buffer[lineAddr],
	    ((optimizedX2 - optimizedX1) + 1)*SCR_PTR_SIZE);

	prev = buffer[lineAddr + 1];

	XSetForeground(xw->display, xw->gc, xw->palette[FG_COLOR(prev)]);
	XSetBackground(xw->display, xw->gc, xw->palette[BG_COLOR(prev)]);

	counter = 0;
	xPos = optimizedX1;

	for (x = optimizedX1; x <= optimizedX2; x++, lineAddr += SCR_PTR_SIZE) {
		line_text[counter] = buffer[lineAddr];

		attr = buffer[lineAddr + 1];

		if (attr != prev) {
			draw_x = xPos*font_size_x;
			draw_y = (line + 1)*font_size_y - font_descent;
			DrawFontString(draw_x, draw_y, line_text, counter);
			counter = 0;
			xPos = x;
			prev = attr;
			XSetForeground(xw->display, xw->gc, xw->palette[FG_COLOR(attr)]);
			XSetBackground(xw->display, xw->gc, xw->palette[BG_COLOR(attr)]);
			x--;
			lineAddr -= SCR_PTR_SIZE;
			continue;
		}
		counter++;
	}

	if (counter) {
		draw_x = xPos*font_size_x;
		draw_y = (line + 1)*font_size_y - font_descent;
		DrawFontString(draw_x, draw_y, line_text, counter);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DrawFontString(int x, int y, char*str, int len)
{
	int i;
	int newlen = 0;
	int index = 0;

	for (i = 0; i < len; i++) {
		if (str[i] == UP_ARROW || str[i] == DOWN_ARROW) {
			if (newlen) {
				XDrawImageString(xw->display, xw->window, xw->gc, x, y, &str[
				    index], newlen);
				index += newlen;
			}
			x += (font_size_x*newlen);
			XDrawImageString(xw->display, xw->window, xw->gc, x, y, " ", 1);
			DrawSpecialChar(str[i], x, (y - font_size_y) + font_descent);
			x += font_size_x;
			index = i + 1;
			newlen = 0;
		} else
			newlen++;
	}

	if (newlen)
		XDrawImageString(xw->display, xw->window, xw->gc, x, y, &str[index],
		    newlen);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DrawSpecialChar(char ch, int x, int y)
{
	int xp, yp;

	XDrawLine(xw->display, xw->window, xw->gc, x + font_size_x / 2 - 1, y + 4,
	    x + font_size_x / 2 - 1, y + font_size_y - 5);

	XDrawLine(xw->display, xw->window, xw->gc, x + font_size_x / 2, y + 3, x +
	    font_size_x / 2, y + font_size_y - 4);

	XDrawLine(xw->display, xw->window, xw->gc, x + font_size_x / 2 + 1, y + 4,
	    x + font_size_x / 2 + 1, y + font_size_y - 5);

	if (ch == UP_ARROW) {
		xp = x + font_size_x / 2;
		yp = y + 3;

		XDrawLine(xw->display, xw->window, xw->gc, xp - 1, yp, xp - 4, yp + 2);
		XDrawLine(xw->display, xw->window, xw->gc, xp, yp, xp - 3, yp + 2);
		XDrawLine(xw->display, xw->window, xw->gc, xp + 1, yp, xp - 2, yp + 2);

		XDrawLine(xw->display, xw->window, xw->gc, xp - 1, yp, xp + 2, yp + 2);
		XDrawLine(xw->display, xw->window, xw->gc, xp, yp, xp + 3, yp + 2);
		XDrawLine(xw->display, xw->window, xw->gc, xp + 1, yp, xp + 4, yp + 2);
	}

	if (ch == DOWN_ARROW) {
		xp = x + font_size_x / 2;
		yp = y + font_size_y - 4;

		XDrawLine(xw->display, xw->window, xw->gc, xp - 1, yp, xp - 4, yp - 2);
		XDrawLine(xw->display, xw->window, xw->gc, xp, yp, xp - 3, yp - 2);
		XDrawLine(xw->display, xw->window, xw->gc, xp + 1, yp, xp - 2, yp - 2);

		XDrawLine(xw->display, xw->window, xw->gc, xp - 1, yp, xp + 2, yp - 2);
		XDrawLine(xw->display, xw->window, xw->gc, xp, yp, xp + 3, yp - 2);
		XDrawLine(xw->display, xw->window, xw->gc, xp + 1, yp, xp + 4, yp - 2);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int OptimizeScreen(char*new, char*prev, int*y1, int*y2)
{
	int y;

	for (y = 0; y < screenYD; y++) {
		if (memcmp(&new[y*screenXD*SCR_PTR_SIZE],
		    &prev[y*screenXD*SCR_PTR_SIZE], screenXD*SCR_PTR_SIZE)) {
			*y1 = y;
			break;
		}
	}

	/* There are no differences in the buffer */
	if (y == screenYD)
		return (0);

	for (y = screenYD - 1; y >= *y1; y--) {
		if (memcmp(&new[y*screenXD*SCR_PTR_SIZE],
		    &prev[y*screenXD*SCR_PTR_SIZE], screenXD*SCR_PTR_SIZE)) {
			*y2 = y;
			break;
		}
	}

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void show_cursor(void)
{
	int draw_x, draw_y, y;
	char attr;

	if (!old_buffer)
		return ;

	if (cursor_enabled) {
		cursor_visible = 1;

		attr = old_buffer[((cursor_x - 1)*SCR_PTR_SIZE) +
		((cursor_y - 1)*(screenXD*SCR_PTR_SIZE)) + 1];

		XSetForeground(xw->display, xw->gc, xw->palette[FG_COLOR(attr)]);

		draw_x = (cursor_x - 1)*font_size_x;
		draw_y = (font_size_y*cursor_y) - font_descent;

		for (y = 0; y < cursor_height; y++) {
			XDrawLine(xw->display, xw->window, xw->gc, draw_x, draw_y - y,
			    draw_x + font_size_x - 1, draw_y - y);
		}
		XFlush(xw->display);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void hide_cursor(void)
{
	char byte[2], attr;
	int draw_x, draw_y;

	if (!old_buffer)
		return ;

	byte[0] = old_buffer[((cursor_x - 1)*SCR_PTR_SIZE) +
	((cursor_y - 1)*(screenXD*SCR_PTR_SIZE))];

	attr = old_buffer[((cursor_x - 1)*SCR_PTR_SIZE) +
	((cursor_y - 1)*(screenXD*SCR_PTR_SIZE)) + 1];

	XSetForeground(xw->display, xw->gc, xw->palette[FG_COLOR(attr)]);
	XSetBackground(xw->display, xw->gc, xw->palette[BG_COLOR(attr)]);

	draw_x = (cursor_x - 1)*font_size_x;
	draw_y = (font_size_y*cursor_y) - font_descent;

	if (byte[0] == UP_ARROW || byte[0] == DOWN_ARROW) {
		XDrawImageString(xw->display, xw->window, xw->gc, draw_x, draw_y, " ",
		    1);
		DrawSpecialChar(byte[0], draw_x, (cursor_y - 1)*font_size_y);
	} else
		XDrawImageString(xw->display, xw->window, xw->gc, draw_x, draw_y, byte,
		    1);

	cursor_visible = 0;
	XFlush(xw->display);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_ScreenSize(int*xd, int*yd)
{
	*xd = screenXD = COLS;
	*yd = screenYD = LINES;

	original = OS_Malloc(screenXD*screenYD*SCR_PTR_SIZE);
	memset(original, 0, screenXD*screenYD*SCR_PTR_SIZE);

	return (1);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void*OS_Malloc(long size)
{
	void*data;

	data = malloc(size);

	if (!data) {
		OS_Printf("ProEdit MP: Out of memory (%d)\n", size);
		OS_Exit();
	}

	return (data);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Free(void*ptr)
{
	free(ptr);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Exit()
{
	if (xw) {
		XWindowAttributes xgw;
		XGetWindowAttributes(xw->display, xw->window, &xgw);
		//   	if (xgw.x != screen_x || xgw.y != screen_y)
		OS_SaveScreenPos(screen_x, screen_y);
		TrashXWindow(xw);
		OS_Free(xw);
	}

	if (original)
		OS_Free(original);

	if (MyPalette)
		OS_Free(MyPalette);

	DisplayMessages();

	exit(0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Printf(char*fmt, ...)
{
	EXIT_MESSAGE*msg;

	va_list args;
	char*buffer;

	buffer = OS_Malloc(1024);
	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end(args);

	#if 1
	msg = (EXIT_MESSAGE*)OS_Malloc(sizeof(EXIT_MESSAGE));
	msg->next = 0;
	msg->prev = 0;
	msg->string = OS_Malloc(strlen(buffer) + 1);
	strcpy(msg->string, buffer);

	OS_Free(buffer);

	AddMessage(msg);
	#else
	printf("%s\n", buffer);
	OS_Free(buffer);
	#endif
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void AddMessage(EXIT_MESSAGE*msg)
{
	if (!head) {
		head = msg;
		tail = msg;
	} else {
		if (tail)
			tail->next = msg;

		msg->prev = tail;
		tail = msg;
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
	EXIT_MESSAGE*msg;

	msg = head;

	while (msg) {
		printf("%s", msg->string);
		msg = msg->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_Bell(void)
{
	printf("%c", 7);
	fflush(stdout);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_Getch(void)
{
	fflush(stdin);
	return (getchar());
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void OnCtrlC(int Arg)
{
	Arg = Arg;

	signal(SIGINT, OnCtrlC);

	printf("\nProEdit: Ctrl-C has been disabled. Exit ProEdit using ALT-X\n");
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void OnBreak(int Arg)
{
	Arg = Arg;

	signal(SIGHUP, OnBreak);
	signal(SIGQUIT, OnBreak);
	signal(SIGTERM, OnBreak);
	signal(SIGTSTP, OnBreak);

	printf(
	    "\nProEdit: Process Termination request. Exit ProEdit using ALT-X\n");
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_GetKey(int*mode, OS_MOUSE*mouse)
{
	struct timeval timeout;
	XEvent ev;
	int ch, blink, clock_counter = 0;

	*mode = 1;

	mouse->buttonStatus = 0;
	mouse->moveStatus = 0;

	blink = GetCursorBlink();

	for (; ; ) {
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000000 / CURSOR_FLASH_PER_SEC;

		if (XNextEventTimeout(xw->display, &ev, &timeout)) {
			ch = ProcessEvent(mode, &ev, mouse);

			if (ch) {
				SetCursorBlink(blink);
				return (ch);
			}
		} else {
			BlinkCursor();

			if (++clock_counter >= CURSOR_FLASH_PER_SEC) {
				clock_counter = 0;
				if (clock_pfn)
					clock_pfn();
			}

			if (need_redraw) {
				need_redraw = 0;
				if (old_buffer) {
					memset(original, 0, screenXD*screenYD*SCR_PTR_SIZE);
					OS_PaintScreen(old_buffer);
				}
			}
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int XNextEventTimeout(Display*display, XEvent*ev, struct timeval*tm)
{
	fd_set readfds;
	int display_fd;

	display_fd = XConnectionNumber(display);

	if (!tm) {
		XNextEvent(display, ev);
		return (1);
	}

	FD_ZERO(&readfds);
	FD_SET(display_fd, &readfds);

	while (!XEventsQueued(display, QueuedAfterFlush)) {
		if (select(display_fd + 1, &readfds, NULL, NULL, tm) <= 0)
			return (0);
	}

	XNextEvent(display, ev);
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetClockPfn(CLOCK_PFN*clockPfn)
{
	clock_pfn = clockPfn;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GetCursorBlink(void)
{
	if (cursor_enabled && cursor_visible)
		return (1);
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SetCursorBlink(int blink)
{
	if (blink) {
		if (!cursor_visible)
			show_cursor();
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void BlinkCursor(void)
{
	if (cursor_enabled) {
		if (cursor_visible)
			hide_cursor();
		else
			show_cursor();
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_PeekKey(int*mode, OS_MOUSE*mouse)
{
	XEvent ev;

	if (XCheckWindowEvent(xw->display, xw->window,
	    KeyPressMask | KeyReleaseMask | ExposureMask, &ev))
		return (ProcessEvent(mode, &ev, mouse));

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ProcessEvent(int*mode, XEvent*ev, OS_MOUSE*mouse)
{
	static int alt = 0, ctrl = 0, shift = 0;
	long KEY;

	if (ev->type == ClientMessage)
		return (0);

	#if 0
	if (ev->type == SelectionNotify) {
		if (ev->xselection.property != None) {
			CopyClipboard(ev->xselection.property);
			*mode = 1;
			return (ED_KEY_ALT_INSERT);
		}
		return (0);
	}

	if (ev->type == SelectionClear) {
		printf("SelectionClear\n");
		return (0);
	}
	#endif

	if (ev->type == SelectionRequest) {
		SetupClipboard(&ev->xselectionrequest);
		return (0);
	}

	if (ev->type == ButtonPress) {
		mouse->xpos = ev->xbutton.x / font_size_x;
		mouse->ypos = ev->xbutton.y / font_size_y;

		if (ev->xbutton.button == Button1) {
			mouse->buttonStatus = 1;
			mouse->button1 = OS_BUTTON_PRESSED;
		}

		if (ev->xbutton.button == Button2)
			return (0);

		if (ev->xbutton.button == Button3) {
			mouse->buttonStatus = 1;
			mouse->button3 = OS_BUTTON_PRESSED;
		}

		return (ED_KEY_MOUSE);
	}

	if (ev->type == ButtonRelease) {
		mouse->xpos = ev->xbutton.x / font_size_x;
		mouse->ypos = ev->xbutton.y / font_size_y;

		if (ev->xbutton.button == Button1) {
			mouse->buttonStatus = 1;
			mouse->button1 = OS_BUTTON_RELEASED;
		}

		if (ev->xbutton.button == Button2) {
			*mode = 1;
			return (ED_KEY_ALT_INSERT);
		}

		if (ev->xbutton.button == Button3) {
			mouse->buttonStatus = 1;
			mouse->button3 = OS_BUTTON_RELEASED;
		}
		return (ED_KEY_MOUSE);
	}

	if (ev->type == MotionNotify) {
		mouse->xpos = ev->xmotion.x / font_size_x;
		mouse->ypos = ev->xmotion.y / font_size_y;
		mouse->moveStatus = 1;

		*mode = 1;
		return (ED_KEY_MOUSE);
	}

	if ((ev->type == KeyPress) || (ev->type == KeyRelease)) {
		KEY = XLookupKeysym((XKeyEvent*)ev, 0);

		if (ev->type == KeyRelease) {
			switch (KEY&0xFFFF) {
			case 65505 :
			case 65506 :
				shift = 0;
				return (ED_KEY_SHIFT_UP);

			case 65507 :
			case 65508 :
				ctrl = 0;
				break;

			case 65511 :
			case 65512 :
			case 65513 :
			case 65514 :
				alt = 0;
				break;
			}
		}

		if (ev->type == KeyPress) {
			switch (KEY&0xFFFF) {
			case 65293 : /* Enter Key is special */
				*mode = 0;
				return (13);

			case 65288 : /* Backspace key is special */
				*mode = 0;
				return (8);

			case 65307 : /* Escape Key is special */
				*mode = 0;
				return (27);

			case 65289 : /* TAB key is special */
				if (ctrl)
					return (ED_ALT_TAB);
				*mode = 0;
				return (9);

			case 65505 :
			case 65506 :
				shift = 1;
				return (ED_KEY_SHIFT_DOWN);

			case 65507 :
			case 65508 :
				ctrl = 1;
				break;

			case 65511 :
			case 65512 :
			case 65513 :
			case 65514 :
				alt = 1;
				break;

			case 65362 :
				if (ctrl)
					return (ED_KEY_CTRL_UP);
				if (alt)
					return (ED_KEY_ALT_UP);
				return (ED_KEY_UP);
			case 65361 :
				if (ctrl)
					return (ED_KEY_CTRL_LEFT);
				if (alt)
					return (ED_KEY_ALT_LEFT);
				return (ED_KEY_LEFT);
			case 65363 :
				if (ctrl)
					return (ED_KEY_CTRL_RIGHT);
				if (alt)
					return (ED_KEY_ALT_RIGHT);
				return (ED_KEY_RIGHT);
			case 65364 :
				if (ctrl)
					return (ED_KEY_CTRL_DOWN);
				if (alt)
					return (ED_KEY_ALT_DOWN);
				return (ED_KEY_DOWN);
			case 65379 :
				if (ctrl)
					return (ED_KEY_CTRL_INSERT);
				if (alt)
					return (ED_KEY_ALT_INSERT);
				return (ED_KEY_INSERT);

			case 65360 :
				return (ED_KEY_HOME);
			case 65365 :
				return (ED_KEY_PGUP);
			case 65535 :
				return (ED_KEY_DELETE);
			case 65367 :
				return (ED_KEY_END);
			case 65366 :
				return (ED_KEY_PGDN);

			case 65470 :
				return (ED_F1);
			case 65471 :
				return (ED_F2);
			case 65472 :
				return (ED_F3);
			case 65473 :
				if (alt)
					return (ED_ALT_F4);
				return (ED_F4);
			case 65474 :
				return (ED_F5);
			case 65475 :
				return (ED_F6);
			case 65476 :
				return (ED_F7);
			case 65477 :
				return (ED_F8);
			case 65478 :
				return (ED_F9);
			case 65479 :
				return (ED_F10);
			case 65296 :
				return (ED_F11);
			case 65297 :
				return (ED_F12);
			}

			if (ctrl)
				return (ProcessCTRLKey((int)KEY));
			if (alt)
				return (ProcessALTKey((int)KEY));

			if (KEY >= 0 && KEY <= 127) {
				*mode = 0;
				if (shift)
					return (ProcessSHIFTKey((int)KEY));
				return (KEY);
			}
		}
	}

	if (ev->type == Expose) {
		need_redraw = 1;
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ProcessSHIFTKey(int ch)
{
	static char shiftNums[] = ")!@#$%^&*(";

	if (ch == 32)
		return (32);

	if (ch >= 'a' && ch <= 'z') {
		return (ch - 32);
	}

	if (ch >= '0' && ch <= '9') {
		return ((int)shiftNums[ch - '0']);
	}

	switch (ch) {
	case '`' :
		return ((int)'~');
	case '-' :
		return ((int)'_');
	case '=' :
		return ((int)'+');
	case '[' :
		return ((int)'{');
	case ']' :
		return ((int)'}');
	case '\\' :
		return ((int)'|');
	case ';' :
		return ((int)':');
	case '\'' :
		return ((int)'"');
	case ',' :
		return ((int)'<');
	case '.' :
		return ((int)'>');
	case '/' :
		return ((int)'?');
	}

	return (ch);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ProcessALTKey(int ch)
{
	if (ch >= 'a' && ch <= 'z')
		ch -= 32;

	switch (ch) {
	case 'A' :
		return (ED_ALT_A);
	case 'B' :
		return (ED_ALT_B);
	case 'C' :
		return (ED_ALT_C);
	case 'D' :
		return (ED_ALT_D);
	case 'E' :
		return (ED_ALT_E);
	case 'F' :
		return (ED_ALT_F);
	case 'G' :
		return (ED_ALT_G);
	case 'H' :
		return (ED_ALT_H);
	case 'I' :
		return (ED_ALT_I);
	case 'J' :
		return (ED_ALT_J);
	case 'K' :
		return (ED_ALT_K);
	case 'L' :
		return (ED_ALT_L);
	case 'M' :
		return (ED_ALT_M);
	case 'N' :
		return (ED_ALT_N);
	case 'O' :
		return (ED_ALT_O);
	case 'P' :
		return (ED_ALT_P);
	case 'Q' :
		return (ED_ALT_Q);
	case 'R' :
		return (ED_ALT_R);
	case 'S' :
		return (ED_ALT_S);
	case 'T' :
		return (ED_ALT_T);
	case 'U' :
		return (ED_ALT_U);
	case 'V' :
		return (ED_ALT_V);
	case 'W' :
		return (ED_ALT_W);
	case 'X' :
		return (ED_ALT_X);
	case 'Y' :
		return (ED_ALT_Y);
	case 'Z' :
		return (ED_ALT_Z);
	}
	return (ch);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ProcessCTRLKey(int ch)
{
	if (ch >= 'a' && ch <= 'z')
		ch -= 32;

	switch (ch) {
	case 'A' :
		return (ED_CTRL_A);
	case 'B' :
		return (ED_CTRL_B);
	case 'C' :
		return (ED_CTRL_C);
	case 'D' :
		return (ED_CTRL_D);
	case 'E' :
		return (ED_CTRL_E);
	case 'F' :
		return (ED_CTRL_F);
	case 'G' :
		return (ED_CTRL_G);
	case 'H' :
		return (ED_CTRL_H);
	case 'I' :
		return (ED_CTRL_I);
	case 'J' :
		return (ED_CTRL_J);
	case 'K' :
		return (ED_CTRL_K);
	case 'L' :
		return (ED_CTRL_L);
	case 'M' :
		return (ED_CTRL_M);
	case 'N' :
		return (ED_CTRL_N);
	case 'O' :
		return (ED_CTRL_O);
	case 'P' :
		return (ED_CTRL_P);
	case 'Q' :
		return (ED_CTRL_Q);
	case 'R' :
		return (ED_CTRL_R);
	case 'S' :
		return (ED_CTRL_S);
	case 'T' :
		return (ED_CTRL_T);
	case 'U' :
		return (ED_CTRL_U);
	case 'V' :
		return (ED_CTRL_V);
	case 'W' :
		return (ED_CTRL_W);
	case 'X' :
		return (ED_CTRL_X);
	case 'Y' :
		return (ED_CTRL_Y);
	case 'Z' :
		return (ED_CTRL_Z);
	}

	return (ch);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void GetFontSize(int*xd, int*yd, char*font_name)
{
	XFontStruct*font_struct;
	Display*display;

	display = XOpenDisplay(NULL);

	if (!display)
		return ;

	font_struct = XLoadQueryFont(display, font_name);

	if (font_struct) {
		font_descent = font_struct->max_bounds.descent;

		*xd = font_struct->max_bounds.width;
		*yd = font_struct->max_bounds.ascent + font_descent;

		XFreeFont(display, font_struct);
	} else {
		*xd = 0;
		*yd = 0;
	}

	XCloseDisplay(display);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
static void change_font_window(char*fontName)
{
	Font font;
	XFontStruct*font_struct;

	if (fontName) {
		font_struct = XLoadQueryFont(xw->display, fontName);

		if (font_struct) {
			font = XLoadFont(xw->display, fontName);

			if (font) {
				XSetFont(xw->display, xw->gc, font);
			} else
				goto font_error;
		}
	}

	font_struct = XQueryFont(xw->display, XGContextFromGC(xw->gc));

	if (!font_struct)
		goto font_error;

	if (old_buffer) {
		memset(original, 0, screenXD*screenYD*SCR_PTR_SIZE);
		OS_PaintScreen(old_buffer);
	}
	return ;

font_error:

	OS_Printf("\nProEdit: Unable to load font: %s\n", fontName);
	OS_Exit();
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_Close(FILE_HANDLE*fp)
{
	fclose((FILE*)fp);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
long OS_Write(void*buffer, long blkSize, long numBlks, FILE_HANDLE*fp)
{
	return (fwrite(buffer, blkSize, numBlks, (FILE*)fp));
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
long OS_Read(void*buffer, long blkSize, long numBlks, FILE_HANDLE*fp)
{
	return (fread(buffer, blkSize, numBlks, (FILE*)fp));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
long OS_ReadLine(char*buffer, int len, FILE_HANDLE*fp)
{
	if (fgets(buffer, len, fp)) {
		len = strlen(buffer);
		while (len && (buffer[len - 1] == 10 || buffer[len - 1] == 13))
			len--;
		buffer[len] = 0;
		return (1);
	}
	return (0);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
long OS_Filesize(FILE_HANDLE*fp)
{
	long size;

	fseek((FILE*)fp, 0, SEEK_END);
	size = ftell((FILE*)fp);
	fseek((FILE*)fp, 0, SEEK_SET);

	return (size);
}

/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_Delete(char*filename)
{
	remove(filename);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
FILE_HANDLE*OS_Open(char*filename, char*mode)
{
	FILE*fp;
	struct stat filestat;

	if (!lstat(filename, &filestat)) {
		if ((filestat.st_mode&(S_IFDIR | S_IFREG)) == 0)
			return (0);
	}

	fp = fopen(filename, mode);

	return ((FILE_HANDLE*)fp);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_PutByte(char ch, FILE_HANDLE*fp)
{
	fputc(ch, (FILE*)fp);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_GetCurrentDir(char*curDir)
{
	getcwd(curDir, MAX_PATH - 1);

	if (curDir[strlen(curDir) - 1] != '/')
		strcat(curDir, "/");
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_SetCurrentDir(char*curDir)
{
	chdir(curDir);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_SetCursorType(int overstrike, int visible)
{
	if (overstrike)
		cursor_height = font_size_y - font_descent;
	else
		cursor_height = font_size_y / 4;

	cursor_enabled = visible;

	if (!visible && cursor_visible)
		hide_cursor();
	else
		if (visible && !cursor_visible)
			show_cursor();
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int*OS_Build_Palette(void)
{
	int*palette, i;

	palette = OS_Malloc(3*17*sizeof(int));

	if (!palette)
		return (0);

	memset(palette, 0, 3*17*sizeof(int));

	for (i = 0; i < 16; i++) {
		if (i&AWS_FG_R)
			palette[i*3 + 0] = 128;
		if (i&AWS_FG_G)
			palette[i*3 + 1] = 128;
		if (i&AWS_FG_B)
			palette[i*3 + 2] = 128;

		if (i&AWS_FG_I) {
			if (i == AWS_FG_I) {
				palette[i*3 + 0] = 64;
				palette[i*3 + 1] = 64;
				palette[i*3 + 2] = 64;
			} else {
				if (palette[i*3 + 0])
					palette[i*3 + 0] = 255;
				if (palette[i*3 + 1])
					palette[i*3 + 1] = 255;
				if (palette[i*3 + 2])
					palette[i*3 + 2] = 255;
			}
		}
	}

	/* End of palette */
	palette[i*3] = -1;
	palette[i*3 + 1] = -1;
	palette[i*3 + 2] = -1;

	return (palette);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char OS_Frame(int index)
{
	static unsigned char titles[] =
	//   {0x18, 0x12, 0x0c, 0x0e, 0x0b, 0x19, 0x02, 0x01,
	//   {0x0f, 0x12, 0x0c, 0x0e, 0x0b, 0x19, 0x02, 0x01,
	{0x0d, 0x12, 0x0c, 0x0e, 0x0b, 0x19, 0x02, 0x01,
	UP_ARROW, DOWN_ARROW, 0x03, 0xac, 0xb7};

	#if 0
	//        !"#$%&'()*+,-./0123456789:;<=>?
	//@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
	//
	//
	#endif

	return ((char)titles[index]);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_Strcasecmp(char*str1, char*str2)
{
	return (strcasecmp(str1, str2));
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
int OS_GetFullPathname(char*filename, char*pathname, int max)
{
	if (filename[0] != '/') {
		getcwd(pathname, max);
		strcat(pathname, "/");
		strcat(pathname, filename);
	} else
		strcpy(pathname, filename);

	return (1);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_PathDepth(char*filename)
{
	int i, depth = 0;

	for (i = 0; i < (int)strlen(filename); i++) {
		if (filename[i] == '/')
			depth++;
	}
	return (depth);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_GetFilename(char*pathname, char*path, char*file)
{
	int i;
	char newpath[MAX_PATH];
	char*ptr = 0;
	struct stat filestat;

	OS_GetFullPathname(pathname, newpath, MAX_PATH);

	for (i = strlen(newpath) - 1; i >= 0; i--)
		if (newpath[i] == '/') {
			ptr = &newpath[i + 1];
			break;
		}

	if (!lstat(newpath, &filestat)) {
		if (S_ISDIR(filestat.st_mode) && path && file) {
			strcpy(path, newpath);
			if (path[strlen(path) - 1] != '/')
				strcat(path, "/");
			strcpy(file, "");
			return ;
		}
	}

	if (file) {
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

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*##########################################f#################################*/
OS_SHELL*OS_ShellCommand(char*cmd)
{
	UNIX_OS_SHELL*shell;
	OS_SHELL*user;

	user = (OS_SHELL*)OS_Malloc(sizeof(OS_SHELL));
	memset(user, 0, sizeof(OS_SHELL));

	shell = (UNIX_OS_SHELL*)OS_Malloc(sizeof(UNIX_OS_SHELL));
	memset(shell, 0, sizeof(UNIX_OS_SHELL));

	user->shellData = shell;

	OS_GetSessionFile(shell->tempFilename, SESSION_SHELL, 0);

	if (!strlen(shell->tempFilename)) {
		OS_Free(user);
		OS_Free(shell);
		return (0);
	}

	shell->fd = mkfifo(shell->tempFilename, S_IRUSR | S_IWUSR);

	if ((shell->pid = fork()) == 0) {
		//      close(0);
		shell->fh = open(shell->tempFilename, O_WRONLY);
		//      close(1);
		dup(shell->fh);
		//      close(2);
		dup(shell->fh);
		chdir(shellPath);
		execlp("/bin/sh", "sh", "-c", cmd, 0);
		exit(0);
	}

	if (shell->pid == -1) {
		close(shell->fd);
		OS_Free(user);
		OS_Free(shell);
		return (0);
	}
	#if 0
	shell->fifo = open(shell->tempFilename, O_RDONLY | O_NONBLOCK);
	#else
	shell->fifo = open(shell->tempFilename, O_RDONLY);
	#endif

	if (shell->fifo < 0) {
		OS_Free(user);
		OS_Free(shell);
		return (0);
	}

	user->command = OS_Malloc(strlen(cmd) + 1);
	user->cwd = OS_Malloc(strlen(shellPath) + 1);
	user->shell_cmd = OS_Malloc(strlen(cmd) + 40);

	sprintf(user->shell_cmd, "/bin/sh sh -c %s", cmd);

	strcpy(user->command, cmd);
	strcpy(user->cwd, shellPath);

	return (user);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_ShellGetLine(OS_SHELL*user)
{
	int bytes, wpid;
	char ch;
	int status, mode = 0;
	UNIX_OS_SHELL*shell;
	OS_MOUSE mouse;

	shell = (UNIX_OS_SHELL*)user->shellData;

	if (user->lineLen) {
		user->lineLen = 0;
		OS_Free(user->line);
	}

	user->line = OS_Malloc(MAX_SHELL_LINE_LEN);

	for (; ; ) {
		if (!shell->stopped) {
			wpid = waitpid(shell->pid, &status, WNOHANG);

			if (wpid) {
				if (wpid == -1) {
					printf("Shell Error: waitpid() returned -1... errno=%d\n",
					    errno);
					break;
				}

				if (WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(
				    status))
					shell->stopped = 1;
			}
		}

		bytes = read(shell->fifo, &ch, 1);

		/* Try to terminate the process? */
		if (OS_PeekKey(&mode, &mouse) == ED_KEY_ESC && mode == 0) {
			if (!kill(shell->pid, SIGINT)) {
				close(shell->fifo);
				close(shell->fd);
				close(shell->fh);
				kill(shell->pid, SIGKILL);
				kill(shell->pid, SIGQUIT);
				kill(shell->pid, SIGHUP);
				sprintf(user->line, "--- Process has been Terminated. ---");
				user->lineLen = strlen(user->line);
				return (1);
			}
		}

		if (bytes == 0 && shell->stopped) {
			if (!user->lineLen)
				break;
			return (1);
		}

		if (bytes < 0) {
			if (shell->stopped)
				break;
			if (errno != EAGAIN)
				break;
		}

		if (bytes == 1) {
			if (ch == 10)
				return (1);

			if (user->lineLen < MAX_SHELL_LINE_LEN)
				user->line[user->lineLen++] = ch;
		}
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_ShellClose(OS_SHELL*user)
{
	UNIX_OS_SHELL*shell;

	shell = (UNIX_OS_SHELL*)user->shellData;

	close(shell->fifo);
	close(shell->fd);
	close(shell->fh);

	if (user->lineLen)
		OS_Free(user->line);

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
void OS_ShellSetPath(char*path)
{
	strcpy(shellPath, path);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_ShellGetPath(char*path)
{
	if (!strlen(shellPath))
		OS_GetCurrentDir(shellPath);

	strcpy(path, shellPath);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
static int X11ErrorHandler(Display*display, XErrorEvent*error)
{
	display = display;
	lastError = error->error_code;

	#if 0
	printf("X11ErrorHandler(error_code=%d)\n", error->error_code);
	#endif
	return (0);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void ClearLastX11Error(void)
{
	#if 0
	printf("ClearLastError(prev=%d)\n", lastError);
	#endif
	lastError = 0;
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
int GetLastX11Error(void)
{
	#if 0
	printf("GetLastX11Error()=%d\n", lastError);
	#endif
	return (lastError);
}

/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
int OS_ValidPath(char*path)
{

	if (access(path, F_OK) == 0)
		return (1);

	return (0);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
static int ValidFile(char*pathname)
{
	struct stat filestat;

	if (access(pathname, F_OK))
		return (0);

	if (lstat(pathname, &filestat))
		return (0);

	if (filestat.st_mode&(S_IFDIR | S_IFREG))
		return (1);

	return (0);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
int OS_CreatePath(char*path)
{
	int retCode;
	char subpath[MAX_PATH];
	char newpath[MAX_PATH];

	strcpy(newpath, path);

	if (newpath[strlen(newpath) - 1] == '/')
		newpath[strlen(newpath) - 1] = 0;

	retCode = mkdir(newpath, S_IRUSR | S_IXUSR | S_IWUSR);

	if (!retCode)
		return (1);

	if (errno == ENOENT) {
		OS_GetFilename(newpath, subpath, 0);
		if (OS_CreatePath(subpath))
			return (OS_CreatePath(newpath));
	}

	return (0);
}



