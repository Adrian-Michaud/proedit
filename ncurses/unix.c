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
#include "../types.h"

extern int nospawn;

typedef struct exitMessages_t
{
	char*string;
	struct exitMessages_t*next;
	struct exitMessages_t*prev;
}EXIT_MESSAGE;

static EXIT_MESSAGE*head;
static EXIT_MESSAGE*tail;

int OS_GetKey(int*mode, OS_MOUSE*mouse);

extern void SetupClipboard(void*reqevent);

static char*clipboardData;
static int clipboardLen;

static int WildcardMatch(char*mask, char*filename);
static int CheckWildcard(char*filename);
static void AddMessage(EXIT_MESSAGE*msg);
static int ValidFile(char*pathname);

CLOCK_PFN*clock_pfn;


#define MAX_PATH   512

#define MAX_MACRO_SIZE 16384
#define RECORD 1
#define STOP   2
#define DONE   3
#define PLAY   4

static long macroIndex = 0, macroSize = 0;
static int macro = 0;

static char macroBuffer1[MAX_MACRO_SIZE];
static char macroBuffer2[MAX_MACRO_SIZE];


static char*session_filenames[] =
{
	"config", // 0
	"sessions", // 1
	"history", // 2
	"session", // 3
	"dictionary", // 4
	"shell", // 5
	"backups", // 6
	"screen", // 7
};

static DIR*fHandle;
static char curPath[MAX_PATH];
static char shellPath[MAX_PATH];
static char findFileMask[MAX_PATH];

typedef struct fileInfoStruct
{
	char cFileName[MAX_PATH];
	int dwFileAttributes;
}FINFO;

static FINFO finfo;

#define FILE_ATTRIBUTE_HIDDEN       1
#define FILE_ATTRIBUTE_SYSTEM       2
#define FILE_ATTRIBUTE_DIRECTORY 4

#define ISDIR(finfo)          (!strcmp(finfo.cFileName,".")||!strcmp(finfo.cFileName,".."))
#define SPECIAL(finfo)  (finfo.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN|| finfo.dwFileAttributes&FILE_ATTRIBUTE_SYSTEM|| finfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
#define DIR2(finfo) (finfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY&&strcmp(finfo.cFileName,".")&&strcmp(finfo.cFileName,".."))
#define SPECIAL2(finfo) (finfo.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN||finfo.dwFileAttributes&FILE_ATTRIBUTE_SYSTEM)

static DIR*UnixFindFirstFile(char*mask, FINFO*fileInfo);
static int UnixFindNextFile(DIR*dirp, char*mask, FINFO*fileInfo);

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
static void OnCtrlC(int Arg)
{
	Arg = Arg;

	signal(SIGINT, OnCtrlC);

	//   printf("\nProEdit: Ctrl-C has been disabled. Exit ProEdit using ALT-X\n");
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

	//   printf("\nProEdit: Process Termination request. Exit ProEdit using ALT-X\n");
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int main(int argc, char**argv)
{
	/* Clean exiting on severals breaks */
	signal(SIGHUP, OnBreak);
	signal(SIGINT, OnCtrlC);
	signal(SIGQUIT, OnBreak);
	signal(SIGTERM, OnBreak);
	signal(SIGTSTP, OnBreak);

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
int OS_GetBackupPath(char*path, char*timedate)
{
	char pathname[MAX_PATH];
	time_t tmer;
	struct tm*tm;
	static int counter = 0;

	OS_GetSessionFile(pathname, 6, 0);

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
			sprintf(filename, "%s/proedit/%s%d", homedir,
			    session_filenames[type], instance);
		else
			sprintf(filename, "%s/proedit/%s", homedir,
			    session_filenames[type]);
	} else
		strcpy(filename, "");
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
		if (ISDIR(finfo) || SPECIAL(finfo))
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

	if (ISDIR(finfo) || SPECIAL(finfo))
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
int OS_DefineMacro(int mode)
{
	if (mode == -1) {
		if (macro == RECORD || macro == STOP)
			return (1);
		return (0);
	}
	if (mode == 1) {
		macro = RECORD;
		macroIndex = 0;
	}
	if (mode == 2) {
		macro = DONE;
		macroSize = macroIndex;
		macroIndex = 0;
	}
	return (0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_PlayMacro(void)
{
	if (macro == DONE) {
		macro = PLAY;
		macroIndex = 0;
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_Key(int*mode, OS_MOUSE*mouse)
{
	int ch, dontSave = 0;

	if (macro == PLAY) {
		ch = macroBuffer1[macroIndex];
		*mode = macroBuffer2[macroIndex];
		macroIndex++;

		if (macroIndex >= macroSize)
			macro = DONE;

		return (ch);
	}

	ch = OS_GetKey(mode, mouse);

	if (*mode && ch == ED_F1)
		dontSave = 1; /* Avoid Macros getting into macros */

	if (*mode && ch == ED_F2)
		dontSave = 1; /* Avoid Macros getting into macros */

	if (macro == RECORD && !dontSave) {
		macroBuffer1[macroIndex] = ch;
		macroBuffer2[macroIndex] = *mode;
		macroIndex++;
		if (macroIndex >= MAX_MACRO_SIZE)
			macro = STOP;
	}
	return (ch);
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

	OS_GetSessionFile(shell->tempFilename, 5, 0);

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
	int status, mode;
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

/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_SetClipboard(char*string, int len)
{
	int i;

	if (clipboardData)
		OS_Free(clipboardData);

	clipboardData = OS_Malloc(len + 1);

	clipboardLen = 0;

	for (i = 0; i < len; i++) {
		if (string[i] == ED_KEY_CR)
			continue;

		clipboardData[clipboardLen++] = string[i];
	}

	clipboardData[clipboardLen] = 0;

	//   XSetSelectionOwner(xw->display, XA_PRIMARY, xw->window, CurrentTime);
	//   XSetSelectionOwner(xw->display, XA_clipboard, xw->window, CurrentTime);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
char*OS_GetClipboard(void)
{
	//   if (GetClipboardData(XA_STRING))
	//      return(clipboardData);
	//
	//   if (GetClipboardData(XA_text))
	//      return(clipboardData);
	//
	//   if (GetClipboardData(XA_text_plain))
	//      return(clipboardData);
	//
	//   if (GetClipboardData(XA_text_uri))
	//      return(clipboardData);

	return (clipboardData);
}

/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_FreeClipboard(char*clip)
{
	if (clip) {
		OS_Free(clip);

		if (clip == clipboardData)
			clipboardData = 0;
	}
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
void DisplayMessages(void)
{
	EXIT_MESSAGE*msg;

	msg = head;

	while (msg) {
		printf("%s", msg->string);
		msg = msg->next;
	}
}



