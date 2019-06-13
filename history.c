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

#define HISTORY_VERSION 2
#define HISTORY_SIG     0x55aa55aa

#define MAX_HISTORY 30

#define LOHI16(a,b) ((unsigned int)(a)|((unsigned int)(b) << 8))
#define LOW16(a)     ((unsigned int)(a) & 0xff)
#define HI16(a)     (((unsigned int)(a) >> 8) & 0xff)

static char*historys[NUM_HISTORY_TYPES][MAX_HISTORY]; /* History text storage */
static int offsets[MAX_HISTORY]; /* History indexs       */

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SelectHistoryText(int historyIndex)
{
	int i, total, line, max = 0;
	char*list[MAX_HISTORY];
	char*title;

	total = QueryHistoryCount(historyIndex);

	if (!total)
		return (0);

	for (i = 0; i < total; i++) {
		list[i] = (char*)OS_Malloc(strlen(QueryHistory(historyIndex, i)) + 1);

		strcpy(list[i], QueryHistory(historyIndex, i));

		if ((int)strlen(list[i]) + 1 > max)
			max = strlen(list[i]) + 1;
	}

	title = "Input History";

	if (max < (int)strlen(title) + 6)
		max = (int)strlen(title) + 6;

	line = PickList(title, total, max, title, list, total);

	for (i = 0; i < total; i++)
		OS_Free(list[i]);

	return (line);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void AddToHistory(int index, char*text)
{
	int i, j;
	char*saved;

	i = QueryHistoryExists(index, text);

	/* If it already exists, move it to the top of the list. */
	if (i) {
		saved = historys[index][i - 1];

		for (j = i - 1; j < offsets[index]-1; j++)
			historys[index][j] = historys[index][j + 1];

		historys[index][j] = saved;
		return ;
	}

	i = offsets[index];

	if (i >= MAX_HISTORY) {
		OS_Free(historys[index][0]);

		for (i = 0; i < MAX_HISTORY - 1; i++)
			historys[index][i] = historys[index][i + 1];

		offsets[index] = MAX_HISTORY - 1;
	}

	historys[index][i] = (char*)OS_Malloc(strlen(text) + 1);

	strcpy(historys[index][i], text);

	offsets[index]++;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void SaveHistory(void)
{
	unsigned int len = 0;
	unsigned char*data;
	unsigned int binLen = 0;
	unsigned int*size;
	int types, i;
	int stringLen;

	for (types = 0; types < NUM_HISTORY_TYPES; types++)
		for (i = 0; i < MAX_HISTORY; i++)
			if (historys[types][i])
				len += strlen(historys[types][i]) + 20;
			else
				len += 20;

	size = (unsigned int*)OS_Malloc(sizeof(offsets) + len);

	data = (unsigned char*)size;

	size[1] = (unsigned int)sizeof(offsets);
	size[2] = (unsigned int)HISTORY_VERSION;
	size[3] = (unsigned int)HISTORY_SIG;
	binLen += sizeof(int)*4;

	memcpy(&data[binLen], offsets, sizeof(offsets));

	binLen += sizeof(offsets);

	for (types = 0; types < NUM_HISTORY_TYPES; types++)
		for (i = 0; i < MAX_HISTORY; i++)
			if (historys[types][i]) {
				stringLen = strlen(historys[types][i]) + 1;

				data[binLen++] = (unsigned char)types;
				data[binLen++] = (unsigned char)i;
				data[binLen++] = (unsigned char)LOW16(stringLen);
				data[binLen++] = (unsigned char)HI16(stringLen);
				memcpy(&data[binLen], historys[types][i], stringLen);
				binLen += stringLen;
			} else {
				data[binLen++] = (unsigned char)types;
				data[binLen++] = (unsigned char)i;
				data[binLen++] = 0;
				data[binLen++] = 0;
			}

	size[0] = binLen;

	OS_SaveSession(data, binLen, SESSION_HISTORY, 0);

	OS_Free(data);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void LoadHistory(void)
{
	unsigned char*data;
	unsigned int*size;
	int index = 0;
	int types, i;
	int len, stringLen;

	size = (unsigned int*)OS_LoadSession(&len, SESSION_HISTORY, 0);

	data = (unsigned char*)size;

	if (data) {
		if (size[0] == (unsigned int)len && size[1] == sizeof(offsets) &&
		    size[2] == HISTORY_VERSION && size[3] == HISTORY_SIG) {
			index = sizeof(int)*4;

			memcpy(offsets, &data[index], sizeof(offsets));

			index += sizeof(offsets);

			for (types = 0; types < NUM_HISTORY_TYPES; types++)
				for (i = 0; i < MAX_HISTORY; i++) {
					if (data[index] == types && data[index + 1] == i) {
						stringLen = LOHI16(data[index + 2], data[index + 3]);

						if (stringLen) {
							historys[types][i] = OS_Malloc(stringLen);

							memcpy(historys[types][i], &data[index + 4],
							    stringLen);
							index += (4 + stringLen);
						} else
							index += 4;
					} else {
						OS_FreeSession(data);
						#ifdef DEBUG_MEMORY
						DeallocHistory();
						#endif
						return ;
					}
				}
		}

		OS_FreeSession(data);
	}
}


#ifdef DEBUG_MEMORY
/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DeallocHistory(void)
{
	int types, i;

	for (types = 0; types < NUM_HISTORY_TYPES; types++)
		for (i = 0; i < MAX_HISTORY; i++)
			if (historys[types][i])
				OS_Free(historys[types][i]);
}
#endif

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int QueryHistoryExists(int index, char*text)
{
	int j, i;

	i = QueryHistoryCount(index);

	for (j = 0; j < i; j++)
		if (!strcmp(text, QueryHistory(index, j)))
			return (j + 1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int QueryHistoryCount(int index)
{
	return (offsets[index]);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
char*QueryHistory(int index, int number)
{
	return (historys[index][number]);
}


