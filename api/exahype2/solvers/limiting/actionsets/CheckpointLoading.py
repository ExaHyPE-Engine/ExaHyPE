# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet

import jinja2
import peano4.solversteps


class CheckpointLoading(AbstractLimiterActionSet):
    """!

    Fill the limiter state of a resumed run from the checkpoint.

    The action set reads the limiter file beside the solver file: Per cell the
    limiter status, the troubled bit and the evolved subcell values. It writes
    the two label attributes and copies the subcell values into both subcell
    arrays, so the resumed run's next sweep reads the same limiter state as
    the writing run's.
    """

    _TemplatePrepareTraversal = """
  if (::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath()) {
    if (_limiterCheckpointLoader == nullptr) {
      constexpr int ValuesPerCell
        = 2 + BasePowerExponent<2 * {{ORDER}} + 1, DIMENSIONS>::value
          * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
      _limiterCheckpointLoader = std::make_unique<::peano4::checkpoint::CellDataLoader>(
        ::runner::RuntimeParameters::getInstance().getCheckpointLoadPath() + "/{{SOLVER_NAME}}Limiter.h5", ValuesPerCell
      );
    }
    _limiterCheckpointLoader->loadLocalRecords();
  }
"""

    _TemplateUnprepareTraversal = """
  if (::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath()) {
    _limiterCheckpointLoader.reset();
  }
"""

    _TemplateTouchCellFirstTime = """
  if ({{PREDICATE}}) {
    constexpr int SubcellValuesPerCell
      = BasePowerExponent<2 * {{ORDER}} + 1, DIMENSIONS>::value
        * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
    const auto cellName = ::peano4::grid::GridControlEvent::forCell(
      ::peano4::grid::GridControlEvent::RefinementControl::Refine, marker.getX(), marker.getH(), DomainOffset, DomainSize
    );
    const double* const record = _limiterCheckpointLoader->record(cellName.level, cellName.latticeIndex);
    if (record == nullptr) {
      std::ostringstream message;
      message << "The checkpoint holds no limiter record for the cell at " << marker.getX() << " on level "
              << cellName.level << ". The checkpoint is incomplete or belongs to a different run.";
      tarch::terminateWithError(message.str());
    }
    fineGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(
      static_cast<::exahype2::dg::LimiterStatus>(static_cast<int>(record[0]))
    );
    fineGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(record[1] != 0.0);
    std::copy_n(record + 2, SubcellValuesPerCell, fineGridCell{{SOLVER_NAME}}Subcell.data());
    std::copy_n(record + 2, SubcellValuesPerCell, fineGridCell{{SOLVER_NAME}}SubcellOld.data());
  }
"""

    def __init__(self, solver, guard):
        super().__init__(solver)
        self.guard = (
            "::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath() and ("
            + guard
            + ")"
        )

    def _get_template_replacement_dictionary(self):
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
        return dictionary

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
            dictionary = self._get_template_replacement_dictionary()
            dictionary["PREDICATE"] = self.guard
            result = jinja2.Template(self._TemplateTouchCellFirstTime).render(
                **dictionary
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_attributes(self):
        return """
  static std::unique_ptr<::peano4::checkpoint::CellDataLoader> _limiterCheckpointLoader;
"""

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::unique_ptr<::peano4::checkpoint::CellDataLoader> {full_qualified_classname}::_limiterCheckpointLoader = nullptr;
"""

    def get_includes(self):
        return (
            super(CheckpointLoading, self).get_includes()
            + """
#include "exahype2/dg/LimiterStatus.h"
#include "peano4/checkpoint/CellDataLoader.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/Termination.h"
"""
        )
