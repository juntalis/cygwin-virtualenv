"""
fs.py
Description: Utility functions and declarations for working with filepaths.
Author: Charles Grunwald (Juntalis) <ch@rles.grunwald.me>

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.
"""
import os
from . import txt

def pdir(fpath):
	""" Get absolute parent directory of a file """
	return os.path.abspath(os.path.dirname(fpath))

def quote(p):
	""" Quote a file path. """
	return '"%s"' % p

def which(cmd, path = None):
	"""Return full path of command or False
	I don't check for executable bits"""

	# Search path
	if path is None:
		path = [ os.getcwd() ]
		if 'PATH' in os.environ:
			path.extend(os.environ['PATH'].split(os.pathsep))
	else:
		if type(path) == str:
			path = path.split(os.pathsep)
	for dir in txt.remove_blanks([ os.path.expandvars(p) for p in path ]):
		fullname = os.path.join(os.path.abspath(dir), cmd)
		if os.path.isfile(fullname):
			return fullname
		# executable extension for NT
		elif 'PATHEXT' in os.environ:
			pathexts = os.environ['PATHEXT'].split(';')
			for pathext in pathexts:
				file_with_ext = fullname + pathext
				if os.path.isfile (file_with_ext):
					return file_with_ext
	return False

def write_bin(filepath, data):
	""" Write binary data to file. """
	fOutput = open(filepath, 'wb')
	fOutput.write(data)
	fOutput.close()

def write_text(filepath, text):
	""" Write text to file. """
	fOutput = open(filepath, 'wt')
	fOutput.write(text)
	fOutput.close()

def write_lines(filepath, lines):
	""" Write lines of text to file. """
	fOutput = open(filepath, 'wt')
	fOutput.write('\n'.join(lines))
	fOutput.close()

def read_text(filepath):
	""" Read text from file. """
	fInput = open(filepath, 'rt')
	content = fInput.read()
	fInput.close()
	return content

def read_lines(filepath):
	""" Read lines of text from file. """
	fInput = open(filepath, 'rt')
	content = fInput.readlines()
	fInput.close()
	return content
