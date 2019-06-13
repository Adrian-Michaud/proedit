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

#ifndef __PROEDIT_H__
#define __PROEDIT_H__

#if 0
#define DEBUG_MEMORY
#endif

#define PROEDIT_MP_TITLE   "ProEdit-MP"
#define PROEDIT_MP_VERSION "2.1"
#define PROEDIT_MP_CLASS   "Beta 1"

#define QUESTION1 1
#define QUESTION2 2
#define QUESTION3 3

#define MAX_FILENAME       256
#define EXTRA_LINE_PADDING 32
#define MAX_LIST_FILES     4096

#define MAX_SEARCH 256

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define ED_KEY_TABPAD 0

#define MAX_UNDOS 4096

#define ADJ_CURSOR_LEFT   1
#define ADJ_CURSOR_RIGHT  2

#define NO_INDENT      0x00
#define CAN_INDENT     0x01
#define CAN_WORDWRAP   0x02
#define NO_UNDO        0x04
#define CAN_REALLOC    0x08

#define COPY_ON       0x01   /* Highlight mode now active     */
#define COPY_BLOCK    0x02   /* Clipboard data is block-mode  */
#define COPY_LINE     0x04   /* Clipboard data is line-mode   */
#define COPY_SELECT   0x08   /* Clipboard data is select-mode */
#define COPY_UPDATE   0x10   /* Clipboard data is active      */

#define LOAD_FILE_NORMAL      0x00
#define LOAD_FILE_CMDLINE     0x01
#define LOAD_FILE_EXISTS      0x02
#define LOAD_FILE_NOWILDCARD  0x04
#define LOAD_FILE_NOPAINT     0x08
#define LOAD_FILE_INTERACTIVE 0x10

#define LINE_FLAG_DIFF1     0x01
#define LINE_FLAG_DIFF2     0x02
#define LINE_FLAG_BOOKMARK  0x04
#define LINE_FLAG_INDENTED  0x08
#define LINE_FLAG_PADDED    0x10
#define LINE_FLAG_WRAPPED   0x20
#define LINE_FLAG_HIGHLIGHT 0x40
#define LINE_FLAG_CUSTOM    0x80

#define FILE_FLAG_MERGED      0x01
#define FILE_FLAG_NONFILE     0x04
#define FILE_FLAG_CRLF        0x08
#define FILE_FLAG_READONLY    0x10
#define FILE_FLAG_NO_COLORIZE 0x20
#define FILE_FLAG_NOBORDER    0x40
#define FILE_FLAG_EDIT_STATUS 0x80
#define FILE_FLAG_UNTITLED    0x100
#define FILE_FLAG_NORMAL      0x200
#define FILE_FLAG_SINGLELINE  0x400
#define FILE_FLAG_ALL         0xFFFF

#define ADD_FILE_SORTED       1
#define ADD_FILE_BOTTOM       2
#define ADD_FILE_TOP          3

#define CONFIG_INT_INDEX                0
#define CONFIG_INT_TABSIZE              1
#define CONFIG_INT_DEFAULT_ROWS         2
#define CONFIG_INT_DEFAULT_COLS         3
#define CONFIG_INT_CASESENSITIVE        4
#define CONFIG_INT_GLOBALFILE           5
#define CONFIG_INT_COLORIZING           6
#define CONFIG_INT_FG_COLOR             7
#define CONFIG_INT_BG_COLOR             8
#define CONFIG_INT_BORDER_COLOR         9
#define CONFIG_INT_DIFF1_FG_COLOR       10
#define CONFIG_INT_DIFF1_BG_COLOR       11
#define CONFIG_INT_DIFF2_FG_COLOR       12
#define CONFIG_INT_DIFF2_BG_COLOR       13
#define CONFIG_INT_PREPROCESSOR_COLOR   14
#define CONFIG_INT_NUMERIC_COLOR        15
#define CONFIG_INT_STRING_COLOR         16
#define CONFIG_INT_CHARACTER_COLOR      17
#define CONFIG_INT_OPERATOR_COLOR       18
#define CONFIG_INT_KEYWORD_COLOR        19
#define CONFIG_INT_COMMENTS_COLOR       20
#define CONFIG_INT_SELECT_FG_COLOR      21
#define CONFIG_INT_SELECT_BG_COLOR      22
#define CONFIG_INT_BOTTOM_FG_COLOR      23
#define CONFIG_INT_BOTTOM_BG_COLOR      24
#define CONFIG_INT_DLG_FG_COLOR         25
#define CONFIG_INT_DLG_BG_COLOR         26
#define CONFIG_INT_HIGHLIGHT_FG_COLOR   27
#define CONFIG_INT_OVERSTRIKE           28
#define CONFIG_INT_BOOKMARK_FG_COLOR    29
#define CONFIG_INT_BOOKMARK_BG_COLOR    30
#define CONFIG_INT_BACKUPS              31
#define CONFIG_INT_AUTOINDENT           32
#define CONFIG_INT_STYLE                33
#define CONFIG_INT_MERGEWS              34
#define CONFIG_INT_HEXADDR_FG_COLOR     35
#define CONFIG_INT_HEXBYTE_FG_COLOR     36
#define CONFIG_INT_HEXTEXT_FG_COLOR     37
#define CONFIG_INT_FORCE_CRLF           38
#define CONFIG_INT_HEX_ENDIAN           39
#define CONFIG_INT_AUTO_SAVE_BUILD      40
#define CONFIG_INT_HEX_COLS             41
#define CONFIG_INT_XML_COMMENTS_COLOR   42
#define CONFIG_INT_TOTAL                43

