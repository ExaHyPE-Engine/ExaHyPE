import mpmath as mp
import os

def compute_projector_to_point_set(quad_points_from, quad_points_to):
  """
  Transforms the degrees of freedom located at Lagrange quadrature_points to degrees of freedom
  located at a set of points as defined by the parameter points.
  
  Let us denote by P the  projection operator. The DoF are computed according to:
  
  u@x =  sum_{m} P_m u^DG_m
  
  Returns:
    Projector:
      The corresponding degrees of freedom located at the set of points defined by the parameter x.
  """
  proj = [[mp.mpf(0) for _ in range(len(quad_points_from))] for _ in range(len(quad_points_to))]
  for idx, point in enumerate(quad_points_to):
      # For each point to which we project, calculate lagrange weights for that point 
      n = len(quad_points_from)
      # weights = []
      for i in range(n):
        numerator = 1.0
        denominator = 1.0
        result = 1.0
        for k in range(n):
            if i != k:
                numerator *= (point - quad_points_from[k])
                denominator *= (quad_points_from[i] - quad_points_from[k])
        result*= numerator / denominator
        proj[idx][i] = result
  return proj

def flatten(xss):
  return ', '.join([mp.nstr(x, n=12) for xs in xss for x in xs])

def different_basis_postprocessing_kernel(condition, order, proj, solver_name):
   return ("""
    if (
      repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()
      and not marker.willBeRefined() and not marker.hasBeenRefined()
      """ + condition + """
    ) {

      constexpr int fromPoints = {{ORDER}} + 1;
      constexpr int toPoints = """ + str(order) + """ + 1;

      constexpr int FluxElementsPerFace   = facedata::{{UNKNOWN_IDENTIFIER}}FluxEstimates::Cardinality / 2;
      constexpr int BasisElementsPerFace  = facedata::{{UNKNOWN_IDENTIFIER}}Estimates::Cardinality / 2;

      constexpr int FluxElementsPerFace2  = facedata::""" + solver_name + """FluxEstimates::Cardinality / 2;
      constexpr int BasisElementsPerFace2 = facedata::""" + solver_name + """Estimates::Cardinality / 2;

      constexpr double proj[toPoints*fromPoints] = {""" + proj + """};

      for (int d = 0; d < DIMENSIONS; d++) {
        // Projected solution estimations
        dg2dgFace(
          fineGridFaces""" + solver_name + """Estimates(d).data() + BasisElementsPerFace2, 
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).data() + BasisElementsPerFace,
          proj, toPoints, fromPoints, ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}));
        dg2dgFace(
          fineGridFaces""" + solver_name + """Estimates(d+DIMENSIONS).data(), 
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d + DIMENSIONS).data(),
          proj, toPoints, fromPoints, ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}));

        // Projected flux estimations
        dg2dgFace(
          fineGridFaces""" + solver_name + """FluxEstimates(d).data() + FluxElementsPerFace2, 
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).data() + FluxElementsPerFace,
          proj, toPoints, fromPoints, {{NUMBER_OF_UNKNOWNS}});
        dg2dgFace(
          fineGridFaces""" + solver_name + """FluxEstimates(d+DIMENSIONS).data(), 
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + DIMENSIONS).data(), 
          proj, toPoints, fromPoints, {{NUMBER_OF_UNKNOWNS}});
      }

    }
""")

def same_basis_postprocessing_kernel(condition, solver_name):
   return ("""
    if (
      repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()
      and not marker.willBeRefined() and not marker.hasBeenRefined()
      """ + condition + """
    ) {

      constexpr int fromPoints = {{ORDER}} + 1;

      constexpr int FluxElementsPerFace   = facedata::{{UNKNOWN_IDENTIFIER}}FluxEstimates::Cardinality / 2;
      constexpr int BasisElementsPerFace  = facedata::{{UNKNOWN_IDENTIFIER}}Estimates::Cardinality / 2;

      // 1 if right side, 0 if left
      //const int ownFaceSide = marker.getSelectedFaceNumber() > DIMENSIONS;
      const int ownFaceSide = fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated(0) ? 0 : 1;

      // Make sure that I have indeed been updated, as if the split is between spacetrees it is possible
      // neither side has been updated
      if( fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated(ownFaceSide) ){
        // Projected solution estimations
        std::copy_n(
          fineGridFace{{UNKNOWN_IDENTIFIER}}Estimates.data() + ownFaceSide * BasisElementsPerFace,
          BasisElementsPerFace,
          fineGridFace""" + solver_name + """QEstimates.data() + ownFaceSide * BasisElementsPerFace
        );
        // Projected flux estimations
        std::copy_n(
          fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.data() + ownFaceSide * FluxElementsPerFace,
          FluxElementsPerFace,
          fineGridFace""" + solver_name + """QFluxEstimates.data() + ownFaceSide * FluxElementsPerFace
        );
      }
    }
""")


