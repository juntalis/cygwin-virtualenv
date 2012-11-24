/**
 * main.c - The meat of the application. Includes all other files
 *          in this repository.
 */
#include "precompiled.h"

// Cygwin registry keys
#define CYGWIN_SUBKEY L"rootdir"
#define CYGWIN_REGKEY REG_SOFTWARE L"\\Cygwin\\setup"

// Our utility functions.
#include "util.c"

#ifndef WITHOUT_VERBOSITY
#	include "verbosity.c"
#else
#	define verbose_array(x) 
#	define verbose_step(x,...) 
#	define verbose(x, ...) 
#	define check_verbosity(a, b, c, d, e) 
#endif

#ifdef USE_CYGWIN
#	include "cygwin.c"
#endif

/** Allocates, and adds quotes to an arg. */
static wchar_t* quote_arg(wchar_t* arg)
{
	wchar_t* result = NULL;
	size_t szcount = 0;
	
	// Check for EMPTYW args. If found, don't both adding quotes.
	szcount = lstrlenW(arg);
	if(szcount == 0) {
		return walloc(0);
	}
	
	szcount += 2;
	result = walloc(szcount);
	szcount = (szcount + 1) * sizeof(wchar_t);
	
	// Now set result to the value of arg, with surrounding quotes.
	if(FAILED(StringCchPrintfW(result, szcount, L"\"%s\"", arg))) {
		fatal_api_call(L"quote_arg");
	}
	
	return result;
}

/**
 * Iterates through our args, quoting all of them and in the cases of paths,
 * converting them to a cygwin-compatible format.
 */
static wchar_t** fix_argv(int argc, wchar_t** argv, bool useCygwin)
{
	int i;
	wchar_t** result = NULL;
	
	// First allocate our array of arguments.
	verbose(L"Allocating new arg buffer..");
	result = waalloc(argc);
	
	// Duplicate arg 0, which does not need quoting.
	#ifdef USE_CYGWIN
	if(!(result[0] = fix_path(argv[0]))) fatal_api_call(L"real_path");
	#else
	if(!(result[0] = _wcsdup(argv[0]))) fatal_api_call(L"wcsdup");
	#endif
	// Now, begin the fixes.
	for(i = 1; i < argc; i++) {
		#ifdef USE_CYGWIN
		wchar_t* check = NULL;
		if(useCygwin && *(argv[i]) && is_file(argv[i])) {
			verbose(L"Detected convertable path argument.");
			verbose_step(argv[i]);
			if(check = fix_path(argv[i])) {
				argv[i] = check;
			} else {
				useCygwin = false;
			}
		}
		#endif
		
		// Quote the argument for our spawn call.
		result[i] = quote_arg(argv[i]);
		
		#ifdef USE_CYGWIN
		if(useCygwin && check) xfree(check);
		#endif
	}
	
	#ifdef USE_CYGWIN
	if(useCygwin) {
		#ifndef WITHOUT_ENVVARS
		fix_env();
		#endif
		FreeLibrary(*phCygwin);
	}
	#endif
	
	return result;
}

/**
 * Execute our command. If no args are specified, we can simply
 * execute the program. (In hindsight, I probably should've done
 * the environment variable stuff anyways, but since I still think
 * it's unnecessary, I don't feel like wasting the time.
 */
static int exec_cmd(wchar_t* cmd, int argc, wchar_t** argv)
{
	int r;
	if(argc > 1) {
		wchar_t** args;
		bool useCygwin = false;
		// Fuck, now we actually have to load cygwin to convert the paths from Windows -> Cygwin format.
		#ifdef USE_CYGWIN
		useCygwin = setup_cygwin();
		verbose(L"Fixing executable name..");
		cmd = real_path(cmd);
		argv[0] = cmd;
		#endif
		
		verbose(L"Fixing up argv..");
		args = fix_argv(argc, argv, useCygwin);
		
		verbose(L"Executing..");
		verbose_array(argc, args);
		r = (int)_wspawnvp(_P_WAIT, cmd,  (const wchar_t* const*)args);
		
		wafree(args);
		#ifdef USE_CYGWIN
		xfree(cmd);
		#endif
	} else {
		r = (int)_wspawnlp(_P_WAIT, cmd, cmd, NULL);
	}
	return r;
}

/**
 * Determine the root folder of our cygwin installation by
 * reading the value of the registry key found at:
 *    HKEY_LOCAL_MACHINE\SOFTWARE\Cygwin\setup
 * or if compiled as a 64-bit program,
 *    HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Cygwin\setup
 */
