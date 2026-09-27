// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

namespace peano4::sfc {
  struct Grammar;

  template <class NonTerminal, int NumberOfChildren>
  struct Production;
} // namespace peano4::sfc

/**
 * Provides the alphabet of turtle grammars for space-filling curves.
 *
 * A curve grammar derives from this struct and declares its non-terminals,
 * its start symbol, its subdivision factor and one production per
 * non-terminal. The turtle steps are the terminals: Left and Right move along
 * axis 0, Down and Up along axis 1, Back and Front along axis 2.
 */
TARGET_DECLARE
struct peano4::sfc::Grammar {
  enum Step : std::uint8_t { Left, Right, Down, Up, Back, Front };

  INLINE_METHOD static constexpr int getAxis(Step step);
  INLINE_METHOD static constexpr int getDirection(Step step);

  template <class NonTerminal, class... Symbols>
  static constexpr peano4::sfc::Production<NonTerminal, (sizeof...(Symbols) + 1) / 2> production(
    NonTerminal leftHandSide,
    Symbols... symbols
  );

  template <std::size_t Position, class NonTerminal, int NumberOfChildren>
  static constexpr void placeSymbols(peano4::sfc::Production<NonTerminal, NumberOfChildren>& result);

  template <std::size_t Position, class NonTerminal, int NumberOfChildren, class Symbol, class... Symbols>
  static constexpr void placeSymbols(
    peano4::sfc::Production<NonTerminal, NumberOfChildren>& result,
    Symbol                                                  symbol,
    Symbols... symbols
  );
};

/**
 * Stores one production of a turtle grammar: The child non-terminals in
 * curve order and the turtle step between every pair of consecutive
 * children.
 */
template <class NonTerminal, int NumberOfChildren>
struct peano4::sfc::Production {
  NonTerminal                                                  leftHandSide{};
  std::array<NonTerminal, NumberOfChildren>                    children{};
  std::array<peano4::sfc::Grammar::Step, NumberOfChildren - 1> steps{};
};
TARGET_DECLARE_END

#include "peano4/sfc/Grammar.cpph"
