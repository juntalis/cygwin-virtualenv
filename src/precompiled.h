/**
 * precompiled.h - Common stuff
 *
 * Contrary to the filename, this header just contains macros, constants
 * and includes used throughout the application, and isn't actually a
 * precompiled header. Be that as it may, I didn't think "pretty.h" was
 * a very respectable header name, so I went with this.
 */

#ifndef _PRECOMPILED_H_
#define _PRECOMPILED_H_
#pragma once
 
/* Library Dependencies */
// Needed for registry stuff
#pragma comment(lib, "advapi32.lib")
// Needed for showing a message box
// on errors.
#ifndef NO_MSGBOX
#	pragma comment(lib, "user32.lib")
#endif

// Disable warnings.
#pragma warning(disable:4996 4995)

/* Force the default use of wchar_t. */
#ifdef _MBCS
#	undef _MBCS
#endif

#ifndef _UNICODE
#	define _UNICODE 1
#endif

#ifndef UNICODE
#	define UNICODE _UNICODE
#endif

/* MSVC-specific Macros */

// Speed up build process with minimal headers.
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#	define VC_EXTRALEAN
#endif

// MSVC's C compiler doesn't support the inline
// keyword.
#ifndef __cplusplus
//	To be safe..
#	ifdef inline
#		undef inline
#	endif
#	define inline __forceinline
#endif

// If our windows version is already defined,
// undefine to make sure we can set the proper
// minimum Windows version.
#ifdef WINVER
#	undef WINVER
#endif

#ifdef _WIN32_WINNT
#	undef _WIN32_WINNT
#endif

#ifdef NTDDI_VERSION
#	undef NTDDI_VERSION
#endif


// Target Windows XP & higher
#define WINVER 0x0502
#define _WIN32_WINNT 0x0502
#define NTDDI_VERSION 0x05020000

// x64/x86 Stuff
#if defined(_M_X64) || defined(_M_IA64)
#	pragma message("WARNING: Compiling a non-X86 version of this app removes the ability to use Cygwin to convert paths. This is not recommended.")
#	ifndef _WIN64
#		define _WIN64
#	endif
#	ifndef WIN64
#		define WIN64
#	endif
	// The "SOFTWARE" folder we care about.
#	define REG_SOFTWARE L"SOFTWARE\\Wow6432Node"

#elif defined(_M_IX86)
#	ifdef _WIN64
#		undef _WIN64
#	endif
#	ifdef WIN64
#		undef WIN64
#	endif
#	define USE_CYGWIN
#	define REG_SOFTWARE L"SOFTWARE"
#else
#	error Did not detect a targetable architecture!
#endif

/* Our includes */
#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <io.h>

/* Pretty-ify heavily-used constants */
#ifndef MAX_ENV
#	define MAX_ENV _MAX_ENV
#endif

/**
 * "Quick" note on the usage of the MAX_PATH macro:
 *
 * Filepaths can be longer on NTFS, but I'm not about to start using a size of
 * 32,000 for statically declared file path buffers, and having to a dynamically
 * allocate every buffer intended for use as a path would be too much of a pain
 * in the ass for this mini-project. Furthermore, support for that length of file
 * path would require a check on every drive accessed by this executable beforehand
 * to see whether or not it used NTFS. That all said, I cba.
 */
#ifndef MAX_PATH
#	define MAX_PATH _MAX_PATH
#endif

#ifndef DEBUG
#	ifdef _DEBUG
#		define DEBUG _DEBUG
#	endif
#endif

/* Pretty up (lowercase) commonly used types */
#ifndef bool
#	define bool BOOL
#	define true TRUE
#	define false FALSE
#endif

typedef WORD word;
typedef DWORD dword;
typedef BYTE byte;
#define ptr_type ULONG_PTR

/* My Constants */
// An empty unicode string. (Because I'm obsessive)
#define EMPTYW L""

/* Macros */
// The two macros below were grabbed off MSDN.
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

// Current function as a wchar string.
#define __WFUNCTION__ WIDEN(__FUNCTION__)

#endif /* _PRECOMPILED_H_ */