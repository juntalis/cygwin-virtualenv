/**
 * verbosity.c - Contains functions and variables for the verbosity 
 *               functionality provided when a user specifies -v on
 *               the application's command line. Can be excluded when
 *               building by specifying --without-verbosity on the build
 *               script's command line. This could probably be a lot more
 *               efficient, but it was an afterthought.
 */

#ifndef _PRECOMPILED_H_
#	error This file contains declarations for main.c and should not be compiled by itself. Instead, compile main.c
#endif

static bool verbose_flag = false;
static int verbose_stepi = 0;

typedef void(*verbose_func)(wchar_t*);

static void verbose_nocheck(wchar_t* message)
{
	wprintf(L"# %s\n", message);
	verbose_stepi = 0;
}

static void verbose_step_nocheck(wchar_t* message)
{
	wprintf(L"# [%d] %s\n", verbose_stepi++, message);
}

/**
 * Steps through and prints each value in an array.
 */
static void verbose_array(int argc, wchar_t* argv[])
{
	int i;
	if(!verbose_flag || !argv || !argc) return;
	for(i = 0; i < argc; i++) {
		if(!argv[i]) continue;
		verbose_step_nocheck(argv[i]);
	}
}

/** Real declaration of both verbose and verbose_step. */ 
static void __verbose(verbose_func func, wchar_t* message, ...)
{
	int lenbuf;
	wchar_t* buf;
	va_list args;
	if(!verbose_flag || !message) return;
	va_start(args, message);
	lenbuf = _vscwprintf(message, args);
	buf = walloc(lenbuf);
	vswprintf(buf, lenbuf+1, message, args);
	va_end(args);
	func(buf);
	xfree(buf);
}

#define verbose_step(...) __verbose(&verbose_step_nocheck, __VA_ARGS__)
#define verbose(...) __verbose(&verbose_nocheck, __VA_ARGS__)

static void check_verbosity(int argc, wchar_t* argv[], wchar_t* sCygRoot, wchar_t* sTarget, wchar_t* sPATH)
{
	int i = 0;
	
	// Check for the verbose flag.
	while(++i < argc && argv[i] && argv[i][0] == L'-') {
		if(argv[i][1] == 'v') {
			verbose_nocheck(L"Detected verbose flag. Enabling debug output.");
			verbose_nocheck(L"Resolved cygwin root..");
			verbose_step_nocheck(sCygRoot);
			verbose_nocheck(L"Resolved virtualenv interpreter..");
			verbose_step_nocheck(sTarget);
			verbose_nocheck(L"Setting minimals for PATH environment variable.");
			verbose_step_nocheck(sPATH);
			verbose_flag = true;
			break;
		}
	}
}