#define MAINTAIN_CRLF                   1
#define FORCE_CRLF                      2
#define FORCE_LF                        3

#define SELECT_FILE_ALL                 1
#define SELECT_FILE_MODIFIED            2
#define SELECT_FILE_UNMODIFIED          3

#define HEX_SEARCH_DEFAULT              1
#define HEX_SEARCH_BIG_ENDIAN           2
#define HEX_SEARCH_LITTLE_ENDIAN        3

#define ED_SPECIAL_SPACE 1
#define ED_SPECIAL_TAB   2
#define ED_SPECIAL_ALL   0xFFFF

typedef struct editLines
{
	char*line;
	int allocSize;
	int len;
	int flags;
	struct editLines*prev;
	struct editLines*next;
}EDIT_LINE;

typedef struct editClipboard
{
	int number_lines;
	int status;
	EDIT_LINE*lines;
	EDIT_LINE*tail;
}EDIT_CLIPBOARD;

typedef struct editDisplay
{
	EDIT_LINE*top_line;
	int line_number;
	int columns;
	int rows;
	int pan;
	int xpos;
	int ypos;
	int xd;
	int yd;
}EDIT_DISPLAY;

typedef struct editCursor
{
	int line_number;
	int offset;
	int xpos;
	int ypos;
	EDIT_LINE*line;
}EDIT_CURSOR;

typedef struct saveCursor
{
	int line;
	int offset;
	int xpos;
	int ypos;
	int pan;
	int displayLine;
}CURSOR_SAVE;

typedef struct bookMarks
{
	char*msg;
	int offset;
	EDIT_LINE*line;
	int instance;
	struct bookMarks*next;
	struct bookMarks*prev;
}EDIT_BOOKMARK;

typedef struct saveCopy
{
	int line;
	int offset;
}COPY_SAVE;

#define UNDO_DONE              0x01
#define UNDO_INSERT_LINE       0x04
#define UNDO_DELETE_LINE       0x08
#define UNDO_CURSOR            0x10
#define UNDO_INSERT_BLOCK      0x20
#define UNDO_DELETE_BLOCK      0x40
#define UNDO_HEX_OVERSTRIKE    0x80
#define UNDO_CARRAGE_RETURN    0x100
#define UNDO_INSERT_TEXT       0x200
#define UNDO_OVERSTRIKE_TEXT   0x400
#define UNDO_CUTLINE           0x800
#define UNDO_DELETE_TEXT       0x1000
#define UNDO_HEX_INSERT        0x2000
#define UNDO_HEX_DELETE        0x4000
#define UNDO_WORDWRAP          0x8000
#define UNDO_UNWORDWRAP        0x10000

typedef struct editUndos
{
	CURSOR_SAVE cursor;
	int len;
	int arg;
	int operationStatus;
	int line_flags;
	void*buffer;
	struct editUndos*next;
	struct editUndos*prev;
}EDIT_UNDOS;

#define CONTENT_FLAG  0x01   /* Paint textual content area   */
#define CURSOR_FLAG   0x02   /* Paint cursor on screen       */
#define FRAME_FLAG    0x04   /* Paint frame with title       */

#define HEX_MODE_HEX  1
#define HEX_MODE_TEXT 2

