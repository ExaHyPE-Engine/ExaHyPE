# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet
from ..ProjectionMatrices import create_subcell_projection_matrices

from exahype2.solvers.LagrangeBasis import render_tensor_1

import jinja2
import peano4.solversteps


class RecomputeSubcells(AbstractLimiterActionSet):
    """Recomputes limited cells on the finite-volume subgrid."""

    _Template_TouchCellFirstTime = """
  if ({{CELL_GUARD}}) {
    if (::exahype2::dg::runsSubcellScheme(fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus())) {
      // The corrector advanced this cell's stamp when it committed the candidate,
      // so the step being recomputed started one step size earlier.
      const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
      const double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp() - timeStepSize;

      constexpr int NumberOfDofsPerAxis     = {{ORDER}} + 1;
      constexpr int NumberOfSubcellsPerAxis = 2 * {{ORDER}} + 1;
      constexpr int NumberOfData            = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};

{% if USES_ADMISSIBILITY_CRITERION and FV_FALLBACK_RECONSTRUCTION %}
      constexpr int NumberOfSubcells    = DIMENSIONS == 2
           ? NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis
           : NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis;
      using Shortcuts [[maybe_unused]]  = {{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts;
      const auto isPhysicallyAdmissible = [&marker](const {{SOLUTION_STORAGE_PRECISION}}* const Q) -> bool {
        [[maybe_unused]] constexpr int NumberOfUnknowns           = {{NUMBER_OF_UNKNOWNS}};
        [[maybe_unused]] constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};
        [[maybe_unused]] const int     numberOfStates             = 1;
        [[maybe_unused]] const auto    x                          = marker.getX();
        [[maybe_unused]] const auto    h                          = marker.getH();
        {{PHYSICAL_ADMISSIBILITY_CRITERION}}
      };
{% endif %}

      const {{SOLUTION_STORAGE_PRECISION}}* oldFaceData[2 * DIMENSIONS];
      {{SOLUTION_STORAGE_PRECISION}}* newFaceData[2 * DIMENSIONS];
      for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
        oldFaceData[faceNumber] = fineGridFaces{{SOLVER_NAME}}SubcellOld(faceNumber).data();
        newFaceData[faceNumber] = fineGridFaces{{SOLVER_NAME}}Subcell(faceNumber).data();
      }

      // The subcell halo never travels across a resolution transition, so the
      // outer layers of such a face are rebuilt in the very traversal that
      // consumes them. A persistent coarse face samples its neighbour's
      // reconciled Galerkin face trace, projected onto the tangential subcell
      // grid. A hanging face is recreated each traversal without valid data, so
      // the cell extrapolates its own face-touching layer; an outgoing wave
      // still reaches the coarse side through its trace sampling.
      for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
        const auto& transitionFaceLabel = fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber);
        const auto  isCoarseTransitionSide =
          transitionFaceLabel.getAboveHanging()[0] or transitionFaceLabel.getAboveHanging()[1];
        const auto isFineTransitionSide = transitionFaceLabel.getIsHanging();
        if (not isCoarseTransitionSide and not isFineTransitionSide) {
          continue;
        }
        const auto axis = faceNumber % DIMENSIONS;
#if DIMENSIONS == 2
        constexpr int TangentialSubcells = NumberOfSubcellsPerAxis;
#else
        constexpr int TangentialSubcells = NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis;
#endif
        {{SOLUTION_STORAGE_PRECISION}} outerHaloProfile[TangentialSubcells * NumberOfData];
        auto* const oldTransitionFace = fineGridFaces{{SOLVER_NAME}}SubcellOld(faceNumber).data();
        if (isCoarseTransitionSide) {
#if DIMENSIONS == 2
          constexpr int TangentialNodes = NumberOfDofsPerAxis;
#else
          constexpr int TangentialNodes = NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif
          constexpr auto    TraceElementsPerSide    = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;
          const auto* const neighbourTraceEstimates = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(faceNumber).data()
            + (faceNumber < DIMENSIONS ? 0 : TraceElementsPerSide);
          {{SOLUTION_STORAGE_PRECISION}} neighbourTrace[TangentialNodes * NumberOfData];
          for (int entry = 0; entry < TangentialNodes * NumberOfData; entry++) {
            neighbourTrace[entry] = static_cast<{{SOLUTION_STORAGE_PRECISION}}>(neighbourTraceEstimates[entry]);
          }
          static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToFvMatrix[] = {{DG_TO_FV_MATRIX}};
          ::exahype2::dg::projectTensorProduct<
            DIMENSIONS - 1, NumberOfDofsPerAxis, NumberOfSubcellsPerAxis, NumberOfData
          >(neighbourTrace, outerHaloProfile, DgToFvMatrix);
        }
        peano4::dForEachExclusive(
          NumberOfSubcellsPerAxis,
          axis,
          0,
          [&](const tarch::la::Vector<DIMENSIONS, int>& tangentialIndex) {
            int tangentialSerialised = 0;
            int tangentialStride     = 1;
            for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
              if (dimension != axis) {
                tangentialSerialised += tangentialIndex(dimension) * tangentialStride;
                tangentialStride *= NumberOfSubcellsPerAxis;
              }
            }
            if (isFineTransitionSide) {
              // The hanging face is recreated each traversal, so the touching
              // layer comes from the persistent cell patch.
              auto ownIndex            = tangentialIndex;
              ownIndex(axis)           = faceNumber < DIMENSIONS ? 0 : NumberOfSubcellsPerAxis - 1;
              const auto serialisedOwn = peano4::dLinearised(ownIndex, NumberOfSubcellsPerAxis);
              for (int value = 0; value < NumberOfData; value++) {
                outerHaloProfile[tangentialSerialised * NumberOfData + value] =
                  fineGridCell{{SOLVER_NAME}}SubcellOld.data()[serialisedOwn * NumberOfData + value];
              }
            }
            for (int layer = 0; layer < {{FV_HALO_WIDTH}}; layer++) {
              auto outerIndex            = tangentialIndex;
              outerIndex(axis)           = faceNumber < DIMENSIONS ? layer : {{FV_HALO_WIDTH}} + layer;
              const auto serialisedOuter = ::toolbox::blockstructured::serialiseVoxelIndexInOverlap(
                outerIndex,
                NumberOfSubcellsPerAxis,
                {{FV_HALO_WIDTH}},
                axis
              );
              for (int value = 0; value < NumberOfData; value++) {
                oldTransitionFace[serialisedOuter * NumberOfData + value] =
                  outerHaloProfile[tangentialSerialised * NumberOfData + value];
              }
            }
          }
        );
      }

      const auto advanceSubcells = [&]<typename Reconstruction>() -> double {
        return ::exahype2::fv::applyFiniteVolume<
          Reconstruction,
          {{FV_TIME_STEPPING}},
          {{FULL_QUALIFIED_SOLVER_NAME}},
          NumberOfSubcellsPerAxis,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          {{EVALUATE_FLUX}},
          {{EVALUATE_NONCONSERVATIVE_PRODUCT}},
          {{EVALUATE_SOURCE}},
          {{EVALUATE_DIFFUSIVE_SOURCE}},
          {{EVALUATE_CUSTOM_RIEMANN}},
          {{EVALUATE_MAX_EIGENVALUE}},
          {{EVALUATE_MAX_EIGENVALUE_OF_FINAL_STATE}},
          {{SOLUTION_STORAGE_PRECISION}},
          {{SOLUTION_STORAGE_PRECISION}}
        >(
          fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
          fineGridCell{{SOLVER_NAME}}Subcell.data(),
          oldFaceData,
          newFaceData,
          marker.getX(),
          marker.getH(),
          timeStamp,
          timeStepSize
        );
      };
      [[maybe_unused]] auto maxEigenvalue = advanceSubcells.template operator()<{{FV_RECONSTRUCTION}}>();

{% if USES_ADMISSIBILITY_CRITERION and FV_FALLBACK_RECONSTRUCTION %}
      auto* const subcellSolution = fineGridCell{{SOLVER_NAME}}Subcell.data();
      bool needsFallbackReconstruction = false;
      for (int subcell = 0; subcell < NumberOfSubcells; subcell++) {
        if (not isPhysicallyAdmissible(subcellSolution + subcell * NumberOfData)) {
          needsFallbackReconstruction = true;
          break;
        }
      }
      if (needsFallbackReconstruction) {
        // The fallback shares the primary scheme's halo layout, so it can
        // overwrite the patch and its face data without reallocating storage.
        maxEigenvalue = advanceSubcells.template operator()<{{FV_FALLBACK_RECONSTRUCTION}}>();
      }
{% endif %}

      // Q is the limiter's current plot buffer. Regular cells retain the accepted
      // DG correction there; a limited cell replaces it here with the projection
      // of its current finite-volume subgrid. Q_old remains the predictor backup.
      auto* const updatedState = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
      static constexpr {{SOLUTION_STORAGE_PRECISION}} FvToDgMatrix[] = {{FV_TO_DG_MATRIX}};
      ::exahype2::dg::projectTensorProduct<DIMENSIONS, NumberOfSubcellsPerAxis, NumberOfDofsPerAxis, NumberOfData>(
        fineGridCell{{SOLVER_NAME}}Subcell.data(),
        updatedState,
        FvToDgMatrix
      );

      // The evolved subgrid is next step's input for a cell that stays on the
      // subcell scheme, so it rolls over unsmoothed by the polynomial round trip.
      constexpr int SubcellValues = DIMENSIONS == 2
        ? NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfData
        : NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfData;
      std::copy_n(
        fineGridCell{{SOLVER_NAME}}Subcell.data(),
        SubcellValues,
        fineGridCell{{SOLVER_NAME}}SubcellOld.data()
      );
{% if IS_ADAPTIVE %}
      // The subgrid resolves waves the smoothed candidate hides, so the admissible
      // step must come from the finite-volume wave speed, not from re-measuring the
      // projected-back polynomial. Folding it through the same statistics with the
      // cell size keeps it consistent with the Galerkin cells: the polynomial order
      // factor in the step formula already accounts for the subcell resolution.
      ::exahype2::dg::MaxEigenvalueStatistics subcellEigenvalueStatistics;
      subcellEigenvalueStatistics.accumulate<DIMENSIONS>(
        marker.getH(),
        [maxEigenvalue]([[maybe_unused]] const int dimension) { return maxEigenvalue; }
      );
      repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(subcellEigenvalueStatistics.maxPhysicalEigenvalue);
      repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(subcellEigenvalueStatistics.maxScaledEigenvalue);
{% endif %}
    }
  }
"""

    def __init__(self, solver, cell_guard):
        super().__init__(solver)
        self._cell_guard = cell_guard

    @staticmethod
    def _cpp_bool(value):
        return "true" if value else "false"

    def _add_action_set_entries_to_dictionary(self, dictionary):
        dictionary["CELL_GUARD"] = self._cell_guard
        dictionary["FV_RECONSTRUCTION"] = (
            self._solver._subcell_limiter_solver.reconstruction
        )
        dictionary["FV_FALLBACK_RECONSTRUCTION"] = (
            self._solver._subcell_limiter_solver.fallback_reconstruction
        )
        dictionary["FV_HALO_WIDTH"] = self._solver._subcell_limiter_solver.halo_width
        dictionary["FV_TIME_STEPPING"] = (
            self._solver._subcell_limiter_solver.time_stepping
        )
        # Only an adaptive solver derives the step from eigenvalues; a fixed step
        # is the user's and needs no fold-in.
        is_adaptive = bool(self._solver._compute_new_time_step_size.strip())
        dictionary["IS_ADAPTIVE"] = is_adaptive
        dictionary["EVALUATE_FLUX"] = self._cpp_bool(dictionary["USE_FLUX"])
        dictionary["EVALUATE_NONCONSERVATIVE_PRODUCT"] = self._cpp_bool(
            dictionary["USE_NCP"]
        )
        dictionary["EVALUATE_SOURCE"] = self._cpp_bool(dictionary["USE_SOURCE"])
        dictionary["EVALUATE_DIFFUSIVE_SOURCE"] = self._cpp_bool(
            dictionary["USE_DIFFUSIVE_SOURCE"]
        )
        dictionary["EVALUATE_CUSTOM_RIEMANN"] = self._cpp_bool(
            self._solver._subcell_limiter_solver.riemann_solver is not None
        )
        dictionary["EVALUATE_MAX_EIGENVALUE"] = self._cpp_bool(
            dictionary["USE_MAX_EIGENVALUE"]
        )
        dictionary["EVALUATE_MAX_EIGENVALUE_OF_FINAL_STATE"] = self._cpp_bool(
            dictionary["USE_MAX_EIGENVALUE"] and is_adaptive
        )
        dg_to_fv, fv_to_dg = create_subcell_projection_matrices(self._solver._basis)
        dictionary["DG_TO_FV_MATRIX"] = render_tensor_1(
            [value for row in dg_to_fv for value in row]
        )
        dictionary["FV_TO_DG_MATRIX"] = render_tensor_1(
            [value for row in fv_to_dg for value in row]
        )
        dictionary["USES_ADMISSIBILITY_CRITERION"] = (
            self._solver._subcell_limiter_uses_admissibility_criterion
        )
        dictionary["PHYSICAL_ADMISSIBILITY_CRITERION"] = (
            self._physical_admissibility_criterion(dictionary)
        )

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            dictionary = {}
            self._solver._init_dictionary_with_default_parameters(dictionary)
            self._solver.add_entries_to_text_replacement_dictionary(dictionary)
            self._add_action_set_entries_to_dictionary(dictionary)
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(
                **dictionary
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        scheme_includes = "".join(
            '#include "' + header + '"\n'
            for header in self._solver._subcell_limiter_solver.includes
        )
        return (
            super(RecomputeSubcells, self).get_includes()
            + """
#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/SubcellProjection.h"
#include "toolbox/blockstructured/Enumeration.h"
"""
            + scheme_includes
        )
