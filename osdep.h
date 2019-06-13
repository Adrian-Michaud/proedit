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

#ifndef __OSDEP_H__
#define __OSDEP_H__

#if !defined(WIN32_CONSOLE) && !defined(WIN32_GUI) && !defined(X11_GUI) && !defined(NCURSES)
#error You must define WIN32_CONSOLE, WIN32_GUI, or X11_GUI before compiling.
#endif

#ifdef SPARC
#define MIN_ALIGN      8
#define MIN_ALIGN_MASK 0xfffffff8
#else
#define MIN_ALIGN_MASK 0xffffffff
#define MIN_ALIGN 1
#endif

#ifdef NCURSES
#define SCR_PTR char
#endif

#ifdef X11_GUI
#define SCR_PTR char
#endif

#ifdef WIN32_GUI
#define _CRT_SECURE_NO_DEPRECATE
#define SCR_PTR char
#endif

#ifdef WIN32_CONSOLE
#define _CRT_SECURE_NO_DEPRECATE
#define SCR_PTR short
#endif

#define SCR_PTR_SIZE (sizeof(SCR_PTR)*2)

typedef void FILE_HANDLE;

#define OS_BUTTON_PRESSED  1
#define OS_BUTTON_RELEASED 0
#define OS_WHEEL_UP        1
#define OS_WHEEL_DOWN      2

#define FILE_TYPE_DIR  1
#define FILE_TYPE_FILE 2

typedef struct osShell_t
{
	char*cwd;
	char*command;
	char*shell_cmd;
	char*line;
	int lineLen;
	void*shellData;
}OS_SHELL;

typedef struct fileInfo_t
{
	long fileSize;
	char asciidate[64];
	int file_type;
}OS_FILE_INFO;

typedef struct mouseInput_t
{
	int buttonStatus;
	int moveStatus;
	int xpos;
	int ypos;
	int button1;
	int button2;
	int button3;
	int wheel;
}OS_MOUSE;

#define OS_MAX_TIMEDATE 64

#define OS_NO_SPECIAL_DIR 1

#define OS_MAX_SCREEN_XDIM 2048

typedef void CLOCK_PFN(void);

int EditorMain(int argc, char**argv);
int EditorInit(int argc, char**argv);

/* Optional OS services that prefer to handle these items. */
int OS_DisplayBottomBar(char*buffer);
int OS_Input(int historyIndex, char*prompt, char*result, int max, int*retCode);
int OS_DisplayHelp(void);
int OS_PickList(char*windowTitle, int count, int width, char*title, char*list[],
    int start, int*retCode);
int OS_PaintFrame(char*title, int line, int numLines);
int OS_UpdateStatusBar(int hexMode, int line_number, int number_lines, int
    column, int undos);
int OS_CenterBottomBar(char*string);

void OS_Exit(void);
int OS_InitScreen(int xd, int yd);
int OS_DosFindFirst(char*mask, char*filename);
int OS_DosFindNext(char*filename);
void OS_DosFindEnd(void);
int OS_ValidPath(char*path);
int OS_CreatePath(char*path);
void OS_GetFilename(char*pathname, char*path, char*file);
int OS_SingleFile(char*dir);

void OS_Printf(char*fmt, ...);
int OS_Getch(void);

void OS_SetCursorType(int overstrike, int visible);
void OS_SetTextPosition(int x_position, int y_position);
void OS_Time(char*string); /* 12:12:12 */
int OS_ScreenSize(int*xd, int*yd);
void OS_PaintScreen(char*buffer);
void OS_PaintStatus(char*buffer);
void*OS_Malloc(long size);
void OS_Free(void*data);
int OS_Key(int*mode, OS_MOUSE*mouse);
int OS_PeekKey(int*mode, OS_MOUSE*mouse);
OS_SHELL*OS_ShellCommand(char*cmd);
void OS_ShellGetPath(char*path);
void OS_ShellSetPath(char*path);
void OS_ShellClose(OS_SHELL*shell);
int OS_ShellGetLine(OS_SHELL*shell);

