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


#ifndef __FIND_H
#define __FIND_H

#define MANUAL_SIG1 0x55
#define MANUAL_SIG2 0xAA

typedef struct findList
{
	unsigned char sig1;
	int len;
	int active;
	char command[MAX_FILENAME];
	char file_pathname[MAX_FILENAME];
	char pathname[MAX_FILENAME];
	char path[MAX_FILENAME];
	char filename[MAX_FILENAME];
	struct findList*next;
	unsigned char sig2;
}EDIT_FIND;

EDIT_FILE*ManualLoadFilename(char*cwd, char*pathname);
EDIT_FILE*LoadExistingFilename(char*cwd, char*pathname);
void AddManualLoadFilename(EDIT_FILE*file, char*pathname);
EDIT_FILE*FindManualLoadFilename(char*pathname);
void LoadFindState(char*cwd, char*command);
void SaveFindState(char*cwd, char*command);

#endif