def dg2dgFace():
   return """
#pragma once
#include "peano4/utils/Loop.h"

template<typename pTo, typename pFrom>
inline void dg2dgFace(pTo* Qto, const pFrom* Qfrom, const double* proj, int toPoints, int fromPoints, int numVars){

#if DIMENSIONS==3
  const int toSize = toPoints*toPoints*numVars;
#else
  const int toSize = toPoints*numVars;
#endif

  std::fill_n(Qto, toSize, 0.0);

  //For each point we project to
  dfore(dofTo, toPoints, DIMENSIONS-1, 0) {
    const int dofToSerialised = dofTo[0]
  #if DIMENSIONS==3
    + dofTo[1] * toPoints
  #endif
    ;

    //Compute the contribution from each contributing point
    dfore(dofFrom, fromPoints, DIMENSIONS-1, 0) {
      const int dofFromSerialised = dofFrom[0]
  #if DIMENSIONS==3
        + dofFrom[1] * fromPoints
  #endif
    ;

      for(int var=0; var<numVars; var++){
        Qto[dofToSerialised*numVars+var] += Qfrom[dofFromSerialised*numVars+var] * proj[dofTo[0]*fromPoints+dofFrom[0]]
  #if DIMENSIONS==3
          * proj[dofTo[1]*fromPoints+dofFrom[1]]
  #endif
        ;
      }
    }

  }
}
"""

def updateSolverStorage(solver, cond):
    s1_load_cell    = solver._load_cell_data_default_guard() + cond
    s1_store_cell   = solver._store_cell_data_default_guard() + cond
    s1_provide_cell = solver._provide_cell_data_to_compute_kernels_default_guard() + cond

    solver._load_cell_data_default_guard = lambda : s1_load_cell
    solver._store_cell_data_default_guard = lambda : s1_store_cell
    solver._provide_cell_data_to_compute_kernels_default_guard = lambda : s1_provide_cell

    # Remove faces load
    s1_load_face    = solver._load_face_data_default_guard() + cond
    s1_store_face   = solver._store_face_data_default_guard() + cond
    s1_provide_face = solver._provide_face_data_to_compute_kernels_default_guard() + cond

    solver._load_face_data_default_guard = lambda : s1_load_face
    solver._store_face_data_default_guard = lambda : s1_store_face
    solver._provide_face_data_to_compute_kernels_default_guard = lambda : s1_provide_face

    solver.set_implementation()

    solver._rhs_estimates_projection.generator.send_condition               += cond
    solver._rhs_estimates_projection.generator.receive_and_merge_condition  += cond
    solver._flux_estimates_projection.generator.send_condition              += cond
    solver._flux_estimates_projection.generator.receive_and_merge_condition += cond

############################################################
    #This is a variant which would also let you remove a bunch of other unnecessary operations as well
    # the effort of storing and loading the cells at every timestamp, however it doesn't work at the
    # moment because the boundary face between one condition and the other needs to be both at the same time
    # so that the cells on either side of said face receive their required information.
    # This is not possible with a simple negated condition, and wouldn't work with a lot of more complicated
    # conditions (e.g. bathimetries)

def fuseADERSolvers(solver1, solver2, cond_1, cond_2):

    solver_1_cond = " and " + cond_1
    solver_2_cond = " and " + cond_2

    updateSolverStorage(solver1, solver_1_cond)
    updateSolverStorage(solver2, solver_2_cond)

