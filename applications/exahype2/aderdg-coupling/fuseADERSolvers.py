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

def postprocessing_kernel(condition, order, proj, solver_name):
   return ("""
    if (
      repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()
      and not marker.willBeRefined() and not marker.hasBeenRefined()
      """ + condition + """
    ) {

      constexpr int fromPoints = {{ORDER}} + 1;
      constexpr int toPoints = """ + str(order) + """ + 1;

      #if Dimensions == 2
      constexpr int SpaceFaceSize      = fromPoints;
      constexpr int SpaceFaceSize2     = toPoints;
      #else
      constexpr int SpaceFaceSize      = fromPoints*fromPoints;
      constexpr int SpaceFaceSize2     = toPoints*toPoints;
      #endif

      constexpr int FluxElementsPerFace   = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};
      constexpr int BasisElementsPerFace  = SpaceFaceSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});

      constexpr int FluxElementsPerFace2  = SpaceFaceSize2 * {{NUMBER_OF_UNKNOWNS}};
      constexpr int BasisElementsPerFace2 = SpaceFaceSize2 * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
                                                               
      constexpr double proj[toPoints*fromPoints] = {""" + proj + """};

      for (int d = 0; d < Dimensions; d++) {
        // Projected solution estimations
        dg2dgFace(
          fineGridFaces""" + solver_name + """Estimates(d).value + BasisElementsPerFace2, 
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value + BasisElementsPerFace,
          proj, toPoints, fromPoints, {{NUMBER_OF_UNKNOWNS}});
        dg2dgFace(
          fineGridFaces""" + solver_name + """Estimates(d+Dimensions).value, 
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d + Dimensions).value,
          proj, toPoints, fromPoints, {{NUMBER_OF_UNKNOWNS}});

        // Projected flux estimations
        dg2dgFace(
          fineGridFaces""" + solver_name + """FluxEstimates(d).value + FluxElementsPerFace2, 
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value + FluxElementsPerFace,
          proj, toPoints, fromPoints, ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}));
        dg2dgFace(
          fineGridFaces""" + solver_name + """FluxEstimates(d+Dimensions).value, 
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + Dimensions).value, 
          proj, toPoints, fromPoints, ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}));
      }

    }
""")

def dg2dgFace():
   return """
#pragma once
#include "peano4/utils/Loop.h"

inline void dg2dgFace(double* Qto, const double* Qfrom, const double* proj, int toPoints, int fromPoints, int numVars){

#if Dimensions==3
  const int toSize = toPoints*toPoints*numVars;
#else
  const int toSize = toPoints*numVars;
#endif

  std::fill_n(Qto, toSize, 0.0);

  //For each point we project to
  dfore(dofTo, toPoints, Dimensions-1, 0) {
    const int dofToSerialised = dofTo[0]
  #if Dimensions==3
    + dofTo[1] * toPoints
  #endif
    ;

    //Compute the contribution from each contributing point
    dfore(dofFrom, fromPoints, Dimensions-1, 0) {
      const int dofFromSerialised = dofFrom[0]
  #if Dimensions==3
        + dofFrom[1] * fromPoints
  #endif
    ;

      for(int var=0; var<numVars; var++){
        Qto[dofToSerialised*numVars+var] += Qfrom[dofFromSerialised*numVars+var] * proj[dofTo[0]*fromPoints+dofFrom[0]]
  #if Dimensions==3
          * proj[dofTo[1]*fromPoints+dofFrom[1]]
  #endif
        ;
      }
    }

  }
}
"""

def fuseADERSolvers(solver1, solver2, condition):
    aderSolver1QuadPoints = solver1._basis.quadrature_points(render=False)
    aderSolver2QuadPoints = solver2._basis.quadrature_points(render=False)

    proj1 = flatten(compute_projector_to_point_set(aderSolver1QuadPoints, aderSolver2QuadPoints)) 
    proj2 = flatten(compute_projector_to_point_set(aderSolver2QuadPoints, aderSolver1QuadPoints))

    # fit timestep sizes to one another
    compute_time_step_size = """
    double timeStepSize = std::min( repositories::""" + solver1.get_name_of_global_instance() + """.getAdmissibleTimeStepSize(),
                                repositories::""" + solver2.get_name_of_global_instance() + """.getAdmissibleTimeStepSize());
    """

    solver1._compute_time_step_size  = compute_time_step_size
    solver2._compute_time_step_size = compute_time_step_size

    solver_1_cond = " and " + condition
    solver_2_cond = " and not ( " + condition + ")"

    # super_condition = (
    #    "( " + condition + 
    #    " or ( std::abs(marker.x()[0]-1.0)<0.0001 || std::abs(marker.x()[0]+1.0)<0.0001 || std::abs(marker.x()[1]-1.0)<0.0001 || std::abs(marker.x()[0]+1.0)<0.0001 ))"
    # )

    # solver1._test_restrictions_temp = super_condition + " and "
    # solver2._test_restrictions_temp = "not ( " + super_condition + " ) and "

    #Add solver guards
    solver1._action_set_prediction.guard += solver_1_cond
#    solver1._action_set_prediction_on_hanging_cells.guard += solver_1_cond
    solver1._action_set_correction.guard += solver_1_cond

    solver2._action_set_prediction.guard += solver_2_cond
#    solver2._action_set_prediction_on_hanging_cells.guard += solver_2_cond
    solver2._action_set_correction.guard += solver_2_cond

    # add postprocessing action set to copy data from one face to the other
    solver1._action_set_postprocess_solution._compute_kernel += postprocessing_kernel(solver_1_cond, str(solver2._order), proj1, solver2._unknown_identifier())
    solver2._action_set_postprocess_solution._compute_kernel += postprocessing_kernel(solver_2_cond, str(solver1._order), proj2, solver1._unknown_identifier())

    # provide helper function to both solvers:
    if not os.path.exists("observers"):
      try:
        os.makedirs("observers")
      except:
         raise Exception("Path /observers could not be created")
    f = open("observers/dg2dgface.h", "w")
    f.write(dg2dgFace())
    f.close()

    # solver1.user_action_set_includes += """\n#include "dg2dgface.h"\n"""
    # solver2.user_action_set_includes += """\n#include "dg2dgface.h"\n"""

    solver1.add_user_solver_includes("""\n#include "observers/dg2dgface.h"\n""")
    solver2.add_user_solver_includes("""\n#include "observers/dg2dgface.h"\n""")
