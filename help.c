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
#include "proedit.h"

char*helpText[] = {
	"B",
	"B $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
	"B $                                                                      $",
	"B $                             ProEdit MP 2.1                           $",
	"B $                                                                      $",
	"B $                    Multi-Platform Programming Editor                 $",
	"B $                                                                      $",
	"B $            Designed/Developed/Produced By Adrian J. Michaud          $",
	"B $                           x86guru@yahoo.com                          $",
	"B $                                                                      $",
	"B $                     (C) 2019. All rights reserved                    $",
	"B $                                                                      $",
	"B $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
	"B",
	"B",
	"T                    ProEdit MP Text Editing Mode Help",
	"H                     ProEdit MP HEX Editor Mode Help",
	"B                   -----------------------------------",
	"B",
	"B                          Alt and Control KEYS:",
	"B                $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
	"T                $ Q $ W $ E $ R $ T $ Y $ U $ I $ O $ P $",
	"H                $ Q $ W $ E $ R $ T $   $ U $ I $   $   $",
	"B                $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
	"T                  $ A $ S $ D $ F $ G $ H $ J $ K $ L $",
	"H                  $ A $ S $ D $ F $ G $ H $   $   $ L $",
	"B              $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
	"T              $ SHIFT $ Z $ X $ C $ V $ B $ N $ M $ SHIFT $",
	"H              $ SHIFT $ Z $ X $ C $ V $ B $ N $   $ SHIFT $",
	"B              $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
	"B               $ CTRL $ ALT $               $ ALT $ CTRL $",
	"B               $$$$$$$$$$$$$$               $$$$$$$$$$$$$$",
	"B",
	"BSHIFT:  Pressing shift and moving the cursor enables text selection",
	"BCTRL-C: Copy Selected text into the clipboard (Also See INSERT)",
	"BCTRL-V: Paste the contents of the clipboard (Also See INSERT)",
	"TALT-C:  Toggle line-mode (row) text selection.",
	"HALT-C:  Toggle text/hex selection.",
	"HALT-V:  Same as ALT-C.",
	"TALT-V:  Toggle block-mode (row and column) text selection.",
	"BALT-E:  Open a new file.",
	"BALT-F:  Display a list of files currently loaded.",
	"BALT-Q:  Quit the current file and unload it from the editor.",
	"BALT-N:  Switch to the next file.",
	"BALT-T:  Change the current file name/path",
	"BALT-W:  Save current file.",
	"BALT-A:  Save current file with a new name.",
	"TALT-P:  Place/Remove a bookmark at current line/column.",
	"TALT-J:  Jump to the next bookmark.",
	"TALT-Y:  Merge Text File(s).",
	"BALT-Z:  Run a Build/Command Shell (See Also: ALT-B)",
	"BALT-B:  Re-Build (Saves Modified files, and re-runs last Build/Command)",
	"TALT-O:  Perform Special Operation on file or Selected text.",
	"BALT-D:  Delete the current line.",
	"TALT-G:  Goto a line/column within the current file.",
	"HALT-G:  Goto an address within the current file.",
	"TALT-I:  Toggle Insert/Overstrike mode.",
	"HALT-I:  Toggle Insert/Overstrike mode (If Hex Insert Mode is enabled).",
	"TALT-K:  Delete text from the cursor position to the end of the line.",
	"BALT-U:  Undo the last change made to the current file.",
	"BCTRL-Z: Undo the last change made (Same as ALT-U).",
	"TALT-S:  Search for text. (Also See ALT-L)",
	"HALT-S:  Search for hex/text. (Also See ALT-L)",
	"BALT-L:  Search again. (Also See ALT-S)",
	"TALT-M:  Find matching operator: {} [] () <>",
	"BALT-R:  Search and replace text.",
	"BALT-X:  Exit the editor.",
	"BALT-H:  Display this help document.",
	"B",
	"B",
	"B                          Center Keypad Keys:",
	"B                     $$$$$$$$$$$$$$$$$$$$$$$$$$$",
	"B                     $                         $",
	"B                     $ $$$$$$$$$$$$$$$$$$$$$$$ $",
	"T                     $ $  Ins $ Home $ Pgup  $ $",
	"H                     $ $      $ Home $ Pgup  $ $",
	"B                     $ $$$$$$$$$$$$$$$$$$$$$$$ $",
	"T                     $ $  Del $ End  $ Pgdn  $ $",
	"H                     $ $      $ End  $ Pgdn  $ $",
	"B                     $ $$$$$$$$$$$$$$$$$$$$$$$ $",
	"B                     $                         $",
	"B                     $         $$$$$$          $",
	"B                     $         $ Up $          $",
	"B                     $ $$$$$$$$$$$$$$$$$$$$$$$ $",
	"B                     $ $ Left $ Down $ Right $ $",
	"B                     $ $$$$$$$$$$$$$$$$$$$$$$$ $",
	"B                     $                         $",
	"B                     $$$$$$$$$$$$$$$$$$$$$$$$$$$",
	"B",
	"T",
	"TClipboard control keys:",
	"T-----------------------",
	"T",
	"TINSERT Key:",
	"T",
	"T   Insert text into clipboard or into file:",
	"T    -If text is highlighted, the selected text gets copied",
	"T     into the ProEdit clipboard and the system clipboard.",
	"T    -If no text is highlighted and the ProEdit clipboard contains",
	"T     data then it will get pasted at the cursor position.",
	"T",
	"TCTRL-INSERT or ALT-INSERT Key:",
	"T",
	"T   Insert text from the system clipboard. If the system clipboard",
	"T   contains data then it will get pasted at the cursor position.",
	"T",
	"TDELETE Key:",
	"T",
	"T   Delete text into clipboard or delete a character:",
	"T    -If text is highlighted, the selected text gets deleted",
	"T     from the file and copied into the ProEdit clipboard.",
	"T    -If no text is highlighted, the character at the cursor",
	"T     position gets deleted.",
	"B",
	"B",
	"TCursor control keys:",
	"T--------------------",
	"HHex Cursor control keys:",
	"H------------------------",
	"B",
	"BHOME:       Move the cursor to the home position:",
	"B             -1st depress - Move cursor to beginning of the line.",
	"B             -2nd depress - Move cursor to top of the page.",
	"B             -3rd depress - Move cursor to top of the file.",
	"BEND:        Move the cursor to the end:",
	"B             -1st depress - Move cursor to the end of the line.",
	"B             -2nd depress - Move cursor to bottom of the page.",
	"B             -3rd depress - Move cursor to end of the file.",
	"HTAB:        Toggle Between Hex/Text mode",
	"BPGUP:       Move the cursor up one page.",
	"BPGDN:       Move the cursor down one page.",
	"BUP:         Move cursor up one line.",
	"BDOWN:       Move cursor down one line.",
	"TLEFT:       Move cursor left one column (pan if necessary).",
	"TRIGHT:      Move cursor right one column (pan if necessary).",
	"HLEFT:       Move cursor left one hex nibble or character.",
	"HRIGHT:      Move cursor right one hex nibble or character.",
	"BCTRL-UP:    Scroll display up one line.",
	"BCTRL-DOWN:  Scroll display down one line.",
	"TCTRL-LEFT:  Move cursor to the next word on the left.",
	"TCTRL-RIGHT: Move cursor to the next word on the right.",
	"HCTRL-LEFT:  Move cursor left to the next byte or character.",
	"HCTRL-RIGHT: Move cursor right to the next byte or character.",
	"B",
	"B",
	"B                          Function Keys:",
	"B$$$$$$$$$$$$$$$$$$$$$ $$$$$$$$$$$$$$$$$$$$$ $$$$$$$$$$$$$$$$$$$$$$$$",
	"T$ F1 $ F2 $ F3 $ F4 $ $ F5 $ F6 $ F7 $ F8 $ $ F9 $ F10 $     $     $",
	"H$ F1 $ F2 $ F3 $ F4 $ $ F5 $    $    $    $ $ F9 $     $     $     $",
	"B$$$$$$$$$$$$$$$$$$$$$ $$$$$$$$$$$$$$$$$$$$$ $$$$$$$$$$$$$$$$$$$$$$$$",
	"B",
	"B",
	"BF1:     Define/Select a macro:",
	"BF2:     Replay previously defined/selected macro (See F1).",
	"BF3:     Enable/Disable file backups.",
	"BF4:     Change default ProEdit configuration options.",
	"BF5:     Toggle case sensitivity for text/expression searching.",
	"TF6:     Toggle global/current file searching.",
	"TF7:     Toggle display of special formatting characters.",
	"TF8:     Toggle colorizing of the current file ON/OFF.",
	"BF9:     General Utilities (Calculator, etc).",
	"TF10:    Toggle word wrap for the current file ON/OFF.",
	"B",
	"B"
};

