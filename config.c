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
#include <errno.h>
#include <stdlib.h>
#include "proedit.h"

/* Change version anytime the configuration options have changed. */
#define CONFIG_VERSION 25

extern int ignoreCase;
extern int globalSearch;
extern int createBackups;
extern int indenting;
extern int colorizing;
extern int merge_nows;
extern int tabsize;
extern int nospawn;
extern int force_crlf;
extern int hex_endian;
extern int auto_build_saveall;

static void ConfigStyleChange(int value);
static void ColorChange(int value);
static void ListChange(int value);
static void NumberChange(int value);

static char*yes_no_list[] = {"Yes", "No"};
static int yes_no_values[] = {1, 0};

static char*style_list[] = {"Default", "Monocrome", "Visual Studio",
	"Custom 1", "Custom 2", "Custom 3", "Custom 4"
};

static char*force_crlf_list[] = {"Maintain", "Force CR/LF", "Force Only LF"};
static int force_crlf_values[] = {MAINTAIN_CRLF, FORCE_CRLF, FORCE_LF};

static char*hex_endian_list[] = {"Default Machine", "Big Endian",
    "Little Endian"};
static int hex_endian_values[] = {HEX_SEARCH_DEFAULT,
	HEX_SEARCH_BIG_ENDIAN,
	HEX_SEARCH_LITTLE_ENDIAN
};

static int style_values[] = {0, 1, 2, 3, 4, 5, 6};

#define INDEX_YES 0
#define INDEX_NO  1

#define INDEX_STYLE_DEFAULT 0
#define INDEX_STYLE_MONO    1
#define INDEX_STYLE_MSDEV   2
#define INDEX_STYLE_CUST1   3
#define INDEX_STYLE_CUST2   4
#define INDEX_STYLE_CUST3   5
#define INDEX_STYLE_CUST4   6


#define INDEX_FG_COLOR_BLACK         0
#define INDEX_FG_COLOR_BLUE          1
#define INDEX_FG_COLOR_GREEN         2
#define INDEX_FG_COLOR_CYAN          3
#define INDEX_FG_COLOR_RED           4
#define INDEX_FG_COLOR_MAGENTA       5
#define INDEX_FG_COLOR_BROWN         6
#define INDEX_FG_COLOR_LIGHTGRAY     7
#define INDEX_FG_COLOR_DARKGRAY      8
#define INDEX_FG_COLOR_BRIGHTBLUE    9
#define INDEX_FG_COLOR_BRIGHTGREEN   10
#define INDEX_FG_COLOR_BRIGHTCYAN    11
#define INDEX_FG_COLOR_BRIGHTRED     12
#define INDEX_FG_COLOR_BRIGHTMAGENTA 13
#define INDEX_FG_COLOR_YELLOW        14
#define INDEX_FG_COLOR_WHITE         15

static char*color_list[] = {"Black", "Blue", "Green", "Cyan",
	"Red", "Magenta", "Brown", "Light Gray", "Dark Gray", "Bright Blue",
	"Bright Green", "Bright Cyan", "Bright Red", "Bright Magenta", "Yellow",
	"White"
};

