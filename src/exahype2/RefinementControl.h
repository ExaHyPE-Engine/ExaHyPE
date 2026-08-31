// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridControlEvent.h"

namespace exahype2 {
  enum class RefinementCommand { Keep, Refine, Erase };

  RefinementCommand getDefaultRefinementCommand();

  class RefinementControl;
  class RefinementControlService;
} // namespace exahype2

/**
 * Collects one solver's refine and erase requests.
 *
 * One container lives in each refinement-criterion action set. It collects one
 * grid-control event per command, and RefinementControlService::merge() reduces
 * the per-observer containers into the rank's request set after the traversal.
 */
class exahype2::RefinementControl final {
public:
  friend class RefinementControlService;

  explicit RefinementControl(int solverNumber = 0);

  void clear();

  void addCommand(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    exahype2::RefinementCommand                  command
  );

  std::string toString() const;

private:
  /**
   * Stores one request with the solver that raised it. Two requests order by
   * their events, then by their solver numbers.
   */
  struct NewEvent {
    peano4::grid::GridControlEvent event{};
    int                            solverNumber{0};

    bool operator<(const NewEvent& other) const noexcept;
    bool operator==(const NewEvent& other) const noexcept;
  };

  using NewEvents = std::vector<NewEvent>;

  /**
   * Stores the requests a criterion has raised, one per cell it looked at.
   */
  NewEvents _newEvents;

  int _solverNumber{0};
};
