"""
common.py
Description: Utility functions and declarations commonly used by some of the other modules/scripts.
Author: Charles Grunwald (Juntalis) <ch@rles.grunwald.me>

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.
"""
import os
import re, string, sys

ws = string.whitespace
wc = string.ascii_letters
nl = '\r\n'
indents = ' \t'

def split_lines(text):
	""" Split lines and remove the ending new line character. """
	return [ l.strip(nl) for l in text.splitlines() ]

def split_lines_trim(text):
	""" Same as  split_lines, but also removes leading and trailing spaces, tabs, etc. """
	return [ l.strip(ws) for l in text.splitlines() ]

def split_lines_rtrim(text):
	""" Same as split_lines, but also removes trailing spaces, tabs, etc. """
	return [ l.strip(nl).rstrip(ws) for l in text.splitlines() ]

def split_lines_ltrim(text):
	""" Same as split_lines, but also removes leading spaces, tabs, etc. """
	return [ l.strip(nl).lstrip(ws) for l in text.splitlines() ]

def remove_blanks(lines):
	""" Remove all empty strings from a string list """
	return filter(lambda l: len(l) > 0, lines)

def split_lines_clean(text):
	""" Returns remove_blanks(split_lines_clean(text)) """
	return remove_blanks(split_lines_trim(text))

def split_lines_nb(text):
	""" Returns remove_blanks(split_lines_clean(text)) """
	return remove_blanks(split_lines(text))

def condense_space(text):
	""" Removes excess spaces from text by replace all multiple consecutive spaces with a single space. """
	return re.sub(" {2,}", " ", text)

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

__all__ = [ c for c in dir() if not c.startswith('_') ]