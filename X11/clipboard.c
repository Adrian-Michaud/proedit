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

#if 0
#define DEBUG_CLIP
#endif

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

#define ATOMNAME(a) a == None ? "None" : XGetAtomName(xw->display,a)

void CopyClipboard(Atom property);
int GetClipboardData(Atom target);

static char*clipboardData;
static int clipboardLen;

extern void ClearLastX11Error(void);
extern int GetLastX11Error(void);

static Atom XA_targets, XA_multiple;
static Atom XA_text_uri, XA_text_plain, XA_text, XA_utf8_string;
static Atom XA_clipboard;

extern XWindow*xw;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void InitX11Clipboard(void)
{
	XA_targets = XInternAtom(xw->display, "TARGETS", False);
	XA_text_uri = XInternAtom(xw->display, "text/uri-list", False);
	XA_text_plain = XInternAtom(xw->display, "text/plain", False);
	XA_text = XInternAtom(xw->display, "TEXT", False);
	XA_multiple = XInternAtom(xw->display, "MULTIPLE", False);
	XA_utf8_string = XInternAtom(xw->display, "UTF8_STRING", False);
	XA_clipboard = XInternAtom(xw->display, "CLIPBOARD", False);
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

	#ifdef DEBUG_CLIP
	printf("OS_SetupClipboard(len=%d)\n", len);
	#endif

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

	XSetSelectionOwner(xw->display, XA_PRIMARY, xw->window, CurrentTime);
	XSetSelectionOwner(xw->display, XA_clipboard, xw->window, CurrentTime);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
char*OS_GetClipboard(void)
{
	#ifdef DEBUG_CLIP
	printf("XA_PRIMARY Current Owner=%x\n", (unsigned int)XGetSelectionOwner(
	    xw->display, XA_PRIMARY));
	printf("XA_CLIPBOARD Current Owner=%x\n", (unsigned int)XGetSelectionOwner(
	    xw->display, XA_clipboard));

	if (GetClipboardData(XA_targets))
		return (clipboardData);
	#endif

	if (GetClipboardData(XA_STRING))
		return (clipboardData);

	if (GetClipboardData(XA_text))
		return (clipboardData);

	if (GetClipboardData(XA_text_plain))
		return (clipboardData);

	if (GetClipboardData(XA_text_uri))
		return (clipboardData);

	return (clipboardData);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
int GetClipboardData(Atom target)
{
	XEvent selectNotify;
	int retry;

	XDeleteProperty(xw->display, xw->window, XA_clipboard);
	XSync(xw->display, False);

	XConvertSelection(xw->display, XA_PRIMARY, target, XA_clipboard, xw->window,
	    CurrentTime);
	XSync(xw->display, False);

	#ifdef DEBUG_CLIP
	printf("Waiting for %s clipboard data...\n", ATOMNAME(target));
	#endif

	for (retry = 0; retry < 10; retry++) {
		if (XPending(xw->display)) {
			while (XCheckTypedWindowEvent(xw->display, xw->window,
			    SelectionNotify, &selectNotify)) {
				#ifdef DEBUG_CLIP
				printf("Trying %s (%s)\n", ATOMNAME(selectNotify.xselection.
				    property), ATOMNAME(selectNotify.xselection.target));
				#endif

				if (selectNotify.xselection.property != None) {
					#ifdef DEBUG_CLIP
					printf("Trying %s\n", ATOMNAME(selectNotify.xselection.
					    property));
					#endif
					CopyClipboard(selectNotify.xselection.property);

					if (clipboardData) {
						#ifdef DEBUG_CLIP
						printf("\nGot %s clipboard data!\n", ATOMNAME(target));
						#endif
						return (1);
					}
				}
			}
		}
		usleep(1000);
	}

	#ifdef DEBUG_CLIP
	printf("OS_GetClipboard() failed...\n");
	#endif
	return (0);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void OS_FreeClipboard(char*clip)
{
	#ifdef DEBUG_CLIP
	printf("OS_FreeClipboard(%x)\n", (unsigned int)clip);
	#endif
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
void CopyClipboard(Atom property)
{
	Atom typeret;
	int format, i;
	unsigned long nitems, bytesleft;
	unsigned char*data = 0;
	#ifdef DEBUG_CLIP
	Atom*atomp;
	#endif

	#ifdef DEBUG_CLIP
	printf("CopyCliboard(%s)\n", ATOMNAME(property));
	#endif

	XGetWindowProperty(xw->display, xw->window, property, 0L, 0L, False,
	    AnyPropertyType, &typeret, &format, &nitems, &bytesleft, &data);

	#ifdef DEBUG_CLIP
	printf(
	    "XGetWindowProperty() typeret=%s, format=%d, nitems=%d, bytesleft=%d, data=%x\n", ATOMNAME(typeret), format, (int)nitems, (int)bytesleft, (unsigned int)data);

	if (bytesleft)
		printf("There is still %d bytes left\n", (int)bytesleft);
	#endif

	XGetWindowProperty(xw->display, xw->window, property, 0L, bytesleft, False,
	    AnyPropertyType, &typeret, &format, &nitems, &bytesleft, &data);

	#ifdef DEBUG_CLIP
	printf(
	    "XGetWindowProperty() typeret=%s, format=%d, nitems=%d, bytesleft=%d, data=%x\n", ATOMNAME(typeret), format, (int)nitems, (int)bytesleft, (unsigned int)data);

	if (bytesleft)
		printf("There is still %d bytes left\n", (int)bytesleft);
	#endif

	if (format == 8) {
		for (i = 0; i < (int)nitems; i++) {
			if (data[i] == 10)
				data[i] = 13;
		}
		if (clipboardData)
			OS_Free(clipboardData);

		clipboardData = OS_Malloc(nitems + 1);
		memcpy(clipboardData, data, nitems);
		clipboardData[nitems] = 0;
		clipboardLen = nitems;
	}

	#ifdef DEBUG_CLIP
	if (format == 32) {
		printf("Atoms:\n");
		atomp = (Atom*)data;
		printf("[");
		for (i = 0; i < (int)nitems; i++) {
			if (ATOMNAME(atomp[i]))
				printf("%s", ATOMNAME(atomp[i]));
			else
				printf("%d", (int)atomp[i]);
			if (i < (int)nitems - 1)
				printf(",");
		}
		printf("]\n");
	}
	#endif

	if (data)
		XFree(data);

	XDeleteProperty(xw->display, xw->window, property);
}


/*##########################################################################*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*#                                                                        #*/
/*##########################################################################*/
void SetupClipboard(XSelectionRequestEvent*reqevent)
{
	Atom alist[6];
	XEvent newevent;

	#ifdef DEBUG_CLIP
	printf("SetupClipboard(target:%s, selection:%s)\n", ATOMNAME(reqevent->
	    target), ATOMNAME(reqevent->selection));
	#endif
	memset(&newevent, 0, sizeof(newevent));

	/* prepare notify event */
	newevent.xselection.type = SelectionNotify;
	newevent.xselection.display = xw->display;
	newevent.xselection.requestor = reqevent->requestor;
	newevent.xselection.selection = reqevent->selection;
	newevent.xselection.target = reqevent->target;
	newevent.xselection.time = reqevent->time;
	newevent.xselection.property = None;

	if (reqevent->target == XA_text_uri || reqevent->target == XA_STRING ||
	    reqevent->target == XA_text_plain || reqevent->target == XA_text) {
		#ifdef DEBUG_CLIP
		printf("clipboardData=%x, len=%d, targ=%s, prop=%s\n", (unsigned int)
		    clipboardData, clipboardLen, ATOMNAME(reqevent->target), ATOMNAME(
		    reqevent->property));
		#endif

		newevent.xselection.property = reqevent->property;

		XChangeProperty(xw->display, reqevent->requestor, reqevent->property,
		    reqevent->target, 8, PropModeReplace, (unsigned char*)clipboardData,
		    clipboardLen);
	} else
		if (reqevent->target == XA_targets) {
			/* transmit data to property on requestors window */
			alist[0] = XA_targets;
			alist[1] = XA_multiple;
			alist[2] = XA_STRING;
			alist[3] = XA_text;
			alist[4] = XA_text_plain;
			alist[5] = XA_text_uri;

			newevent.xselection.property = reqevent->property;

			XChangeProperty(xw->display, reqevent->requestor,
			    reqevent->property, reqevent->target, 32, PropModeReplace, (
			    unsigned char*)alist, 6);
		}

	/* send a notify event to let requestor know the data is ready */
	XSendEvent(xw->display, reqevent->requestor, False, NoEventMask, &newevent);
}


