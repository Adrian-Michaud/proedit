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
#include <termios.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include "../types.h"
#include <curses.h>

#define MAX_PATH   512

extern CLOCK_PFN*clock_pfn;

#define MY_KEYS (KEY_MAX + 1)
#define CURSOR_FLASH_PER_SEC 3

typedef struct PE_KEY_EVENT_T{
	int ch;
	int escaped;
}PE_KEY_EVENT;

static int screenXD, screenYD;
int OS_GetKey(int*mode, OS_MOUSE*mouse);
static void OS_Paint_Line(char*buffer, int line);
static int ProcessEvent(int*mode, PE_KEY_EVENT*ev, OS_MOUSE*mouse);
void DisplayMessages(void);

#define BAR_ULCORNER   1
#define BAR_HORZ       2
#define BAR_URCORNER   3
#define BAR_LLCORNER   4
#define BAR_LRCORNER   5
#define BAR_VIRT       6
#define BAR_SCROLL     7
#define BAR_SCROLL_POS 8
#define BAR_DARROW     9
#define BAR_UARROW     10

int new_term = 0;
struct termios orig_term;

static int OptimizeScreen(char*new, char*prev, int*y1, int*y2);
static char*original;

static int cursor_enabled = 0;
static int cursor_os = 0;

static chtype line_text[1024];
static chtype text_lut[256];

static chtype attr_lut[256];

static char*old_buffer = 0;

#define FONT_STRING "-misc-fixed-bold-r-normal--15-120-100-100-c-90-iso8859-1"

static int cursor_x, cursor_y;

#define FG_COLOR(color) ((color) & 0x0f)
#define BG_COLOR(color) (((color) & 0xf0) >> 4)

int color_lut[16];

void hide_cursor(void)
{
	curs_set(0);
}

void show_cursor(void)
{
	if (cursor_enabled) {
		if (cursor_os)
			curs_set(1);
		else
			curs_set(2);

		move(cursor_y - 1, cursor_x - 1);
	}
}

void resizeHandler(int sig)
{
	int yd, xd;

	(void)sig;
	getmaxyx(stdscr, yd, xd); /* get the new screen size */

	//    screenXD = xd;
	//    screenYD = yd;
}

