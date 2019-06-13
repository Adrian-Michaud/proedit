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

#ifdef DEBUG_MEMORY

#undef OS_Malloc
#undef OS_Free
#undef OS_Exit

typedef struct mallocHeader
{
	int sig1;
	int size;
	int unaligned_size;
	int freed;
	char*file;
	int line;
	struct mallocHeader*next;
	struct mallocHeader*prev;
	int sig2;
}MALLOC_HEADER;

static MALLOC_HEADER*mallocList;

#define MALLOC_SIG1 0x12345678
#define MALLOC_SIG2 0x87654321

long total_mallocs;
long total_frees;
long total_mem;
long max_mem;

void DumpMallocList(void);
void RemoveMallocList(MALLOC_HEADER*header);
void AddMallocList(MALLOC_HEADER*header);
void DisplayAlloc(MALLOC_HEADER*header);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void*DebugMalloc(long size, char*file, int line)
{
	MALLOC_HEADER*header;
	MALLOC_HEADER*tail;
	int unaligned_size;

	unaligned_size = size;

	size = (size + (MIN_ALIGN - 1))&MIN_ALIGN_MASK;

	header = (MALLOC_HEADER*)malloc(sizeof(MALLOC_HEADER) + size + sizeof(
	    MALLOC_HEADER));

	if (!header) {
		OS_Printf("\nProEdit MP: Out of memory error in %s on line %d\n", file,
		    line);
		OS_Exit();
	}

	header->size = size;
	header->unaligned_size = unaligned_size;
	header->sig1 = MALLOC_SIG1;
	header->sig2 = MALLOC_SIG2;
	header->file = file;
	header->line = line;
	header->freed = 0;

	tail = (MALLOC_HEADER*)((unsigned long)header + sizeof(MALLOC_HEADER) +
	    size);

	tail->size = size;
	tail->unaligned_size = unaligned_size;
	tail->sig1 = MALLOC_SIG1;
	tail->sig2 = MALLOC_SIG2;
	tail->file = file;
	tail->line = line;
	tail->freed = 0;

	total_mallocs++;
	total_mem += size;

	if (total_mem > max_mem)
		max_mem = total_mem;

	AddMallocList(header);

	return ((char*)header + sizeof(MALLOC_HEADER));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DebugExit(void)
{
	DumpMallocList();
	OS_Exit();
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DebugFree(void*data, char*file, int line)
{
	MALLOC_HEADER*header, *tail;

	header = (MALLOC_HEADER*)((unsigned long)data - sizeof(MALLOC_HEADER));
	tail = (MALLOC_HEADER*)((unsigned long)data + header->size);

	if (header->freed) {
		OS_Printf("\nProEdit MP: Memory already freed error in %s on line %d\n",
		    file, line);
		DisplayAlloc(header);
		OS_Exit();
	}

	if (header->sig1 != (int)MALLOC_SIG1 || header->sig2 != (int)MALLOC_SIG2) {
		OS_Printf("\nProEdit MP: Bogus Memory Free from %s on line %d\n", file,
		    line);
		DisplayAlloc(header);
		OS_Exit();
	}

	if (tail->sig1 != (int)MALLOC_SIG1 || tail->sig2 != (int)MALLOC_SIG2) {
		OS_Printf("\nProEdit MP: Buffer overwrite detected in %s on line %d\n",
		    file, line);
		DisplayAlloc(header);
		OS_Exit();
	}


	header->freed = 1;

	total_mem -= header->size;

	total_frees++;

	RemoveMallocList(header);

	OS_Free(header);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void AddMallocList(MALLOC_HEADER*header)
{
	header->prev = 0;
	header->next = mallocList;

	if (mallocList)
		mallocList->prev = header;

	mallocList = header;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void RemoveMallocList(MALLOC_HEADER*header)
{
	if (header->prev)
		header->prev->next = header->next;

	if (header->next)
		header->next->prev = header->prev;

	if (mallocList == header)
		mallocList = header->next;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DumpMallocList(void)
{
	MALLOC_HEADER*walk;

	walk = mallocList;

	while (walk) {
		DisplayAlloc(walk);
		OS_Printf("\n");
		walk = walk->next;
	}
}

/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void DisplayAlloc(MALLOC_HEADER*header)
{
	if (header) {
		OS_Printf("sig1  = %08x (%08x)\n", header->sig1, MALLOC_SIG1);
		OS_Printf("size  = %d  \n", header->size);
		OS_Printf("freed = %d  \n", header->freed);
		OS_Printf("file  = %s  \n", header->file);
		OS_Printf("line  = %d  \n", header->line);
		OS_Printf("next  = %08x\n", header->next);
		OS_Printf("prev  = %08x\n", header->prev);
		OS_Printf("sig2  = %08x (%08x)\n", header->sig2, MALLOC_SIG2);
	}
}

#endif /* DEBUG_MEMORY */


