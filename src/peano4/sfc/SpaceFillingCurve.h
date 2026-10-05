// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/sfc/Grammar.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace peano4::sfc {
  template <class CurveGrammar>
  class SpaceFillingCurve;
} // namespace peano4::sfc

/**
 * Compiles a turtle grammar into the child order of a space-filling curve. A curve state is the
 * index of a grammar non-terminal. For every state, the table lists the children of a cell in curve
 * order with their positions and states.
 */
TARGET_DECLARE
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

    [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getPositionWithinParent() const;
  };

  using ChildSequence = std::array<Child, ChildrenPerCell>;
  using ChildOrder    = std::array<ChildSequence, NumberOfStates>;

  INLINE_METHOD static const ChildSequence& getChildren(int state);

  INLINE_METHOD static constexpr ChildOrder deriveChildOrder();

private:
  using Position = std::array<int, DIMENSIONS>;
  using Point    = std::array<double, DIMENSIONS>;

  INLINE_METHOD static constexpr bool productionsAreInSymbolOrder();
  INLINE_METHOD static constexpr bool everyProductionTilesTheCell();
  INLINE_METHOD static constexpr bool childrenConnect();

  INLINE_METHOD static constexpr std::array<Position, ChildrenPerCell> getPositions(int state);
  INLINE_METHOD static constexpr std::array<Point, NumberOfStates> getLimitPointsWithin(int child);

  static_assert(
    std::is_same_v<
      typename std::remove_cv_t<decltype(CurveGrammar::Productions)>::value_type,
      peano4::sfc::Production<NonTerminal, ChildrenPerCell>
    >,
    "A rule of the space-filling curve must list as many children as a cell contains."
  );
  static_assert(
    productionsAreInSymbolOrder(),
    "The space-filling curve must list each rule at the position of its symbol."
  );
  static_assert(
    everyProductionTilesTheCell(),
    "A rule of the space-filling curve must visit every child of a cell once in unit steps."
  );
  static_assert(childrenConnect(), "The space-filling curve must enter each child where it leaves the previous child.");
};
TARGET_DECLARE_END

#include "peano4/sfc/SpaceFillingCurve.cpph"