static void get_cygwin_root(wchar_t* sBuffer, dword szBuffer)
{
	HKEY hkeyCygSetup;
	dword dwValType;
	long lRet;
	
	if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, CYGWIN_REGKEY, 0L, KEY_READ , &hkeyCygSetup) != ERROR_SUCCESS) {
		fatal_api_call(L"RegOpenKeyExW");
	}
	
	lRet = RegQueryValueExW(hkeyCygSetup, CYGWIN_SUBKEY, NULL, &dwValType, (LPBYTE)sBuffer, &szBuffer);
	if(lRet != ERROR_SUCCESS) {
		if(lRet == ERROR_MORE_DATA) {
			fatal(ERROR_MORE_DATA, L"Our buffer was not large enough to contain the value found at the registry key: HKEY_LOCAL_MACHINE\\" CYGWIN_SUBKEY);
		} else {
			fatal_api_call(L"RegQueryValueExW");
		}
	}
	
	if(dwValType != REG_SZ && dwValType != REG_EXPAND_SZ) {
		fatal(2, L"Found a non-string value at the following registry key: HKEY_LOCAL_MACHINE\\" CYGWIN_SUBKEY);
	}
	
	if(RegCloseKey(hkeyCygSetup) != ERROR_SUCCESS) {
		fatal_api_call(L"RegCloseKey");
	}
	
	if(!is_folder(sBuffer)) {
		fatal(ERROR_PATH_NOT_FOUND, L"Did not find an existing folder at %s", sBuffer);
	}
}

/** Entry Point */
int wmain(int argc, wchar_t* argv[])
{
	wchar_t	sParentDir[MAX_PATH+1] = EMPTYW,
			sExecutable[MAX_PATH+1] = EMPTYW,
			sTarget[MAX_PATH+1] = EMPTYW,
			sPATH[MAX_ENV+1] = EMPTYW,
			sCygRoot[MAX_PATH+1] = EMPTYW,
			sSystemDir[MAX_PATH+1] = EMPTYW,
			sWinDir[MAX_PATH+1] = EMPTYW,
			*sExecutableName;
	int i = 0;
	size_t	szPath = 0, szParent = 0, szName = 0;
	
	/* First, get the path to our real executable */
	// Get our module filepath.
	if(!(szPath = GetModuleFileNameW(NULL, sExecutable, MAX_PATH))) {
		fatal_api_call(L"GetModuleFileNameW");
	}
	
	// Get our parent folder
	if(!get_dirname(sExecutable, szPath, sParentDir, &szParent)) {
		fatal(ERROR_PATH_NOT_FOUND, L"Could not get the parent folder of our executable.");
	}
	
	// Get a string containing just the last part of the module path (ex: \python.exe)
	sExecutableName = ((wchar_t*)&(sExecutable[szParent]));
	
	// We still need to get the bin folder path, so we'll continue working.
	if(!get_dirname(sParentDir, szParent, sParentDir, &szParent)) {
		fatal(ERROR_PATH_NOT_FOUND, L"Could not get the root folder of our virtual environment.");
	}
	
	#ifndef WITHOUT_ENVVARS
	// Possibly needed later.
	virtRootWin = sParentDir;
	#endif
	
	// Finally, append bin\exename to the root virtualenv folder.
	if(FAILED(StringCchPrintfW(sTarget, MAX_PATH+1, L"%s\\bin%s", sParentDir, sExecutableName))) {
		fatal_api_call(L"StringCchPrintfW (Building path to real executable)");
	}
	
	// Make sure that our real executable exists.
	if(FAILED(sTarget)) {
		fatal(ERROR_FILE_NOT_FOUND, L"Did not find an existing file at %s", sTarget);
	}
	
	/* Now that we have the real path, set up our environment variables. */
	// First, find our cygwin root folder
	get_cygwin_root(sCygRoot, MAX_PATH + 1);
	
	// And then our system directory.
	if(!(szPath = (size_t)GetSystemDirectoryW(sSystemDir, MAX_PATH))) {
		fatal_api_call(L"GetSystemDirectoryW");
	}
	
	// Get our Windows folder (parent folder of our system folder)
	if(!get_dirname(sSystemDir, szPath, sWinDir, &szParent)) {
		fatal(1, L"Could not get our Windows directory.");
	}
	
	// Build our PATH variable.
	if(FAILED(StringCchPrintfW(
		sPATH,
		MAX_ENV+1,
		// PATH=DirOfExe;Cygwin\bin;Cygwin\usr\bin;Cygwin\usr\local\bin;WindowsDir;SystemDir
		L"%s\\bin;%s\\bin;%s\\usr\\bin;%s\\usr\\local\\bin;%s;%s",
		sParentDir,
		sCygRoot, sCygRoot, sCygRoot,
		sWinDir,
		sSystemDir
	))) {
		fatal_api_call(L"StringCchPrintfW (Building PATH variable)");
	}
	
	// Check for verbosity fag. (maybe)
	check_verbosity(argc, argv, sCygRoot, sTarget, sPATH);
	
	if(!SetEnvironmentVariableW(L"PATH", sPATH)) {
		fatal_api_call(L"SetEnvironmentVariableW");
	}
	argv[0] = sTarget;
	return exec_cmd(sTarget, argc, argv);
}