#define LINE_OP_EDIT          0x01
#define LINE_OP_DELETE        0x02
#define LINE_OP_INSERT        0x04
#define LINE_OP_GETTING_FOCUS 0x08
#define LINE_OP_LOSING_FOCUS  0x10
#define LINE_OP_INSERT_CHAR   0x20

typedef void COLORIZE_PFN(EDIT_LINE*line, int pan, int len, SCR_PTR*screen,
    int address, char attr);

typedef int LINE_PFN(void*file, EDIT_LINE*line, int op, int arg);

typedef struct lineCallbacks
{
	int opMask;
	LINE_PFN*pfn;
	struct lineCallbacks*next;
	struct lineCallbacks*prev;
}LINE_CALLBACK;

typedef struct editFile
{
	char*title;
	char*pathname;
	char*diff1_filename;
	char*diff2_filename;
	char*window_title;
	int wordwrap;
	int file_flags;
	int paint_flags;
	int border;
	int client;
	int home_count;
	int end_count;
	int numberUndos;
	int userUndos;
	int display_special;
	int undo_disabled;
	int number_lines;
	int modified;
	int force_modified;
	int undoStatus;
	int copyStatus;
	int shift;
	int selectblock;
	int overstrike;
	int scrollbar;
	int hideCursor;
	int hexMode;
	int hex_columns;
	int forceHex;
	int forceText;
	char*hexData;
	EDIT_BOOKMARK*bookmarks;
	EDIT_BOOKMARK*bookmark_walk;
	EDIT_LINE*lines;
	EDIT_UNDOS*undoHead;
	EDIT_UNDOS*undoTail;
	EDIT_CURSOR cursor;
	EDIT_DISPLAY display;
	COPY_SAVE copyFrom;
	COPY_SAVE copyTo;
	OS_MOUSE mouse;
	COLORIZE_PFN*colorize;
	LINE_CALLBACK*linePfns;
	int callbackMask;
	int userArg;
	struct editFile*prev;
	struct editFile*next;
}EDIT_FILE;

#define UNDO_RUNNING 1
#define UNDO_BEGIN   2

#define HISTORY_NEWFILE      0
#define HISTORY_SEARCH       1
#define HISTORY_GOTOLINE     2
#define HISTORY_REPLACE      3
#define HISTORY_GOTOADDRESS  4
#define HISTORY_HEX_SEARCH   5
#define HISTORY_HEX_REPLACE  6
#define HISTORY_HEX_ENTRY    7
#define HISTORY_CALCULATOR   8
#define HISTORY_CB_PATH      9
#define HISTORY_BUILD_CMD    10
#define HISTORY_CHECKOUT_CMD 11
#define HISTORY_MACROS       12
#define HISTORY_RESERVED2    13
#define HISTORY_RESERVED3    14
#define HISTORY_RESERVED4    15

#define NUM_HISTORY_TYPES    16

#define INS_ABOVE_CURSOR    0
#define INS_BELOW_CURSOR    1

#define WHITESPACE_BEFORE   1
#define WHITESPACE_AFTER    2

typedef EDIT_FILE*USER_INPUT_PFN(EDIT_FILE*file, int mode, int key);


int InitDisplay(void);
int GetScreenYDim(void);
int GetScreenXDim(void);
void CloseDisplay(void);
int ProcessCmdLine(int argv, char**argc, int preload);
EDIT_FILE*LoadFileWildcard(EDIT_FILE*file, char*pathname, int mode);
EDIT_FILE*LoadFileRecurse(EDIT_FILE*file, char*pathname, int mode);
void InitDisplayFile(EDIT_FILE*file);
void InitCursorFile(EDIT_FILE*file);
int AbortRequest(void);

int Input(int historyIndex, char*prompt, char*result, int max);
void Paint(EDIT_FILE*file);
void PaintHex(EDIT_FILE*file);
EDIT_FILE*ProcessHexInput(EDIT_FILE*file, int mode, int ch);
EDIT_FILE*ProcessUserInput(EDIT_FILE*file, USER_INPUT_PFN*pfn);
void ProcessNormal(EDIT_FILE*file, int ch);
EDIT_FILE*ProcessSpecial(EDIT_FILE*file, int ch);
EDIT_FILE*SwitchFile(EDIT_FILE*file);
EDIT_FILE*NextFile(EDIT_FILE*file);
EDIT_FILE*ValidateFile(EDIT_FILE*file, char*pathname);

int NumberFiles(int mask);

