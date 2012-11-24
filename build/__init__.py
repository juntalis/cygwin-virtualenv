from ctypes import c_void_p as _vp, c_ulonglong as _ull, sizeof as _szof
isx64 = _szof(_vp) == _szof(_ull)
del _szof, _ull, _vp