#     # Checks for periodic bc, in which case the boundaries should still be covered in case
#     # the neigbour on the other side exists
#     s1_face_cond = (" and (" + cond_1 + """
#   or (PeriodicBC.test(0) and (marker.x()[0]==DomainOffset[0] or (marker.x()[0]==DomainOffset[0]+DomainSize[0]) ) ) 
#   or (PeriodicBC.test(1) and (marker.x()[1]==DomainOffset[1] or (marker.x()[1]==DomainOffset[1]+DomainSize[1]) ) )
# #if DIMENSIONS==3
#   or (PeriodicBC.test(2) and (marker.x()[2]==DomainOffset[2] or (marker.x()[2]==DomainOffset[2]+DomainSize[2]) ) )
# #endif
#   )
# """)
#     solver1._rhs_estimates_projection.generator.send_condition += s1_face_cond
#     solver1._rhs_estimates_projection.generator.receive_and_merge_condition += s1_face_cond

    # s2_load_cell    = solver2._load_cell_data_default_guard() + solver_2_cond
    # s2_store_cell   = solver2._store_cell_data_default_guard() + solver_2_cond
    # s2_provide_cell = solver2._provide_cell_data_to_compute_kernels_default_guard() + solver_2_cond

    # solver2._load_cell_data_default_guard = lambda : s2_load_cell
    # solver2._store_cell_data_default_guard = lambda : s2_store_cell
    # solver2._provide_cell_data_to_compute_kernels_default_guard = lambda : s2_provide_cell

    # solver1.set_implementation()
    # solver2.set_implementation()

############################################################
#Variant 2: quick and dirty but functional and easy to make work, also more flexible
# def fuseADERSolvers(solver1, solver2, condition):
#     solver_1_cond = " and " + condition
#     solver_2_cond = " and not ( " + condition + ")"

    # # #Add solver guards
    # solver1._action_set_prediction.guard                  += solver_1_cond
    # # solver1._action_set_prediction_on_hanging_cells.guard += solver_1_cond
    # solver1._action_set_correction.guard                  += solver_1_cond
    # solver1._action_set_correction._riemann_guard         += solver_1_cond #doesn't do anything at the moment, this variable isn't connected to anything

    # solver2._action_set_prediction.guard                  += solver_2_cond
    # # solver2._action_set_prediction_on_hanging_cells.guard += solver_2_cond
    # solver2._action_set_correction.guard                  += solver_2_cond
    # # solver2._action_set_correction._riemann_guard         += solver_2_cond #doesn't do anything at the moment, this variable isn't connected to anything

############################################################

    # fit timestep sizes to one another
    compute_time_step_size = """
    double timeStepSize = std::min( repositories::""" + solver1.get_name_of_global_instance() + """.getAdmissibleTimeStepSize(),
                                repositories::""" + solver2.get_name_of_global_instance() + """.getAdmissibleTimeStepSize());
    """

    solver1._compute_time_step_size = compute_time_step_size
    solver2._compute_time_step_size = compute_time_step_size

    aderSolver1QuadPoints = solver1._basis.quadrature_points(render=False)
    aderSolver2QuadPoints = solver2._basis.quadrature_points(render=False)

    if(aderSolver1QuadPoints!=aderSolver2QuadPoints):
      proj1 = flatten(compute_projector_to_point_set(aderSolver1QuadPoints, aderSolver2QuadPoints))
      proj2 = flatten(compute_projector_to_point_set(aderSolver2QuadPoints, aderSolver1QuadPoints))
      # add postprocessing action set to interpolate and copy data from one face to the other
      solver1._action_set_postprocess_solution._compute_cell_kernel += different_basis_postprocessing_kernel(solver_1_cond, str(solver2._order), proj1, solver2._unknown_identifier())
      solver2._action_set_postprocess_solution._compute_cell_kernel += different_basis_postprocessing_kernel(solver_2_cond, str(solver1._order), proj2, solver1._unknown_identifier())

      # provide helper function to both solvers:
      if not os.path.exists("observers"):
        try:
          os.makedirs("observers")
        except:
          raise Exception("Path /observers could not be created")
      f = open("observers/dg2dgface.h", "w")
      f.write(dg2dgFace())
      f.close()

      solver1.add_user_solver_includes("""\n#include "observers/dg2dgface.h"\n""")
      solver2.add_user_solver_includes("""\n#include "observers/dg2dgface.h"\n""")
    else:
      # add postprocessing action set to copy data from one face to the other
      solver1._action_set_postprocess_solution._face_compute_kernel += same_basis_postprocessing_kernel(solver_1_cond + solver_2_cond, solver2.name)
      solver2._action_set_postprocess_solution._face_compute_kernel += same_basis_postprocessing_kernel(solver_1_cond + solver_2_cond, solver1.name)
