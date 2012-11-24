/**
 *  cygwin.c - Cygwin-specific stuff. Only included when compiled
 *             as a 32-bit application.
 */

#ifndef _PRECOMPILED_H_
#	error This file contains declarations for main.c and should not be compiled by itself. Instead, compile main.c
#endif

#define CYGWIN_DLL L"cygwin1.dll"

/**
 * Set in main.c - Will contain a string representing the
 * Windows path to our virtualenv's root folder.
 */
static wchar_t* virtRootWin = NULL;

/**
 * Set either in cygwin.c or in envvars.c
 */
static wchar_t* virtRootCyg = NULL;

typedef HMODULE* PHMODULE; // Thought this was already declared.

/** Our base cygwin function type. */
typedef void(*cygwin_func)();

/** Function tab entry */
typedef struct { const char *name; cygwin_func proc; } cygwin_func_entry;

/** Typedefs taken from Python for defining ssize_t on MSVC */
typedef _W64 int ssize_t;

/** Possible 'what' values in calls to cygwin_conv_path/cygwin_create_path. */
enum {
	CCP_POSIX_TO_WIN_A = 0, /* from is char*, to is char*       */
	CCP_POSIX_TO_WIN_W,      /* from is char*, to is wchar_t*    */
	CCP_WIN_A_TO_POSIX,      /* from is char*, to is char*       */
	CCP_WIN_W_TO_POSIX,      /* from is wchar_t*, to is char*    */

	/* Or these values to the above as needed. */
	CCP_ABSOLUTE = 0,      /* Request absolute path (default). */
	CCP_RELATIVE = 0x100    /* Request to keep path relative.   */
};

/** Used to describe the enum values above. */
typedef unsigned int cygwin_conv_path_t;

/** Finally, our function tab. */
static cygwin_func_entry cygwin_funcs[] = {
	/**
	 * If size is 0, cygwin_conv_path returns the required buffer size in bytes.
	 * Otherwise, it returns 0 on success, or -1 on error and errno is set to
	 * one of the below values:
	 *
	 * EINVAL        what has an invalid value.
	 * EFAULT        from or to point into nirvana.
	 * ENAMETOOLONG  the resulting path is longer than 32K, or, in case
	 *               of what == CCP_POSIX_TO_WIN_A, longer than MAX_PATH.
	 * ENOSPC        size is less than required for the conversion.
	 */
	{ "cygwin_conv_path", NULL },
#	define cygwin_conv_path ((ssize_t(*)(cygwin_conv_path_t, const void*, void*, size_t))(cygwin_funcs[0].proc))
	{ "cygwin_conv_path_list", NULL },
#	define cygwin_conv_path_list ((ssize_t(*)(cygwin_conv_path_t, const void*, void*, size_t))(cygwin_funcs[1].proc))

	/**
	 * Allocate a buffer for the conversion result using malloc(3), and return
	 * a pointer to it.  Returns NULL if something goes wrong with errno set
	 * to one of the above values, or to ENOMEM if malloc fails.
	 */
	{ "cygwin_create_path", NULL },
#	define cygwin_create_path ((void*(*)(cygwin_conv_path_t, const void*))(cygwin_funcs[2].proc))

	/**
	 * For the purposes of freeing the buffer allocated in the previous function,
	 * we're also going to grab cygwin's implementation of free, just to be safe.
	 */
	{ "free", NULL },
#	define cygwin_free ((void(*)(void*))(cygwin_funcs[3].proc))

	/**
	 * Left NULL to allow us to iterate and setup the last 4 functions at once.
	 */
	{ NULL, NULL }
};

static HMODULE hCygwin = NULL;
static PHMODULE phCygwin = &hCygwin;

/** Load our cygwin DLL or return false on failure. */
static bool load_cygwin_library()
{
	verbose(L"Attempting to load cygwin DLL..");
	if(!(*phCygwin = GetModuleHandleW(CYGWIN_DLL))) {
		if(!(*phCygwin = LoadLibraryW(CYGWIN_DLL))) {
			verbose(L"Failed. Skipping path conversions.");
			return false;
		}
	}
			
	return true;
}

