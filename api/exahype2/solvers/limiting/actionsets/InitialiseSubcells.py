# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet

import jinja2
import peano4.solversteps


class InitialiseSubcells(AbstractLimiterActionSet):
    """Samples initial conditions on the finite-volume subgrid."""

    _Template_TouchCellFirstTime = """
  if ({{GUARD}}) {
    constexpr int NumberOfSubcellsPerAxis = 2 * {{ORDER}} + 1;
    constexpr int NumberOfData            = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    auto* const   subcellSolution         = fineGridCell{{SOLVER_NAME}}SubcellOld.data();
    peano4::dForEach(NumberOfSubcellsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& subcell) {
      const auto index = peano4::dLinearised(subcell, NumberOfSubcellsPerAxis) * NumberOfData;
      repositories::{{SOLVER_INSTANCE}}.initialCondition(
        subcellSolution + index,
        ::exahype2::fv::getVolumeCentre(marker.getX(), marker.getH(), NumberOfSubcellsPerAxis, subcell),
        ::exahype2::fv::getVolumeSize(marker.getH(), NumberOfSubcellsPerAxis)
      );
    });
{% if USES_DMP %}
    // The nodal interpolant of a discontinuous initial state rings before the
    // first candidate is ever tested, and that ringing radiates a spurious
    // startup wave train. A cell whose interpolant already leaves the range of
    // the directly sampled subcells therefore starts on the subcell scheme.
    constexpr int NumberOfObservables = {{NUMBER_OF_DMP_OBSERVABLES}};
    constexpr int NumberOfSubcells    = DIMENSIONS == 2
         ? NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis
         : NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis;
#if DIMENSIONS == 2
    constexpr int NumberOfNodes = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
    constexpr int NumberOfNodes = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif
    using Shortcuts [[maybe_unused]] = {{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts;
    const auto mapStateToObservables = [](
      const {{SOLUTION_STORAGE_PRECISION}}* const Q,
      {{SOLUTION_STORAGE_PRECISION}}* const observables
    ) {
      {{DMP_OBSERVABLES}}
    };
    {{SOLUTION_STORAGE_PRECISION}} sampledMinimum[NumberOfObservables];
    {{SOLUTION_STORAGE_PRECISION}} sampledMaximum[NumberOfObservables];
    {{SOLUTION_STORAGE_PRECISION}} interpolantMinimum[NumberOfObservables];
    {{SOLUTION_STORAGE_PRECISION}} interpolantMaximum[NumberOfObservables];
    std::fill_n(
      sampledMinimum,
      NumberOfObservables,
      std::numeric_limits<{{SOLUTION_STORAGE_PRECISION}}>::infinity()
    );
    std::fill_n(
      sampledMaximum,
      NumberOfObservables,
      -std::numeric_limits<{{SOLUTION_STORAGE_PRECISION}}>::infinity()
    );
    std::copy_n(sampledMinimum, NumberOfObservables, interpolantMinimum);
    std::copy_n(sampledMaximum, NumberOfObservables, interpolantMaximum);
    ::exahype2::dg::reduceObservableExtrema<NumberOfObservables>(
      sampledMinimum,
      sampledMaximum,
      subcellSolution,
      NumberOfSubcells,
      NumberOfData,
      mapStateToObservables
    );
    ::exahype2::dg::reduceObservableExtrema<NumberOfObservables>(
      interpolantMinimum,
      interpolantMaximum,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      NumberOfNodes,
      NumberOfData,
      mapStateToObservables
    );
    // The interpolant matches the sampled state at its nodes, so its ringing
    // only shows between them: at the Lobatto points and subcell averages.
    constexpr int NumberOfDofsPerAxis = {{ORDER}} + 1;
    static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToLobattoMatrix[] = {{DMP_DG_TO_LOBATTO_MATRIX}};
    static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToSubcellMatrix[] = {{DMP_DG_TO_SUBCELL_MATRIX}};
    ::exahype2::dg::reduceProjectedObservableExtrema<
      DIMENSIONS,
      NumberOfDofsPerAxis,
      NumberOfDofsPerAxis,
      NumberOfData,
      NumberOfObservables
    >(
      interpolantMinimum,
      interpolantMaximum,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      DgToLobattoMatrix,
      mapStateToObservables
    );
    ::exahype2::dg::reduceProjectedObservableExtrema<
      DIMENSIONS,
      NumberOfDofsPerAxis,
      NumberOfSubcellsPerAxis,
      NumberOfData,
      NumberOfObservables
    >(
      interpolantMinimum,
      interpolantMaximum,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      DgToSubcellMatrix,
      mapStateToObservables
    );
{% if DMP_RELAXATION_PARAMETER_IMPLEMENTATION %}
    const auto relaxationParameterForObservable = [](
      [[maybe_unused]] const int observable,
      [[maybe_unused]] const {{SOLUTION_STORAGE_PRECISION}} specifiedRelaxationParameter
    ) -> {{SOLUTION_STORAGE_PRECISION}} {
      {{DMP_RELAXATION_PARAMETER_IMPLEMENTATION}}
    };
{% endif %}
    for (int observableNumber = 0; observableNumber < NumberOfObservables; observableNumber++) {
      const auto observableInRange =
        ::exahype2::dg::satisfiesDiscreteMaximumPrinciple(
          interpolantMinimum[observableNumber],
          interpolantMaximum[observableNumber],
          sampledMinimum[observableNumber],
          sampledMaximum[observableNumber],
{% if DMP_RELAXATION_PARAMETER_IMPLEMENTATION %}
          relaxationParameterForObservable(
            observableNumber,
            static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_RELAXATION_PARAMETER}})
          ),
{% else %}
          static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_RELAXATION_PARAMETER}}),
{% endif %}
          static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_DIFFERENCES_SCALING}})
        );
      if (not observableInRange) {
        fineGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(true);
      }
    }
{% endif %}
  }
"""

    def __init__(self, solver, guard):
        super().__init__(solver)
        self._guard = guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            dictionary = {}
            self._solver._init_dictionary_with_default_parameters(dictionary)
            self._solver.add_entries_to_text_replacement_dictionary(dictionary)
            dictionary["GUARD"] = self._guard
            dictionary["USES_DMP"] = self._solver._subcell_limiter_uses_dmp
            dictionary["NUMBER_OF_DMP_OBSERVABLES"] = (
                self._solver._subcell_limiter_number_of_dmp_observables
            )
            dictionary["DMP_OBSERVABLES"] = self._rendered_dmp_observables(dictionary)
            if dictionary["USES_DMP"]:
                self._add_dmp_sampling_matrices_to_dictionary(dictionary)
            dictionary["DMP_RELAXATION_PARAMETER"] = (
                self._solver._subcell_limiter_dmp_relaxation_parameter
            )
            dictionary["DMP_RELAXATION_PARAMETER_IMPLEMENTATION"] = (
                self._dmp_relaxation_parameter_implementation(dictionary)
            )
            dictionary["DMP_DIFFERENCES_SCALING"] = (
                self._solver._subcell_limiter_dmp_differences_scaling
            )
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(
                **dictionary
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(InitialiseSubcells, self).get_includes()
            + """
#include "exahype2/dg/Limiter.h"
#include "exahype2/fv/PatchUtils.h"
"""
        )
