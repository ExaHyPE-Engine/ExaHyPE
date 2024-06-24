# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import IntEnum

class Architectures:
    class Alignment(IntEnum):
        noarch = 16
        wsm    = 16
        snb    = 32
        hsw    = 32
        knc    = 64
        knl    = 64
        skx    = 64

    class SimdWidth(IntEnum):
        noarch = 1
        wsm    = 2
        snb    = 4
        hsw    = 4
        knc    = 8
        knl    = 8
        skx    = 8

    @staticmethod
    def get_alignment(architecture: str) -> int:
        return Architectures.Alignment[architecture].value

    @staticmethod
    def get_simd_width(architecture: str) -> int:
        return Architectures.SimdWidth[architecture].value

    @staticmethod
    def get_pad_size(architecture: str, size_without_padding: int) -> int:
        return Architectures.get_size_with_padding(
            architecture, size_without_padding) - size_without_padding

    @staticmethod
    def get_size_with_padding(architecture: str, size_without_padding: int) -> int:
        return Architectures.get_simd_width(architecture) * int(
            size_without_padding + (Architectures.get_simd_width(architecture) - 1)
            / Architectures.get_simd_width(architecture))
