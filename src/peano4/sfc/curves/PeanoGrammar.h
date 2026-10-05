// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/sfc/Grammar.h"
#include "tarch/accelerator/CompileTarget.h"

namespace peano4::sfc {
  struct PeanoGrammar;
} // namespace peano4::sfc

/**
 * The Peano curve as a turtle grammar after Michael Bader, Space-Filling Curves, Springer 2013,
 * section 3.3, with the axes swapped so that the curve runs along axis 0 first. P enters a cell at
 * its origin corner, and Q, R and S are P reflected along axis 1, axis 0 and both axes. In 3D, every
 * production stacks along axis 2 its own 2D production, the one reflected along both other axes and
 * its own again.
 */
TARGET_DECLARE
struct peano4::sfc::PeanoGrammar final : peano4::sfc::Grammar {
  static constexpr std::string_view Name{"Peano"};

  static constexpr int SubdivisionFactor{3};

  enum NonTerminal : std::uint8_t { P, Q, R, S };

  static constexpr NonTerminal StartSymbol{P};

  // clang-format off
#if DIMENSIONS == 2
  static constexpr std::array Productions{
    production(
      P,
      P, Right, Q, Right, P, Up,
      R, Left, S, Left, R, Up,
      P, Right, Q, Right, P
    ),
    production(
      Q,
      Q, Right, P, Right, Q, Down,
      S, Left, R, Left, S, Down,
      Q, Right, P, Right, Q
    ),
    production(
      R,
      R, Left, S, Left, R, Up,
      P, Right, Q, Right, P, Up,
      R, Left, S, Left, R
    ),
    production(
      S,
      S, Left, R, Left, S, Down,
      Q, Right, P, Right, Q, Down,
      S, Left, R, Left, S
    )
  };
#elif DIMENSIONS == 3
  static constexpr std::array Productions{
    production(
      P,
      P, Right, Q, Right, P, Up, R, Left, S, Left, R, Up, P, Right, Q, Right, P, Front,
      S, Left, R, Left, S, Down, Q, Right, P, Right, Q, Down, S, Left, R, Left, S, Front,
      P, Right, Q, Right, P, Up, R, Left, S, Left, R, Up, P, Right, Q, Right, P
    ),
    production(
      Q,
      Q, Right, P, Right, Q, Down, S, Left, R, Left, S, Down, Q, Right, P, Right, Q, Back,
      R, Left, S, Left, R, Up, P, Right, Q, Right, P, Up, R, Left, S, Left, R, Back,
      Q, Right, P, Right, Q, Down, S, Left, R, Left, S, Down, Q, Right, P, Right, Q
    ),
    production(
      R,
      R, Left, S, Left, R, Up, P, Right, Q, Right, P, Up, R, Left, S, Left, R, Back,
      Q, Right, P, Right, Q, Down, S, Left, R, Left, S, Down, Q, Right, P, Right, Q, Back,
      R, Left, S, Left, R, Up, P, Right, Q, Right, P, Up, R, Left, S, Left, R
    ),
    production(
      S,
      S, Left, R, Left, S, Down, Q, Right, P, Right, Q, Down, S, Left, R, Left, S, Front,
      P, Right, Q, Right, P, Up, R, Left, S, Left, R, Up, P, Right, Q, Right, P, Front,
      S, Left, R, Left, S, Down, Q, Right, P, Right, Q, Down, S, Left, R, Left, S
    )
  };
#endif
  // clang-format on
};
TARGET_DECLARE_END
