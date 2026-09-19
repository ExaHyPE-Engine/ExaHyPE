// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/sfc/Grammar.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace peano4::sfc {
  template <class CurveGrammar>
  class SpaceFillingCurve;
} // namespace peano4::sfc

/**
 * Compiles a turtle grammar into the child-order table of a space-filling
 * curve.
 *
 * A curve state is the index of a grammar non-terminal. For every state the
 * table lists the children of a cell in curve order, each with its position
 * within the parent and the state of the child. The table is derived from
 * the productions when the class is instantiated, and the static assertions
 * reject a grammar whose productions are listed out of symbol order, do not
 * tile the cell with unit steps, or do not connect at the child boundaries.
 */
template <class CurveGrammar>
class peano4::sfc::SpaceFillingCurve final {
public:
  using NonTerminal = typename CurveGrammar::NonTerminal;

  static constexpr std::string_view Name{CurveGrammar::Name};

  static constexpr int SubdivisionFactor{CurveGrammar::SubdivisionFactor};
  static constexpr int ChildrenPerCell{peano4::power(SubdivisionFactor, DIMENSIONS)};
  static constexpr int NumberOfStates{static_cast<int>(CurveGrammar::Productions.size())};

  static constexpr std::uint8_t RootState{static_cast<std::uint8_t>(CurveGrammar::StartSymbol)};

  struct Child {
    std::uint8_t                         linearisedPosition{0};
    std::uint8_t                         state{0};
    std::array<std::uint8_t, DIMENSIONS> position{};

    [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> positionWithinParent() const;
  };

  using ChildSequence = std::array<Child, ChildrenPerCell>;
  using ChildOrder    = std::array<ChildSequence, NumberOfStates>;

  INLINE_METHOD static const ChildSequence& children(int state);
  INLINE_METHOD static double curveParameter(const tarch::la::Vector<DIMENSIONS, double>& unitPoint, int level);

  static constexpr ChildOrder deriveChildOrder();

private:
  using Position        = std::array<int, DIMENSIONS>;
  using Point           = std::array<double, DIMENSIONS>;
  using SequenceIndices = std::array<std::array<std::uint8_t, ChildrenPerCell>, NumberOfStates>;

  static constexpr SequenceIndices deriveSequenceIndices();

  static constexpr bool productionsAreInSymbolOrder();
  static constexpr bool everyProductionTilesTheCell();
  static constexpr bool childrenConnect();

  static constexpr std::array<Position, ChildrenPerCell> positionsOf(int state);
  static constexpr std::array<Point, NumberOfStates>     limitPointsWithin(int child);

  static_assert(
    std::is_same_v<
      typename std::remove_cv_t<decltype(CurveGrammar::Productions)>::value_type,
      peano4::sfc::Production<NonTerminal, ChildrenPerCell>>,
    "Every production lists SubdivisionFactor^DIMENSIONS children."
  );
  static_assert(productionsAreInSymbolOrder(), "Every production is listed at the index of its left-hand side.");
  static_assert(
    everyProductionTilesTheCell(),
    "Every production visits each child once with unit steps inside the cell."
  );
  static_assert(childrenConnect(), "The exit point of every child meets the entry point of its successor.");
};

#include "peano4/sfc/SpaceFillingCurve.cpph"
