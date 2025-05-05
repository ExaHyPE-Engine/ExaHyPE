# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor

class UpdateCellWithEnclaveTasking(ReconstructPatchAndApplyFunctor):
    """!
    Update cell in primary enclave sweep

    This action set is used in the primary sweeps only. In the secondary sweep,
    its counterpart, the action set MergeEnclaveTaskOutcome, is active and works in all data
    computed by tasks which have been spawned here.

    We extend the superclass ReconstructPatchAndApplyFunctor and hence have
    access to the reconstructed data including a halo layer of one. Furthermore,
    the superclass provides us with a guard which we should use, as this guard
    ensures that we reconstruct the patch plus halo if and only if certain
    conditions are met. By default, we compute only on unrefined octants.

    Our condition whether to spawn a task or to compute the new time step
    data immediately depends on peano4::datamanagement::CellMarker::willBeSkeletonCell().
    If this predicate holds, we compute stuff straightaway. Otherwise, we
    span a new enclave task. While this check does the job in almost all
    cases, there are special situations where you might want to label more
    cells as skeleton cells.


    ## Modify templates

    You can alter the template. Typical codes augment _Template_TouchCellFirstTime_Preamble
    for example. However, there are two things to consider:

    - _Template_TouchCellFirstTime_Preamble is a member of the class and
      initialised in the constructor.
    - The constructor is used to create an object in the create_action_sets()
      of the using class.

    If you want to alter the preamble, you thus should specialise
    create_action_sets() and invoke the supertype's create_action_sets(). After
    that, alter self._action_set_update_cell._Template_TouchCellFirstTime_Preamble.
    We recommend to add an entry and not to replace the preamble, as the
    preamble already consists meaningful code.
    """

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

      if (marker.willBeSkeletonCell()) {
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
        fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
        repositories::{{SOLVER_INSTANCE}}.update(
          newTimeStepSize,
          timeStamp + timeStepSize,
          std::min({marker.h()(0), marker.h()(1)
#if Dimensions == 3
            , marker.h()(2)
#endif
          })
        );
      } else { // is an enclave cell
        assertion(marker.willBeEnclaveCell());
        assertion(not marker.willBeRefined());

        tarch::multicore::spawnTask(new tasks::{{SOLVER_NAME}}EnclaveTask(
          marker,
          timeStamp,
          timeStepSize,
          oldQWithHalo,
          newQ
        ));
      }

      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);
""")

    def __init__(self, solver):
        ReconstructPatchAndApplyFunctor.__init__(
            self,
            patch=solver._patch,
            patch_overlap=solver._patch_overlap_new,
            functor_implementation="""
      #error Please switch to your Riemann solver of choice
      """,
            # Guard does not use willBeRefined() in contrast to SingleSweep
            guard ="not marker.hasBeenRefined() and ("
                  + "repositories::"
                  + solver.get_name_of_global_instance()
                  + ".getSolverState()=="
                  + solver._name
                  + "::SolverState::Primary or "
                  + "repositories::"
                  + solver.get_name_of_global_instance()
                  + ".getSolverState()=="
                  + solver._name
                  + "::SolverState::PrimaryAfterGridInitialisation"
                  + ")",
            add_assertions_to_halo_exchange=True,
        )

        self._Template_TouchCellFirstTime_Preamble = (
            """
  if (
    repositories::"""
            + solver.get_name_of_global_instance()
            + """.getSolverState()=="""
            + solver._name
            + """::SolverState::Primary or
    repositories::"""
            + solver.get_name_of_global_instance()
            + """.getSolverState()=="""
            + solver._name
            + """::SolverState::PrimaryAfterGridInitialisation
  ) {{
    fineGridCell"""
            + solver._name
            + """CellLabel.setHasUpdated(false);
  }}
"""
            + self._Template_TouchCellFirstTime_Preamble
        )

        self._solver = solver


    def _add_action_set_entries_to_dictionary(self, d):
        """
        First ask the solver to add its symbols, and then re-construct the
        functor which should not contain any symbols after that anymore.
        Next, we call the superclass routine which supplements all those
        instructions that any reconstruction wants to have.
        """

        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)

        self.functor_implementation = self.SolveRiemannProblemsOverPatch.render(**d)

        super(UpdateCellWithEnclaveTasking, self)._add_action_set_entries_to_dictionary(d)

    def get_includes(self):
        return (
            ReconstructPatchAndApplyFunctor.get_includes(self)
            + """
#include "repositories/SolverRepository.h"
#include "tasks/"""
            + self._solver._name
            + """EnclaveTask.h"
#include "exahype2/fv/PatchUtils.h"
"""
            + self._solver._get_default_includes()
            + self._solver.user_action_set_includes
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
