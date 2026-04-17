# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class AlignmentHelper:
    """
    A simple helper class to handle memory alignment/padding calculations
    based on a specific SIMD width.
    """

    def __init__(self, simd_width):
        assert simd_width > 0
        self._simd_width = simd_width

    def get_size_with_padding(self, size_without_padding):
        """
        Returns the 'size' padded to the next multiple of the SIMD width.
        Logic: roundup(size / width) * width
        """
        # return size_without_padding
        return self._simd_width * int((size_without_padding + (self._simd_width - 1)) / self._simd_width)

    def get_pad_size(self, size_without_padding):
        return self.get_size_with_padding(size_without_padding) - size_without_padding