void OS_SetClockPfn(CLOCK_PFN*clockPfn);

int OS_GetKey(int*mode, OS_MOUSE*mouse);
void OS_Bell(void);
int OS_GetFileInfo(char*path, OS_FILE_INFO*info);
int OS_GetFullPathname(char*filename, char*pathname, int max);
void OS_GetCurrentDir(char*curDir);
void OS_SetCurrentDir(char*curDir);
char OS_Frame(int index);
int OS_Strcasecmp(char*str1, char*str2);
int OS_ReadDirectory(char*path, char*mask, char**dirs, int max, int opts);
void OS_DeallocDirs(char**dirs, int numDirs);
int OS_PathDepth(char*filename);
void OS_JoinPath(char*pathname, char*dir, char*filename);
void OS_SetWindowTitle(char*pathname);

int OS_SaveSession(void*session, int len, int type, int instance);
void*OS_LoadSession(int*len, int type, int instance);
void OS_FreeSession(void*session);
void OS_GetSessionFile(char*filename, int type, int instance);

void OS_SetClipboard(char*string, int len);
char*OS_GetClipboard(void);
void OS_FreeClipboard(char*clip);

int OS_GetBackupPath(char*path, char*timedate);

/* File Handling (Should be use common STDIO file streams on most systems) */
FILE_HANDLE*OS_Open(char*filename, char*mode);
void OS_Close(FILE_HANDLE*fp);
long OS_Read(void*buffer, long blkSize, long numBlks, FILE_HANDLE*fp);
long OS_ReadLine(char*buffer, int len, FILE_HANDLE*fp);
long OS_Write(void*buffer, long blkSize, long numBlks, FILE_HANDLE*fp);
long OS_Filesize(FILE_HANDLE*fp);
void OS_PutByte(char ch, FILE_HANDLE*fp);
void OS_Delete(char*filename);

#define SESSION_CONFIG    0    //  "config"
#define SESSION_FILES     1    //  "sessions"
#define SESSION_HISTORY   2    //  "history"
#define SESSION_DATA      3    //  "session"
#define SESSION_WORDS     4    //  "dictionary"
#define SESSION_SHELL     5    //  "shell"
#define SESSION_BACKUPS   6    //  "backups"
#define SESSION_SCREEN    7    //  "screen"
#define SESSION_MACROS    8    //  "macros"

#define ED_KEY_ALT_UP      1
#define ED_KEY_ALT_RIGHT   2
#define ED_KEY_ALT_DOWN    3
#define ED_KEY_ALT_LEFT    4
#define ED_ALT_D           5
#define ED_ALT_U           6
#define ED_ALT_X           7
#define ED_ALT_Q           8
#define ED_ALT_E           9
#define ED_ALT_N           10
#define ED_ALT_L           11
#define ED_ALT_B           12
#define ED_ALT_C           13
#define ED_ALT_S           14
#define ED_F1              15
#define ED_F2              16
#define ED_KEY_PGUP        17
#define ED_KEY_PGDN        18
#define ED_KEY_HOME        19
#define ED_KEY_END         20
#define ED_KEY_INSERT      21
#define ED_KEY_DELETE      22
#define ED_KEY_UP          23
#define ED_KEY_RIGHT       24
#define ED_KEY_DOWN        25
#define ED_KEY_LEFT        26
#define ED_KEY_CTRL_UP     27
#define ED_KEY_CTRL_RIGHT  28
#define ED_KEY_CTRL_DOWN   29
#define ED_KEY_CTRL_LEFT   30
#define ED_ALT_V           31
#define ED_ALT_K           32
#define ED_ALT_G           33
#define ED_F5              34
#define ED_F6              35
#define ED_ALT_R           36
#define ED_ALT_W           37
#define ED_F3              38
#define ED_ALT_J           39
#define ED_F4              40
#define ED_ALT_F           41
#define ED_F7              42
#define ED_F8              43
#define ED_F9              44
#define ED_ALT_H           45
#define ED_F10             46
#define ED_F11             47
#define ED_F12             48
#define ED_ALT_P           49
#define ED_ALT_Z           50
#define ED_ALT_A           51
#define ED_ALT_M           52
#define ED_ALT_T           53
#define ED_ALT_O           54
#define ED_ALT_F4          55
#define ED_ALT_I           56
#define ED_ALT_Y           57
#define ED_KEY_CTRL_INSERT 58
#define ED_KEY_ALT_INSERT  59
#define ED_ALT_TAB         60

