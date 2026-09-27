// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/sfc/Grammar.h"
#include "tarch/accelerator/accelerator.h"

namespace peano4::sfc {
  struct PeanoGrammar;
} // namespace peano4::sfc

/**
 * Declares the Peano curve through its turtle grammar.
 *
 * The two-dimensional productions are those of Michael Bader, Space-Filling
 * Curves, Springer 2013, Sect. 3.3, with the two axes swapped so the curve
 * runs along axis 0 first. P enters a cell at its origin corner, Q is P
 * reflected along axis 1, R is P reflected along axis 0 and S is P reflected
 * along both axes. In three dimensions every production stacks three
 * two-dimensional productions along axis 2: The production of its own symbol,
 * the one of the symbol reflected along both other axes, and its own again.
 */
TARGET_DECLARE
struct peano4::sfc::PeanoGrammar final : peano4::sfc::Grammar {
  static constexpr std::string_view Name{"Peano"};

  static constexpr int SubdivisionFactor{3};

  enum NonTerminal : std::uint8_t { P, Q, R, S };

  static constexpr NonTerminal StartSymbol{P};

  // The productions are laid out one turtle row per line, so the table reads like the textbook figure.
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
