# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.solversteps.ActionSet import ActionSet


class ParticleAMR(ActionSet):
    """Controls mesh refinement from particle counts accumulated per cell.

    ParticleTreeAnalysis provides bottom-up subtree counts and records whether
    a cell has refined children. A leaf refines above max_particles_per_cell
    while every child extent remains larger than min_h. A refined cell coarsens
    below min_particles_per_cell when it has no refined children and every
    extent is smaller than max_h. The child check limits coarsening to one level
    per traversal.

    The erase event covers 1.4 cell widths. Staying below two cell widths keeps
    an erase event from reaching halfway into a neighbouring cell, which can
    otherwise remove a retained cell and cause mesh oscillation. This action is
    available only in builds without MPI because its grid-event exchange is
    process-local.
    """

    def __init__(
        self,
        particle_set,
        particle_tree_analysis,
        min_particles_per_cell=1,
        max_particles_per_cell=65536,
        min_h=0.1,
        max_h=0.01,
    ):
        super().__init__(descend_invocation_order=1)
        if min_particles_per_cell < 0:
            raise ValueError(
                "Minimum particle count per cell has to be non-negative: "
                f"{min_particles_per_cell}."
            )
        if max_particles_per_cell < min_particles_per_cell:
            raise ValueError(
                "Maximum particle count per cell has to be at least the minimum: "
                f"{max_particles_per_cell} < {min_particles_per_cell}."
            )
        if min_h <= 0.0:
            raise ValueError(f"Minimum mesh width has to be positive: {min_h}.")
        if max_h <= 0.0:
            raise ValueError(f"Maximum mesh width has to be positive: {max_h}.")

        self._dictionary = {
            "CELL_DATA_NAME": particle_tree_analysis.cell_marker.name,
            "MIN_PARTICLES_PER_CELL": min_particles_per_cell,
            "MAX_PARTICLES_PER_CELL": max_particles_per_cell,
            "MIN_H": min_h,
            "MAX_H": max_h,
        }

    def user_should_modify_template(self):
        return False

    __Template_TouchCellLastTime = jinja2.Template(
        """
  if (
    not marker.willBeRefined()
    and fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() > {{MAX_PARTICLES_PER_CELL}}
    and tarch::la::min(marker.getH()) / 3.0 > {{MIN_H}}
  ) {
    constexpr double RefinementRegionScale = 0.8;
    auto gridControlEvent = peano4::grid::GridControlEvent{};
    gridControlEvent.setRefinementControl(
      peano4::grid::GridControlEvent::RefinementControl::Refine
    );
    gridControlEvent.offset =
      marker.getOffset() + marker.getH() * (1.0 - RefinementRegionScale) / 2.0;
    gridControlEvent.width = marker.getH() * RefinementRegionScale;
    gridControlEvent.targetH = marker.getH() / 3.0 * 1.1;
    _localGridControlEvents.push_back(gridControlEvent);
    logDebug("Add grid control event " << gridControlEvent.toString() << ".");
  } else if (
    marker.willBeRefined()
    and marker.hasBeenRefined()
    and fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() <= {{MIN_PARTICLES_PER_CELL}}
    and not fineGridCell{{CELL_DATA_NAME}}.getParentOfRefinedCell()
    and tarch::la::max(marker.getH()) < {{MAX_H}}
  ) {
    constexpr double CoarseningRegionScale = 1.4;
    auto gridControlEvent = peano4::grid::GridControlEvent{};
    gridControlEvent.setRefinementControl(
      peano4::grid::GridControlEvent::RefinementControl::Erase
    );
    gridControlEvent.offset =
      marker.getOffset() + marker.getH() * (1.0 - CoarseningRegionScale) / 2.0;
    gridControlEvent.width = marker.getH() * CoarseningRegionScale;
    gridControlEvent.targetH = marker.getH() * CoarseningRegionScale;
    _localGridControlEvents.push_back(gridControlEvent);
    logDebug("Add grid control event " << gridControlEvent.toString() << ".");
  }
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  static tarch::multicore::BooleanSemaphore semaphore;
  tarch::multicore::Lock gridControlEventLock(semaphore);
  peano4::grid::GridControlEvent::merge(_localGridControlEvents);
  _newGridControlEvents.insert(
    _newGridControlEvents.end(),
    _localGridControlEvents.begin(),
    _localGridControlEvents.end()
  );
  peano4::grid::GridControlEvent::merge(_newGridControlEvents);
"""
    )

    def get_body_of_operation(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return """
  _localGridControlEvents.clear();
"""
        elif operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            return self.__Template_TouchCellLastTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self.__Template_EndTraversal.render(**self._dictionary)
        return ""

    def get_body_of_getGridControlEvents(self):
        return """
  return _oldGridControlEvents;
"""

    def get_attributes(self):
        return """
  std::vector<peano4::grid::GridControlEvent>        _localGridControlEvents;
  static std::vector<peano4::grid::GridControlEvent> _oldGridControlEvents;
  static std::vector<peano4::grid::GridControlEvent> _newGridControlEvents;
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::vector<peano4::grid::GridControlEvent> {full_qualified_classname}::_oldGridControlEvents;
std::vector<peano4::grid::GridControlEvent> {full_qualified_classname}::_newGridControlEvents;
"""

    def get_includes(self):
        return """
#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"
"""

    def get_body_of_unprepareTraversal(self):
        return """
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    // The merged events never leave the rank that raised them, so a refinement region stops at the
    // subdomain boundary and a neighbour never sees the veto that protects a particle-carrying
    // region from its erase marker.
    tarch::terminateWithError(
      "Particle-driven mesh refinement exchanges no grid control events between ranks. Run it on one rank."
    );
  }
  _oldGridControlEvents.clear();
  _oldGridControlEvents.swap(_newGridControlEvents);
  peano4::grid::GridControlEvent::merge(_oldGridControlEvents);
"""
