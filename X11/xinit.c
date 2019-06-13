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

#include "../osdep.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include "xinit.h"

char*ERR_XI_STR[] =
{
	"X initialisation OK!",
	"No Shared Memory available",
	"cannot open Display",
	"bad color depth",
	"can't create Window",
	"can't alloc memory for virtual screen",
	"cannot create XImage",
	"can't alloc memory for Shared memory segment info",
	"cannot create Shared Memory XImage",
	"Shared memory segment info error",
	"Shared memory virtual screen allocation failed",
	"cannot attach Shared Memory segment to display"
};

int XShmMajor, XShmMinor;
Bool XShmPixmaps;

int
OpenXWindow(XWnd, xp, yp, width, height, bpp, title, Palette, createbuffer,
    usesharedmemory, shmpixmap, verbose)
XWindow*XWnd;
int xp;
int yp;
int width;
int height;
int bpp;
char*title;
int*Palette;
BOOL createbuffer;
BOOL usesharedmemory;
BOOL shmpixmap;
BOOL verbose;
{
	int err;

	if ((err = InitXWindow(XWnd, xp, yp, width, height, bpp, title, Palette,
	    createbuffer, usesharedmemory, shmpixmap))) {
		if (verbose)
			OS_Printf("\nX initialisation error:\n *** %s\n", ERR_XI_STR[err]);
		return  ERR_XI_FAILURE;
	}

	if (!verbose)
		return  ERR_XI_OK;

	switch (XWnd->videoaccesstype) {
	case VIDEO_XI_STANDARD :
		OS_Printf("  # using conventional Xlib calls.\n\n");
		break;
	case VIDEO_XI_SHMSTD :
		OS_Printf("  # Using Xlib shared memory extension %d.%d\n\n", XShmMajor,
		    XShmMinor);
		break;
	case VIDEO_XI_SHMPIXMAP :
		OS_Printf("  # Using shared memory pixmaps extension %d.%d\n\n",
		    XShmMajor, XShmMinor);
	}

	return  ERR_XI_OK;
}

int
InitXWindow(XWnd, xp, yp, width, height, bpp, title, Palette, createbuffer,
    usesharedmemory, shmpixmap)