EDIT_LINE*GetLine(EDIT_FILE*file, int line_number);

EDIT_FILE*LoadSavedSessions(int session);
void DisplaySessions(void);
void ClearSessions(void);
void LogSession(EDIT_FILE*file);
void SaveSessions(int session);
void ToggleBackups(void);

void AddLineCallback(EDIT_FILE*file, LINE_PFN*pfn, int opMask);
void RemoveLineCallback(EDIT_FILE*file, LINE_PFN*pfn);
void RemoveAllCallbacks(EDIT_FILE*file);
void LineCallbackMask(EDIT_FILE*file, int mask);
void LineCallbackUnMask(EDIT_FILE*file, int mask);
int CallLineCallbacks(EDIT_FILE*file, EDIT_LINE*line, int op, int arg);
void DefineMacro(EDIT_FILE*file);
void ReplayMacro(void);
void SaveMacros(void);
void LoadMacros(void);
void ProcessBackspace(EDIT_FILE*file);
int CanBackspace(EDIT_FILE*file);
void InsertCharacter(EDIT_FILE*file, int ch);
void PaintFrame(EDIT_FILE*file);
void MouseCommand(EDIT_FILE*file);
void PaintCursor(EDIT_FILE*file);
void PaintContent(EDIT_FILE*file);
void KeyHome(EDIT_FILE*file);
void KeyEnd(EDIT_FILE*file);
void InsertText(EDIT_FILE*file, char*text, int len, int options);
void OverstrikeCharacter(EDIT_FILE*file, char ch, int options);
int WhitespaceLine(EDIT_LINE*line);
int WhitespaceTest(EDIT_FILE*file, int mode);
void StripLinePadding(EDIT_LINE*line);
int CursorPgup(EDIT_FILE*file);
int CursorPgdn(EDIT_FILE*file);
int CursorLeft(EDIT_FILE*file);
int CursorRight(EDIT_FILE*file);
int CursorUp(EDIT_FILE*file);
int CursorDown(EDIT_FILE*file);
int CursorLeftText(EDIT_FILE*file);
int CursorRightText(EDIT_FILE*file);
int ScrollUp(EDIT_FILE*file);
int ScrollDown(EDIT_FILE*file);
int DisplayLine(EDIT_FILE*file, int line_number);
int CursorLine(EDIT_FILE*file, int line_number);
void CursorOffset(EDIT_FILE*file, int offset);
int LineUp(EDIT_FILE*file);
int LineDown(EDIT_FILE*file);
void DeleteText(EDIT_FILE*file, int len, int options);
int MouseClick(EDIT_FILE*file);
int MouseHexClick(EDIT_FILE*file);
void PasteHexCharset(EDIT_FILE*file);

void SaveUndo(EDIT_FILE*file, int type, int arg);
void PrintLine(char*msg, EDIT_LINE*line);
void DeleteUndos(EDIT_FILE*file);
void UndoBegin(EDIT_FILE*file);
void DeallocLines(EDIT_LINE*lines);
void Undo(EDIT_FILE*file);
void MouseCursor(EDIT_FILE*file, int xpos, int ypos);
void Configure(EDIT_FILE*file);
void WordWrapLine(EDIT_FILE*file);
int DeWordWrapLine(EDIT_FILE*file);
void WordWrapFile(EDIT_FILE*file);
void DeWordWrapFile(EDIT_FILE*file);
void DeleteCharacter(EDIT_FILE*file, int options);
int SaveAllModified(EDIT_FILE*file);
int ForceSaveAllModified(EDIT_FILE*file);
int CheckoutFile(EDIT_FILE*file, char*pathname);

int SaveModified(EDIT_FILE*file, int*write_all);
int FileModified(EDIT_FILE*file);