static unsigned char fg_values[] =
{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

static unsigned char bg_values[] =
{0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0};

typedef void CONFIG_PFN(int value);

typedef struct config_number
{
	int key;
	int value;
	int min;
	int max;
	CONFIG_PFN*callback;
}CONFIG_NUMBER;

typedef struct config_list
{
	int key;
	int index;
	int num_values;
	char**strings;
	int*values;
	CONFIG_PFN*callback;
}CONFIG_LIST;

typedef struct config_color
{
	int key;
	int index;
	int num_values;
	char**strings;
	unsigned char*values;
	CONFIG_PFN*callback;
}CONFIG_COLOR;


CONFIG_NUMBER config_numbers_def[] =
{
	{CONFIG_INT_TABSIZE, 4, 1, 32, (CONFIG_PFN*)NumberChange}, /* N0 */
	{CONFIG_INT_DEFAULT_ROWS, 50, 25, 132, (CONFIG_PFN*)NumberChange}, /* N1 */
	{CONFIG_INT_DEFAULT_COLS, 80, 40, 160, (CONFIG_PFN*)NumberChange}, /* N2 */
	{CONFIG_INT_HEX_COLS, 16, 2, 256, (CONFIG_PFN*)NumberChange}, /* N3 */
	{CONFIG_INT_INDEX, -1, 0, 255, (CONFIG_PFN*)0}, /* N4 */
};

CONFIG_LIST config_lists_def[] =
{
	{CONFIG_INT_CASESENSITIVE, INDEX_NO, /* L0 */
	2, yes_no_list, yes_no_values, (CONFIG_PFN*)ListChange},

	{CONFIG_INT_GLOBALFILE, INDEX_NO,
	2, yes_no_list, yes_no_values, (CONFIG_PFN*)ListChange}, /* L1 */

	{CONFIG_INT_OVERSTRIKE, INDEX_NO,
	2, yes_no_list, yes_no_values, (CONFIG_PFN*)ListChange}, /* L2 */

	{CONFIG_INT_BACKUPS, INDEX_NO,
	2, yes_no_list, yes_no_values, (CONFIG_PFN*)ListChange}, /* L3 */

	{CONFIG_INT_AUTOINDENT, INDEX_YES,
	2, yes_no_list, yes_no_values, (CONFIG_PFN*)ListChange}, /* L4 */

	{CONFIG_INT_COLORIZING, INDEX_YES,
	2, yes_no_list, yes_no_values, (CONFIG_PFN*)ListChange}, /* L5 */

	{CONFIG_INT_STYLE, INDEX_STYLE_CUST1,
	7, style_list, style_values, (CONFIG_PFN*)ConfigStyleChange}, /* L6 */

	{CONFIG_INT_MERGEWS, INDEX_YES,
	2, yes_no_list, yes_no_values, (CONFIG_PFN*)ListChange}, /* L7 */

	{CONFIG_INT_FORCE_CRLF, 0,
	3, force_crlf_list, force_crlf_values, (CONFIG_PFN*)ListChange}, /* L8 */

	{CONFIG_INT_HEX_ENDIAN, 0,
	3, hex_endian_list, hex_endian_values, (CONFIG_PFN*)ListChange}, /* L9 */

	{CONFIG_INT_AUTO_SAVE_BUILD, INDEX_YES,
	2, yes_no_list, yes_no_values, (CONFIG_PFN*)ListChange}, /* L10 */

};


CONFIG_COLOR config_colors_def[] =
{
	{CONFIG_INT_FG_COLOR, INDEX_FG_COLOR_YELLOW, /* C0 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BG_COLOR, INDEX_FG_COLOR_BLUE, /* C1 */
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF1_FG_COLOR, INDEX_FG_COLOR_WHITE, /* C2 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF1_BG_COLOR, INDEX_FG_COLOR_GREEN, /* C3 */
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF2_FG_COLOR, INDEX_FG_COLOR_WHITE, /* C4 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF2_BG_COLOR, INDEX_FG_COLOR_RED, /* C5 */
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_PREPROCESSOR_COLOR, INDEX_FG_COLOR_BRIGHTGREEN, /* C6 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_NUMERIC_COLOR, INDEX_FG_COLOR_BRIGHTCYAN, /* C7 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_STRING_COLOR, INDEX_FG_COLOR_BRIGHTMAGENTA, /* C8 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_CHARACTER_COLOR, INDEX_FG_COLOR_BRIGHTMAGENTA, /* C9 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_OPERATOR_COLOR, INDEX_FG_COLOR_WHITE, /* C10 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_KEYWORD_COLOR, INDEX_FG_COLOR_WHITE, /* C11 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_COMMENTS_COLOR, INDEX_FG_COLOR_BRIGHTRED, /* C12 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BORDER_COLOR, INDEX_FG_COLOR_WHITE, /* C13 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_SELECT_FG_COLOR, INDEX_FG_COLOR_BLACK, /* C14 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_SELECT_BG_COLOR, INDEX_FG_COLOR_WHITE, /* C15 */
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOTTOM_FG_COLOR, INDEX_FG_COLOR_WHITE, /* C16 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOTTOM_BG_COLOR, INDEX_FG_COLOR_MAGENTA, /* C17 */
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOOKMARK_FG_COLOR, INDEX_FG_COLOR_WHITE, /* C18 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOOKMARK_BG_COLOR, INDEX_FG_COLOR_BLACK, /* C19 */
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DLG_FG_COLOR, INDEX_FG_COLOR_WHITE, /* C20 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DLG_BG_COLOR, INDEX_FG_COLOR_MAGENTA, /* C21 */
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXADDR_FG_COLOR, INDEX_FG_COLOR_BRIGHTCYAN, /* C22 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXBYTE_FG_COLOR, INDEX_FG_COLOR_YELLOW, /* C23 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXTEXT_FG_COLOR, INDEX_FG_COLOR_WHITE, /* C24 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HIGHLIGHT_FG_COLOR, INDEX_FG_COLOR_WHITE, /* C25 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_XML_COMMENTS_COLOR, INDEX_FG_COLOR_RED, /* C26 */
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

};

CONFIG_COLOR config_colors_mono[] =
{
	{CONFIG_INT_FG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF1_FG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF1_BG_COLOR, INDEX_FG_COLOR_LIGHTGRAY,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF2_FG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF2_BG_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_PREPROCESSOR_COLOR, INDEX_FG_COLOR_LIGHTGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_NUMERIC_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_STRING_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_CHARACTER_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_OPERATOR_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_KEYWORD_COLOR, INDEX_FG_COLOR_LIGHTGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_COMMENTS_COLOR, INDEX_FG_COLOR_LIGHTGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BORDER_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_SELECT_FG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_SELECT_BG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOTTOM_FG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOTTOM_BG_COLOR, INDEX_FG_COLOR_LIGHTGRAY,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOOKMARK_FG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOOKMARK_BG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DLG_FG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DLG_BG_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXADDR_FG_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXBYTE_FG_COLOR, INDEX_FG_COLOR_LIGHTGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXTEXT_FG_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HIGHLIGHT_FG_COLOR, INDEX_FG_COLOR_LIGHTGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_XML_COMMENTS_COLOR, INDEX_FG_COLOR_LIGHTGRAY,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

};

CONFIG_COLOR config_colors_msdev[] =
{
	{CONFIG_INT_FG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF1_FG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF1_BG_COLOR, INDEX_FG_COLOR_GREEN,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF2_FG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DIFF2_BG_COLOR, INDEX_FG_COLOR_RED,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_PREPROCESSOR_COLOR, INDEX_FG_COLOR_BRIGHTBLUE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_NUMERIC_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_STRING_COLOR, INDEX_FG_COLOR_BRIGHTRED,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_CHARACTER_COLOR, INDEX_FG_COLOR_BRIGHTRED,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_OPERATOR_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_KEYWORD_COLOR, INDEX_FG_COLOR_BRIGHTBLUE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_COMMENTS_COLOR, INDEX_FG_COLOR_GREEN,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BORDER_COLOR, INDEX_FG_COLOR_BLUE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_SELECT_FG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_SELECT_BG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOTTOM_FG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOTTOM_BG_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOOKMARK_FG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_BOOKMARK_BG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DLG_FG_COLOR, INDEX_FG_COLOR_WHITE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_DLG_BG_COLOR, INDEX_FG_COLOR_DARKGRAY,
	16, color_list, bg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXADDR_FG_COLOR, INDEX_FG_COLOR_BLACK,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXBYTE_FG_COLOR, INDEX_FG_COLOR_BRIGHTBLUE,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HEXTEXT_FG_COLOR, INDEX_FG_COLOR_BRIGHTRED,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_HIGHLIGHT_FG_COLOR, INDEX_FG_COLOR_BRIGHTRED,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

	{CONFIG_INT_XML_COMMENTS_COLOR, INDEX_FG_COLOR_BRIGHTGREEN,
	16, color_list, fg_values, (CONFIG_PFN*)ColorChange},

};

#define NUM_CONFIG_COLORS (sizeof(config_colors_def)/sizeof(CONFIG_COLOR))
#define NUM_CONFIG_NUMBERS (sizeof(config_numbers_def)/sizeof(CONFIG_NUMBER))
#define NUM_CONFIG_LISTS (sizeof(config_lists_def)/sizeof(CONFIG_LIST))


CONFIG_NUMBER config_numbers[NUM_CONFIG_NUMBERS];
CONFIG_LIST config_lists[NUM_CONFIG_LISTS];
CONFIG_COLOR config_colors[NUM_CONFIG_COLORS];

CONFIG_COLOR config_colors_custom1[NUM_CONFIG_COLORS];
CONFIG_COLOR config_colors_custom2[NUM_CONFIG_COLORS];
CONFIG_COLOR config_colors_custom3[NUM_CONFIG_COLORS];
CONFIG_COLOR config_colors_custom4[NUM_CONFIG_COLORS];

CONFIG_NUMBER config_numbers_custom1[NUM_CONFIG_NUMBERS];
CONFIG_NUMBER config_numbers_custom2[NUM_CONFIG_NUMBERS];
CONFIG_NUMBER config_numbers_custom3[NUM_CONFIG_NUMBERS];
CONFIG_NUMBER config_numbers_custom4[NUM_CONFIG_NUMBERS];

CONFIG_LIST config_lists_custom1[NUM_CONFIG_LISTS];
CONFIG_LIST config_lists_custom2[NUM_CONFIG_LISTS];
CONFIG_LIST config_lists_custom3[NUM_CONFIG_LISTS];
CONFIG_LIST config_lists_custom4[NUM_CONFIG_LISTS];

CONFIG_COLOR*style_colors[] = {config_colors_def,
	config_colors_mono,
	config_colors_msdev,
	config_colors_custom1,
	config_colors_custom2,
	config_colors_custom3,
	config_colors_custom4
};

CONFIG_NUMBER*style_numbers[] = {config_numbers_def,
	config_numbers_def,
	config_numbers_def,
	config_numbers_custom1,
	config_numbers_custom2,
	config_numbers_custom3,
	config_numbers_custom4
};

CONFIG_LIST*style_lists[] = {config_lists_def,
	config_lists_def,
	config_lists_def,
	config_lists_custom1,
	config_lists_custom2,
	config_lists_custom3,
	config_lists_custom4
};


typedef struct configData
{
	int size;
	int version;
	int config_int_total;
	int csum;
	int data[CONFIG_INT_TOTAL];
	int style;

	CONFIG_NUMBER config_numbers_custom1[NUM_CONFIG_NUMBERS];
	CONFIG_NUMBER config_numbers_custom2[NUM_CONFIG_NUMBERS];
	CONFIG_NUMBER config_numbers_custom3[NUM_CONFIG_NUMBERS];
	CONFIG_NUMBER config_numbers_custom4[NUM_CONFIG_NUMBERS];

	CONFIG_LIST config_lists_custom1[NUM_CONFIG_LISTS];
	CONFIG_LIST config_lists_custom2[NUM_CONFIG_LISTS];
	CONFIG_LIST config_lists_custom3[NUM_CONFIG_LISTS];
	CONFIG_LIST config_lists_custom4[NUM_CONFIG_LISTS];

	CONFIG_COLOR config_colors_custom1[NUM_CONFIG_COLORS];
	CONFIG_COLOR config_colors_custom2[NUM_CONFIG_COLORS];
	CONFIG_COLOR config_colors_custom3[NUM_CONFIG_COLORS];
	CONFIG_COLOR config_colors_custom4[NUM_CONFIG_COLORS];
}CONFIG_DATA;

static char*config_list[] = {
	"",
	" Editor Options:",
	"",
	" Tab Size                     : $N0                ",
	" Case Sensitive Search        : $L0                ",
	" Global File Search           : $L1                ",
	" Default Overstrike On        : $L2                ",
	" Default Columns              : $N2                ",
	" Default Rows                 : $N1                ",
	" Create Backup Files          : $L3                ",
	" Auto Indent Text             : $L4                ",
	" Auto content Colorizing      : $L5                ",
	" CR/LF handling               : $L8                ",
	" ",
	" Color Options:",
	"",
	" Default Style                : $L6                ",
	" Text: Forground Color        : $C0                ",
	" Text: Background Color       : $C1                ",
	" Border Color                 : $C13               ",
	" Highlight: Forground Color   : $C14               ",
	" Highlight: Background Color  : $C15               ",
	" Status Bar: Forground Color  : $C16               ",
	" Status Bar: Background Color : $C17               ",
	" Bookmark: Forground Color    : $C18               ",
	" Bookmark: Background Color   : $C19               ",
	" Dialog Box: Forground Color  : $C20               ",
	" Dialog Box: Background Color : $C21               ",
	" Build Error Forground Color  : $C25               ",
	"",
	" Auto Content Colorizing Colors:",
	"",
	" Preprocessor Directives      : $C6                ",
	" Numeric Values               : $C7                ",
	" String Constants             : $C8                ",
	" Character Constants          : $C9                ",
	" Operators                    : $C10               ",
	" Keywords                     : $C11               ",
	" Comments                     : $C12               ",
	" XML                          : $C26               ",
	"",
	" Hex Editor Options:",
	"",
	" Number of Columns            : $N3                ",
	" Hex Search Endianess         : $L9                ",
	" Address Color                : $C22               ",
	" Byte Color                   : $C23               ",
	" Text Color                   : $C24               ",
	"",
	" File Merge Options:",
	"",
	" Ignore Whitespace in Files   : $L7                ",
	" Diff File1 Forground Color   : $C2                ",
	" Diff File1 Background Color  : $C3                ",
	" Diff File2 Forground Color   : $C4                ",
	" Diff File2 Background Color  : $C5                ",
	"",
	" Build/Shell Options:",
	"",
	" Auto Save Files before build : $L10               ",
	"",
	" Press [F1] to load Defaults.",
	"",
	" Press [Left]/[Right] Arrows to change values.",
	" Press [Up]/[Down] Arrows to change options.",
};

#define NUMBER_CONFIG_LINES_TEXT (sizeof(config_list)/sizeof(char *))

static int ConfigLine(char*in, char*out, int*offset, int*size, int*index);

static void DefaultConfig(void);
static void DefaultStyleConfig(void);

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int GetConfigInt(int item)
{
	int i;

	for (i = 0; i < (int)NUM_CONFIG_COLORS; i++)
		if (config_colors[i].key == item)
			return (config_colors[i].values[config_colors[i].index]);

	for (i = 0; i < (int)NUM_CONFIG_NUMBERS; i++)
		if (config_numbers[i].key == item)
			return (config_numbers[i].value);

	for (i = 0; i < (int)NUM_CONFIG_LISTS; i++)
		if (config_lists[i].key == item)
			return (config_lists[i].values[config_lists[i].index]);

	OS_Printf("GetConfigInt(): Failed to resolve config item %d\n", item);
	OS_Exit();
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetConfigInt(int item, int value)
{
	int i, j;

	for (i = 0; i < (int)NUM_CONFIG_COLORS; i++)
		if (config_colors[i].key == item)
			for (j = 0; j < config_colors[i].num_values; j++)
				if (config_colors[i].values[j] == value) {
					config_colors[i].index = j;
					return ;
				}

	for (i = 0; i < (int)NUM_CONFIG_LISTS; i++)
		if (config_lists[i].key == item)
			for (j = 0; j < config_lists[i].num_values; j++)
				if (config_lists[i].values[j] == value) {
					config_lists[i].index = j;
					return ;
				}

	for (i = 0; i < (int)NUM_CONFIG_NUMBERS; i++)
		if (config_numbers[i].key == item) {
			config_numbers[i].value = value;
			return ;
		}

	OS_Printf("SetConfigInt(): Failed to resolve config item %d\n", item);
	OS_Exit();
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void LoadConfig(void)
{
	int i, len, csum = 0;
	CONFIG_DATA*data;

	DefaultConfig();

	data = (CONFIG_DATA*)OS_LoadSession(&len, SESSION_CONFIG, 0);

	if (data) {
		if (len == sizeof(CONFIG_DATA)) {
			if ((data->version == CONFIG_VERSION) && (data->size == sizeof(
			    CONFIG_DATA)) && (data->config_int_total == CONFIG_INT_TOTAL)) {
				for (i = 0; i < CONFIG_INT_TOTAL; i++)
					csum += data->data[i];

				if (csum == data->csum) {
					for (i = 0; i < (int)NUM_CONFIG_COLORS; i++) {
						config_colors_custom1[i].index =
						data->config_colors_custom1[i].index;

						config_colors_custom2[i].index =
						data->config_colors_custom2[i].index;

						config_colors_custom3[i].index =
						data->config_colors_custom3[i].index;

						config_colors_custom4[i].index =
						data->config_colors_custom4[i].index;

					}

					for (i = 0; i < (int)NUM_CONFIG_LISTS; i++) {
						config_lists_custom1[i].index =
						data->config_lists_custom1[i].index;

						config_lists_custom2[i].index =
						data->config_lists_custom2[i].index;

						config_lists_custom3[i].index =
						data->config_lists_custom3[i].index;

						config_lists_custom4[i].index =
						data->config_lists_custom4[i].index;

					}

					for (i = 0; i < (int)NUM_CONFIG_NUMBERS; i++) {
						config_numbers_custom1[i].value =
						data->config_numbers_custom1[i].value;

						config_numbers_custom2[i].value =
						data->config_numbers_custom2[i].value;

						config_numbers_custom3[i].value =
						data->config_numbers_custom3[i].value;

						config_numbers_custom4[i].value =
						data->config_numbers_custom4[i].value;

					}

					for (i = 0; i < CONFIG_INT_TOTAL; i++)
						SetConfigInt(i, data->data[i]);

					SetupColors();
					SetupConfig();
					OS_FreeSession(data);
					return ;
				}
			}
		}
		OS_FreeSession(data);
	}

	SaveConfig();
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SaveConfig(void)
{
	int i, csum = 0;
	CONFIG_DATA data;

	for (i = 0; i < CONFIG_INT_TOTAL; i++) {
		data.data[i] = GetConfigInt(i);
		csum += data.data[i];
	}

	data.version = CONFIG_VERSION;
	data.size = sizeof(CONFIG_DATA);
	data.csum = csum;
	data.config_int_total = CONFIG_INT_TOTAL;
	data.style = GetConfigInt(CONFIG_INT_STYLE);

	memcpy(data.config_colors_custom1, config_colors_custom1, sizeof(
	    config_colors));
	memcpy(data.config_colors_custom2, config_colors_custom2, sizeof(
	    config_colors));
	memcpy(data.config_colors_custom3, config_colors_custom3, sizeof(
	    config_colors));
	memcpy(data.config_colors_custom4, config_colors_custom4, sizeof(
	    config_colors));

	memcpy(data.config_lists_custom1, config_lists_custom1,
	    sizeof(config_lists));
	memcpy(data.config_lists_custom2, config_lists_custom2,
	    sizeof(config_lists));
	memcpy(data.config_lists_custom3, config_lists_custom3,
	    sizeof(config_lists));
	memcpy(data.config_lists_custom4, config_lists_custom4,
	    sizeof(config_lists));

	memcpy(data.config_numbers_custom1, config_numbers_custom1, sizeof(
	    config_numbers));
	memcpy(data.config_numbers_custom2, config_numbers_custom2, sizeof(
	    config_numbers));
	memcpy(data.config_numbers_custom3, config_numbers_custom3, sizeof(
	    config_numbers));
	memcpy(data.config_numbers_custom4, config_numbers_custom4, sizeof(
	    config_numbers));

	OS_SaveSession(&data, sizeof(data), SESSION_CONFIG, 0);

	SetupColors();
	SetupConfig();
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DefaultConfig(void)
{
	memcpy(config_numbers, config_numbers_def, sizeof(config_numbers));
	memcpy(config_lists, config_lists_def, sizeof(config_lists));
	memcpy(config_colors, config_colors_def, sizeof(config_colors));

	memcpy(config_colors_custom1, config_colors_def, sizeof(config_colors));
	memcpy(config_colors_custom2, config_colors_def, sizeof(config_colors));
	memcpy(config_colors_custom3, config_colors_def, sizeof(config_colors));
	memcpy(config_colors_custom4, config_colors_def, sizeof(config_colors));

	memcpy(config_lists_custom1, config_lists_def, sizeof(config_lists));
	memcpy(config_lists_custom2, config_lists_def, sizeof(config_lists));
	memcpy(config_lists_custom3, config_lists_def, sizeof(config_lists));
	memcpy(config_lists_custom4, config_lists_def, sizeof(config_lists));

	memcpy(config_numbers_custom1, config_numbers_def, sizeof(config_numbers));
	memcpy(config_numbers_custom2, config_numbers_def, sizeof(config_numbers));
	memcpy(config_numbers_custom3, config_numbers_def, sizeof(config_numbers));
	memcpy(config_numbers_custom4, config_numbers_def, sizeof(config_numbers));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DefaultStyleConfig(void)
{
	int style;

	style = GetConfigInt(CONFIG_INT_STYLE);

	switch (style) {
	case INDEX_STYLE_CUST1 :
		{
			memcpy(config_lists_custom1, config_lists_def,
			    sizeof(config_lists));

			memcpy(config_numbers_custom1, config_numbers_def, sizeof(
			    config_numbers));

			memcpy(config_colors_custom1, config_colors_def, sizeof(
			    config_colors));

			break;
		}

	case INDEX_STYLE_CUST2 :
		{
			memcpy(config_lists_custom2, config_lists_def,
			    sizeof(config_lists));

			memcpy(config_numbers_custom2, config_numbers_def, sizeof(
			    config_numbers));

			memcpy(config_colors_custom2, config_colors_def, sizeof(
			    config_colors));

			break;
		}

	case INDEX_STYLE_CUST3 :
		{
			memcpy(config_lists_custom3, config_lists_def,
			    sizeof(config_lists));

			memcpy(config_numbers_custom3, config_numbers_def, sizeof(
			    config_numbers));

			memcpy(config_colors_custom3, config_colors_def, sizeof(
			    config_colors));

			break;
		}

	case INDEX_STYLE_CUST4 :
		{
			memcpy(config_lists_custom4, config_lists_def,
			    sizeof(config_lists));

			memcpy(config_numbers_custom4, config_numbers_def, sizeof(
			    config_numbers));

			memcpy(config_colors_custom4, config_colors_def, sizeof(
			    config_colors));

			break;
		}

	}

	memcpy(config_colors, style_colors[style], sizeof(config_colors));
	memcpy(config_numbers, style_numbers[style], sizeof(config_numbers));
	memcpy(config_lists, style_lists[style], sizeof(config_lists));

	SetConfigInt(CONFIG_INT_STYLE, style);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ConfigLine(char*in, char*out, int*offset, int*size, int*index)
{
	int i, val = 0, type = 0;
	int len, j = 0;

	len = strlen(in);

	memset(out, 32, len);

	for (i = 0; i < len; i++) {
		out[i] = in[i];

		if (in[i] == '$') {
			type = in[i + 1];

			if (offset)
				*offset = i + 1;

			if (in[i + 1] == 'N') {
				val = atoi(&in[i + 2]);
				j = sprintf(&out[i], "[%d]", config_numbers[val].value);
			}
			if (in[i + 1] == 'C') {
				val = atoi(&in[i + 2]);
				j = sprintf(&out[i], "[%s]", config_colors[val].strings[
				    config_colors[val].index]);
			}
			if (in[i + 1] == 'L') {
				val = atoi(&in[i + 2]);
				j = sprintf(&out[i], "[%s]", config_lists[val].strings[
				    config_lists[val].index]);
			}
			type = in[i + 1];

			if (index)
				*index = val;

			if (size)
				*size = j - 2;

			i += (j - 1);
		}
	}
	out[i] = 0;
	return (type);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void Configure(EDIT_FILE*file)
{
	OS_MOUSE mouse;
	int i, x, y, xd, yd, ch, mode;
	EDIT_FILE*config = 0;
	EDIT_LINE*current = 0;
	char line[256], type;
	int num_options = 0, offset = 0, len = 0, index = 0;
	int options[256], option_index;
	CURSOR_SAVE cursor;

	config = AllocFile("ProEdit Configuration Options");

	xd = 0;
	yd = NUMBER_CONFIG_LINES_TEXT + 3;

	option_index = GetConfigInt(CONFIG_INT_INDEX);

	for (i = 0; i < (int)NUMBER_CONFIG_LINES_TEXT; i++) {
		if ((type = (char)ConfigLine(config_list[i], line, 0, 0, &index))) {
			if (type == 'L' && config_lists[index].key == CONFIG_INT_STYLE) {
				if (option_index == -1)
					option_index = num_options;
			}

			options[num_options++] = i;
		}

		current = AddFileLine(config, current, line, strlen(line), 0);
		if ((int)strlen(config_list[i]) > xd)
			xd = strlen(config_list[i]);
	}

	InitDisplayFile(config);

	if (yd > file->display.yd - 3)
		yd = file->display.yd - 3;
	else
		config->scrollbar = 0;

	xd += 4;

	if (xd > file->display.xd - 2)
		xd = file->display.xd - 2;

	x = file->display.xd / 2 - xd / 2;
	y = (file->display.yd - 1) / 2 - yd / 2;

	config->display.xpos = x;
	config->display.ypos = y;
	config->display.xd = xd;
	config->display.yd = yd;
	config->display.columns = xd - 2;
	config->display.rows = yd - 3;

	config->hideCursor = 1;
	config->client = 2;
	config->border = 2;

	config->file_flags |= FILE_FLAG_NONFILE;

	InitCursorFile(config);

	CursorTopFile(config);

	config->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	for (; ; ) {
		type = (char)ConfigLine(config_list[options[option_index]], line, &
		    offset, &len, &index);

		GotoPosition(config, options[option_index]+1, offset + 1);

		memcpy(config->cursor.line->line, line, config->cursor.line->len);

		SetupSelectBlock(config, options[option_index], offset, len);

		Paint(config);

		ch = OS_Key(&mode, &mouse);

		if (mode) {
			if (ch == ED_ALT_X)
				break;

			switch (ch) {
			case ED_KEY_END :
			case ED_KEY_PGDN :
				option_index = num_options - 1;
				break;

			case ED_KEY_HOME :
			case ED_KEY_PGUP :
				option_index = 0;
				break;

			case ED_KEY_UP :
				if (option_index)
					option_index--;
				break;

			case ED_KEY_DOWN :
				if (option_index < num_options - 1)
					option_index++;
				break;

			case ED_F1 :
				DefaultStyleConfig();
				for (i = 0; i < num_options; i++) {
					SaveCursor(config, &cursor);
					ConfigLine(config_list[options[i]], line, 0, 0, 0);
					GotoPosition(config, options[i]+1, 1);
					memcpy(config->cursor.line->line, line, config->cursor.
					    line->len);
					SetCursor(config, &cursor);
				}
				EnablePainting(0);
				SetupColors();
				PaintFrame(file);
				PaintContent(file);
				UpdateStatusBar(file);
				config->paint_flags |= CONTENT_FLAG | FRAME_FLAG;
				EnablePainting(1);
				break;

			case ED_KEY_LEFT :
			case ED_KEY_RIGHT :
				{
					switch (type) {
					case 'N' :
						if (ch == ED_KEY_LEFT) {
							if (config_numbers[index].value > config_numbers[
							    index].min)
								config_numbers[index].value--;
						} else {
							if (config_numbers[index].value < config_numbers[
							    index].max)
								config_numbers[index].value++;
						}
						if (config_numbers[index].callback)
							config_numbers[index].callback(index);

						break;

					case 'C' :
						if (ch == ED_KEY_LEFT) {
							if (!config_colors[index].index)
								config_colors[index].index =
								config_colors[index].num_values - 1;
							else
								config_colors[index].index--;
						} else {
							if (config_colors[index].index == config_colors[
							    index].num_values - 1)
								config_colors[index].index = 0;
							else
								config_colors[index].index++;
						}
						if (config_colors[index].callback)
							config_colors[index].callback(index);

						EnablePainting(0);
						SetupColors();
						PaintFrame(file);
						PaintContent(file);
						UpdateStatusBar(file);
						config->paint_flags |= CONTENT_FLAG | FRAME_FLAG;
						EnablePainting(1);
						break;

					case 'L' :
						if (ch == ED_KEY_LEFT) {
							if (!config_lists[index].index)
								config_lists[index].index =
								config_lists[index].num_values - 1;
							else
								config_lists[index].index--;
						} else {
							if (config_lists[index].index ==
							    config_lists[index].num_values - 1)
								config_lists[index].index = 0;
							else
								config_lists[index].index++;
						}
						if (config_lists[index].callback)
							config_lists[index].callback(index);

						if (config_lists[index].key == CONFIG_INT_STYLE) {
							for (i = 0; i < num_options; i++) {
								SaveCursor(config, &cursor);
								ConfigLine(config_list[options[i]], line, 0, 0,
								    0);
								GotoPosition(config, options[i]+1, 1);
								memcpy(config->cursor.line->line, line,
								    config->cursor.line->len);
								SetCursor(config, &cursor);
							}
							EnablePainting(0);
							SetupColors();
							PaintFrame(file);
							PaintContent(file);
							UpdateStatusBar(file);
							config->paint_flags |= CONTENT_FLAG | FRAME_FLAG;
							EnablePainting(1);
						}
						break;
					}
					config->paint_flags |= CONTENT_FLAG;
					break;
				}
			}
		} else {
			if (ch == ED_KEY_ESC || ch == ED_KEY_CR)
				break;
		}
	}

	SetConfigInt(CONFIG_INT_INDEX, option_index);

	DeallocFile(config);

	SaveConfig();

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ToggleColorize(EDIT_FILE*file)
{
	if (file->colorize) {
		CenterBottomBar(1, "[-] Colorizing OFF [-]");
		file->colorize = 0;
	} else {
		file->colorize = ConfigColorize(file, file->pathname);

		if (file->colorize)
			CenterBottomBar(1, "[+] Colorizing ON [+]");
		else
			CenterBottomBar(1, "[-] Colorizing not supported [-]");
	}

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ToggleWordWrap(EDIT_FILE*file)
{
	/* Don't wordwrap non-files. */
	if (file->file_flags&FILE_FLAG_NONFILE)
		return ;

	if (file->wordwrap) {
		DeWordWrapFile(file);
		CenterBottomBar(1, "[-] Word Wrap OFF [-]");
	} else {
		WordWrapFile(file);
		CenterBottomBar(1, "[+] Word Wrap ON [+]");
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ToggleDisplaySpecial(EDIT_FILE*file)
{
	if (file->display_special) {
		CenterBottomBar(1, "[-] Formatting Characters OFF [-]");
		file->display_special = 0;
	} else {
		CenterBottomBar(1, "[+] Formatting Characters ON [+]");
		file->display_special = ED_SPECIAL_ALL;
	}

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void Overstrike(EDIT_FILE*file)
{
	if (file->overstrike) {
		CenterBottomBar(1, "[+] Insert Mode ON [+]");
		file->overstrike = 0;
	} else {
		CenterBottomBar(1, "[+] Overstrike Mode ON [+]");
		file->overstrike = 1;
	}

	file->paint_flags |= CURSOR_FLAG;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetupConfig(void)
{
	ignoreCase = !GetConfigInt(CONFIG_INT_CASESENSITIVE);
	globalSearch = GetConfigInt(CONFIG_INT_GLOBALFILE);
	createBackups = GetConfigInt(CONFIG_INT_BACKUPS);
	indenting = GetConfigInt(CONFIG_INT_AUTOINDENT);
	colorizing = GetConfigInt(CONFIG_INT_COLORIZING);
	merge_nows = GetConfigInt(CONFIG_INT_MERGEWS);
	tabsize = GetConfigInt(CONFIG_INT_TABSIZE);
	force_crlf = GetConfigInt(CONFIG_INT_FORCE_CRLF);
	hex_endian = GetConfigInt(CONFIG_INT_HEX_ENDIAN);
	auto_build_saveall = GetConfigInt(CONFIG_INT_AUTO_SAVE_BUILD);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ConfigStyleChange(int value)
{
	int style;

	style = config_lists[value].index;

	if (config_lists[value].key == CONFIG_INT_STYLE) {
		memcpy(config_colors, style_colors[style], sizeof(config_colors));
		memcpy(config_numbers, style_numbers[style], sizeof(config_numbers));
		memcpy(config_lists, style_lists[style], sizeof(config_lists));

		SetupColors();
	}

	config_lists[value].index = style;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ColorChange(int value)
{
	switch (GetConfigInt(CONFIG_INT_STYLE)) {
	case INDEX_STYLE_CUST1 :
		config_colors_custom1[value].index = config_colors[value].index;
		break;

	case INDEX_STYLE_CUST2 :
		config_colors_custom2[value].index = config_colors[value].index;
		break;

	case INDEX_STYLE_CUST3 :
		config_colors_custom3[value].index = config_colors[value].index;
		break;

	case INDEX_STYLE_CUST4 :
		config_colors_custom4[value].index = config_colors[value].index;
		break;
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void NumberChange(int value)
{
	switch (GetConfigInt(CONFIG_INT_STYLE)) {
	case INDEX_STYLE_CUST1 :
		config_numbers_custom1[value].value = config_numbers[value].value;
		break;

	case INDEX_STYLE_CUST2 :
		config_numbers_custom2[value].value = config_numbers[value].value;
		break;

	case INDEX_STYLE_CUST3 :
		config_numbers_custom3[value].value = config_numbers[value].value;
		break;

	case INDEX_STYLE_CUST4 :
		config_numbers_custom4[value].value = config_numbers[value].value;
		break;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ListChange(int value)
{
	switch (GetConfigInt(CONFIG_INT_STYLE)) {
	case INDEX_STYLE_CUST1 :
		config_lists_custom1[value].index = config_lists[value].index;
		break;

	case INDEX_STYLE_CUST2 :
		config_lists_custom2[value].index = config_lists[value].index;
		break;

	case INDEX_STYLE_CUST3 :
		config_lists_custom3[value].index = config_lists[value].index;
		break;

	case INDEX_STYLE_CUST4 :
		config_lists_custom4[value].index = config_lists[value].index;
		break;
	}
}



