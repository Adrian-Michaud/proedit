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
#include <stdlib.h>
#include "proedit.h"

static int recursiveLoad = 0; /* Load command line files recurse dirs  */

static void DisplayCmdHelp(void);
static int GotoLineSwitch(EDIT_FILE*file, char*cmd);
static int ChangeRowsCols(char*cmd);
static int HighlightSwitch(EDIT_FILE*file, char*cmd);

char last_search[MAX_SEARCH];
char last_replace[MAX_SEARCH];

int ignoreCase = 0;
int globalSearch = 0;
int searchReplace = 0;
int globalSearchReplace = 0;
int forceText = 0;
int forceHex = 0;
int createBackups = 0;
int loadOptions = LOAD_FILE_CMDLINE;
int indenting = 0;
int stripWhitespace = 0;
int colorizing = 0;
int merge_nows = 0;
int tabsize = 0;
int force_crlf = MAINTAIN_CRLF;
int hex_endian = HEX_SEARCH_DEFAULT;
int force_new = 0;
int auto_build_saveall = 1;
int force_rows = 0;
int force_cols = 0;
int nospawn = 0;

static int loadAttempted = 0;
static int sessionNumber = 0;

static char*exeName;

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int EditorInit(int argv, char**argc)
{
	int retCode;

	exeName = argc[0];

	if (!ProcessCmdLine(argv, argc, 1))
		return (0);

	LoadConfig();

	retCode = InitDisplay();

	return (retCode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int EditorMain(int argv, char**argc)
{
	EDIT_FILE*file;

	if (!ProcessCmdLine(argv, argc, 0)) {
		CloseDisplay();
		return (0);
	}

	/* Disable forcing of hex/text */
	forceHex = 0;
	forceText = 0;

	LoadHistory();

	/* get first file */
	file = NextFile(0);

	if (!file && !loadAttempted) {
		if (!force_new)
			file = LoadSavedSessions(sessionNumber);
		else
			file = CreateUntitled();
	}

	while (file) {
		Paint(file);

		UpdateStatusBar(file);

		file = ProcessUserInput(file, 0);
	}

	SaveHistory();
	SaveMacros();

	#ifdef DEBUG_MEMORY
	DeallocHistory();
	FreeWordLists();
	CloseCalculator();
	ReleaseClipboard(GetClipboard());
	#endif

	SaveSessions(sessionNumber);

	CloseDisplay();

	OS_Exit();

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int ProcessCmdLine(int argv, char**argc, int preload)
{
	int i;
	EDIT_FILE*lastFile = 0;

	for (i = 1; i < argv; i++) {
		if (!OS_Strcasecmp(argc[i], "-nospawn")) {
			nospawn = 1;
			continue;
		}

		if (preload) {
			if (!OS_Strcasecmp(argc[i], "-?") || !OS_Strcasecmp(argc[i],
			    "-help") || !OS_Strcasecmp(argc[i], "--?") || !OS_Strcasecmp(
			    argc[i], "--help")) {
				DisplayCmdHelp();
				return (0);
			}

			if (argc[i][0] == '-' && (argc[i][1] == 's' || argc[i][1] == 'S') &&
			    (argc[i][2] == 'e' || argc[i][2] == 'E')) {
				DisplaySessions();
				return (0);
			}

			if (argc[i][0] == '-' && (argc[i][1] == 'd' || argc[i][1] == 'D') &&
			    (argc[i][2] == 'i' || argc[i][2] == 'I') && (argc[i][3] == 'm'
			    || argc[i][3] == 'M')) {
				if (!ChangeRowsCols(&argc[i][4]))
					return (0);
			}
		} else {
			if (!OS_Strcasecmp(argc[i], "-r")) {
				recursiveLoad = 1;
			} else
				if (argc[i][0] == '-' && (argc[i][1] == 'h' || argc[i][1] ==
				    'H') && (argc[i][2] == 'e' || argc[i][2] == 'E')) {
					forceHex = 1;
					forceText = 0;
				} else
					if (argc[i][0] == '-' && (argc[i][1] == 'n' || argc[i][1] ==
					    'N') && (argc[i][2] == 'e' || argc[i][2] == 'E')) {
						force_new = 1;
					} else
						if (argc[i][0] == '-' && (argc[i][1] == 'm' ||
						    argc[i][1] == 'M') && (argc[i][2] == 'u' ||
						    argc[i][2] == 'U')) {
							loadOptions |= LOAD_FILE_EXISTS;
						} else
							if (argc[i][0] == '-' && (argc[i][1] == 'n' ||
							    argc[i][1] == 'N') && (argc[i][2] == 'o' ||
							    argc[i][2] == 'o')) {
								loadOptions |= LOAD_FILE_NOWILDCARD;
							} else
								if (argc[i][0] == '-' && (argc[i][1] == 't' ||
								    argc[i][1] == 'T') && (argc[i][2] == 'e' ||
								    argc[i][2] == 'E')) {
									forceText = 1;
									forceHex = 0;
								} else
									if (argc[i][0] == '-' && (argc[i][1] ==
									    'c' || argc[i][1] == 'C') &&
									    (argc[i][2] == 'r' || argc[i][2] ==
									    'R')) {
										force_crlf = FORCE_CRLF;
										SetConfigInt(CONFIG_INT_FORCE_CRLF,
										    force_crlf);
									} else
										if (!OS_Strcasecmp(argc[i], "-lf")) {
											force_crlf = FORCE_LF;
											SetConfigInt(CONFIG_INT_FORCE_CRLF,
											    force_crlf);
										} else
											if (argc[i][0] == '-' &&
											    (argc[i][1] == 'g' ||
											    argc[i][1] == 'G')) {
												if (!GotoLineSwitch(lastFile, &
												    argc[i][2]))
													return (0);
											} else
												if (argc[i][0] == '-' &&
												    (argc[i][1] == 'e' ||
												    argc[i][1] == 'E') &&
												    (argc[i][2] == 'n' ||
												    argc[i][2] == 'N') &&
												    (argc[i][3] == 'd' ||
												    argc[i][3] == 'D')) {
													GotoPosition(lastFile,
													    lastFile->number_lines +
													    1, 1);
												} else
													if (argc[i][0] == '-' && (
													    argc[i][1] == 's' ||
													    argc[i][1] == 'S')) {
														sessionNumber = atoi(&
														    argc[i][2]);
													} else
														if (argc[i][0] == '-' &&
														    (argc[i][1] == 'h'
														    || argc[i][1] ==
														    'H')) {
															if (!HighlightSwitch
															    (lastFile,
															    &argc[i][2]))
																return (0);
														} else
															if (argc[i][0] ==
															    '-' &&
															    (argc[i][1] ==
															    'f' ||
															    argc[i][1] ==
															    'F')) {
																strcpy(
																    last_search,
																    &argc[i][2
																    ]);
															} else
																if (!
																    OS_Strcasecmp(argc[i], "-c")) {
																	ignoreCase =
																	    0;
																	SetConfigInt
																	    (
																	    CONFIG_INT_CASESENSITIVE, 1);
																} else
																	/* The -dim switch is handled in preload */
																	if (argc[i][
																	    0] ==
																	    '-' &&
																	    (argc[i]
																	    [1] ==
																	    'd' ||
																	    argc[i][
																	    1] ==
																	    'D') &&
																	    (argc[i]
																	    [2] ==
																	    'i' ||
																	    argc[i][
																	    2] ==
																	    'I') &&
																	    (argc[i]
																	    [3] ==
																	    'm' ||
																	    argc[i][
																	    3] ==
																	    'M')) {
																		ChangeRowsCols(&argc[i][4]);
																	} else
																		if (argc
																		    [i][
																		    0]
																		    ==
																		    '-'
																		    &&
																		    (
																		    argc
																		    [i][
																		    1]
																		    ==
																		    't'
																		    ||
																		    argc
																		    [i][
																		    1]
																		    ==
																		    'T')
																		    &&
																		    (
																		    argc
																		    [i][
																		    2]
																		    ==
																		    'a'
																		    ||
																		    argc
																		    [i][
																		    2]
																		    ==
																		    'A')
																		    &&
																		    (
																		    argc
																		    [i][
																		    3]
																		    ==
																		    'b'
																		    ||
																		    argc
																		    [i][
																		    3]
																		    ==
																		    'B')) {
																			tabsize = atoi(&argc[i][4]);
																			SetConfigInt(CONFIG_INT_TABSIZE, tabsize);
																		} else
																			if (
																			    argc[i][0] == '-') {
																				OS_Printf("ProEdit error: Unknown command line option: %s\n", argc[i]);
																				return (0);
																			}
																			    else {
																				loadAttempted++;

																				if (recursiveLoad)
																					lastFile = LoadFileRecurse(lastFile, argc[i], loadOptions);
																				else
																					lastFile = LoadFileWildcard(lastFile, argc[i], loadOptions);
																			}
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
static int ChangeRowsCols(char*cmd)
{
	int rows = 0, cols = 0;

	sscanf(cmd, "%d,%d", &cols, &rows);

	if (rows < 25 || cols < 40) {
		OS_Printf(
		    "ProEdit error: The -dim command is out of range (-dim%d,%d)\n",
		    cols, rows);
		return (0);
	}
	force_cols = cols;
	force_rows = rows;
	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int GotoLineSwitch(EDIT_FILE*file, char*cmd)
{
	int line = 1, col = 1;
	int address = 0;

	if (!file) {
		OS_Printf(
		    "ProEdit error: The -g command line option can only be used after\n");
		OS_Printf(
		    "               a filename has been specified on the command line.\n");
		return (0);
	}

	if (file->hexMode)
		sscanf(cmd, "%d", &address);
	else
		sscanf(cmd, "%d,%d", &line, &col);

	if (line <= 0)
		line = 1;

	if (col <= 0)
		col = 1;

	if (file->hexMode)
		GotoHex(file, address);
	else
		GotoPosition(file, line, col);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int HighlightSwitch(EDIT_FILE*file, char*cmd)
{
	int line = 1, col1 = 1, col2 = 1, len = 0;
	long address = 0;

	if (!file) {
		OS_Printf(
		    "ProEdit error: The -h command line option can only be used after\n");
		OS_Printf(
		    "               a filename has been specified on the command line.\n");
		return (0);
	}

	if (file->hexMode)
		sscanf(cmd, "%ld,%d", &address, &len);
	else
		sscanf(cmd, "%d,%d,%d", &line, &col1, &col2);

	if (line <= 0)
		line = 1;

	if (col1 <= 0)
		col1 = 1;

	if (col2 <= 0)
		col2 = 1;

	col1 = MIN(col1, col2);
	col2 = MAX(col1, col2);

	if (file->hexMode) {
		SetupHexSelectBlock(file, (int)address, len);
		file->hexMode = HEX_MODE_TEXT;
	} else
		SetupSelectBlock(file, line - 1, col1 - 1, (col2 - col1) + 1);

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DisplayCmdHelp(void)
{
	OS_Printf("\n%s %s Multi-Platform programming editor", PROEDIT_MP_TITLE,
	    PROEDIT_MP_VERSION);
	OS_Printf("\nDesigned/Developed/Produced by Adrian J. Michaud");
	OS_Printf("\nLast ProEdit Core Build: %s %s", __DATE__, __TIME__);
	OS_Printf("\nEMail: x86guru@yahoo.com");
	OS_Printf("\n\n");

	OS_Printf("Usage: %s [filename] [options] [filename] [options] ...\n\n",
	    exeName);
	OS_Printf("Command line options:\n\n");
	OS_Printf(
	    " -text . . . . . . . . . . . . . .  Force text mode for all files\n");
	OS_Printf(
	    " -hex  . . . . . . . . . . . . . .  Force hex mode for all files\n");
	OS_Printf(" -end  . . . . . . . . . . . . . .  Goto end of file\n");
	OS_Printf(
	    " -g<line>,[column] . . . . . . . .  Goto line,column (For Text Files)\n");
	OS_Printf(
	    " -g<address> . . . . . . . . . . .  Goto Address (For Binary Files)\n");
	OS_Printf(
	    " -h<line>,<from column>,<to column> Highlight (For Text Files)\n");
	OS_Printf(
	    " -h<address>,<len>  . . . . . . . . Highlight (For Binary files)\n");
	OS_Printf(
	    " -mustexist . . . . . . . . . . . . Only load files that exist\n");
	OS_Printf(
	    " -nowildcards . . . . . . . . . . . Don't use wildcards when loading files\n");
	OS_Printf(" -sessions  . . . . . . . . . . . . Display saved sessions\n");
	OS_Printf(
	    " -s<Session Number> . . . . . . . . Load/Save Specified Session Number\n");
	OS_Printf(" -new . . . . . . . . . . . . . . . Force new session\n");
	OS_Printf(
	    " -crlf  . . . . . . . . . . . . . . Force: Save Text files using a CR/LF\n");
	OS_Printf(
	    " -lf  . . . . . . . . . . . . . . . Force: Save Text files using only a LF\n");
	OS_Printf(
	    " -c . . . . . . . . . . . . . . . . Set case sensitivity defaut ON\n");
	OS_Printf(" -r . . . . . . . . . . . . . . . . Recurse into directories\n");
	OS_Printf(" -help. . . . . . . . . . . . . . . Display this help\n");
	OS_Printf(
	    " -dim<Rows>,<Cols>  . . . . . . . . Change default Display Size\n");
	OS_Printf(" -f<string> . . . . . . . . . . . . Setup the Search string\n");
	OS_Printf(" -tab<Tab Offset> . . . . . . . . . Change Default Tab Size\n");
	OS_Printf(" -nospawn . . . . . . . . . . . . . Do not spawn new window\n");
	OS_Printf(" Note: [...] are optional <...> are required\n");
}


