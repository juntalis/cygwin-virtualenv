#!/usr/bin/env python
# encoding: utf-8
"""
proc.py
by Charles

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.
"""
import os as _os, subprocess as _subprocess, time as _time
from txt import split_lines_clean
from fs import quote, write_lines, which as _which
import tempfile as _tmp

__all__ = [ 'outputof', 'envcmd', 'which', 'executable', 'PIPE', 'OUT' ]

OUT = _subprocess.STDOUT
PIPE = _subprocess.PIPE
_NULL = None

def outputof(cmd, *args, **kwargs):
	kwargs.setdefault('cwd', _os.getcwd())
	stdout = not kwargs.has_key('stdout') or kwargs['stdout'] == PIPE
	stderr = kwargs.has_key('stderr') and kwargs['stderr'] == PIPE
	if not stdout and not stderr:
		raise Exception('Must specify at least one pipe to listen on.')
	kwargs['stdout'] = PIPE
	kwargs['stderr'] = PIPE
	check_return = True
	if kwargs.has_key('check_return'):
		check_return = kwargs['check_return']
		del kwargs['check_return']
	subp = _subprocess.Popen([ cmd ] + list(args), **kwargs)
	pout, perr = subp.communicate()
	if check_return and subp.returncode != 0:
		errmsg = 'STDOUT: %s\n\nSTDERR: %s' % (pout, perr)
		print errmsg
		raise _subprocess.CalledProcessError(subp.returncode, cmd, errmsg)
	if not stderr: return pout
	elif not stdout: return perr
	else: return pout, perr

def envcmd(batfile, *args, **kwargs):
	kwargs['stderr'] = None
	kwargs['stdout'] = PIPE
	kwargs['check_return'] = True
	cmd = _os.getenv('COMSPEC', default='cmd.exe')
	tmp = _os.path.join(_tmp.gettempdir(), 'msvcenv%x' % _time.time() + '.cmd')
	cmdline = [
		'@if not exist "%s" exit 1' % batfile,
		'@(call %s)>nul 2>nul' % ' '.join([ quote(batfile) ] + list(args)),
		'@set'
	]
	write_lines(tmp, cmdline)
	args = [ '/C', tmp ]
	lines = split_lines_clean(outputof(cmd, *args, **kwargs))
	_os.unlink(tmp)
	results = {}
	for line in lines:
		sep = line.find('=')
		key, val = line[:sep].upper(), line[sep+1:]
		if _os.environ.has_key(key) and val.lower() == _os.environ[key].lower(): continue
		results[key] = val
	return results

class Executable(object):
	_oldenv = {}

	def __init__(self, cmd, *args, **kwargs):
		self.cmd = cmd
		self._kwargs = kwargs
		self._args = args

	def __setenv__(self, env):
		self._oldenv = _os.environ.copy()
		_os.environ.update(env)

	def __resetenv__(self):
		_os.environ = self._oldenv

	def __call__(self, *args, **kwargs):
		procenv = False
		if kwargs.has_key('env'):
			procenv =  True
			self.__setenv__(kwargs['env'])
			del kwargs['env']
		elif kwargs.has_key('environ'):
			procenv = True
			self.__setenv__(kwargs['environ'])
			del kwargs['environ']
		_args = tuple(list(self._args) + list(args))
		_kwargs = self._kwargs.copy()
		_kwargs.update(kwargs)
		result = outputof(self.cmd, *_args, **_kwargs)
		if procenv: self.__resetenv__()
		return result

def which(cmd, path = None):
	return Executable(_which(cmd, path))