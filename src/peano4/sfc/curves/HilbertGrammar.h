// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/sfc/Grammar.h"

namespace peano4::sfc {
  struct HilbertGrammar;
} // namespace peano4::sfc

/**
 * Declares the Hilbert curve through its turtle grammar.
 *
 * The two-dimensional productions are those of Michael Bader, Space-Filling
 * Curves, Springer 2013, Sect. 3.1. H enters a cell at its origin corner and
 * leaves it at the corner on the far side of axis 0. A is H reflected along
 * the diagonal, C is H rotated by 180 degrees and B is C reflected along the
 * diagonal.
 *
 * The three-dimensional productions are Bader's grammar with twelve
 * non-terminals, Sect. 8.1.3. A non-terminal names the signed axis
 * permutation that maps the pattern xyz onto it: The letters give the images
 * of the axes x, y, z, and an upper-case letter stands for a barred letter in
 * the book, a negated axis. The pattern xyz enters the cell at its origin
 * corner and leaves it at the corner on the far side of axis 2.
 */
struct peano4::sfc::HilbertGrammar final : peano4::sfc::Grammar {
  static constexpr std::string_view Name{"Hilbert"};

  static constexpr int SubdivisionFactor{2};

#if DIMENSIONS == 2
  enum NonTerminal : std::uint8_t { H, A, B, C };

  static constexpr NonTerminal StartSymbol{H};

  static constexpr std::array Productions{
    production(H, A, Up, H, Right, H, Down, B),
    production(A, H, Right, A, Up, A, Left, C),
    production(B, C, Left, B, Down, B, Right, H),
    production(C, B, Down, C, Left, C, Up, A)
  };
#elif DIMENSIONS == 3
  // The case of each letter carries the axis sign the class comment describes.
  enum NonTerminal : std::uint8_t {
    xyz,
    xYZ,
    yzx,
    yZX,
    zxy,
    zXY,
    XyZ,
    XYz,
    YzX,
    YZx,
    ZxY,
    ZXy // NOLINT(readability-identifier-naming)
  };

  static constexpr NonTerminal StartSymbol{xyz};

  static constexpr std::array Productions{
    production(xyz, yzx, Up, zxy, Right, zxy, Down, XYz, Front, XYz, Up, ZxY, Left, ZxY, Down, yZX),
    production(xYZ, yZX, Down, zXY, Right, zXY, Up, XyZ, Back, XyZ, Down, ZXy, Left, ZXy, Up, yzx),
    production(yzx, zxy, Right, xyz, Front, xyz, Left, YzX, Up, YzX, Right, xYZ, Back, xYZ, Left, ZXy),
    production(yZX, zXY, Right, xYZ, Back, xYZ, Left, YZx, Down, YZx, Right, xyz, Front, xyz, Left, ZxY),
    production(zxy, xyz, Front, yzx, Up, yzx, Back, zXY, Right, zXY, Front, YZx, Down, YZx, Back, XyZ),
    production(zXY, xYZ, Back, yZX, Down, yZX, Front, zxy, Right, zxy, Back, YzX, Up, YzX, Front, XYz),
    production(XyZ, YzX, Up, ZxY, Left, ZxY, Down, xYZ, Back, xYZ, Up, zxy, Right, zxy, Down, YZx),
    production(XYz, YZx, Down, ZXy, Left, ZXy, Up, xyz, Front, xyz, Down, zXY, Right, zXY, Up, YzX),
    production(YzX, ZxY, Left, XyZ, Back, XyZ, Right, yzx, Up, yzx, Left, XYz, Front, XYz, Right, zXY),
    production(YZx, ZXy, Left, XYz, Front, XYz, Right, yZX, Down, yZX, Left, XyZ, Back, XyZ, Right, zxy),
    production(ZxY, XyZ, Back, YzX, Up, YzX, Front, ZXy, Left, ZXy, Back, yZX, Down, yZX, Front, xyz),
    production(ZXy, XYz, Front, YZx, Down, YZx, Back, ZxY, Left, ZxY, Front, yzx, Up, yzx, Back, xYZ)
  };
#endif
};
