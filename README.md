## Python Compatibility Stub for Cygwin Virtual Environment

Simple Windows executable written to facilitate running the python interpreter (and module scripts) in a Cygwin virtual environment without actually having to drop down into Cygwin to do so. I'm not sure if anyone else needs this functionality, but I wanted the ability to use my normal Windows-based Python IDE as I write code that I intend to also run on Linux. (And while Cygwin's python isn't exactly the same as the one found on most Linux distros, it's close enough for my purposes, and is easier to use than dropping into a VM constantly) 

##### To use:

* On creating a cygwin virtual environment, create another subdirectory in it called, "Scripts". (This is normally what you'd find instead of the bin folder)
* Drop the usual Windows virtualenv batch scripts into this folder. (In this project's Scripts folder, I included a rewrite of the activate.bat file to allow it to be used in any Windows virtualenv without modifications)
* Drop the python.exe compiled from this project into the aforementioned folder.
* Just like you normally would with Windows python, run the Scripts\activate.bat file to activate your virtual environment.

##### What it does:

* Resolves the current filename of the executable in the bin folder. For example: `Scripts\python.exe` would resolve to `bin\python.exe`, wheras `Scripts\pip.exe` would resolve to `bin\pip.exe`.
* If the previously resolved file is a cygwin symbolic link, follow the links until you reach an actual executable. (Though at this point, I've only implemented recursion checking for the previous link, so be careful with this)
* Alter the environment variables for the launched Python to properly resolve the various DLLs that it depends on.
* Convert any paths found in the command-line to their Cygwin equivalents. (though I've yet to implement the ability to find paths in arguments like: `--output=filepath`)

##### Misc

Running the executable with the normal Python verbosity flag ( -v ) will give you a detailed output of what the stub is doing.

Example:

	(.vip) C:\ShellEnv\home\Charles\workspace\sshfavs>python.exe -v sshfavs.py
	# Detected verbose flag. Enabling debug output.
	# Resolved cygwin root..
	# [0] C:\ShellEnv
	# Resolved virtualenv interpreter..
	# [0] C:\ShellEnv\home\Charles\workspace\sshfavs\.vip\bin\python.exe
	# Setting minimals for PATH environment variable.
	# [0] C:\ShellEnv\home\Charles\workspace\sshfavs\.vip\bin;C:\ShellEnv\bin;C:\ShellEnv\usr\bin;C:\ShellEnv\usr\local\bin;C:\windows;C:\windows\system32
	# Attempting to load cygwin DLL..
	# Attempting to initialize cygwin context..
	# Populating functions tab..
	# Fixing executable name..
	# Checking C:\ShellEnv\home\Charles\workspace\sshfavs\.vip\bin\python.exe for symbolic link..
	# Detected symbolic link..
	# Symbolic link target resolved to a relative path. Prefixing bin dir..
	# Converted file path/path list:
	# [0] fix_path_type(x)
	# [1]   x -> C:\ShellEnv\home\Charles\workspace\sshfavs\.vip
	# [2]   r <- /home/Charles/workspace/sshfavs/.vip
	# Converted file path/path list:
	# [0] fix_path_type(x)
	# [1] x -> /home/Charles/workspace/sshfavs/.vip/bin/python2.7.exe
	# [2]   r <- C:\ShellEnv\home\Charles\workspace\sshfavs\.vip\bin\python2.7.exe
	# [3] readlink(x)
	# [4]   x -> C:\ShellEnv\home\Charles\workspace\sshfavs\.vip\bin\python.exe
	# [5]   r <- C:\ShellEnv\home\Charles\workspace\sshfavs\.vip\bin\python2.7.exe
	# Checking C:\ShellEnv\home\Charles\workspace\sshfavs\.vip\bin\python2.7.exe for symbolic link..
	# Fixing up argv..
	# Allocating new arg buffer..
	# Converted file path/path list:
	# [0] fix_path_type(x)
	# [1]   x -> C:\ShellEnv\home\Charles\workspace\sshfavs\.vip\bin\python2.7.exe
	# [2]   r <- /home/Charles/workspace/sshfavs/.vip/bin/python2.7.exe
	# Detected convertable path argument.
	# [0] sshfavs.py
	# Converted file path/path list:
	# [0] fix_path_type(x)
	# [1]   x -> sshfavs.py
	# [2]   r <- /home/Charles/workspace/sshfavs/sshfavs.py
	# Detected environment variable, VIRTUAL_ENV..
	# [0] Attempting to convert path..
	# Converted file path/path list:
	# [0] fix_path_type(x)
	# [1]   x -> C:\ShellEnv\home\Charles\workspace\sshfavs\.vip
	# [2]   r <- /home/Charles/workspace/sshfavs/.vip
	# Executing..
	# [0] /home/Charles/workspace/sshfavs/.vip/bin/python2.7.exe
	# [1] "-v"
	# [2] "/home/Charles/workspace/sshfavs/sshfavs.py"
	# installing zipimport hook
	...

##### Building

Building this executable requires a Windows version of Python, (This is due to the fact that for shits and giggles, I added functionality into the build script for extracting the main icon of the current python executable, and using it for the stub executable) and the MSVC compiler. If there's a demand for it, I'll see about adding in functionality for compiling with mingw, and writing a proper build script.

##### Credits

I stole a few bits from the [pefile module](http://code.google.com/p/pefile/) for the build script.