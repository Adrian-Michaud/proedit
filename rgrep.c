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

#include "osdep.h" /* Platform dependent interface */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rgrep.h"
#include <unistd.h>

#define TAB_SIZE 4

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */


#define DEFAULT_PROEDIT_EXE "pe"

#ifdef WIN32_CONSOLE
#include <windows.h>
#endif

static void DisplayHelp(void);
static void ShowUsage(void);
static int SearchFileWildcard(char*pathname);
static int SearchFileRecurse(char*pathname);
static int SearchFile(char*filename);
static void ViewFile(char*filename, int line, int index, int offset, int len,
    int hexMode);
static int DisplayHit(char*filename, char*buffer, int index, int len, int line,
    int offset, int bufLen);
static void ShowText(char*filename, char*buffer, int offset, int len, int size,
    int line, int column);
static void ShowHex(char*filename, char*buffer, int index, int len, int size);
static void DisplayBanner(void);
static void SearchBanner(char*search);

static char*exeName;
static char proedit[MAX_FILENAME];

#define SEARCH_QUIT      1
#define SEARCH_SKIP_FILE 2
#define SEARCH_SKIP_DIR  3

static int recursiveLoad = 1;
static int ignoreCase = 1;
static int verboseMode = 0;
static int tabsize = TAB_SIZE;
static int debugMode = 0;
static int hitsOnly = 0;
static int _path_offset = 0;
static int _cols = 0;

static char searchString[MAX_SEARCH_STRING];

