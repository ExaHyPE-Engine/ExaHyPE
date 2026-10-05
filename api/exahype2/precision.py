# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

PRECISIONS = {
    "SP": "float",
    "float": "float",
    "single": "float",
    "fp32": "float",
    "DP": "double",
    "double": "double",
    "fp64": "double",
    "quadruple": "long double",
    "long double": "long double",
    "fp128": "long double",
    "float16_t": "std::float16_t",
    "std::float16_t": "std::float16_t",
    "_Float16": "_Float16",
    "fp16": "_Float16",
    "bfloat16_t": "std::bfloat16_t",
    "std::bfloat16_t": "std::bfloat16_t",
    "__bf16": "__bf16",
    "bfloat": "__bf16",
    "bf16": "__bf16",
}


def cpp_type(precision):
    """Returns the C++ type of a precision name such as ``"float"`` or ``"fp64"``."""
    if precision not in PRECISIONS:
        raise ValueError(
            "The solver supports the precisions {} only (got '{}').".format(
                ", ".join(PRECISIONS), precision
            )
        )
    return PRECISIONS[precision]


class Precision:
    """The storage precision of the solution and the compute precision of the kernels."""

    def __init__(self, storage="double", compute=None):
        self.storage = cpp_type(storage)
        self.compute = cpp_type(storage if compute is None else compute)

    @classmethod
    def of(cls, value):
        """Returns ``value`` as a Precision: A name, a Precision or None for double."""
        if value is None:
            return cls()
        if isinstance(value, cls):
            return value
        return cls(value)

    @property
    def types(self):
        """Returns the distinct C++ types, the compute type first."""
        result = [self.compute]
        if self.storage not in result:
            result.append(self.storage)
        return result

    def __repr__(self):
        return "Precision(storage={!r}, compute={!r})".format(
            self.storage, self.compute
        )