XWindow*XWnd;
int xp;
int yp;
int width;
int height;
int bpp; /* put bpp=-1 to run on every bpp displays */
char*title;
int*Palette; /* Put NULL if you don't need a colormap */
BOOL createbuffer;
BOOL usesharedmemory;
BOOL shmpixmap;
{
	/*
	#ifndef __USE_X_SHAREDMEMORY__
	  if(usesharedmemory)
	    return ERR_XI_NOSHAREDMEMORY;
	#endif
	*/

	XWnd->width = width;
	XWnd->height = height;

	XWnd->display = XOpenDisplay(NULL);
	if (!XWnd->display) {
		OS_Printf("XOpenDisplay() failed\n");
		return  ERR_XI_DISPLAY;
	}

	XWnd->screennum = DefaultScreen(XWnd->display);
	XWnd->screenptr = DefaultScreenOfDisplay(XWnd->display);

	XWnd->visual = DefaultVisualOfScreen(XWnd->screenptr);

	XWnd->depth = DefaultDepth(XWnd->display, XWnd->screennum);

	if (bpp != -1 && XWnd->depth != bpp) {
		OS_Printf("Bad depth...\n");
		return  ERR_XI_BADDEPTH;
	}

	switch (XWnd->depth) { /* I'm not sure that is good.. */
	case 8 :
		XWnd->pixelsize = 1;
		break;
	case 16 :
		XWnd->pixelsize = 2;
		break;
	case 24 :
		XWnd->pixelsize = 4;
		break;
		default :
		XWnd->pixelsize = 1;
	}

	XWnd->window = XCreateWindow(XWnd->display, RootWindowOfScreen(XWnd->
	    screenptr), xp, yp, XWnd->width, XWnd->height, 0, XWnd->depth,
	    InputOutput, XWnd->visual, 0, NULL);
	if (!XWnd->window) {
		OS_Printf("XCreateWindow() failed\n");
		return  ERR_XI_WINDOW;
	}

	{
		XSizeHints Hints;

		Hints.flags = PSize | PMinSize | PMaxSize;
		Hints.min_width = Hints.max_width = Hints.base_width = width;
		Hints.min_height = Hints.max_height = Hints.base_height = height;
		XSetWMNormalHints(XWnd->display, XWnd->window, &Hints);
		XStoreName(XWnd->display, XWnd->window, title);
	}

	XSelectInput(XWnd->display, XWnd->window, ExposureMask | KeyPressMask |
	    KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
	    | EnterWindowMask | LeaveWindowMask);

	XWnd->gc = XCreateGC(XWnd->display, XWnd->window, 0, NULL);
	XMapRaised(XWnd->display, XWnd->window);

	/* Create a colormap if requested */
	if (Palette) {
		int i, j;
		XColor color;

		XWnd->colormap = DefaultColormapOfScreen(XWnd->screenptr);
		XWnd->colorcells = 0;
		while (Palette && Palette[3*XWnd->colorcells++] != -1);
		XWnd->palette = malloc(XWnd->colorcells*sizeof(PIXEL));
		for (i = j = 0; i < XWnd->colorcells; i++) {
			color.flags = DoRed | DoGreen | DoBlue;
			color.red = Palette[i*3] << 8;
			color.green = Palette[i*3 + 1] << 8;
			color.blue = Palette[i*3 + 2] << 8;
			if (XAllocColor(XWnd->display, XWnd->colormap, &color))
				XWnd->palette[j++] = color.pixel;
		}
		XSetWindowColormap(XWnd->display, XWnd->window, XWnd->colormap);
	} else
		XWnd->palette = NULL;

	XWnd->videoaccesstype = VIDEO_XI_STANDARD;
	XWnd->screensize = XWnd->height*XWnd->width*XWnd->pixelsize;

	if (!createbuffer) {
		return  ERR_XI_OK;
	}

	#ifdef __USE_X_SHAREDMEMORY__
	XWnd->videomemory = NULL;
	if (usesharedmemory) {
		if (XShmQueryVersion(XWnd->display, &XShmMajor, &XShmMinor,
		    &XShmPixmaps)) {
			if (shmpixmap && XShmPixmaps == True && XShmPixmapFormat(XWnd->
			    display) == ZPixmap)
				XWnd->videoaccesstype = VIDEO_XI_SHMPIXMAP;
			else
				XWnd->videoaccesstype = VIDEO_XI_SHMSTD;
		}
	}
	#endif

	switch (XWnd->videoaccesstype) {
		#ifdef __USE_X_SHAREDMEMORY__

	case VIDEO_XI_SHMPIXMAP :
		XWnd->shmseginfo = (XShmSegmentInfo*)malloc(sizeof(XShmSegmentInfo));
		if (!XWnd->shmseginfo) {
			OS_Printf("error1\n");
			return  ERR_XI_SHMALLOC;
		}

		memset(XWnd->shmseginfo, 0, sizeof(XShmSegmentInfo));

		XWnd->shmseginfo->shmid = shmget(IPC_PRIVATE, XWnd->screensize,
		    IPC_CREAT | 0777);
		if (XWnd->shmseginfo->shmid < 0) {
			OS_Printf("error2\n");
			return  ERR_XI_SHMSEGINFO;
		}

		XWnd->videomemory = (byte*)(XWnd->shmseginfo->shmaddr =
		shmat(XWnd->shmseginfo->shmid, 0, 0));
		XWnd->virtualscreen = XWnd->videomemory;

		XWnd->shmseginfo->readOnly = False;
		if (!XShmAttach(XWnd->display, XWnd->shmseginfo)) {
			OS_Printf("error3\n");
			return  ERR_XI_SHMATTACH;
		}

		XWnd->pixmap = XShmCreatePixmap(XWnd->display, XWnd->window,
		    (char*)XWnd->videomemory, XWnd->shmseginfo, XWnd->width, XWnd->
		    height, XWnd->depth);
		XSetWindowBackgroundPixmap(XWnd->display, XWnd->window, XWnd->pixmap);
		break;

	case VIDEO_XI_SHMSTD :
		XWnd->shmseginfo = (XShmSegmentInfo*)malloc(sizeof(XShmSegmentInfo));
		if (!XWnd->shmseginfo) {
			OS_Printf("error4\n");
			return  ERR_XI_SHMALLOC;
		}

		memset(XWnd->shmseginfo, 0, sizeof(XShmSegmentInfo));

		XWnd->ximage = XShmCreateImage(XWnd->display, XWnd->visual, XWnd->depth,
		    ZPixmap, NULL, XWnd->shmseginfo, XWnd->width, XWnd->height);
		if (!XWnd->ximage) {
			OS_Printf("error5\n");
			return  ERR_XI_SHMXIMAGE;
		}

		XWnd->shmseginfo->shmid = shmget(IPC_PRIVATE, XWnd->ximage->
		    bytes_per_line*XWnd->ximage->height, IPC_CREAT | 0777);
		if (XWnd->shmseginfo->shmid < 0) {
			OS_Printf("error6\n");
			return  ERR_XI_SHMSEGINFO;
		}

		XWnd->virtualscreen = (byte*)(XWnd->ximage->data =
		XWnd->shmseginfo->shmaddr =
		shmat(XWnd->shmseginfo->shmid, NULL, 0));
		if (!XWnd->virtualscreen) {
			OS_Printf("error7\n");
			return  ERR_XI_SHMVIRTALLOC;
		}

		XWnd->shmseginfo->readOnly = False;

		if (!XShmAttach(XWnd->display, XWnd->shmseginfo)) {
			OS_Printf("error8\n");
			return  ERR_XI_SHMATTACH;
		}
		break;
		#endif

	case VIDEO_XI_STANDARD :
		XWnd->virtualscreen = (byte*)malloc(XWnd->screensize*sizeof(byte));
		if (!XWnd->virtualscreen) {
			OS_Printf("error9\n");
			return  ERR_XI_VIRTALLOC;
		}
		XWnd->ximage = XCreateImage(XWnd->display, XWnd->visual, XWnd->depth,
		    ZPixmap, 0, (char*)XWnd->virtualscreen, XWnd->width, XWnd->height,
		    32, XWnd->width*XWnd->pixelsize);
		if (!XWnd->ximage) {
			OS_Printf("error10\n");
			return  ERR_XI_XIMAGE;
		}
		break;
	}


	return  ERR_XI_OK;
}

