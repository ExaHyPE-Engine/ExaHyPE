# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

from coupling.kernels.Limiter import compute_projector_to_point_set, flatten


def different_basis_postprocessing_kernel(condition, order, proj, solver_name):
    return (
        """
    if (
      repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()
      and not marker.willBeRefined() and not marker.hasBeenRefined()
      """
        + condition
        + """
    ) {
      constexpr int fromPoints = {{ORDER}} + 1;
      constexpr int toPoints = """
        + str(order)
        + """ + 1;

      constexpr int FluxElementsPerFace   = facedata::{{UNKNOWN_IDENTIFIER}}FluxEstimates::Cardinality / 2;
      constexpr int BasisElementsPerFace  = facedata::{{UNKNOWN_IDENTIFIER}}Estimates::Cardinality / 2;

      constexpr int FluxElementsPerFace2  = facedata::"""
        + solver_name
        + """FluxEstimates::Cardinality / 2;
      constexpr int BasisElementsPerFace2 = facedata::"""
        + solver_name
        + """Estimates::Cardinality / 2;

      constexpr double proj[toPoints * fromPoints] = {"""
        + proj
        + """};

      for (int d = 0; d < DIMENSIONS; d++) {
        dg2dgFace(
          fineGridFaces"""
        + solver_name
        + """Estimates(d).data() + BasisElementsPerFace2,
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).data() + BasisElementsPerFace,
          proj, toPoints, fromPoints, ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}));
        dg2dgFace(
          fineGridFaces"""
        + solver_name
        + """Estimates(d + DIMENSIONS).data(),
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d + DIMENSIONS).data(),
          proj, toPoints, fromPoints, ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}));

        dg2dgFace(
          fineGridFaces"""
        + solver_name
        + """FluxEstimates(d).data() + FluxElementsPerFace2,
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).data() + FluxElementsPerFace,
          proj, toPoints, fromPoints, {{NUMBER_OF_UNKNOWNS}});
        dg2dgFace(
          fineGridFaces"""
        + solver_name
        + """FluxEstimates(d + DIMENSIONS).data(),
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + DIMENSIONS).data(),
          proj, toPoints, fromPoints, {{NUMBER_OF_UNKNOWNS}});
      }
    }
"""
    )


def same_basis_postprocessing_kernel(condition, solver_name):
    return (
        """
    if (
      repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()
      and not marker.willBeRefined() and not marker.hasBeenRefined()
      """
        + condition
        + """
    ) {
      constexpr int FluxElementsPerFace   = facedata::{{UNKNOWN_IDENTIFIER}}FluxEstimates::Cardinality / 2;
      constexpr int BasisElementsPerFace  = facedata::{{UNKNOWN_IDENTIFIER}}Estimates::Cardinality / 2;

      // Entry 0 of the face holds the left side and entry 1 the right side.
      const int ownFaceSide = fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated(0) ? 0 : 1;

      // A face between two trees can have neither side updated.
      if (fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated(ownFaceSide)) {
        std::copy_n(
          fineGridFace{{UNKNOWN_IDENTIFIER}}Estimates.data() + ownFaceSide * BasisElementsPerFace,
          BasisElementsPerFace,
          fineGridFace"""
        + solver_name
        + """QEstimates.data() + ownFaceSide * BasisElementsPerFace
        );
        std::copy_n(
          fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.data() + ownFaceSide * FluxElementsPerFace,
          FluxElementsPerFace,
          fineGridFace"""
        + solver_name
        + """QFluxEstimates.data() + ownFaceSide * FluxElementsPerFace
        );
      }
    }
"""
    )


