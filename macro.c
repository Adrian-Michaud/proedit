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
#include <stdarg.h>
#include "proedit.h"

#define MAX_MACRO_NAME 64

#define MAX_MACROS     255
#define MAX_MACRO_SIZE 1024
#define RECORD 1
#define DONE   2
#define PLAY   3

typedef struct MACRO_T{
	char name[MAX_MACRO_NAME];
	long macroIndex;
	long macroSize;
	int mode;
	char macroBuffer1[MAX_MACRO_SIZE];
	char macroBuffer2[MAX_MACRO_SIZE];
}MACRO;

static MACRO*macros[MAX_MACROS];

static int macroMode;
static int macroSelect = 0;
static int macrosLoaded = 0;
static int macrosDirty = 0;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void LoadMacros(void)
{
	FILE_HANDLE*fp;
	char filename[MAX_FILENAME];
	int total;
	int i;

	if (!macrosLoaded) {
		macrosLoaded = 1;

		OS_GetSessionFile(filename, SESSION_MACROS, 0);
		fp = OS_Open(filename, "rb");

		if (fp) {
			OS_Read(&total, sizeof(int), 1, fp);

			for (i = 1; i < total + 1; i++) {
				macros[i] = OS_Malloc(sizeof(MACRO));
				OS_Read(macros[i]->name, MAX_MACRO_NAME, 1, fp);
				OS_Read(&macros[i]->macroSize, sizeof(long), 1, fp);
				OS_Read(macros[i]->macroBuffer1, macros[i]->macroSize, 1, fp);
				OS_Read(macros[i]->macroBuffer2, macros[i]->macroSize, 1, fp);
				macros[i]->macroIndex = 0;
				macros[i]->mode = DONE;
			}
			OS_Close(fp);
		}
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int OS_Key(int*mode, OS_MOUSE*mouse)
{
	int ch, dontSave = 0;

	if (macroMode == PLAY) {
		ch = macros[macroSelect]->macroBuffer1[macros[macroSelect]->macroIndex];
		*mode = macros[macroSelect]->macroBuffer2[macros[macroSelect]->
		    macroIndex];
		macros[macroSelect]->macroIndex++;

		if (macros[macroSelect]->macroIndex >= macros[macroSelect]->macroSize) {
			macros[macroSelect]->mode = DONE;
			macroMode = DONE;
		}
		return (ch);
	}

	ch = OS_GetKey(mode, mouse);

	if (*mode && ch == ED_KEY_MOUSE)
		dontSave = 1;

	if (*mode && ch == ED_F1)
		dontSave = 1; /* Avoid Macros getting into macros */

	if (*mode && ch == ED_F2)
		dontSave = 1; /* Avoid Macros getting into macros */

	if (macroMode == RECORD && !dontSave) {
		macros[macroSelect]->macroBuffer1[macros[macroSelect]->macroIndex] = ch;
		macros[macroSelect]->macroBuffer2[macros[macroSelect]->macroIndex] = *
		    mode;
		macros[macroSelect]->macroIndex++;
		if (macros[macroSelect]->macroIndex >= MAX_MACRO_SIZE) {
			macros[macroSelect]->macroSize = MAX_MACRO_SIZE;
			macros[macroSelect]->mode = DONE;
			macroMode = DONE;
		}
	}

	return (ch);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SelectMacro(char*name)
{
	int i;

	LoadMacros();

	for (i = 1; i < MAX_MACROS; i++) {
		if (macros[i] && !OS_Strcasecmp(macros[i]->name, name))
			return (i);
	}

	// Try to allocate for a new name
	for (i = 0; i < MAX_MACROS; i++) {
		if (!macros[i]) {
			return (i);
		}
	}
	// Return error
	return (-1);
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SaveMacros(void)
{
	FILE_HANDLE*fp;
	char filename[MAX_FILENAME];
	int i;
	int total = 0;

	if (macrosDirty) {
		macrosDirty = 0;
		OS_GetSessionFile(filename, SESSION_MACROS, 0);

		fp = OS_Open(filename, "wb");

		if (fp) {
			for (i = 1; i < MAX_MACROS; i++)
				if (macros[i] && macros[i]->mode == DONE)
					total++;

			OS_Write(&total, sizeof(int), 1, fp);

			for (i = 1; i < MAX_MACROS; i++) {
				if (macros[i] && macros[i]->mode == DONE) {
					OS_Write(macros[i]->name, MAX_MACRO_NAME, 1, fp);
					OS_Write(&macros[i]->macroSize, sizeof(long), 1, fp);
					OS_Write(macros[i]->macroBuffer1, macros[i]->macroSize, 1,
					    fp);
					OS_Write(macros[i]->macroBuffer2, macros[i]->macroSize, 1,
					    fp);
				}
			}
			OS_Close(fp);
		}
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DefineMacro(EDIT_FILE*file)
{
	char macroName[MAX_MACRO_NAME];
	int newmacro = 0;

	if (macroMode == RECORD) {
		CenterBottomBar(1, "[+] Macro Defined. Press <F2> To Envoke Macro [+]");
		macros[macroSelect]->macroSize = macros[macroSelect]->macroIndex;
		macros[macroSelect]->mode = DONE;
		macroMode = DONE;
	} else {
		file->paint_flags |= CURSOR_FLAG;
		strcpy(macroName, "");

		if (!Input(HISTORY_MACROS, "Macro Name:", macroName, MAX_MACRO_NAME)) {
			file->paint_flags |= CURSOR_FLAG;
			return ;
		}

		if (strlen(macroName)) {
			macroSelect = SelectMacro(macroName);
			if (macroSelect == -1) {
				CenterBottomBar(1, "[-] Maximum Macros Defined [-]");
				file->paint_flags |= CURSOR_FLAG;
				macroMode = 0;
				return ;
			}
		} else {
			newmacro = 1;
			macroSelect = 0;
		}

		if (!macros[macroSelect]) {
			macros[macroSelect] = OS_Malloc(sizeof(MACRO));
			strcpy(macros[macroSelect]->name, macroName);
			if (macroSelect)
				macrosDirty = 1;
			newmacro = 1;
		}

		if (newmacro) {
			CenterBottomBar(1,
			    "[+] Begin Defining Macro, Press <F1> To End Macro Definition [+]");
			macros[macroSelect]->macroIndex = 0;
			macros[macroSelect]->mode = RECORD;
			macroMode = RECORD;
		} else {
			CenterBottomBar(1,
			    "[+] Macro Selected. Press <F2> To Envoke Macro [+]");
			macros[macroSelect]->mode = DONE;
			macroMode = DONE;
		}

		file->paint_flags |= CURSOR_FLAG;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ReplayMacro(void)
{
	if (macroMode == DONE) {
		macros[macroSelect]->macroIndex = 0;
		macroMode = PLAY;
	}
}