#define NUMBER_MANUAL_LINES_TEXT (sizeof(helpText)/sizeof(char *))


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DisplayHelp(EDIT_FILE*file)
{
	int i, len, x, y, xd, yd, ch, mode;
	EDIT_FILE*help = 0;
	EDIT_LINE*current = 0;
	char line[80 + MAX_FILENAME];

	if (OS_DisplayHelp())
		return ;

	xd = file->display.xd - 4;
	yd = file->display.yd - 4;

	x = file->display.xd / 2 - xd / 2;
	y = (file->display.yd - 1) / 2 - yd / 2;

	help = AllocFile("ProEdit Commands");

	InitDisplayFile(help);

	help->display.xpos = x;
	help->display.ypos = y;
	help->display.xd = xd;
	help->display.yd = yd;
	help->display.columns = xd - 2;
	help->display.rows = yd - 3;

	help->hideCursor = 1;
	help->client = 2;
	help->border = 2;

	help->file_flags |= FILE_FLAG_NONFILE;

	for (i = 0; i < (int)NUMBER_MANUAL_LINES_TEXT; i++) {
		strcpy(line, helpText[i]);

		len = strlen(line);

		for (ch = 0; ch < len; ch++) {
			if (line[ch] == '$')
				line[ch] = OS_Frame(6);
		}

		switch (line[0]) {
		case 'B' :
			current = AddFileLine(help, current, &line[1], len - 1, 0);
			break;
		case 'H' :
			if (file->hexMode)
				current = AddFileLine(help, current, &line[1], len - 1, 0);
			break;
		case 'T' :
			if (!file->hexMode)
				current = AddFileLine(help, current, &line[1], len - 1, 0);
			break;
		}
	}
	InitCursorFile(help);

	CursorTopFile(help);

	help->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

	for (; ; ) {
		help->cursor.line_number = help->display.line_number;
		help->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;

		Paint(help);

		ch = OS_Key(&mode, &help->mouse);

		if (mode) {
			if (ch == ED_ALT_X)
				break;

			switch (ch) {
			case ED_KEY_MOUSE :
				{
					if (help->mouse.wheel == OS_WHEEL_UP)
						ScrollUp(help);
					if (help->mouse.wheel == OS_WHEEL_DOWN)
						ScrollDown(help);
					break;
				}

			case ED_KEY_HOME :
				while (LineUp(help));
				help->display.pan = 0;
				break;

			case ED_KEY_END :
				while (LineDown(help));
				help->display.pan = 0;
				break;

			case ED_KEY_LEFT :
				if (help->display.pan)
					help->display.pan--;
				break;

			case ED_KEY_RIGHT :
				help->display.pan++;
				break;

			case ED_KEY_PGDN :
				{
					CursorPgdn(help);
					break;
				}

			case ED_KEY_PGUP :
				{
					CursorPgup(help);
					break;
				}

			case ED_KEY_UP :
				{
					LineUp(help);
					break;
				}

			case ED_KEY_DOWN :
				{
					LineDown(help);
					break;
				}
			}
		} else {
			if (ch == ED_KEY_ESC || ch == ED_KEY_CR)
				break;
		}
	}

	DeallocFile(help);

	file->paint_flags |= CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG;
}




