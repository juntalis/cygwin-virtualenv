/**
 * envvars.c - Contains functions for modifying a select number of
 *             environment variables for cygwin compatibility.
 *
 * This file is included by cygwin.c by default, but should a user
 * choose not to use this functionality, they can exclude it by
 * specifying, "--without-envvars" on the build script's command
 * line.
 *
 * Note: To be honest, in the few times I tried this application
 * before adding this section, I actually didn't come across any
 * problem with not converting these variables. (At least, as far
 * as my copy of Python 2.7 goes) Still, since I don't know all
 * the internal workings of Python or of Cygwin, and since I don't
 * know how older versions of Python would handle it, I'm going to
 * leave this in by default for the time being. (Unless, of course
 * someone knows something I don't and tells me they're completely
 * unnecessary)
 */

#ifndef _PRECOMPILED_H_
#	error This file contains declarations for main.c and should not be compiled by itself. Instead, compile main.c
#endif

/** Conversion tab entry */
typedef struct { const wchar_t *name; dword flags; } env_entry;

/**
 * Bitwise flags to help interpret the environment
 * variable tab.
 */
#define NONE		0x000L
#define SPATH		0x001L // Single path
#define LPATH		0x002L // Pathlist
#define VROOT		0x004L // Set to the root folder of our virtual env.
#define UNSET		0x008L // Unset if set

/**
 * All environment variables that should be converted to
 * their cygwin-alternatives. (if present)
 */
static const env_entry vars_tab[] = {
	{ L"PYTHONSTARTUP", SPATH 			},
	{ L"PYTHONPATH",	LPATH 			},
	{ L"PYTHONHOME",	UNSET 			},
	{ L"VIRTUAL_ENV",	(SPATH | VROOT)	},
	{ NULL,				NONE			}
};

/* Handles changes made to our environment variables */
static void fix_env()
{
	int i = -1;
	wchar_t* virtRoot;
	if(!virtRootCyg) {
		verbose(L"Pre-converting virtual environment root, in case var found to be missing from environment.");
		virtRoot = fix_path(virtRootWin);
	} else {
		virtRoot = virtRootCyg;
	}
	
	while(vars_tab[++i].name && *phCygwin) {
		wchar_t current[MAX_ENV+1] = EMPTYW, *converted;
		
		// Get our environment variable.
		if(GetEnvironmentVariableW(vars_tab[i].name, current, MAX_ENV+1)) {
			verbose(L"Detected environment variable, %s..", vars_tab[i].name);
			// If it exists, check against the following flags..
			if(vars_tab[i].flags & UNSET) {
				verbose_step(L"Unsetting..");
				converted = NULL;
			} else if(vars_tab[i].flags & SPATH) {
				verbose_step(L"Attempting to convert path..");
				converted = fix_path(current);
			} else if(vars_tab[i].flags & LPATH) {
				verbose_step(L"Attempting to convert path list..");
				converted = fix_path_list(current);
			}
			
			// Set it to our new value.
			if(!SetEnvironmentVariableW(vars_tab[i].name, converted)) {
				fatal_api_call(L"SetEnvironmentVariableW");
			}
			
			xfree(converted);
		} else {
			// Otherwise..
			if(vars_tab[i].flags & VROOT) {
				if(verbose_flag) {
					verbose(L"Detected lack of environment variable, %s", vars_tab[i].name);
					verbose(L"Setting to converted virtual environment path:");
					verbose_step(virtRoot);
					
				}
				if(!SetEnvironmentVariableW(vars_tab[i].name, virtRoot)) {
					fatal_api_call(L"SetEnvironmentVariableW");
				}
			}
		}
	}
	xfree(virtRoot);
}
