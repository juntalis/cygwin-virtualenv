import sys, os, proc, txt
from . import isx64
try:
	import _winreg as reg
except ImportError:
	print "You don't seem to have the _winreg extension module installed. Instead, I'll try the ctypes implementation of _winreg, but please be advised that this is experimental. The preferred way to build this program is through regular Win32 Python."
	import winreg as reg

# Registry query helper
KEY_ALL_READ = ( reg.KEY_QUERY_VALUE | reg.KEY_READ )

def read_reg_value(key, subkey=None, root=reg.HKEY_LOCAL_MACHINE):
	try:
		if subkey is None:
			result = reg.QueryValue(root, key)
		else:
			try:
				hk = reg.OpenKeyEx(root, key, 0, KEY_ALL_READ)
				if hk is None: return None
				result = reg.QueryValueEx(hk, subkey)[0]
			except:
				CloseKey(hk)
				raise Exception()
	except:
		result = None
	return result

# Just to shorten up access to the path functions..
dname = os.path.dirname
bname = os.path.basename
isdir = os.path.isdir
isfile = os.path.isfile
ext = os.path.splitext
pj = os.path.join


if isx64:
	MSVCROOTKEY = 'SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio'
	MSVCEXPROOTKEY = 'SOFTWARE\\Wow6432Node\\Microsoft\\VCExpress'
else:
	MSVCROOTKEY = 'SOFTWARE\\Microsoft\\VisualStudio'
	MSVCEXPROOTKEY = 'SOFTWARE\\Microsoft\\VCExpress'

MSVC_BASE_LABEL = 'Microsoft Visual C++'
MSVC_VALUEKEY = 'ProductDir'
MSVC_ENVCMD = 'vcvars32.bat'
MSVC_KEYS = [
	(10, '10', u'%s\\10.0\\Setup\\VC' % MSVCROOTKEY),
	(10, '10 Express', u'%s\\10.0\\Setup\\VC' % MSVCEXPROOTKEY),
	(11, '11', u'%s\\11.0\\Setup\\VC' % MSVCROOTKEY),
	(9, '9', u'%s\\9.0\\Setup\\VC' % MSVCROOTKEY),
	(9, '9 Express', u'%s\\9.0\\Setup\\VC' % MSVCEXPROOTKEY),
	(8, '8', u'%s\\8.0\\Setup\\VC' % MSVCROOTKEY),
	(7, '8 Express', u'%s\\8.0\\Setup\\VC' % MSVCEXPROOTKEY),
	(7, '7', u'%s\\7.1\\Setup\\VC' % MSVCROOTKEY),
	(6, '6', u'%s\\6.0\\Setup\\Microsoft Visual C++' % MSVCROOTKEY),
]

SDK_ROOT_KEY = 'SOFTWARE\\Microsoft\\MicrosoftSDK\\InstalledSDKs'
MSSDK_BASE_LABEL = 'Windows SDK'
MSSDK_VALUEKEY = 'InstallationFolder'
MSSDK_ENVCMD = 'SetEnv.cmd'
MSSDK_KEYS = [
	([10], 71, '7.1', u'SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v7.1'),
	([10], 701, '7.0 (VS2010)', u'SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v7.0A'),
	([10], 70, '7.0', u'SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v7.0'),
	([11], 801, '8.0 (VS2012)', u'SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v8.0A'),
	([9], 61, '6.1', u'SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v6.1'),
	([9], 601, '6.0 (VS2008)', u'SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v6.0A'),
	([9], 60, '6.0', u'SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v6.0'),
	([10, 9, 8, 7, 6], 502, '2003 SP2', u'%s\\D2FF9F89-8AA2-4373-8A31-C838BF4DBBE1' % SDK_ROOT_KEY), # Don't actually know what these support, but whatever.
	([10, 9, 8, 7, 6], 501, '2003 SP1', u'%s\\8F9E5EF3-A9A5-491B-A889-C58EFFECE8B3' % SDK_ROOT_KEY),
]

def find_msvc_root(vers=None):
	result = None
	keys = MSVC_KEYS
	if vers is not None:
		keys = filter(lambda k: k[0] == vers, keys)
	for vcvers, label, key in keys:
		print 'Checking for installation of %s %s..' % (MSVC_BASE_LABEL, label)
		
		check = read_reg_value(key, MSVC_VALUEKEY)
		if check is None or len(check) == 0 or not isdir(check):
			continue
		
		print 'Found installation. Using VC%d configuration.' % vcvers
		result = vcvers, check
		break
	
	return result

def find_sdk_root(vers=None, vcvers=None):
	result = None, 0
	keys = MSSDK_KEYS
	if vers is not None:
		keys = filter(lambda k: k[1] == vers, keys)
	if vcvers is not None:
		keys = filter(lambda k: vcvers in k[0], keys)
	for supports, sdkvers, label, key in keys:
		print 'Checking for installation of %s %s..' % (MSSDK_BASE_LABEL, label)
		
		check = read_reg_value(key, MSSDK_VALUEKEY)
		if check is None or len(check) == 0 or not isdir(check):
			continue
		
		print 'Found installation. Using VC%d configuration.' % sdkvers
		result = sdkvers, check
		break
	return result