/** Initialize the cygwin context or false on failure. */
static bool init_cygwin_library()
{
	cygwin_func cygwin_dll_init = NULL;
	verbose(L"Attempting to initialize cygwin context..");
	if((cygwin_dll_init = (cygwin_func)GetProcAddress(*phCygwin,"cygwin_dll_init")) == NULL) {
		verbose(L"Failed. Skipping path conversions.");
		FreeLibrary(*phCygwin);
		return false;
	}
	cygwin_dll_init();
	return true;
}

/**
 * Load our cygwin DLL, initialize the cygwin environment, and
 * populate our function tab.
 */
static bool setup_cygwin()
{
	int i = -1;
	
	// Load the cygwin dll into our application.
	if(!load_cygwin_library()) return false;
	
	// Init the cygwin environment. (Required)
	if(!init_cygwin_library()) {
		FreeLibrary(*phCygwin);
		return false;
	}
	
	// Populate the remaining function tab.
	verbose(L"Populating functions tab..");
	while(cygwin_funcs[++i].name != NULL) {
		if(!(cygwin_funcs[i].proc = (cygwin_func)GetProcAddress(*phCygwin, cygwin_funcs[i].name))) {
			verbose(L"Failed to load a cygwin function. Skipping path conversions.");
			FreeLibrary(*phCygwin);
			return false;
		}
	}
	
	return true;
}

/** Does any necessary path format conversion and quoting for paths/path lists needing them. */
#define fix_path(x)			fix_path_type((void*)x, false, CCP_WIN_W_TO_POSIX)
#define fix_path_list(x)	fix_path_type((void*)x, true, CCP_WIN_W_TO_POSIX)
static wchar_t* fix_path_type(void* arg, bool islist, cygwin_conv_path_t convtyp)
{
	const char* cygpath = NULL;
	wchar_t* result = NULL;
	size_t	szcount = 0, szresult = 0;
	ssize_t(*conversion_func)(cygwin_conv_path_t, const void*, void*, size_t);
	
	// We'll use cygwin to allocate the buffer for the original value.
	if(!(cygpath = (const char*)cygwin_create_path(convtyp, (const void*)arg))) {
		FreeLibrary(*phCygwin);
		*phCygwin = NULL;
		return NULL;
	}
	
	// Figure out which function to use.
	if(islist) {
		conversion_func = cygwin_conv_path_list;
	} else {
		conversion_func = cygwin_conv_path;
	}
	
	// Since cygwin allocated our buffer, we can assume it's big enough to hold the output.
	// For now, though, we'll just use MAX_ENV+1
	if(conversion_func(convtyp, (const void*)arg, (void*)cygpath, (MAX_ENV+1) * sizeof(char)) == -1) {
		cygwin_free((void*)cygpath);
		FreeLibrary(*phCygwin);
		*phCygwin = NULL;
		return NULL;
	}
	
	// Finally, allocate our result buffer.
	szcount = lstrlenA(cygpath) + 1;
	szresult = szcount * sizeof(wchar_t);
	if(!szresult || !(result = (wchar_t*)malloc(szresult))) {
		cygwin_free((void*)cygpath);
		FreeLibrary(*phCygwin);
		*phCygwin = NULL;
		return NULL;
	}
	
	// Zero out the allocated data.
	memset((void*)result, 0, szresult);
	
	// And convert the char string to our resulting wchar string.
	mbstowcs(result, cygpath, szcount);
	
	// At this point, we can free up the buffer cygwin created.
	cygwin_free((void*)cygpath);
	if(!verbose_flag) return result;

	verbose(L"Converted file path/path list:");
	verbose_step(L"fix_path_type(x)");
	if(*(((char*)arg) + 1) != '\0') {
		printf("# [%d] x -> %s\n", verbose_stepi++, arg);
	} else {
		verbose_step(L"  x -> %s", arg);
	}
	verbose_step(L"  r <- %s", result);
	return result;
}

