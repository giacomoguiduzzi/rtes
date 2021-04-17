/*
 * Copyright (C) 2019 Andrew Bonneville.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#ifndef SYSDBG_H_
#define SYSDBG_H_

#ifdef __cplusplus

#include <cstdio>
#include <cstddef>
extern "C" {

#else

#include <stdio.h>
#include <stddef.h>

#endif


typedef struct {
	size_t freeHeapSizeNow;
	size_t freeHeapSizeMin;
}_SysMemState;

void _SysMemCheckpoint(_SysMemState *);
int _SysMemDifference(_SysMemState *, _SysMemState *, _SysMemState *);
void _SysMemDumpStatistics(_SysMemState *);

#ifdef __cplusplus
}
#endif /* extern "C" */


#ifdef __cplusplus
#define _SysCheckMemory() SysCheckMemory(__FILE__, __LINE__, __func__)
struct SysCheckMemory
{
	_SysMemState state1;
	_SysMemState state2;
	_SysMemState state3;
	SysCheckMemory()
	{
		_SysMemCheckpoint(&state1);
	}

	SysCheckMemory(const char* file, size_t line, const char *func)
	{
		filename = file;
		lineNumber = line;
		funcName = func;

		_SysMemCheckpoint(&state1);
	}

	~SysCheckMemory()
	{
		_SysMemCheckpoint(&state2);

		if( _SysMemDifference( &state3, &state1, &state2) ) {
			std::printf("*** Error - Heap Memory Leak Detected ***\n");
			std::printf("File: %s\nFunc: %s\nLine: %u\n", filename, funcName, lineNumber);
			_SysMemDumpStatistics( &state3 );
		}
	}

	const char *filename;
	size_t lineNumber;
	const char *funcName;
};
#endif /* SysCheckMemory */


#endif /* SYSDBG_H_ */
