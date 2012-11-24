/**
 * util.c - Utility functions that were taking up too
 *          much room in main.c
 */

#ifndef _PRECOMPILED_H_
#	error This file contains declarations for main.c and should not be compiled by itself. Instead, compile main.c
#endif

/** Fatal error handlers. */
#define fatal_api_call(f) fatal(0L, f)
#ifdef DEBUG
static void (*fatal)(dword dw, wchar_t* message, ...) = ((void(*)(dword dw, wchar_t* message, ...))NULL);
#else
static void fatal(dword dw, wchar_t* message, ...) 
{
	void *lpDisplayBuf, *lpMsgBuf;
	
	if(dw == 0) {
		// If no return code was specified, we assume that the message
		// contains a function name that failed. In that case, we retrieve
		// the system error message for the last-error code
		dw = GetLastError();
		
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(wchar_t*) &lpMsgBuf,
			0,
			NULL
		);

		// Allocate our buffer for the error message.
		lpDisplayBuf = (void*)LocalAlloc(
			LMEM_ZEROINIT,
			(lstrlenW((const wchar_t*)lpMsgBuf) + lstrlenW((const wchar_t*)message) + 47) * sizeof(wchar_t)
		);
		StringCchPrintfW(
			(wchar_t*)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(wchar_t),
			L"FATAL: %s failed with error %d: %s",
			message,
			dw,
			lpMsgBuf
		);
	} else {
		// Otherwise, we assume that the error message is a format string.
		va_list args = NULL;
		
		// Allocate buffer for our resulting format string.
		lpMsgBuf = (void*)LocalAlloc(
			LMEM_ZEROINIT,
			(lstrlenW((const wchar_t*)message) + 8) * sizeof(wchar_t)
		);
		StringCchPrintfW(
			(wchar_t*)lpMsgBuf,
			LocalSize(lpMsgBuf) / sizeof(wchar_t),
			L"FATAL: %s",
			message
		);
		
		// Might as well use the maximum allowed buffer, since there's no way I know of the
		// get the size of the resulting buff.
		lpDisplayBuf = (void*)LocalAlloc(LMEM_ZEROINIT, STRSAFE_MAX_CCH * sizeof(wchar_t));
		va_start(args, lpMsgBuf);
		StringCbVPrintfW(
			(wchar_t*)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(wchar_t),
			lpMsgBuf,
			args
		);
		va_end(args);
	}
	#ifndef NO_MSGBOX
	MessageBoxW(GetConsoleWindow(), (const wchar_t*)lpDisplayBuf, L"Fatal Error", MB_OK);
	#else
	wprintf(L"%s\n", lpDisplayBuf);
	#endif
	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw); 
}
#endif

/**
 * Inline helpers for allocating memory for strings/arrays
 * and zero'ing it out.
 */
static inline void* xalloc(size_t count, size_t sz)
{
	wchar_t* result = NULL;
	size_t szresult = (count + 1) * sz;
	if(!(result = (wchar_t*)malloc(szresult))) {
		fatal_api_call(L"xalloc");
	}
	memset((void*)result, 0, szresult);
	return result;
}

#define walloc(c) (wchar_t*)xalloc((size_t)c, (size_t)sizeof(wchar_t))
#define waalloc(c) (wchar_t**)xalloc((size_t)c, (size_t)sizeof(wchar_t*))

/**
 * Helper for adding another entry to a wstring array. Arg is a
 * pointer to the actual array pointer so that we can return a pointer
 * to the last writable entry in the array, and still be able to change
 * the address the array is pointing to, in case the memory block has to
 * be moved to resize.
 */
static inline wchar_t* waadd(wchar_t*** aptr, wchar_t* entry)
{
	size_t szcount = -1;
	wchar_t	*result = NULL;
	wchar_t **ptr;
	
	if(!aptr || !(ptr = *aptr)) fatal(1, L"NULL pointer passed to waadd");
	while(ptr[++szcount] != NULL);
	ptr[szcount] = _wcsdup(entry);
	result = ptr[szcount++];
	if(!(*aptr = (wchar_t**)_recalloc((void*)ptr, szcount, sizeof(wchar_t*)))) {
		fatal_api_call(L"waadd");
	}
	ptr = *aptr;
	ptr[szcount] = NULL;
	return result;
}

/**
 * Check if a wchar string array contains a particular entry. Additional
 * arg for whether or not to worry about case sensitivity.
 */
