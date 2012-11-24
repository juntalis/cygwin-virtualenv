# coding=utf-8
import sys, os, struct, string
from ctypes import *
from ctypes.wintypes import *
from . import isx64

# Ctypes crap
kernel32 = WinDLL('kernel32.dll')
ULONG_PTR = c_ulonglong if isx64 else DWORD
LONG_PTR = c_longlong if isx64 else c_long

ENUMRESNAMEPROC = WINFUNCTYPE(BOOL, HMODULE, ULONG_PTR, ULONG_PTR, LONG)

EnumResourceNames = kernel32.EnumResourceNamesW
EnumResourceNames.restype = BOOL
EnumResourceNames.argtypes = [ HMODULE, ULONG_PTR, ENUMRESNAMEPROC, LONG_PTR ]

FindResource = kernel32.FindResourceW
FindResource.restype = HRSRC
FindResource.argtypes = [ HMODULE, ULONG_PTR, ULONG_PTR ]

FindResourceEx = kernel32.FindResourceExW
FindResourceEx.restype = HRSRC
FindResourceEx.argtypes = [ HMODULE, ULONG_PTR, ULONG_PTR, WORD ]

LoadResource = kernel32.LoadResource
LoadResource.restype = HGLOBAL
LoadResource.argtypes = [ HMODULE, HRSRC ]

FreeResource = kernel32.FreeResource
FreeResource.restype = BOOL
FreeResource.argtypes = [ HGLOBAL ]

SizeofResource = kernel32.SizeofResource
SizeofResource.restype = DWORD
SizeofResource.argtypes = [ HMODULE, HRSRC ]

LockResource = kernel32.LockResource
LockResource.restype = LPVOID
LockResource.argtypes = [ HGLOBAL ]

CloseHandle = kernel32.CloseHandle
CloseHandle.restype = BOOL
CloseHandle.argtypes = [ HANDLE ]

GetModuleHandle = kernel32.GetModuleHandleW
GetModuleHandle.restype = HMODULE
GetModuleHandle.argtypes = [ LPCWSTR ]

LoadLibraryEx = kernel32.LoadLibraryExW
LoadLibraryEx.restype = HMODULE
LoadLibraryEx.argtypes = [ LPCWSTR, HANDLE, DWORD ]

FreeLibrary = kernel32.FreeLibrary
FreeLibrary.restype = BOOL
FreeLibrary.argtypes = [ HMODULE ]

# LoadLibraryEx flags
DONT_RESOLVE_DLL_REFERENCES = 0x01
LOAD_LIBRARY_AS_DATAFILE = 0x02
LOAD_LIBRARY_AS_IMAGE_RESOURCE = 0x20

# Constants
NULL = 0

# Resource types - Took this bit from the pefile module.
# Not actually using all of these at the moment, but I
# figured I'd leave some of them in case I ever want to
# expand on this functionality of this code to allow
# extracting other resource types.
resource_type = [
	('RT_CURSOR',          1),
	('RT_BITMAP',          2),
	('RT_ICON',            3),
	('RT_MENU',            4),
	('RT_DIALOG',          5),
	('RT_STRING',          6),
	('RT_FONTDIR',         7),
	('RT_FONT',            8),
	('RT_ACCELERATOR',     9),
	('RT_RCDATA',          10),
	('RT_MESSAGETABLE',    11),
	('RT_GROUP_CURSOR',    12),
	('RT_GROUP_ICON',      14),
	('RT_VERSION',         16),
	('RT_DLGINCLUDE',      17),
	('RT_PLUGPLAY',        19),
	('RT_VXD',             20),
	('RT_ANICURSOR',       21),
	('RT_ANIICON',         22),
	('RT_HTML',            23),
	('RT_MANIFEST',        24) ]

RESOURCE_TYPE = dict([(e[1], e[0]) for e in resource_type]+resource_type)


# Globals paths.
def _get_paths(me):
	from fs import pdir as p
	r = p(p(me))
	return r, os.path.join(r, 'res')

_root_, _resdir_ = _get_paths(__file__)
del _get_paths

# Taken from the pefile module, as well.

STRUCT_SIZEOF_TYPES = {
	'x': 1, 'c': 1, 'b': 1, 'B': 1,
	'h': 2, 'H': 2,
	'i': 4, 'I': 4, 'l': 4, 'L': 4, 'f': 4,
	'q': 8, 'Q': 8, 'd': 8,
	's': 1 }

