# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


class GridWriter(ActionSet):
    """!

    Write the grid group of the current checkpoint.

    The group carries what every solver of the checkpoint reads: The domain,
    the base depth, the refinement plan, the refinement-control state and the
    checkpoint time. The per-solver cell data goes into the solver groups of
    the same file, written by peano4.checkpoint.CellDataWriter.
    """

    def __init__(self):
        super().__init__(descend_invocation_order=1)

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_body_of_prepareTraversal(self):
        return """
  ::peano4::checkpoint::CheckpointFile::getInstance().open(
    ::runner::RuntimeParameters::getInstance().getCheckpointOutputPath()
  );
"""

    def get_body_of_unprepareTraversal(self):
        return """
  ::peano4::checkpoint::writeGrid(
    DomainOffset,
    DomainSize,
    ::exahype2::RefinementControlService::getInstance().serialiseForCheckpoint(),
    ::exahype2::RefinementControlService::CheckpointTableColumns,
    repositories::getMinTimeStamp()
  );
  ::peano4::checkpoint::CheckpointFile::getInstance().close();
"""

    def get_body_of_operation(self, operation_name):
        return "\n"

    def get_includes(self):
        return """
#include "Constants.h"
#include "exahype2/RefinementControlService.h"
#include "peano4/checkpoint/CheckpointFile.h"
#include "peano4/checkpoint/GridCheckpoint.h"
#include "repositories/SolverRepository.h"
#include "runner/RuntimeParameters.h"
"""