static inline size_t _wacontains(wchar_t** ptr, wchar_t* entry, bool csensitive)
{
	size_t szentry, i = -1;
	wchar_t* readPart;
	if(!ptr || !entry) {
		fatal(1, L"NULL pointer passed to _wacontains");
	}
	
	szentry = wcslen(entry);
	if(!szentry) { return -1; }
	
	while(readPart = ptr[++i]) {
		size_t szread = wcslen(readPart);
		if(szread != szentry) {
			continue;
		}
		if(csensitive) {
			if(wcscmp(entry, readPart) == 0) {
				return i;
			}
		} else {
			if(_wcsicmp(entry, readPart) == 0) {
				return i;
			}
		}
	}
	
	return -1;
}

#define wacontains(x,y) _wacontains(x,y,true)
#define waicontains(x,y) _wacontains(x,y,false)

/**
 * Inline helper for safely freeing allocated data.
 * Along with wafree, these functions were grabbed
 * from the cygspawn project:
 *   https://github.com/mturk/cygspawn
 */
static inline void xfree(void *m)
{
	if (m != NULL) free(m);
}

/** Helper for freeing a wchar string array. */
static void wafree(wchar_t **array)
{
	wchar_t **ptr = array;
	if (array == NULL) return;
	while (*ptr != NULL) xfree((void*)*(ptr++));
	xfree(array);
}

/** Simple inline helper to check if a path exists. */
static inline bool exists(wchar_t* path)
{
	return _waccess((const wchar_t*)path, 00) != -1;
}

/**
 * Simple inline helper to check if a given path exists, and
 * represent a folder.
 */
static inline bool is_folder(wchar_t* path)
{
	return GetFileAttributesW((const wchar_t*)path) & FILE_ATTRIBUTE_DIRECTORY;
}

/**
 * Simple inline helper to check if a given path exists, and
 * represent a file.
 */
static inline bool is_file(wchar_t* path)
{
	return exists(path) && !is_folder(path);
}

/**
 * Yet another inline helper for determining whether
 * a given path has the SYSTEM attribute flagged on it.
 */
static inline bool has_system_attr(wchar_t* path)
{
	return GetFileAttributesW(path) & FILE_ATTRIBUTE_SYSTEM;
}

/** Inline helper to get the parent folder and length of a the parent quickly */
static inline bool get_dirname(wchar_t* sExecutable, size_t lpszExecutable, wchar_t* sParentOut, size_t* lpszParentOut)
{
	if(!lstrcpynW(sParentOut, sExecutable, (int)lpszExecutable)) return false;
	*lpszParentOut = lpszExecutable;
	while(sParentOut[--*lpszParentOut] != L'\\' && *lpszParentOut) sParentOut[*lpszParentOut] = L'\0';
	
	// Remove the trailing new line.
	if(*lpszParentOut) sParentOut[*lpszParentOut] = L'\0';
	return (*lpszParentOut > 0) && is_folder(sParentOut);
}


/** Get size of file, given its handle. */
static inline size_t get_file_length(FILE* pFile)
{
	long lCurrPos, lEndPos;
	size_t result = -1;
	lCurrPos = ftell(pFile);
	if(lCurrPos == -1)
		return result;
	if(fseek(pFile, 0L, SEEK_END) == -1)
		return result;
	lEndPos = ftell(pFile);
	if(lEndPos == -1)
		return result;
	result = (size_t)(lEndPos - lCurrPos);
	if(fseek(pFile, 0L, SEEK_SET) == -1)
		return -1;
	return result;
}

/** Allocate a buffer for the contents of a file and read the file into it. */
static inline byte* file_to_buffer(wchar_t* sPath, size_t* size)
{
	FILE* fp;
	byte* buffer = NULL;
	fp = _wfopen((const wchar_t*)sPath, L"rb");
	if (fp == NULL) {
		fatal(1, L"Could not read file: %s", sPath);
	}
	
	*size = get_file_length(fp);
	if(*size == -1) {
		fatal(1, L"Could not get size of file: %s", sPath);
	}
	
	if(!(buffer = (byte*)xalloc(*size, 1))) {
		fatal(1, L"Could not allocate %d bytes of memory for the contents of file: %s", *size, sPath);
	}
	ZeroMemory(buffer, *size);
	fread(buffer, 1, *size, fp);
	fclose(fp);
	return buffer;
}