chtype setup_color(unsigned char attr)
{
	static int index = 1;
	int fg = FG_COLOR(attr);
	int bg = BG_COLOR(attr);

	init_pair(index, color_lut[fg], color_lut[bg]);

	attr_lut[attr] = COLOR_PAIR(index) | A_BOLD;

	index++;

	return (attr_lut[attr]);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_InitScreen(int xd, int yd)
{
	int i;
	struct termios term;

	tcgetattr(STDIN_FILENO, &term);

	memcpy(&orig_term, &term, sizeof(term));

	term.c_iflag &= ~IXON;

	tcsetattr(STDIN_FILENO, TCSANOW, &term);

	new_term = 1;

	(void)xd;
	(void)yd;

	newterm(0, stdout, stdin);

	getmaxyx(stdscr, screenYD, screenXD);

	signal(SIGWINCH, resizeHandler);

	(void)cbreak(); /* take input chars one at a time, no wait for \n */
	(void)noecho(); /* don't echo input */

	scrollok(stdscr, FALSE);
	keypad(stdscr, TRUE);

	for (i = 0; i < 32; i++) {
		text_lut[i] = ACS_DEGREE;
		//		text_lut[i] = '?';//ACS_DEGREE;
	}

	for (i = 32; i < 127; i++) {
		text_lut[i] = (chtype)i;
	}

	for (i = 127; i < 256; i++) {
		text_lut[i] = (chtype)'?';
	}

	text_lut[BAR_HORZ] = ACS_HLINE;
	text_lut[BAR_ULCORNER] = ACS_ULCORNER;
	text_lut[BAR_URCORNER] = ACS_URCORNER;
	text_lut[BAR_LLCORNER] = ACS_LLCORNER;
	text_lut[BAR_LRCORNER] = ACS_LRCORNER;
	text_lut[BAR_VIRT] = ACS_VLINE;
	text_lut[BAR_SCROLL] = ACS_VLINE;
	text_lut[BAR_SCROLL_POS] = ACS_DIAMOND;
	text_lut[BAR_UARROW] = ACS_TTEE;
	text_lut[BAR_DARROW] = ACS_BTEE;

	if (has_colors()) {
		start_color();

		color_lut[0] = COLOR_BLACK; // 0
		color_lut[AWS_FG_R] = COLOR_RED; // 1
		color_lut[AWS_FG_G] = COLOR_GREEN; // 2
		color_lut[AWS_FG_R | AWS_FG_G] = COLOR_YELLOW; // 3
		color_lut[AWS_FG_B] = COLOR_BLUE; // 4
		color_lut[AWS_FG_R | AWS_FG_B] = COLOR_MAGENTA; // 5
		color_lut[AWS_FG_G | AWS_FG_B] = COLOR_CYAN; // 6
		color_lut[AWS_FG_R | AWS_FG_G | AWS_FG_B] = COLOR_WHITE; // 7
		color_lut[AWS_FG_I | 0] = 8 + COLOR_BLACK; // 8
		color_lut[AWS_FG_I | AWS_FG_R] = 8 + COLOR_RED; // 9
		color_lut[AWS_FG_I | AWS_FG_G] = 8 + COLOR_GREEN; // 10
		color_lut[AWS_FG_I | AWS_FG_R | AWS_FG_G] = 8 + COLOR_YELLOW; // 11
		color_lut[AWS_FG_I | AWS_FG_B] = 8 + COLOR_BLUE; // 12
		color_lut[AWS_FG_I | AWS_FG_R | AWS_FG_B] = 8 + COLOR_MAGENTA; // 13
		color_lut[AWS_FG_I | AWS_FG_G | AWS_FG_B] = 8 + COLOR_CYAN; // 14
		color_lut[AWS_FG_I | AWS_FG_R | AWS_FG_G | AWS_FG_B] = 8 + COLOR_WHITE;
		    // 15
	}

	return (1);
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
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_SetTextPosition(int x_position, int y_position)
{
	cursor_x = x_position;
	cursor_y = y_position;

	move(cursor_y - 1, cursor_x - 1);

	wrefresh(stdscr);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_PaintScreen(char*buffer)
{
	int y, newYPos = 12345, newBottom = 56789;

	old_buffer = buffer;

	if (OptimizeScreen(buffer, original, &newYPos, &newBottom)) {
		//	  hide_cursor();
		for (y = newYPos; y <= newBottom; y++)
			OS_Paint_Line(buffer, y);
		show_cursor();
		wrefresh(stdscr);
	}

}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void OS_PaintStatus(char*buffer)
{
	//   	hide_cursor();

	OS_Paint_Line(buffer, screenYD - 1);

	show_cursor();

	wrefresh(stdscr);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void OS_Paint_Line(char*buffer, int line)
{
	int x, lineAddr;
	int optimizedX1, optimizedX2;
	int len;
	chtype attr;

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

	// Update original buffer line for ScreenOptimize routine
	lineAddr = (line*screenXD*SCR_PTR_SIZE) + (optimizedX1*SCR_PTR_SIZE);

	len = ((optimizedX2 - optimizedX1) + 1);

	memcpy(&original[lineAddr], &buffer[lineAddr], len*SCR_PTR_SIZE);

	for (x = 0; x < len; x++) {
		if (!(attr = attr_lut[(unsigned char)buffer[lineAddr + 1]]))
			attr = setup_color((unsigned char)buffer[lineAddr + 1]);

		line_text[x] = text_lut[(unsigned char)buffer[lineAddr]] | attr;
		lineAddr += SCR_PTR_SIZE;
	}

	mvaddchnstr(line, optimizedX1, line_text, len);
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
int OS_ScreenSize(int*xd, int*yd)
{
	*xd = screenXD;
	*yd = screenYD;

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
void OS_Exit(void)
{
	if (original)
		OS_Free(original);

	endwin();

	if (new_term) {
		tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
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

int GetKeyEvent(PE_KEY_EVENT*ev, struct timeval*timeout)
{
	int ch;

	(void)timeout;

	if ((ch = wgetch(stdscr)) != ERR) {
		ev->ch = ch;
		ev->escaped = (ch >= MY_KEYS);
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
int OS_GetKey(int*mode, OS_MOUSE*mouse)
{
	struct timeval timeout;
	PE_KEY_EVENT ev;
	int ch, clock_counter = 0;

	*mode = 1;

	mouse->buttonStatus = 0;
	mouse->moveStatus = 0;

	for (; ; ) {
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000000 / CURSOR_FLASH_PER_SEC;

		if (GetKeyEvent(&ev, &timeout)) {
			ch = ProcessEvent(mode, &ev, mouse);

			if (ch)
				return (ch);
		} else {
			if (++clock_counter >= CURSOR_FLASH_PER_SEC) {
				clock_counter = 0;
				if (clock_pfn)
					clock_pfn();
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
int OS_PeekKey(int*mode, OS_MOUSE*mouse)
{
	//PE_KEY_EVENT ev;

	//	if (GetKeyEvent(&ev, 0))
	//		return(ProcessEvent(mode, &ev, mouse);
	(void)mode;
	(void)mouse;

	return (0);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ProcessEvent(int*mode, PE_KEY_EVENT*ev, OS_MOUSE*mouse)
{

	(void)mouse;

	*mode = 1;

	switch (ev->ch) {
	case KEY_UP :
		return (ED_KEY_UP);
	case KEY_DOWN :
		return (ED_KEY_DOWN);
	case KEY_LEFT :
		return (ED_KEY_LEFT);
	case KEY_RIGHT :
		return (ED_KEY_RIGHT);
	case KEY_F(1) :
		return (ED_F1);
	case KEY_F(2) :
		return (ED_F2);
	case KEY_F(3) :
		return (ED_F3);
	case KEY_F(4) :
		return (ED_F4);
	case KEY_F(5) :
		return (ED_F5);
	case KEY_F(6) :
		return (ED_F6);
	case KEY_F(7) :
		return (ED_F7);
	case KEY_F(8) :
		return (ED_F8);
	case KEY_F(9) :
		return (ED_F9);
	case KEY_F(10) :
		return (ED_F10);
	case KEY_F(11) :
		return (ED_F11);
	case KEY_F(12) :
		return (ED_F12);
	case KEY_HOME :
		return (ED_KEY_HOME);
	case KEY_END :
		return (ED_KEY_END);
	case KEY_NPAGE :
		return (ED_KEY_PGDN);
	case KEY_PPAGE :
		return (ED_KEY_PGUP);
	case KEY_DC :
		return (ED_KEY_DELETE);
	case KEY_IC :
		return (ED_KEY_INSERT);

	case 1 :
		return (ED_ALT_A);
	case 2 :
		return (ED_ALT_B);
	case 3 :
		return (ED_ALT_C);
	case 4 :
		return (ED_ALT_D);
	case 5 :
		return (ED_ALT_E);
	case 6 :
		return (ED_ALT_F);
	case 7 :
		return (ED_ALT_G);
	case 8 :
		return (ED_ALT_H);
		//		case 9:
		//			return(ED_ALT_I);
		//		case 10:
		//			return(ED_ALT_J);
	case 11 :
		return (ED_ALT_K);
	case 12 :
		return (ED_ALT_L);
	case 13 :
		return (ED_ALT_M);
	case 14 :
		return (ED_ALT_N);
	case 15 :
		return (ED_ALT_O);
	case 16 :
		return (ED_ALT_P);
	case 17 :
		return (ED_ALT_Q);
	case 18 :
		return (ED_ALT_R);
	case 19 :
		return (ED_ALT_S);
	case 20 :
		return (ED_ALT_T);
	case 21 :
		return (ED_ALT_U);
	case 22 :
		return (ED_ALT_V);
	case 23 :
		return (ED_ALT_W);
	case 24 :
		return (ED_ALT_X);
	case 25 :
		return (ED_ALT_Y);
	case 26 :
		return (ED_ALT_Z);

	}

	*mode = 0;

	switch (ev->ch) {
	case 10 :
		return (ED_KEY_CR);
	case 27 :
		return (ED_KEY_ESC);
	case KEY_BACKSPACE :
		return (ED_KEY_BS);
	}

	if ((ev->ch < 32) || (ev->ch > 127))
		return (0);

	return (ev->ch);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_SetCursorType(int overstrike, int visible)
{
	cursor_os = overstrike;

	cursor_enabled = visible;

	if (!visible)
		hide_cursor();
	else
		show_cursor();

	wrefresh(stdscr);
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
	{BAR_ULCORNER, BAR_HORZ, BAR_URCORNER, BAR_LLCORNER, BAR_LRCORNER, BAR_VIRT,
		BAR_SCROLL, BAR_SCROLL_POS, BAR_DARROW, BAR_UARROW, 0, 0, 0, 0
	};

	return ((char)titles[index]);
}


