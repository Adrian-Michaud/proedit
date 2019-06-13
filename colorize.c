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
#include "color_c.h"
#include "color_v.h"
#include "color_cs.h"
#include "color_html.h"

extern int colorizing;

typedef void COLORIZE_SETUP_PFN(void);
typedef void LINE_COLOR_PFN(EDIT_LINE*line);

typedef struct colorizingFile
{
	char*fileExtension;
	COLORIZE_PFN*pfn;
	COLORIZE_SETUP_PFN*setup;
	LINE_PFN*linePfn;
	LINE_COLOR_PFN*colorize;
}FILE_COLORIZE;

FILE_COLORIZE colorizingSupport[] =
{
	{".cs", Colorize_CS, SetupColors_CS, (LINE_PFN*)Colorize_CS_LinePfn,
	    ColorLines_CS},
	{".c#", Colorize_CS, SetupColors_CS, (LINE_PFN*)Colorize_CS_LinePfn,
	    ColorLines_CS},
	{".c", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".h", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".cpp", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".cc", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".i", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".ic", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".cxx", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".hpp", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".hxx", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".g++", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".c++", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".h++", Colorize_C, SetupColors_C, (LINE_PFN*)Colorize_C_LinePfn,
	    ColorLines_C},
	{".v", Colorize_V, SetupColors_V, (LINE_PFN*)Colorize_V_LinePfn,
	    ColorLines_V},
	{".htm", Colorize_Html, SetupColors_Html, (LINE_PFN*)Colorize_Html_LinePfn,
	    ColorLines_Html},
	{".aspx", Colorize_Html, SetupColors_Html, (LINE_PFN*)Colorize_Html_LinePfn,
	    ColorLines_Html},
	{".html", Colorize_Html, SetupColors_Html, (LINE_PFN*)Colorize_Html_LinePfn,
	    ColorLines_Html},
	{".css", Colorize_Html, SetupColors_Html, (LINE_PFN*)Colorize_Html_LinePfn,
	    ColorLines_Html},
};

#define NUMBER_FILE_COLORIZE (sizeof(colorizingSupport)/sizeof(FILE_COLORIZE))


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SetupColorizers(void)
{
	int i;

	for (i = 0; i < (int)NUMBER_FILE_COLORIZE; i++)
		colorizingSupport[i].setup();
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
COLORIZE_PFN*ConfigColorize(EDIT_FILE*file, char*pathname)
{
	char filename[MAX_FILENAME];
	int i, len, j;

	if (file->file_flags&(FILE_FLAG_NONFILE | FILE_FLAG_NO_COLORIZE))
		return (0);

	OS_GetFilename(pathname, 0, filename);

	len = strlen(filename);

	for (i = len - 1; i >= 0; i--) {
		if (filename[i] == '.') {
			for (j = 0; j < (int)NUMBER_FILE_COLORIZE; j++)
				if (!OS_Strcasecmp(&filename[i], colorizingSupport[j].
				    fileExtension)) {
					AddLineCallback(file, colorizingSupport[j].linePfn,
					    LINE_OP_EDIT | LINE_OP_DELETE | LINE_OP_INSERT);

					colorizingSupport[j].colorize(file->lines);
					return (colorizingSupport[j].pfn);
				}
		}
	}
	return (0);
}