void ToggleWordWrap(EDIT_FILE*file);
void DeleteBlock(EDIT_FILE*file);
int TestCursor(EDIT_FILE*file, CURSOR_SAVE*saved);
void SetCursor(EDIT_FILE*file, CURSOR_SAVE*saved);
void InsertBlock(EDIT_CLIPBOARD*clipboard, EDIT_FILE*file);
void SaveBlock(EDIT_CLIPBOARD*clipboard, EDIT_FILE*file);
void DeleteSelectLine(EDIT_FILE*file, int line);
void HighlightLine(EDIT_FILE*file, EDIT_LINE*line, int highlight);
char*GetSelectLine(EDIT_FILE*file, EDIT_LINE*line, int lineNo, int*len);
void CursorHome(EDIT_FILE*file);
void CursorTopPage(EDIT_FILE*file);
void CursorTopFile(EDIT_FILE*file);
void CursorEnd(EDIT_FILE*file);
void CursorEndPage(EDIT_FILE*file);
void CursorEndFile(EDIT_FILE*file);
void CopyBlock(EDIT_FILE*file, int type);
void PasteSystemBlock(EDIT_FILE*file);
void AdjustCursorTab(EDIT_FILE*file, int direction);
void CarrageReturn(EDIT_FILE*file, int indent);
void AddGlobalClipboard(EDIT_CLIPBOARD*clipboard);
void InitClipboard(EDIT_CLIPBOARD*clipboard);
void InitBookmarks(void);
void Overstrike(EDIT_FILE*file);
void CutLine(EDIT_FILE*file, int offset, int options);
int CopyCheck(EDIT_FILE*file, EDIT_LINE*line, int x, int y);
void ReallocLine(EDIT_LINE*line, int allocLen);
void IndentLine(EDIT_FILE*file);
void InitFileIndenting(EDIT_FILE*file);

int IndentPadding(EDIT_FILE*file, int offset);
int CursorIndentOffset(EDIT_FILE*file);
void DeleteHexBytes(EDIT_FILE*file, int len);
void DeleteHexBlock(EDIT_FILE*file);
void InsertHexBytes(EDIT_FILE*file, char*data, int len);
void OverstrikeHexBytes(EDIT_FILE*file, char*data, int len);

int HexCopyCheck(EDIT_FILE*file, int offset);

void InsertLine(EDIT_FILE*file, char*text, int len, int below, int flags);
void TabulateLine(EDIT_LINE*line, char*buf, int len);
int InsertTabulate(int offset, char*buf, int len);

char*TabulateString(char*string, int len, int*newLen);
void DeleteLine(EDIT_FILE*file);
void UpdateStatusBar(EDIT_FILE*file);

void EnablePainting(int enable);
int DisableUndo(EDIT_FILE*file);
void EnableUndo(EDIT_FILE*file, int undo);
void PasteCharset(EDIT_FILE*file);
void FindMatch(EDIT_FILE*file);
int MatchOperator(EDIT_FILE*file, char match, char find, int forward);


void AddFile(EDIT_FILE*new_file, int mode);
int QueryFiles(char*dir);
int SearchForFile(char*dir, char*filename);

void DisplayHelp(EDIT_FILE*file);
void ToggleDisplaySpecial(EDIT_FILE*file);

void SelectBlockCheck(EDIT_FILE*file, int enabled);
void DeleteSelectBlock(EDIT_FILE*file);
void CancelSelectBlock(EDIT_FILE*file);
void SetupSelectBlock(EDIT_FILE*file, int line, int offset, int len);
void SetupHexSelectBlock(EDIT_FILE*file, int offset, int len);

void CopySelectBlock(EDIT_FILE*file);
void PasteSelectBlock(EDIT_FILE*file);
void Operation(EDIT_FILE*file);
int RunOperation(EDIT_FILE*file, int op);
int RunOperationAll(EDIT_FILE*file, int op);

void HideCursor(void);
void SetupColors(void);

void ToggleColorize(EDIT_FILE*file);
void Calculator(EDIT_FILE*file);
void CloseCalculator(void);
COLORIZE_PFN*ConfigColorize(EDIT_FILE*file, char*pathname);
void SetupColorizers(void);
EDIT_FILE*Utilities(EDIT_FILE*file);

int CursorHexLeft(EDIT_FILE*file);
int CursorHexRight(EDIT_FILE*file);
int CursorHexUp(EDIT_FILE*file);
int CursorHexDown(EDIT_FILE*file);
int LineHexUp(EDIT_FILE*file);
int LineHexDown(EDIT_FILE*file);
int CursorHexPgup(EDIT_FILE*file);
int CursorHexPgdn(EDIT_FILE*file);
void CursorHexHome(EDIT_FILE*file);
void CursorHexEnd(EDIT_FILE*file);

char*SaveScreen(void);
void RestoreScreen(char*scr);

void ImportBuffer(EDIT_FILE*file, char*buf, long max);
int PickList(char*windowTitle, int count, int width, char*title, char*list[],
    int start);

void ProcessContent(EDIT_FILE*file, EDIT_LINE*line, int pan, int len, int
    line_number, int address);
