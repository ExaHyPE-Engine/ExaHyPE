// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LevelJumpEvents.h"

#include "exahype2/RefinementControlService.h"

/**
 * Returns the grid control events the refinement control service holds.
 */
std::vector<peano4::grid::GridControlEvent> exahype2::amr::gridControlEvents() {
  return exahype2::RefinementControlService::getInstance().getGridControlEvents();
}