#define ED_CTRL_A          70
#define ED_CTRL_B          71
#define ED_CTRL_C          72
#define ED_CTRL_D          73
#define ED_CTRL_E          74
#define ED_CTRL_F          75
#define ED_CTRL_G          76
#define ED_CTRL_H          77
#define ED_CTRL_I          78
#define ED_CTRL_J          79
#define ED_CTRL_K          80
#define ED_CTRL_L          81
#define ED_CTRL_M          82
#define ED_CTRL_N          83
#define ED_CTRL_O          84
#define ED_CTRL_P          85
#define ED_CTRL_Q          86
#define ED_CTRL_R          87
#define ED_CTRL_S          88
#define ED_CTRL_T          89
#define ED_CTRL_U          90
#define ED_CTRL_V          91
#define ED_CTRL_W          92
#define ED_CTRL_X          93
#define ED_CTRL_Y          94
#define ED_CTRL_Z          95

#define ED_KEY_MOUSE       100

#define ED_KEY_SHIFT_UP    101
#define ED_KEY_SHIFT_DOWN  102

#define ED_KEY_CR     13     /* Carage Return                */
#define ED_KEY_ESC    27     /* Escape                       */
#define ED_KEY_LF     10     /* Line Feed                    */
#define ED_KEY_BS     8      /* Backspace                    */
#define ED_KEY_TAB    9      /* Tab                          */
#define ED_KEY_SPACE  32     /* Space (Whitespace)           */

#define AWS_FG_R      0x04   /* Red Forground Bits           */
#define AWS_FG_G      0x02   /* Green Forground Bits         */
#define AWS_FG_B      0x01   /* Blue Foreground Bits         */
#define AWS_FG_I      0x08   /* Forground Intensity ON       */

#define AWS_BG_R      0x40   /* Red Background Bits          */
#define AWS_BG_G      0x20   /* Green Background Bits        */
#define AWS_BG_B      0x10   /* Blue Background Bits         */

#define BLANK         AWS_BG_R|AWS_BG_G|AWS_BG_B
#define BOTTOM        AWS_FG_R|AWS_FG_G|AWS_FG_B|AWS_FG_I|AWS_BG_R|AWS_BG_B

#define DEFAULT_COLOR 0

#define HELP_ALIVE    10

/* Brite Yellow forground, Blue Background */
#define COLOR1 AWS_FG_R|AWS_FG_G|AWS_FG_I|AWS_BG_B
/* Brite White forground, Blue Background */
#define COLOR2 AWS_FG_R|AWS_FG_G|AWS_FG_B|AWS_FG_I|AWS_BG_B
/* Brite White forground, Magenta Background */
#define COLOR3 AWS_FG_R|AWS_FG_G|AWS_FG_B|AWS_FG_I|AWS_BG_B|AWS_BG_R
/* Brite White forground, Black Background */
#define COLOR4 AWS_FG_R|AWS_FG_G|AWS_FG_B|AWS_FG_I
/* Brite Yellow forground, Black Background */
#define COLOR5 AWS_FG_R|AWS_FG_G|AWS_FG_I
/* Brite Red Forground, Black Background */
#define COLOR6 AWS_FG_R|AWS_FG_I
/* Brite Green Forground, Black Background */
#define COLOR7 AWS_FG_G|AWS_FG_I
/* Brite Blue Forground, Black Background */
#define COLOR8 AWS_FG_B|AWS_FG_I
/* Black Forground, White Background */
#define COLOR9 AWS_BG_R|AWS_BG_G|AWS_BG_B

#endif /* __OSDEP_H__ */





