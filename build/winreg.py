"""
winreg.py

Sorry, but I'm only implementing the used functions right now.
"""

from ctypes import *
from ctypes.wintypes import *
from . import isx64

# Our DLL
advapi32 = WinDLL('advapi32.dll')

# Types
ULONG_PTR = c_ulonglong if isx64 else DWORD
LONG_PTR = c_longlong if isx64 else c_long
PLONG = POINTER(LONG)
LPDWORD = POINTER(DWORD)
LPBYTE = c_char_p # It's actually unsigned char*, but for our purposes, this should owrk.
HKEY = HANDLE
ACCESS_MASK = DWORD
REGSAM = DWORD

# Constants
HKEY_LOCAL_MACHINE = HKEY(0x80000002)
KEY_WOW64_32KEY = HKEY_LOCAL_MACHINE
KEY_ALL_ACCESS = 0xF003F
KEY_QUERY_VALUE = 0x0001
KEY_READ = 0x20019
NULL = 0
MAX_ENV = 16383
MAX_PATH = 260


# Function prototypes
RegOpenKey = advapi32.RegOpenKeyW
RegOpenKey.restype = LONG
RegOpenKey.argtypes = [ HKEY, LPCWSTR, POINTER(HKEY) ]

RegOpenKeyEx = advapi32.RegOpenKeyExW
RegOpenKeyEx.restype = LONG
RegOpenKeyEx.argtypes = [ HKEY, LPCWSTR, DWORD, REGSAM, POINTER(HKEY) ]

RegQueryValue = advapi32.RegQueryValueW
RegQueryValue.restype = LONG
RegQueryValue.argtypes = [ HKEY, LPCWSTR, LPWSTR, PLONG ]

RegQueryValueEx = advapi32.RegQueryValueExA
RegQueryValueEx.restype = LONG
RegQueryValueEx.argtypes = [ HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD ]

RegCloseKey = advapi32.RegCloseKey
RegCloseKey.restype = LONG
RegCloseKey.argtypes = [ HKEY ]

# Function wrappers
def OpenKey(key, subkey):
	result = HKEY(0)
	if int(RegOpenKey(key, subkey, byref(result))) != 0:
		return None
	return result

def OpenKeyEx(key, subkey, opts, flags):
	result = HKEY(0)
	if int(RegOpenKeyEx(key, subkey, opts, flags, byref(result))) != 0:
		return None
	return result

def QueryValue(key, value):
	dwValue = LONG(0)
	RegQueryValue(key, value, cast(NULL, LPWSTR), byref(dwValue))
	buf = create_unicode_buffer(dwValue.value+1)
	if int(RegQueryValue(key, value, buf, byref(dwValue))) != 0:
		return None
	return buf.value

def QueryValueEx(key, value):
	dwValue = DWORD(0)
	dwType = DWORD(0)
	RegQueryValueEx(key, value, cast(NULL, LPDWORD), cast(NULL, LPDWORD), cast(NULL, LPBYTE), byref(dwValue))
	buf = create_string_buffer(dwValue.value+1)
	if RegQueryValueEx(key, value, cast(NULL, LPDWORD), byref(dwType), buf, byref(dwValue)) and GetLastError() != 0:
		return None, -1
	return buf.value, int(dwType.value)

def CloseKey(key):
	return int(RegCloseKey(key)) == 0