class Structure(object):
	"""Prepare structure object to extract members from data.

	Format is a list containing definitions for the elements
	of the structure.
	"""
	
	def __init__(self, format, name=None, file_offset=0):
		# Format is forced little endian, for big endian non Intel platforms
		self.__format__ = ''
		self.__keys__ = []
		#self.values = {}
		self.__format_length__ = 0
		self.__field_offsets__ = dict()
		self.__set_format__(format)
		self.__all_zeroes__ = False
		self.__unpacked_data_elms__ = None
		self.__file_offset__ = file_offset
		if name:
			self.name = name
		else:
			self.name = 'Structure'


	def __get_format__(self):
		return self.__format__

	def get_field_absolute_offset(self, field_name):
		"""Return the offset within the field for the requested field in the structure."""
		return self.__file_offset__ + self.__field_offsets__[field_name]

	def get_field_relative_offset(self, field_name):
		"""Return the offset within the structure for the requested field."""
		return self.__field_offsets__[field_name]

	def get_file_offset(self):
		return self.__file_offset__

	def set_file_offset(self, offset):
		self.__file_offset__ = offset

	def all_zeroes(self):
		"""Returns true is the unpacked data is all zeros."""

		return self.__all_zeroes__

	def sizeof_type(self, t):
		count = 1
		_t = t
		if t[0] in string.digits:
			# extract the count
			count = int( ''.join([d for d in t if d in string.digits]) )
			_t = ''.join([d for d in t if d not in string.digits])
		return STRUCT_SIZEOF_TYPES[_t] * count

	def __set_format__(self, format):

		offset = 0
		for elm in format:
			if ',' in elm:
				elm_type, elm_name = elm.split(',', 1)
				self.__format__ += elm_type

				elm_names = elm_name.split(',')
				names = []
				for elm_name in elm_names:
					if elm_name in self.__keys__:
						search_list = [x[:len(elm_name)] for x in self.__keys__]
						occ_count = search_list.count(elm_name)
						elm_name = elm_name+'_'+str(occ_count)
					names.append(elm_name)
					self.__field_offsets__[elm_name] = offset

				offset += self.sizeof_type(elm_type)

				# Some PE header structures have unions on them, so a certain
				# value might have different names, so each key has a list of
				# all the possible members referring to the data.
				self.__keys__.append(names)

		self.__format_length__ = struct.calcsize(self.__format__)


	def sizeof(self):
		"""Return size of the structure."""
		return self.__format_length__


	def __unpack__(self, data):

		if len(data) > self.__format_length__:
			data = data[:self.__format_length__]

		# OC Patch:
		# Some malware have incorrect header lengths.
		# Fail gracefully if this occurs
		# Buggy malware: a29b0118af8b7408444df81701ad5a7f
		#
		elif len(data) < self.__format_length__:
			raise PEFormatError('Data length less than expected header length.')


		if data.count(chr(0)) == len(data):
			self.__all_zeroes__ = True

		self.__unpacked_data_elms__ = struct.unpack(self.__format__, data)
		for i in xrange(len(self.__unpacked_data_elms__)):
			for key in self.__keys__[i]:
				#self.values[key] = self.__unpacked_data_elms__[i]
				setattr(self, key, self.__unpacked_data_elms__[i])


	def __pack__(self):

		new_values = []

		for i in xrange(len(self.__unpacked_data_elms__)):

			for key in self.__keys__[i]:
				new_val = getattr(self, key)
				old_val = self.__unpacked_data_elms__[i]

				# In the case of Unions, when the first changed value
				# is picked the loop is exited
				if new_val != old_val:
					break

			new_values.append(new_val)

		return struct.pack(self.__format__, *new_values)


	def __str__(self):
		return '\n'.join( self.dump() )

	def __repr__(self):
		return '<Structure: %s>' % (' '.join( [' '.join(s.split()) for s in self.dump()] ))


	def dump(self, indentation=0):
		"""Returns a string representation of the structure."""

		dump = []

		dump.append('[%s]' % self.name)

		# Refer to the __set_format__ method for an explanation
		# of the following construct.
		for keys in self.__keys__:
			for key in keys:

				val = getattr(self, key)
				if isinstance(val, int) or isinstance(val, long):
					val_str = '0x%-8X' % (val)
					if key == 'TimeDateStamp' or key == 'dwTimeStamp':
						try:
							val_str += ' [%s UTC]' % time.asctime(time.gmtime(val))
						except exceptions.ValueError, e:
							val_str += ' [INVALID TIME]'
				else:
					val_str = ''.join(filter(lambda c:c != '\0', str(val)))

				dump.append('0x%-8X 0x%-3X %-30s %s' % (
					self.__field_offsets__[key] + self.__file_offset__,
					self.__field_offsets__[key], key+':', val_str))

		return dump

