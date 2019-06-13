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

static EDIT_BOOKMARK*BookmarkExists(EDIT_FILE*file);
static int BookmarkLineHandler(EDIT_FILE*file, EDIT_LINE*line, int op, int arg);
static EDIT_BOOKMARK*GetFirstBookmark(EDIT_FILE*file);
static EDIT_FILE*GetNextBookmarkFile(EDIT_FILE*file);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int BookmarkLineHandler(EDIT_FILE*file, EDIT_LINE*line, int op, int arg)
{
	EDIT_BOOKMARK*walk;

	arg = arg;

	if ((op&LINE_OP_DELETE) && (line->flags&LINE_FLAG_BOOKMARK)) {
		walk = file->bookmarks;

		while (walk) {
			if (walk->line == line) {
				DeleteBookmark(file, walk);
				return (1);
			}
			walk = walk->next;
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
EDIT_FILE*JumpNextBookmark(EDIT_FILE*file)
{
	if (!file->bookmark_walk) {
		file->bookmark_walk = GetFirstBookmark(file);
		file = GetNextBookmarkFile(file);
	}

	if (file->bookmark_walk) {
		file = GotoBookmark(file, file->bookmark_walk);

		file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);
	} else
		CenterBottomBar(1, "[-] Nothing is currently bookmarked [-]");

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_FILE*GetNextBookmarkFile(EDIT_FILE*file)
{
	EDIT_FILE*base, *walk;

	base = file;
	walk = NextFile(file);

	while (base != walk) {
		if (walk->bookmarks) {
			walk->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);

			walk->bookmark_walk = GetFirstBookmark(walk);

			return (walk);
		}

		walk = NextFile(walk);
	}

	return (walk);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_BOOKMARK*GetFirstBookmark(EDIT_FILE*file)
{
	EDIT_BOOKMARK*walk;

	walk = file->bookmarks;

	while (walk) {
		if (!walk->next)
			break;

		walk = walk->next;
	}

	return (walk);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void CreateBookmark(EDIT_FILE*file)
{
	EDIT_BOOKMARK*bookmark;

	bookmark = BookmarkExists(file);

	if (bookmark) {
		DeleteBookmark(file, bookmark);

		file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);

		CenterBottomBar(1, "[-] Bookmark has been removed. [-]");
		return ;
	}

	AddBookmark(file, file->cursor.line, file->cursor.offset, 0);

	CenterBottomBar(1, "[+] Created/Added a bookmark for this file [+]");

	file->paint_flags |= (CONTENT_FLAG | CURSOR_FLAG | FRAME_FLAG);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_BOOKMARK*AddBookmark(EDIT_FILE*file, EDIT_LINE*line, int offset, char*msg)
{
	EDIT_BOOKMARK*bookmark;

	/* If bookmark already exists, increase instance number */
	if (line->flags&LINE_FLAG_BOOKMARK) {
		bookmark = BookmarkExists(file);
		if (bookmark) {
			bookmark->instance++;
			return (bookmark);
		}
	}

	bookmark = (EDIT_BOOKMARK*)OS_Malloc(sizeof(EDIT_BOOKMARK));

	if (msg) {
		bookmark->msg = OS_Malloc(strlen(msg) + 1);
		strcpy(bookmark->msg, msg);
	} else
		bookmark->msg = 0;

	bookmark->line = line;
	bookmark->instance = 0;
	bookmark->offset = offset;
	bookmark->line->flags |= LINE_FLAG_BOOKMARK;

	bookmark->next = file->bookmarks;
	bookmark->prev = 0;

	if (file->bookmarks)
		file->bookmarks->prev = bookmark;
	else
		AddLineCallback(file, (LINE_PFN*)BookmarkLineHandler, LINE_OP_DELETE);

	file->bookmarks = bookmark;

	if (!file->bookmark_walk)
		file->bookmark_walk = bookmark;

	return (bookmark);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeleteBookmark(EDIT_FILE*file, EDIT_BOOKMARK*bookmark)
{

	bookmark = ValidateBookmark(file, bookmark);

	if (!bookmark)
		return ;

	if (bookmark->instance) {
		bookmark->instance--;
		return ;
	}

	if (file->bookmark_walk == bookmark)
		file->bookmark_walk = bookmark->next;

	if (file->bookmarks == bookmark)
		file->bookmarks = bookmark->next;

	if (bookmark->prev)
		bookmark->prev->next = bookmark->next;

	if (bookmark->next)
		bookmark->next->prev = bookmark->prev;

	bookmark->line->flags &= ~LINE_FLAG_BOOKMARK;

	if (!file->bookmarks)
		RemoveLineCallback(file, (LINE_PFN*)BookmarkLineHandler);

	OS_Free(bookmark);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_BOOKMARK*ValidateBookmark(EDIT_FILE*file, EDIT_BOOKMARK*bookmark)
{
	EDIT_BOOKMARK*walk;

	walk = file->bookmarks;

	while (walk) {
		if (walk == bookmark)
			return (walk);

		walk = walk->next;
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*GotoBookmark(EDIT_FILE*file, EDIT_BOOKMARK*bookmark)
{
	EDIT_LINE*walk;
	int line_number = 0;

	walk = file->lines;

	while (walk) {
		if (walk == bookmark->line) {
			if (walk->flags&LINE_FLAG_BOOKMARK) {
				file->bookmark_walk = bookmark->prev;

				GotoPosition(file, line_number + 1, bookmark->offset + 1);
				if (bookmark->msg)
					CenterBottomBar(1, bookmark->msg);
				return (file);
			}
		}
		line_number++;
		walk = walk->next;
	}
	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static EDIT_BOOKMARK*BookmarkExists(EDIT_FILE*file)
{
	EDIT_BOOKMARK*walk;

	if (file->cursor.line->flags&LINE_FLAG_BOOKMARK) {
		walk = file->bookmarks;

		while (walk) {
			if (walk->line == file->cursor.line)
				return (walk);

			walk = walk->next;
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
void DestroyBookmarks(EDIT_FILE*file)
{
	while (file->bookmarks)
		DeleteBookmark(file, file->bookmarks);
}



