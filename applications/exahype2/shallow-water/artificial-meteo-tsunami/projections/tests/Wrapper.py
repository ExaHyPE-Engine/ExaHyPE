import ctypes
from ctypes import Structure, c_double
from pathlib import Path
import sys


try:
    _script_dir = Path(__file__).parent
except NameError:
    _script_dir = Path(sys.argv[0]).parent if len(sys.argv) > 0 else Path.cwd()


_lib_path = _script_dir / "build" / "libmylib.so"
_lib = ctypes.CDLL(str(_lib_path))


class Pair(Structure):
    _fields_ = [
        ("first", c_double),
        ("second", c_double),
    ]


_lib.ellipsoidTransform.argtypes = (c_double, c_double)
_lib.ellipsoidTransform.restype = Pair
_lib.ellipsoidTransformInverse.argtypes = (c_double, c_double)
_lib.ellipsoidTransformInverse.restype = Pair

_lib.sphereTransform.argtypes = (c_double, c_double)
_lib.sphereTransform.restype = Pair
_lib.sphereTransformInverse.argtypes = (c_double, c_double)
_lib.sphereTransformInverse.restype = Pair

ellipsoidTransform = _lib.ellipsoidTransform
ellipsoidTransformInverse = _lib.ellipsoidTransformInverse

sphereTransform = _lib.sphereTransform
sphereTransformInverse = _lib.sphereTransformInverse