def find_toolset(vcvers=None, sdkvers=None):
	vcroot, sdkroot = None, None
	if vcvers is None:
		vcvers, vcroot = find_msvc_root()
	else:
		vcvers, vcroot = find_msvc_root(vcvers)
	
	if sdkvers is None and vcroot is None:
		sdkvers, sdkroot = find_sdk_root()
	elif sdkvers is not None:
		sdkvers, sdkroot = find_sdk_root(sdkvers)
	
	if sdkroot is None and vcroot is None:
		raise Exception('Could not locate a toolset to use!')
	
	buildenv = {}
	if vcroot is not None:
		buildenv = proc.envcmd(pj(vcroot, 'bin', MSVC_ENVCMD))
	
	if sdkroot is not None:
		sdkenv = proc.envcmd(pj(sdkroot, 'Bin', MSSDK_ENVCMD), '/Release', '/x86')
		for k in sdkenv.keys():
			ku = k.upper()
			if ku in [ bk.upper() for bk in buildenv.keys() ]:
				if ku in [ 'PATH', 'INCLUDE', 'LIB', 'LIBPATH' ]:
					newv = txt.remove_blanks(sdkenv[k].split(';'))
					oldv = txt.remove_blanks([ v.lower() for v in buildenv[k].split(';') ])
					for v in newv:
						if v.lower() not in oldv:
							buildenv[k] = (v + ';' + buildenv[k])
				else:
					buildenv[k] = sdkenv[k]
			else:
				buildenv[k] = sdkenv[k]
	return buildenv

def remove_slash_arg(arg):
		arg = list(arg)
		if arg[0] == '/': arg[0] = '-'
		return ''.join(arg)

def compile(srcs, buildenv=None, cflags=None, objdir='obj'):
	if buildenv is None:
		buildenv = os.environ
	if cflags is None:
		cflags = []
	if objdir is None:
		objdir = 'obj'
	
	# Find cl.exe
	cl = proc.which('cl.exe', buildenv['PATH'] if buildenv.has_key('PATH') else os.environ['PATH'])
	rc = proc.which('rc.exe', buildenv['PATH'] if buildenv.has_key('PATH') else os.environ['PATH'])
	
	# Set up obj dir.
	objdir = os.path.abspath(objdir)
	if not isdir(objdir):
		os.makedirs(objdir)
	
	# Set up CFLAGS
	cflags += ['-nologo', '-W3', '-WX-', '-O2', '-Ob2', '-Oi', '-GA', '-GL', '-GF', '-Gm-', '-GS-', '-Gy', '-fp:precise', '-DNDEBUG=1', '-D_NDEBUG=1', '-MD', '-D_CRT_SECURE_NO_DEPRECATE', '-Oy', '-arch:SSE2']
	cflags = list(set([ remove_slash_arg(v) for v in cflags]))
	objbase = objdir + '\\%s.obj'
	resbase = objdir + '\\%s.res'
	clargs = cflags + [ '-c' ]
	
	# Set up object file list and compile source files.
	objfiles = []
	for src in srcs:
		print 'Compiling %s..' % src
		pparts = ext(src)
		if pparts[1].lower() == '.rc':
			obj = resbase % bname(pparts[0])
			_rcargs = tuple(['/fo%s' % obj, src])
			rc(*_rcargs)
		else:
			obj = objbase % bname(pparts[0])
			_clargs = tuple(clargs + [ '-Fo%s' % obj, src ])
			cl(*_clargs)
		objfiles.append(obj)
	return objfiles

def link(objfiles, output, buildenv=None, linkflags=None, outdir='bin'):
	if buildenv is None: buildenv = os.environ
	if linkflags is None: linkflags = []
	if outdir is None: outdir = 'bin'
	if output is None: raise Exception('Need an output basename!')
	
	# Find cl.exe
	link = proc.which('link.exe', buildenv['PATH'] if buildenv.has_key('PATH') else os.environ['PATH'])
	
	# Set up output
	outdir = os.path.abspath(outdir)
	if not isdir(outdir):
		os.makedirs(outdir)
	output = pj(outdir, output)
	if ext(output)[1] != '.exe': output += '.exe'
	
	# Set up link flags
	linkflags += ['-nologo', '-OPT:REF', '-OPT:ICF', '-LTCG', '-MACHINE:X86', '-OUT:%s' % output ]
	linkflags += objfiles
	linkargs = tuple(set([ remove_slash_arg(v) for v in linkflags]))
	
	# Set up object file list and compile source files.
	print 'Linking objects..'
	return link(*linkargs)