def dg2dg_face_header():
    return """
#pragma once
#include "peano4/Loop.h"

template<typename To, typename From>
inline void dg2dgFace(To* qTo, const From* qFrom, const double* proj, int toPoints, int fromPoints, int numVars) {
#if DIMENSIONS == 3
  const int toSize = toPoints * toPoints * numVars;
#else
  const int toSize = toPoints * numVars;
#endif

  std::fill_n(qTo, toSize, 0.0);

  peano4::dForEachExclusive(toPoints, DIMENSIONS - 1, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dofTo) {
    const int dofToSerialised = dofTo[0]
#if DIMENSIONS == 3
      + dofTo[1] * toPoints
#endif
    ;

    peano4::dForEachExclusive(fromPoints, DIMENSIONS - 1, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dofFrom) {
      const int dofFromSerialised = dofFrom[0]
#if DIMENSIONS == 3
        + dofFrom[1] * fromPoints
#endif
      ;

      for (int var = 0; var < numVars; var++) {
        qTo[dofToSerialised * numVars + var] += qFrom[dofFromSerialised * numVars + var] * proj[dofTo[0] * fromPoints + dofFrom[0]]
#if DIMENSIONS == 3
          * proj[dofTo[1] * fromPoints + dofFrom[1]]
#endif
        ;
      }
    });
  });
}
"""


def update_solver_storage(solver, cond):
    s1_load_cell = solver._load_cell_data_default_guard() + cond
    s1_store_cell = solver._store_cell_data_default_guard() + cond
    s1_provide_cell = (
        solver._provide_cell_data_to_compute_kernels_default_guard() + cond
    )

    solver._load_cell_data_default_guard = lambda: s1_load_cell
    solver._store_cell_data_default_guard = lambda: s1_store_cell
    solver._provide_cell_data_to_compute_kernels_default_guard = lambda: s1_provide_cell

    s1_load_face = solver._load_face_data_default_guard() + cond
    s1_store_face = solver._store_face_data_default_guard() + cond

    solver._load_face_data_default_guard = lambda: s1_load_face
    solver._store_face_data_default_guard = lambda: s1_store_face

    # The solver reads these conditions when it creates its face projections, which
    # happens once the project builds the solver.
    s1_send = solver._projection_send_condition() + cond
    s1_receive = solver._projection_receive_condition() + cond

    solver._projection_send_condition = lambda: s1_send
    solver._projection_receive_condition = lambda: s1_receive


def fuse_ader_solvers(solver1, solver2, cond_1, cond_2):
    solver_1_cond = " and " + cond_1
    solver_2_cond = " and " + cond_2

    update_solver_storage(solver1, solver_1_cond)
    update_solver_storage(solver2, solver_2_cond)

    # Fit the time step sizes to one another.
    time_step_size = (
        "std::min( repositories::"
        + solver1.instance_name
        + ".getTimeStepSize(), repositories::"
        + solver2.instance_name
        + ".getTimeStepSize())"
    )

    solver1.use_time_step_size(time_step_size)
    solver2.use_time_step_size(time_step_size)

    quadrature_points_1 = [float(point) for point in solver1.basis.quadrature_points]
    quadrature_points_2 = [float(point) for point in solver2.basis.quadrature_points]

    if quadrature_points_1 != quadrature_points_2:
        proj1 = flatten(
            compute_projector_to_point_set(quadrature_points_1, quadrature_points_2)
        )
        proj2 = flatten(
            compute_projector_to_point_set(quadrature_points_2, quadrature_points_1)
        )
        # Add a postprocessing kernel that interpolates the face data of each solver onto the other.
        solver1.postprocess_solution.add_cell_kernel(
            different_basis_postprocessing_kernel(
                solver_1_cond, str(solver2.order), proj1, solver2._unknown_identifier()
            )
        )
        solver2.postprocess_solution.add_cell_kernel(
            different_basis_postprocessing_kernel(
                solver_2_cond, str(solver1.order), proj2, solver1._unknown_identifier()
            )
        )

        os.makedirs("observers", exist_ok=True)
        with open("observers/dg2dgface.h", "w") as header:
            header.write(dg2dg_face_header())

        solver1.add_includes("observers/dg2dgface.h")
        solver2.add_includes("observers/dg2dgface.h")
    else:
        # Add a postprocessing kernel that copies the face data of each solver to the other.
        solver1.postprocess_solution.add_face_kernel(
            same_basis_postprocessing_kernel(
                solver_1_cond + solver_2_cond, solver2.name
            )
        )
        solver2.postprocess_solution.add_face_kernel(
            same_basis_postprocessing_kernel(
                solver_1_cond + solver_2_cond, solver1.name
            )
        )