void
TrashXWindow(XWindow*XWnd)
{
	if (XWnd && XWnd->display && XWnd->window) {
		switch (XWnd->videoaccesstype) {
			#ifdef __USE_X_SHAREDMEMORY__
		case VIDEO_XI_SHMSTD :
			XShmDetach(XWnd->display, XWnd->shmseginfo);
			if (XWnd->ximage)
				XDestroyImage(XWnd->ximage);
			if (XWnd->shmseginfo->shmaddr)
				shmdt(XWnd->shmseginfo->shmaddr);
			if (XWnd->shmseginfo->shmid >= 0)
				shmctl(XWnd->shmseginfo->shmid, IPC_RMID, NULL);
			free(XWnd->shmseginfo);
			break;

		case VIDEO_XI_SHMPIXMAP :
			XShmDetach(XWnd->display, XWnd->shmseginfo);
			XFreePixmap(XWnd->display, XWnd->pixmap);
			if (XWnd->shmseginfo->shmaddr)
				shmdt(XWnd->shmseginfo->shmaddr);
			if (XWnd->shmseginfo->shmid >= 0)
				shmctl(XWnd->shmseginfo->shmid, IPC_RMID, NULL);
			free(XWnd->shmseginfo);
			break;
			#endif
		case VIDEO_XI_STANDARD :
			if (XWnd->ximage)
				XDestroyImage(XWnd->ximage);
			if (XWnd->virtualscreen)
				free(XWnd->virtualscreen);
			break;
		}
		XFreeGC(XWnd->display, XWnd->gc);
		XDestroyWindow(XWnd->display, XWnd->window);
		XCloseDisplay(XWnd->display);
	}
}

void
PutXImage(XWindow*XWnd)
{
	#ifdef __USE_X_SHAREDMEMORY__
	switch (XWnd->videoaccesstype) {
	case VIDEO_XI_SHMSTD :
		XShmPutImage(XWnd->display, XWnd->window, XWnd->gc, XWnd->ximage, 0, 0,
		    0, 0, XWnd->width, XWnd->height, False);
		/* XFlush(XWnd->display); */ /* Not needed, done by XPending */
		break;

	case VIDEO_XI_SHMPIXMAP :
		XClearWindow(XWnd->display, XWnd->window);
		/* XSync(XWnd->display,FALSE); */ /* Not needed, done by XPending */
		break;

	case VIDEO_XI_STANDARD :
		#endif
		XPutImage(XWnd->display, XWnd->window, XWnd->gc, XWnd->ximage, 0, 0, 0,
		    0, XWnd->width, XWnd->height);
		/* XFlush(XWnd->display);  */ /* Not needed, done by XPending */
		#ifdef __USE_X_SHAREDMEMORY__
		break;
	}
	#endif
}