class IconExtractor(object):
	_module_ = None
	_filename_ = None
	_icons_ = {'data': {}, 'headers': {}}
	_icongrps_ = {}
	icon_groups = {}
	_offsets_ = {}
	
	# Struct format strings.
	_GRPICONDIR_fmt_ = ('H,_reserved','H,restype', 'H,count')
	_GRPICONDIRENTRY_fmt_ = ('B,width', 'B,height', 'B,colorcount', 'B,_reserved', 'H,planes', 'H,bits', 'L,bytesize', 'H,id')
	_ICONDIRENTRY_fmt_ = ('B,width', 'B,height', 'B,colorcount', 'B,_reserved', 'H,planes', 'H,bits', 'L,bytesize', 'L,offset')
	
	def __init__(self, module=None, extract=True):
		tmodule = type(module)
		if tmodule in [ HMODULE, int, long ]:
			self._module_ = module
		elif tmodule in [ str, unicode ]:
			self._filename_ = module
		elif module is None:
			self._filename_ = cast(NULL, LPCWSTR)
		else:
			raise TypeError('Did not find a supportable type for module argument.')
		if extract:
			self.process_icons()

	def _load_module(self):
		self._module_ = LoadLibraryEx(
			self._filename_,
			0,
			DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE
		)
		if self._module_ == NULL:
			raise Exception("Can't read resources from file %s" % self._filename_)
	
	def extract_resource(self, hModule, lpType, lpName):
		hResInfo = FindResource(hModule, lpName, lpType)
		szResource = SizeofResource(hModule, hResInfo)
		hResData = LoadResource(hModule, hResInfo)
		try:
			hGlobalData = LockResource(hResData)
			result = int(lpName), string_at(hGlobalData, szResource)
		except:
			result = int(lpName), hGlobalData
		finally:
			FreeResource(hResData)
		return result
	
	def icon_callback(self, hModule, lpType, lpName, lParam):
		icoName, icoRes = self.extract_resource(hModule, lpType, lpName)
		if icoName is None:
			icoName = len(self._icons_.keys())
		self._icons_['data'][icoName] = icoRes
		return True
	
	def icondir_callback(self, hModule, lpType, lpName, lParam):
		icodirName, icodirRes = self.extract_resource(hModule, lpType, lpName)
		if icodirName is None:
			icodirName = len(self._icongrps_.keys())
		self._icongrps_[icodirName] = icodirRes
		return True
	
	def extract_icons(self):
		if self._module_ is None: 
			self._load_module()
			freelib = True
		else:
			freelib = False
		EnumResourceNames(self._module_, RESOURCE_TYPE['RT_ICON'], ENUMRESNAMEPROC(self.icon_callback), NULL)
		EnumResourceNames(self._module_, RESOURCE_TYPE['RT_GROUP_ICON'], ENUMRESNAMEPROC(self.icondir_callback), NULL)
		if freelib:
			FreeLibrary(self._module_)
	
	def _Structure(self, fmtyp, idx):
		fmt = getattr(self, '_%s_fmt_' % fmtyp)
		data = self._icongrps_[idx]
		if self._offsets_[idx] > len(data):
			return None
		st = Structure(fmt, name=fmtyp)
		st.__unpack__(data[self._offsets_[idx]:])
		self._offsets_[idx] += st.sizeof()
		return st
	
	def GRPICONDIRENTRY(self, i, idx):
		self._icons_['headers'][i] = self._Structure('GRPICONDIRENTRY', idx)
	
	def GRPICONDIR(self, idx):
		self._offsets_[idx] = 0
		self.icon_groups[idx] = self._Structure('GRPICONDIR', idx)
		i = 0
		while i < self.icon_groups[idx].count:
			self.GRPICONDIRENTRY(i+1, idx)
			i += 1
	
	def ICONDIRENTRY(self, i):
		st = Structure(self._ICONDIRENTRY_fmt_, name='ICONDIRENTRY')
		data = self._icons_['headers'][i].__pack__()[:-2] + '\0\0\0\0'
		st.__unpack__(data)
		return st
	
	def combine_icons(self, grp, keys):
		data = self._icons_['data']
		headers = {}
		result = grp.__pack__()
		offset = grp.sizeof()
		for k in keys:
			headers[k] = self.ICONDIRENTRY(k)
			offset += headers[k].sizeof()
		for k in keys:
			headers[k].offset = offset
			result += headers[k].__pack__()
			offset += headers[k].bytesize
		for k in keys:
			result += data[k]
		return result
	
	def icon_group(self, icon_group):
		grp = self.icon_groups[icon_group]
		data = self._icons_['data']
		in_group = lambda i: i >= icon_group or i <= grp.count
		icokeys = filter(in_group, data.keys())
		return self.combine_icons(grp, icokeys)
	
	def icon(self, i):
		icon_group = None
		for grp in self.icon_groups.keys():
			if i > self.icon_groups[grp].count: continue
			icon_group = grp
			break
		if icon_group is None:
			raise Exception('No icon with index: %d' % i)
		icon_group = self.icon_groups[icon_group]
		icon_group.count = 1
		
		return self.combine_icons(icon_group, [ i ])
	
	def process_icons(self):
		global _resdir_
		if len(self._icons_['data'].keys()) == 0:
			self.extract_icons()
		for k in self._icongrps_:
			self.GRPICONDIR(k)

def get_python_icon(outfile = 'python'):
	from fs import write_bin
	global _resdir_
	if not os.path.isdir(_resdir_):
		os.makedirs(_resdir_)
	icox = IconExtractor()
	buf = icox.icon_group(1)
	p = os.path.join(_resdir_, outfile + '.ico')
	write_bin(p, buf)
	return p