void SaveCursor(EDIT_FILE*file, CURSOR_SAVE*);
int TabulateLength(char*buf, int index, int len, int max);
void ReleaseClipboard(EDIT_CLIPBOARD*clipboard);
void CenterBottomBar(int pending, char*fmt, ...);
void ProgressBar(char*title, int current, int total);
void DisplayBottomBar(char*fmt, ...);
int DisableBottomBar(void);
void EnableBottomBar(int bar);

int Question(char*string, ...);

EDIT_FILE*JumpNextBookmark(EDIT_FILE*file);
void CreateBookmark(EDIT_FILE*file);
EDIT_FILE*GotoBookmark(EDIT_FILE*file, EDIT_BOOKMARK*bookmark);

EDIT_BOOKMARK*AddBookmark(EDIT_FILE*file, EDIT_LINE*line, int offset, char*msg);
void DeleteBookmark(EDIT_FILE*file, EDIT_BOOKMARK*bookmark);
EDIT_BOOKMARK*ValidateBookmark(EDIT_FILE*file, EDIT_BOOKMARK*bookmark);

void DestroyBookmarks(EDIT_FILE*file);


EDIT_LINE*AddFileLine(EDIT_FILE*file, EDIT_LINE*current, char*line, int len,
    int flags);

EDIT_FILE*SelectFile(EDIT_FILE*current, int mode);
EDIT_FILE*Build(EDIT_FILE*file);
EDIT_FILE*BuildShell(EDIT_FILE*file);
EDIT_FILE*SearchFile(EDIT_FILE*file);
EDIT_FILE*SearchAgain(EDIT_FILE*file);
EDIT_FILE*SearchReplace(EDIT_FILE*file);
EDIT_FILE*CloseAllUnmodified(EDIT_FILE*file);
int ProcessShellChdir(char*cmd);


EDIT_FILE*ExitFiles(EDIT_FILE*file);

void DeallocFile(EDIT_FILE*edit);
EDIT_FILE*CreateUntitled(void);
EDIT_FILE*AllocFile(char*filename);
EDIT_FILE*DisconnectFile(EDIT_FILE*file);

void ToggleFileSearch(void);
void ToggleCase(void);

EDIT_FILE*QuitFile(EDIT_FILE*file, int session, int*write_all);
EDIT_FILE*CloseFile(EDIT_FILE*file);

EDIT_FILE*LoadNewFile(EDIT_FILE*file);
void RenameFile(EDIT_FILE*file);
EDIT_FILE*FileAlreadyLoaded(char*filename);

int SaveFile(EDIT_FILE*file);
int SaveFileAs(EDIT_FILE*file, char*pathname);

void CopyRegionUpdate(EDIT_FILE*file);
void SetRegionFile(EDIT_FILE*file);
void DisplayClipboard(EDIT_CLIPBOARD*clipboard);
int SpellCheck(EDIT_FILE*file, EDIT_CLIPBOARD*clipboard);
void FreeWordLists(void);

void AddClipboardLine(EDIT_CLIPBOARD*clipboard, char*buffer, int len,
    int status);

void GotoLine(EDIT_FILE*file);
void GotoPosition(EDIT_FILE*file, int line, int column);
void GotoHex(EDIT_FILE*file, long address);
int ConvertToHex(char*hexString, char*hexData);


EDIT_FILE*Merge(EDIT_FILE*current);

EDIT_CLIPBOARD*GetClipboard(void);
void CopyFileClipboard(EDIT_FILE*file, EDIT_CLIPBOARD*clipboard);


int SelectHistoryText(int historyIndex);
void AddToHistory(int index, char*text);
int QueryHistoryExists(int index, char*text);
void DeallocHistory(void);
int QueryHistoryCount(int index);
char*QueryHistory(int index, int number);
void SaveHistory(void);
void LoadHistory(void);

#ifdef DEBUG_MEMORY
#define OS_Malloc(size) DebugMalloc((size),__FILE__,__LINE__);
#define OS_Free(ptr)    DebugFree((ptr),__FILE__,__LINE__);
#define OS_Exit         DebugExit

void*DebugMalloc(long size, char*file, int line);
void DebugFree(void*ptr, char*file, int line);
void DebugExit(void);
#endif

void SetupConfig(void);
void SetConfigInt(int item, int value);
int GetConfigInt(int item);
void SaveConfig(void);
void LoadConfig(void);

char*SessionFilename(int index);

#endif /* __PROEDIT_H__ */



