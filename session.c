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

#define MAX_SESSIONS 100

#define SESSION_VERSION 3
#define SESSION_SIG     0xaa55aa00

typedef struct sessionFiles
{
	int size;
	int version;
	int total;
	char cwd[MAX_FILENAME];
	int session_id;
	int reuse;
	unsigned int sig;
}SAVED_SESSIONS;

typedef struct sessionData
{
	int size;
	int version;
	int total;
	char pathname[MAX_FILENAME];
	CURSOR_SAVE cursor;
	int forceHex;
	int forceText;
	int rows;
	int cols;
	int hexMode;
	struct sessionData*next;
	struct sessionData*prev;
	unsigned int sig;
}FILE_SESSION;

static FILE_SESSION*sessions;
static int number_sessions;

extern int forceHex;
extern int forceText;

static int GetSessionId(int override);
static void CreateSessions(void);

static char*session_filenames[] =
{
	"config",
	"sessions",
	"history",
	"session",
	"dictionary",
	"shell",
	"backups",
	"screen",
	"macros",
};


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GetSessionId(int override)
{
	SAVED_SESSIONS*sessions;
	int i, num, len;
	char cwd[MAX_FILENAME];

	OS_GetCurrentDir(cwd);

	sessions = (SAVED_SESSIONS*)OS_LoadSession(&len, SESSION_FILES, 0);

	/* Create the sessions. */
	if (!sessions) {
		CreateSessions();
		return (GetSessionId(override));
	}

	num = len / sizeof(SAVED_SESSIONS);

	if (!override) {
		for (i = 0; i < num; i++) {
			if (sessions[i].sig != SESSION_SIG || sessions[i].version !=
			    SESSION_VERSION || sessions[i].total != num ||
			    sessions[i].size != sizeof(SAVED_SESSIONS)) {
				OS_FreeSession(sessions);
				CreateSessions();
				return (GetSessionId(override));
			}

			/* Does this session alreasdy exist? */
			if (!OS_Strcasecmp(cwd, sessions[i].cwd)) {
				i = sessions[i].session_id;
				OS_FreeSession(sessions);
				return (i);
			}
		}
	}

	for (i = 0; i < num; i++) {
		if ((!override && !strlen(sessions[i].cwd)) || (override ==
		    sessions[i].session_id)) {
			/* Erase the session data. */
			if (!strlen(sessions[i].cwd)) {
				OS_SaveSession(0, 0, SESSION_DATA, sessions[i].session_id);
				strcpy(sessions[i].cwd, cwd);
				OS_SaveSession(sessions, num*sizeof(SAVED_SESSIONS),
				    SESSION_FILES, 0);
			}

			i = sessions[i].session_id;
			OS_FreeSession(sessions);
			return (i);
		}
	}

	/* If all sessions are full, rotate the last one. */
	for (i = 0; i < num; i++) {
		if (sessions[i].reuse) {
			sessions[i].reuse = 0;

			if (i + 1 == num)
				sessions[0].reuse = 1;
			else
				sessions[i + 1].reuse = 1;

			/* Erase the session data. */
			OS_SaveSession(0, 0, SESSION_DATA, sessions[i].session_id);
			strcpy(sessions[i].cwd, cwd);
			break;
		}
	}

	if (i == num) {
		OS_FreeSession(sessions);
		return (0);
	}

	i = sessions[i].session_id;

	OS_SaveSession(sessions, num*sizeof(SAVED_SESSIONS), SESSION_FILES, 0);

	OS_FreeSession(sessions);

	return (i);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void CreateSessions(void)
{
	SAVED_SESSIONS*sessions;
	int i;

	sessions = (SAVED_SESSIONS*)OS_Malloc(MAX_SESSIONS*sizeof(SAVED_SESSIONS));
	memset(sessions, 0, MAX_SESSIONS*sizeof(SAVED_SESSIONS));

	for (i = 0; i < MAX_SESSIONS; i++) {
		if (i == 0)
			sessions[i].reuse = 1;

		sessions[i].sig = SESSION_SIG;
		sessions[i].size = sizeof(SAVED_SESSIONS);
		sessions[i].version = SESSION_VERSION;
		sessions[i].total = MAX_SESSIONS;
		sessions[i].session_id = i + 1;
	}

	OS_SaveSession(sessions, MAX_SESSIONS*sizeof(SAVED_SESSIONS), SESSION_FILES,
	    0);

	OS_Free(sessions);

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
EDIT_FILE*LoadSavedSessions(int sessionId)
{
	EDIT_FILE*file = 0;
	FILE_SESSION*session_data;
	int i, len, num;

	sessionId = GetSessionId(sessionId);

	if (sessionId) {
		session_data = (FILE_SESSION*)
		OS_LoadSession(&len, SESSION_DATA, sessionId);

		if (session_data) {
			num = len / sizeof(FILE_SESSION);

			for (i = 0; i < num; i++) {
				if (session_data[i].sig != SESSION_SIG || session_data[i].
				    version != SESSION_VERSION || session_data[i].total != num
				    || session_data[i].size != sizeof(FILE_SESSION))
					break;

				forceText = session_data[i].forceText;
				forceHex = session_data[i].forceHex;

				file = LoadFileWildcard(file, session_data[i].pathname,
				    LOAD_FILE_EXISTS);

				if (file) {
					/* Make sure there isn't a mistmatch. */
					if ((file->hexMode && !session_data[i].hexMode) || (!file->
					    hexMode && session_data[i].hexMode)) {
						CloseFile(file);
						file = 0;
					} else {
						/* Make sure previous rows x cols were the same before */
						/* Restoring cursor position.                          */
						if (session_data[i].rows == file->display.rows &&
						    session_data[i].cols == file->display.columns) {
							if (TestCursor(file, &session_data[i].cursor))
								SetCursor(file, &session_data[i].cursor);
						}

						file->hexMode = session_data[i].hexMode;
					}
				}
			}
			OS_FreeSession(session_data);
		}
	}
	/* get first file */
	if (!file)
		file = NextFile(0);

	if (!file)
		file = CreateUntitled();

	return (file);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void ClearSessions(void)
{
	FILE_SESSION*walk, *next;

	walk = sessions;

	while (walk) {
		next = walk->next;
		OS_Free(walk);
		walk = next;
	}
	number_sessions = 0;

	sessions = 0;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SaveSessions(int sessionId)
{
	int index = 0;
	FILE_SESSION*data, *walk;

	if (number_sessions) {
		sessionId = GetSessionId(sessionId);

		if (sessionId) {
			data = (FILE_SESSION*)OS_Malloc(number_sessions*sizeof(
			    FILE_SESSION));

			if (data) {
				walk = sessions;

				while (walk) {
					walk->total = number_sessions;

					memcpy(&data[index++], walk, sizeof(FILE_SESSION));
					walk = walk->next;
				}

				OS_SaveSession(data, number_sessions*sizeof(FILE_SESSION),
				    SESSION_DATA, sessionId);

				OS_Free(data);
			}
		}
	}

	ClearSessions();
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void LogSession(EDIT_FILE*file)
{
	FILE_SESSION*session;

	session = (FILE_SESSION*)OS_Malloc(sizeof(FILE_SESSION));

	memset(session, 0, sizeof(FILE_SESSION));

	strcpy(session->pathname, file->pathname);

	SaveCursor(file, &session->cursor);

	session->hexMode = file->hexMode;
	session->forceHex = file->forceHex;
	session->forceText = file->forceText;
	session->rows = file->display.rows;
	session->cols = file->display.columns;
	session->sig = SESSION_SIG;
	session->size = sizeof(FILE_SESSION);
	session->version = SESSION_VERSION;

	if (sessions) {
		session->next = sessions;
		sessions->prev = session;
	}

	sessions = session;

	number_sessions++;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DisplaySessions(void)
{
	SAVED_SESSIONS*sessions;
	FILE_SESSION*session_data;
	int i, i2, num, num2, len, len2;

	sessions = (SAVED_SESSIONS*)OS_LoadSession(&len, SESSION_FILES, 0);

	/* Dump the sessions. */
	if (sessions) {
		num = len / sizeof(SAVED_SESSIONS);

		for (i = 0; i < num; i++)
			if (strlen(sessions[i].cwd)) {
				OS_Printf("Session %d: %s\n", sessions[i].session_id,
				    sessions[i].cwd);

				session_data = (FILE_SESSION*)
				OS_LoadSession(&len2, SESSION_DATA, sessions[i].session_id);

				if (session_data) {
					num2 = len2 / sizeof(FILE_SESSION);

					for (i2 = 0; i2 < num2; i2++) {
						if (session_data[i2].sig == SESSION_SIG &&
						    session_data[i2].version == SESSION_VERSION &&
						    session_data[i2].total == num2 && session_data[i2].
						    size == sizeof(FILE_SESSION)) {
							OS_Printf("    %s\n", session_data[i2].pathname);
						}
					}
					OS_FreeSession(session_data);
				}
			}
		OS_FreeSession(sessions);
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char*SessionFilename(int index)
{
	return (session_filenames[index]);
}



