# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

import peano4.solversteps

from .AbstractFVActionSet import AbstractFVActionSet


class CheckpointLoading(AbstractFVActionSet):
    """!

    Fill the cells of a resumed Finite Volumes run from the checkpoint.

    The action set replaces the initial condition when a checkpoint load path
    is configured: It copies each cell's record into the solver's unknowns,
    stamps the cell label with the checkpoint time, and reinstates the
    solver's scalar state once the traversal ends. The grid itself is rebuilt
    from the checkpoint before this step runs.
    """

    _TemplatePrepareTraversal = """
  if (::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath()) {
    if (_checkpointLoader == nullptr) {
      constexpr int ValuesPerCell
        = BasePowerExponent<{{NUMBER_OF_VOLUMES_PER_AXIS}}, DIMENSIONS>::value
          * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
      _checkpointLoader = std::make_unique<::peano4::checkpoint::CellDataLoader>(
        ::runner::RuntimeParameters::getInstance().getCheckpointLoadPath() + "/{{SOLVER_NAME}}.h5", ValuesPerCell
      );
    }
    _checkpointLoader->loadLocalRecords();
  }
"""

    _TemplateUnprepareTraversal = """
  if (::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath()) {
    repositories::{{SOLVER_INSTANCE}}.setTimeStamp(_checkpointLoader->getTimeStamp());
    {{CHECKPOINT_SCALAR_RESTORE}}
    _checkpointLoader.reset();
  }
"""

    _TemplateTouchCellFirstTime = """
  if ({{PREDICATE}}) {
    constexpr int ValuesPerCell
      = BasePowerExponent<{{NUMBER_OF_VOLUMES_PER_AXIS}}, DIMENSIONS>::value
        * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
    const auto cellName = ::peano4::grid::GridControlEvent::forCell(
      ::peano4::grid::GridControlEvent::RefinementControl::Refine, marker.getX(), marker.getH(), DomainOffset, DomainSize
    );
    const double* record = _checkpointLoader->record(cellName.level, cellName.latticeIndex);
    if (record == nullptr) {
      std::ostringstream message;
      message << "The checkpoint holds no record for the cell at " << marker.getX() << " on level " << cellName.level
              << ". The checkpoint is incomplete or belongs to a different run.";
      tarch::terminateWithError(message.str());
    }
    std::copy_n(record, ValuesPerCell, {{CELL_ACCESSOR}}.data());
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(_checkpointLoader->getTimeStamp());
    coarseGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(_checkpointLoader->getTimeStamp());
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
  }
"""

    def __init__(self, solver, guard):
        AbstractFVActionSet.__init__(self, solver)
        self.guard = (
            "::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath() and ("
            + guard
            + ")"
        )

    def _get_template_replacement_dictionary(self):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        d["CHECKPOINT_SCALAR_RESTORE"] = jinja2.Template(
            self._solver._time_stepping_code_snippets.create_checkpoint_scalar_restore()
        ).render(**d)
        return d

    def get_body_of_prepareTraversal(self):
        return jinja2.Template(self._TemplatePrepareTraversal).render(
            **self._get_template_replacement_dictionary()
        )

    def get_body_of_unprepareTraversal(self):
        return jinja2.Template(self._TemplateUnprepareTraversal).render(
            **self._get_template_replacement_dictionary()
        )

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = self._get_template_replacement_dictionary()
            d["PREDICATE"] = self.guard
            d["CELL_ACCESSOR"] = (
                "fineGridCell" + self._solver.current_unknown_identifier()
            )
            result = jinja2.Template(self._TemplateTouchCellFirstTime).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_attributes(self):
        return """
  static std::unique_ptr<::peano4::checkpoint::CellDataLoader> _checkpointLoader;
"""

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::unique_ptr<::peano4::checkpoint::CellDataLoader> {full_qualified_classname}::_checkpointLoader = nullptr;
"""

    def get_includes(self):
        return (
            super(CheckpointLoading, self).get_includes()
            + """
#include "peano4/checkpoint/CellDataLoader.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/Termination.h"
"""
        )
