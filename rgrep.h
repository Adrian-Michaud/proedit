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

#ifndef __RGREP_H__
#define __RGREP_H__

#if 0
#define DEBUG_MEMORY
#endif

#define RGREP_MP_VERSION "RecursiveGrep MP 2.0"

#define MAX_FILENAME      256
#define MAX_LIST_FILES    4096
#define MAX_SEARCH_STRING 1024

#ifdef DEBUG_MEMORY
#define OS_Malloc(size) DebugMalloc((size),__FILE__,__LINE__);
#define OS_Free(ptr)    DebugFree((ptr),__FILE__,__LINE__);
#define OS_Exit         DebugExit

void*DebugMalloc(long size, char*file, int line);
void DebugFree(void*ptr, char*file, int line);
void DebugExit(void);
#endif

#endif /* __RGREP_H__ */



