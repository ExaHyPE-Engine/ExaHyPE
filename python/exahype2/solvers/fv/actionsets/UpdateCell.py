# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor

class UpdateCell(ReconstructPatchAndApplyFunctor):
  SolveRiemannProblemsOverPatch = jinja2.Template("""
      const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      {{COMPUTE_TIME_STEP_SIZE}}
      assertion2(tarch::la::greaterEquals(timeStamp,    0.0), timeStamp, timeStepSize);
      assertion2(tarch::la::greaterEquals(timeStepSize, 0.0), timeStamp, timeStepSize);

       #if PeanoDebug > 0
       ::exahype2::fv::validatePatch(
         oldQWithHalo,
         {{NUMBER_OF_UNKNOWNS}},
         {{NUMBER_OF_AUXILIARY_VARIABLES}},
         {{NUMBER_OF_VOLUMES_PER_AXIS}},
         {{HALO_SIZE}}, // Halo size
         std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
       ); // // Previous time step has to be valid
       #endif

      {{PREPROCESS_RECONSTRUCTED_PATCH}}
      {% if COMPUTE_MAX_EIGENVALUE %}
      const double maxEigenvalue = {{COMPUTE_KERNEL_CALL}}
      {% else %}
      const double _ = {{COMPUTE_KERNEL_CALL}}
      {% endif %}
      {{POSTPROCESS_UPDATED_PATCH}}

      #if PeanoDebug > 0
      ::exahype2::fv::validatePatch(
        newQ,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        0, // Halo size
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
      ); // Outcome has to be valid
      #endif

      {{COMPUTE_NEW_TIME_STEP_SIZE}}
      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);
      repositories::{{SOLVER_INSTANCE}}.update(
        newTimeStepSize,
        timeStamp + timeStepSize,
        std::min({marker.h()(0), marker.h()(1)
#if Dimensions == 3
          , marker.h()(2)
#endif
        })
      );
""")

  def __init__(self, solver):
    """
    """
    ReconstructPatchAndApplyFunctor.__init__(self,
      patch = solver._patch,
      patch_overlap = solver._patch_overlap_new,
      functor_implementation = """
#error Please switch to your Riemann solver of choice
""",
      guard = "not marker.willBeRefined() and not marker.hasBeenRefined()",
      add_assertions_to_halo_exchange = False,
    )
    self._solver = solver

    self._Template_TouchCellFirstTime_Epilogue += """
  else if (not marker.willBeRefined() and not marker.hasBeenRefined()) {{
    const double timeStamp    = fineGridCell{SOLVER_NAME}CellLabel.getTimeStamp();
    const double timeStepSize = fineGridCell{SOLVER_NAME}CellLabel.getTimeStepSize();

    assertion2(::tarch::la::greaterEquals(timeStepSize, 0.0), timeStepSize, timeStamp);
    assertion2(::tarch::la::greaterEquals(timeStamp,    0.0), timeStepSize, timeStamp);

    repositories::{SOLVER_INSTANCE}.update(0.0, timeStamp, marker.h()(0));
  }}
"""

    self._Template_TouchCellFirstTime_Preamble = """
  fineGridCell""" + solver._name + """CellLabel.setHasUpdated(false);
""" + self._Template_TouchCellFirstTime_Preamble

  def _add_action_set_entries_to_dictionary(self,d):
    """
    This is our plug-in point to alter the underlying dictionary
    """
    super(UpdateCell,self)._add_action_set_entries_to_dictionary(d)

    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)

    d["CELL_FUNCTOR_IMPLEMENTATION"] = self.SolveRiemannProblemsOverPatch.render(**d)

  def get_includes(self):
    return """
#include "exahype2/fv/PatchUtils.h"
""" + self._solver._get_default_includes() + self._solver.user_action_set_includes

  def get_action_set_name(self):
      return __name__.replace(".py", "").replace(".", "_")