static const byte cyglink_sig[] = { '!', '<', 's', 'y', 'm', 'l', 'i', 'n', 'k', '>' };
static const word dummy_val = 0xfeff;

typedef struct _cyglink {
	byte tag[10];					// !<symlink>
	word dummy;						// Still not sure what these two bytes actually stand for.
									// Regardless, they seem to always contain \xff\xfe
	wchar_t target[ANYSIZE_ARRAY];	// And lastly, our target as a unicode string.
} cyglink, *pcyglink;

static bool is_buf_symlink(byte* buffer, size_t szbuf)
{
	int i;
	pcyglink slink = NULL;
	if(szbuf < sizeof(cyglink)) return false;
	
	slink = (pcyglink)buffer;
	for(i = 0; i < 10; i++) {
		if(slink->tag[i] == cyglink_sig[i]) continue;
		return false;
	}
	
	if(slink->dummy != dummy_val) return false;
	return true;
}

static wchar_t* readlink(wchar_t* path)
{
	size_t szbuf = 0;
	wchar_t* result = path;
	char* cygbuf = NULL;
	byte* buffer = NULL;
	pcyglink slink = NULL;
	
	verbose(L"Checking %s for symbolic link..", path);
	
	// Make sure path is a file flagged as a system file.
	if(!is_file(path) || !has_system_attr(path)) goto cleanup;
	
	// Allocate our buffer and read the file into it.
	buffer = file_to_buffer(path, &szbuf);
	
	// Check if our buffer contains a symbolic link.
	if(!is_buf_symlink(buffer, szbuf)) goto cleanup;
	
	// If we get to this point, we're definitely working
	// with a symbolic link, so cast it to the struct.
	verbose(L"Detected symbolic link..");
	slink = (pcyglink)buffer;
	
	// Get the string length of our link's target.
	szbuf = wcslen(slink->target);
	if(szbuf == 0) {
		fatal(1, L"Empty path found as target of the symbolic link at %s.", path);
	}
	
	// Before converting it, we need to check whether it's a relative
	// or absolute file path.
	if(slink->target[0] != L'/') {
		verbose(L"Symbolic link target resolved to a relative path. Prefixing bin dir..");
		virtRootCyg = fix_path(virtRootWin);
		// len(virtualenv root) + len('/bin/') + len(target)
		szbuf += wcslen(virtRootCyg) + 5;
		result = walloc(szbuf++);
		swprintf(result, szbuf, L"%s/bin/%s", virtRootCyg, slink->target);
	} else {
		// Duping it to allow freeing the memory a few
		// lines later without issue.
		verbose(L"Symbolic link target appears to be an absolute path. Continuing..");
		result = _wcsdup(slink->target);
	}
	
	// Convert it to a ANSI string to allow converting
	// it with cygwin.
	cygbuf = xalloc(szbuf, sizeof(char));
	wcstombs(cygbuf, result, szbuf+1);
	xfree(result);
	if(!(result = fix_path_type((void*)cygbuf, false, CCP_POSIX_TO_WIN_A))) {
		result = path;
		goto cleanup;
	}
	verbose_step(L"readlink(x)");
	verbose_step(L"  x -> %s", path);
	verbose_step(L"  r <- %s", result);
cleanup:
	xfree(buffer);
	xfree(cygbuf);
	#ifdef WITHOUT_ENVVARS
	xfree(virtRootCyg);
	#endif
	return result;
}

static wchar_t* real_path(wchar_t* path)
{
	wchar_t *result,
	*last = _wcsdup(path);
	
	while(true) {
		result = last;
		last = readlink(result);
		if(result == last) { break; }
		if(_wcsicmp(path, last) == 0) {
			fatal(1, L"Detected recursive symlinks at target %s", path);
		}
		xfree(result);
	}
	return result;
}

#ifndef WITHOUT_ENVVARS
#	include "envvars.c"
#endif