#ifdef WIN32_CONSOLE
WORD originalScreenAttrs;
#endif

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int EditorInit(int argv, char**argc)
{
	#ifdef WIN32_CONSOLE
	CONSOLE_SCREEN_BUFFER_INFO scrInfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &scrInfo);
	originalScreenAttrs = scrInfo.wAttributes;
	#endif

	argv = argv;

	exeName = argc[0];

	strcpy(proedit, DEFAULT_PROEDIT_EXE);
	return (1);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetupBaseDir(char*search)
{
	char filename[MAX_FILENAME];

	if (search[0] == '/' || search[0] == '\\') {
		_path_offset = 0;
		return ;
	}

	OS_GetCurrentDir(filename);

	_path_offset = strlen(filename);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ParseSearchString(void)
{
	int len;
	int i;
	int index = 0;

	len = strlen(searchString);

	for (i = 0; i < len; i++) {
		if (searchString[i] == '\\') {
			if (searchString[i + 1] == 'n' || searchString[i + 1] == 'N') {
				searchString[index++] = 10;
				i++;
				continue;
			}
			if (searchString[i + 1] == 't' || searchString[i + 1] == 'T') {
				searchString[index++] = 9;
				i++;
				continue;
			}
		}
		searchString[index++] = searchString[i];
	}
	searchString[index] = 0;
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int EditorMain(int argv, char**argc)
{
	int i, file_search = 0;
	int retCode;
	#if 0
	#ifndef WIN32_CONSOLE
	struct winsize ws;

	if (isatty(fileno(stdout))) {
		ioctl(fileno(stdout), TIOCGWINSZ, &ws);
		_cols = ws.ws_col;
	}
	#endif
	#endif

	if (_cols < 80)
		_cols = 80;

	if (argv < 2) {
		DisplayHelp();
		return (0);
	}

	for (i = 1; i < argv; i++) {
		if (!OS_Strcasecmp(argc[i], "-?") || !OS_Strcasecmp(argc[i], "-help") ||
		    !OS_Strcasecmp(argc[i], "--?") || !OS_Strcasecmp(argc[i],
		    "--help")) {
			DisplayHelp();
			break;
		} else
			if (!OS_Strcasecmp(argc[i], "-r")) {
				recursiveLoad = 0;
			} else
				if (!OS_Strcasecmp(argc[i], "-h")) {
					hitsOnly = 1;
				} else
					if (!OS_Strcasecmp(argc[i], "-debug")) {
						debugMode = 1;
					} else
						if (!OS_Strcasecmp(argc[i], "-v")) {
							verboseMode = 1;
						} else
							if (!OS_Strcasecmp(argc[i], "-i")) {
								ignoreCase = 0;
							} else
								if (argc[i][0] == '-' && (argc[i][1] == 't' ||
								    argc[i][1] == 'T')) {
									tabsize = atol(&argc[i][2]);
								} else
									if (argc[i][0] == '-' && (argc[i][1] ==
									    'e' || argc[i][1] == 'E')) {
										strcpy(proedit, &argc[i][2]);
									} else {
										if (!strlen(searchString)) {
											strcpy(searchString, argc[i]);
											ParseSearchString();
										} else {
											file_search = 1;

											SearchBanner(argc[i]);

											if (recursiveLoad)
												retCode = SearchFileRecurse(argc
												    [i]);
											else
												retCode = SearchFileWildcard(
												    argc[i]);

											if (retCode == SEARCH_QUIT)
												break;
										}
									}
	}

	if (!strlen(searchString))
		ShowUsage();
	else {
		if (!file_search) {
			SearchBanner("*");
			if (recursiveLoad)
				SearchFileRecurse("*");
			else
				SearchFileWildcard("*");
		}
	}

	OS_Exit();
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SearchBanner(char*search)
{
	static int first = 0;

	if (first == 0) {
		first = 1;
		DisplayBanner();

		SetupBaseDir(search);

		if (verboseMode) {
			printf("File Search      : \"%s\"\n", search);
			printf("Search String    : \"%s\"\n", searchString);
			printf("Tab Size         : %d\n", tabsize);
			printf("Case Sensitive   : %s\n", ignoreCase ? "No" : "Yes");
			printf("Recursive Mode   : %s\n", recursiveLoad ? "Yes" : "No");
			printf("ProEdit filename : \"%s\"\n", proedit);
			printf("\n");
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DisplayHelp(void)
{
	DisplayBanner();
	ShowUsage();
	printf("Command line options:\n\n");
	printf(" -i . . . . . . . . . . . . . . . . Don't Ignore Case\n");
	printf(
	    " -r . . . . . . . . . . . . . . . . Don't Recurse into directories\n");
	printf(" -v . . . . . . . . . . . . . . . . Verbose Mode\n");
	printf(" -t<tab_size> . . . . . . . . . . . Change default [%d] tab size\n",
	    tabsize);
	printf(
	    " -e<ProEdit Filename> . . . . . . . Change default [\"%s\"] ProEdit filename\n", proedit);
	printf(" -help. . . . . . . . . . . . . . . Display this help\n");
	printf(" -debug . . . . . . . . . . . . . . Enable Debug Mode\n");
	printf(" Note: [...] are optional <...> are required\n");
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DisplayBanner(void)
{
	printf("\n%s Multi-Platform File searching utility", RGREP_MP_VERSION);
	printf("\nDesigned/Developed/Produced by Adrian J. Michaud");
	printf("\nLast %s Core Build: %s %s", RGREP_MP_VERSION, __DATE__, __TIME__);
	printf("\nEMail: adrian@gnuxtools.com");
	printf("\n\n");
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ShowUsage(void)
{
	printf("Usage: %s <search> [options] <filename>\n\n", exeName);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SearchFileWildcard(char*pathname)
{
	int retCode = 0;
	char*filename;
	char*fullpath;

	filename = OS_Malloc(MAX_FILENAME);
	fullpath = OS_Malloc(MAX_FILENAME);

	if (debugMode)
		printf("OS_GetFullPathname(%s)\n", pathname);

	if (OS_GetFullPathname(pathname, fullpath, MAX_FILENAME)) {
		if (debugMode)
			printf("fullpath='%s'\n", fullpath);

		OS_DosFindFirst(fullpath, filename);

		while (strlen(filename)) {
			if (verboseMode)
				printf("Searching %s\n", filename);

			retCode = SearchFile(filename);

			if (retCode == SEARCH_QUIT || retCode == SEARCH_SKIP_DIR)
				break;

			OS_DosFindNext(filename);
		}

		OS_DosFindEnd();
	}

	OS_Free(filename);
	OS_Free(fullpath);

	return (retCode);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SearchFileRecurse(char*pathname)
{
	int i, numDirs, retCode;
	char*filename;
	char*path;
	char*mask;
	char*orgDir;
	char**dirs;

	filename = OS_Malloc(MAX_FILENAME);
	orgDir = OS_Malloc(MAX_FILENAME);
	path = OS_Malloc(MAX_FILENAME);
	mask = OS_Malloc(MAX_FILENAME);
	dirs = OS_Malloc(MAX_LIST_FILES*sizeof(char*));

	OS_GetCurrentDir(orgDir);

	retCode = SearchFileWildcard(pathname);

	if (retCode != SEARCH_QUIT) {
		OS_GetFilename(pathname, path, mask);

		if (!strlen(mask))
			strcpy(mask, "*");

		numDirs = OS_ReadDirectory(path, "*", dirs, MAX_LIST_FILES,
		    OS_NO_SPECIAL_DIR);

		for (i = 0; i < numDirs; i++) {
			OS_JoinPath(filename, dirs[i], mask);

			if (verboseMode)
				printf("Searching in directory: %s\n", filename);

			retCode = SearchFileRecurse(filename);

			if (retCode == SEARCH_QUIT)
				break;
		}

		OS_DeallocDirs(dirs, numDirs);
	}

	OS_SetCurrentDir(orgDir);

	OS_Free(orgDir);
	OS_Free(path);
	OS_Free(mask);
	OS_Free(dirs);
	OS_Free(filename);

	return (retCode);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int FunctionCheck(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (1);

	if (ch >= 'A' && ch <= 'Z')
		return (1);

	if (ch == '_')
		return (1);

	if (ch >= '0' && ch <= '9')
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SearchFile(char*filename)
{
	FILE_HANDLE*fp;
	long filesize, i, j, line = 0, offset = 0, len, searchLen;
	char*buffer;
	int retCode = 0;
	char ch;
	char ch2;

	if (debugMode)
		printf("OS_Open(%s)\n", filename);

	fp = OS_Open(filename, "rb");

	if (!fp)
		return (0);

	filesize = OS_Filesize(fp);

	if (filesize == -1) {
		OS_Close(fp);
		return (0);
	}
	if (debugMode)
		printf("Before OS_Read(size=%ld)\n", filesize);

	if (filesize < 0) {
		printf("Filesize for %s is bad..  %ld\n", filename, filesize);
	}

	buffer = OS_Malloc(filesize);

	if (OS_Read(buffer, filesize, 1, fp)) {
		searchLen = strlen(searchString);

		len = (filesize - searchLen) + 1;

		for (i = 0; i < len; i++, offset++) {
			if (buffer[i] == 9)
				offset += (tabsize - (offset%tabsize));

			if (buffer[i] == 10) {
				line++;
				offset = 0;
			}
			for (j = 0; j < searchLen; j++) {
				ch = buffer[i + j];
				ch2 = searchString[j];

				if (ch2 == '?')
					continue;

				// check for identifier
				if (ch2 == '^' && !FunctionCheck(ch))
					continue;

				if (ignoreCase) {
					if (ch >= 'a' && ch <= 'z')
						ch -= 32;

					if (ch2 >= 'a' && ch2 <= 'z')
						ch2 -= 32;
				}

				if (ch != ch2)
					break;
			}
			if (j == searchLen) {
				retCode = DisplayHit(filename, buffer, i, searchLen, line,
				    offset, filesize);

				if (retCode)
					break;
			}
		}
	}
	if (debugMode)
		printf("After OS_Read()\n");

	OS_Free(buffer);
	OS_Close(fp);

	return (retCode);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int DisplayHit(char*filename, char*buffer, int index, int len, int line,
    int offset, int buflen)
{
	int ch;
	int i;
	int hexMode = 0;
	int count = 0;

	// If file has more than 1% non displayable characters, consider it a binary file
	for (i = 0; i < buflen; i++) {
		if (buffer[i] == 10 || buffer[i] == 13 || buffer[i] == 9)
			continue;

		if ((unsigned char)buffer[i] > 127 || buffer[i] < 32) {
			if (++count > buflen / 100) {
				hexMode = 1;
				break;
			}
		}
	}

	if (hexMode)
		ShowHex(filename, buffer, index, len, buflen);
	else
		ShowText(filename, buffer, index, len, buflen, line, offset);

	if (hitsOnly)
		return (0);

	printf(
	    "\n[S]kip Directory, [N]ext File, [L]oad ProEdit, [Q]uit, [Enter]=Continue: ");

	for (; ; ) {
		ch = OS_Getch();

		if (ch == 's' || ch == 'S') {
			printf("\n");
			return (SEARCH_SKIP_DIR);
		}

		if (ch == 'n' || ch == 'N') {
			printf("\n");
			return (SEARCH_SKIP_FILE);
		}

		if (ch == 'l' || ch == 'L') {
			printf("\n");
			ViewFile(filename, line, index, offset, len, hexMode);
			return (0);
		}

		if (ch == 'q' || ch == 'Q') {
			printf("\n");
			return (SEARCH_QUIT);
		}

		if (ch == 13 || ch == 10) {
			printf("\n");
			return (0);
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ViewFile(char*filename, int line, int index, int offset, int len,
    int hexMode)
{
	char cmd[512];

	if (hexMode)
		sprintf(cmd, "%s -tab%d -hex \"%s\" -g%d -h%d,%d \"-f%s\"", proedit,
		    tabsize, filename, index + len, index, len - 1, searchString);
	else
		sprintf(cmd, "%s -tab%d -text \"%s\" -g%d,%d -h%d,%d,%d \"-f%s\"",
		    proedit, tabsize, filename, line + 1, offset + len, line + 1,
		    offset, offset + len - 1, searchString);

	if (verboseMode)
		printf("Launching ProEdit: %s\n\n", cmd);

	system(cmd);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ShowText(char*filename, char*buffer, int index, int len, int size,
    int line, int column)
{
	int i, counter = 0, j, col, foundLine;
	int color = 0;

	foundLine = line;

	if (hitsOnly) {
		printf("%s: (%d:%d) ", &filename[_path_offset], line, column);
		for (j = index; j > 0; j--)
			if (buffer[j] == 10) {
				j++;
				break;
			}

		for (i = j; i < size; i++) {
			if (buffer[i] == 10)
				break;

			if ((i >= index) && (i < (index + len))) {
				#ifdef WIN32_CONSOLE
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY |
				    BACKGROUND_BLUE);
				printf("%c", buffer[i]);
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    originalScreenAttrs);
				#else
				if (!color) {
					color = 1;
					if (isatty(fileno(stdout)))
						printf(BOLDBLUE);
				}
				printf("%c", buffer[i]);
				#endif
			} else {
				#ifndef WIN32_CONSOLE
				if (color) {
					color = 0;
					if (isatty(fileno(stdout)))
						printf(RESET);
				}
				#endif

				printf("%c", buffer[i]);
			}
		}
		#ifndef WIN32_CONSOLE
		if (color) {
			color = 0;
			if (isatty(fileno(stdout)))
				printf(RESET);
		}
		#endif

		printf("\n");
		return ;
	}

	printf("\nFound in %s:\n", filename);

	for (j = index; j >= 0; j--) {
		if (buffer[j] == 10) {
			if (++counter >= 4)
				break;
			line--;
		}
	}

	color = 0;
	for (i = line; i < line + 7; i++) {
		printf("\n%d: ", i + 1);
		col = 0;
		for (j = j + 1; j < size; j++, col++) {
			if (buffer[j] == 10)
				break;

			if (buffer[j] <= 32)
				buffer[j] = 32;

			if (col < (_cols - 1)) {
				if ((j >= index) && (j < (index + len))) {
					#ifdef WIN32_CONSOLE
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
					    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
					    | BACKGROUND_BLUE);
					printf("%c", buffer[j]);
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
					    originalScreenAttrs);
					#else
					if (!color) {
						color = 1;
						if (isatty(fileno(stdout)))
							printf(BOLDBLUE);
					}
					printf("%c", buffer[j]);
					#endif
				} else {
					#ifndef WIN32_CONSOLE
					if (color) {
						color = 0;
						if (isatty(fileno(stdout)))
							printf(RESET);
					}
					#endif

					printf("%c", buffer[j]);
				}
			}
		}
		#ifndef WIN32_CONSOLE
		if (color) {
			color = 0;
			if (isatty(fileno(stdout)))
				printf(RESET);
		}
		#endif
	}
	printf("\n");
	printf("\n(Found on Line %d, Column %d)\n", foundLine + 1, column);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ShowHex(char*filename, char*buffer, int index, int len, int size)
{
	int offset, count = 0, i, j, addr = 0;
	int color = 0;

	if (hitsOnly) {
		printf("%s (offset %d)\n", &filename[_path_offset], index);
		return ;
	}

	printf("\nFound in %s:\n", filename);

	offset = (index&0xfffffff0) - 0x30;

	if (offset < 0)
		offset = 0;

	for (i = offset; i < offset + 0x70 && i < size; i++) {
		if (!count) {
			addr = i;
			if (i <= index && i + 16 > index)
				printf("\n==>%08x: ", i);
			else
				printf("\n   %08x: ", i);
		}

		printf(" %02x", (unsigned char)buffer[i]);

		count++;

		if (count == 16 || i == (size - 1)) {
			for (j = count; j < 16; j++)
				printf("   ");

			printf(" ");

			for (j = 0; j < count; j++) {
				if (buffer[addr + j] < 32)
					printf(".");
				else {
					if (((addr + j) >= index) && ((addr + j) < (index + len))) {
						#ifdef WIN32_CONSOLE
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
						    FOREGROUND_RED | FOREGROUND_GREEN |
						    FOREGROUND_INTENSITY | BACKGROUND_BLUE);
						printf("%c", buffer[addr + j]);
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
						    originalScreenAttrs);

						#else
						if (!color) {
							color = 1;
							if (isatty(fileno(stdout)))
								printf(BOLDBLUE);
						}

						printf("%c", buffer[addr + j]);
						#endif
					} else {
						#ifndef WIN32_CONSOLE
						if (color) {
							color = 0;
							if (isatty(fileno(stdout)))
								printf(RESET);
						}
						#endif
						printf("%c", buffer[addr + j]);
					}
				}
			}
			count = 0;
		}
	}

	printf("\n");
	printf("\n   (Filesize: 0x%x Bytes, Found text at offset: 0x%x):\n", size,
	    index);

}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char*SessionFilename(int index)
{
	(void)index;
	return ("");
}


