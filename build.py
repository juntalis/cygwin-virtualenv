from build.msvc import *
from build.iconex import get_python_icon

if __name__=='__main__':
	buildenv = find_toolset()
	get_python_icon()
	objs = compile([ 'src/main.c', 'res/cygpython.rc'], buildenv, cflags=[ '-Isrc' ])
	link(objs, 'python', buildenv)