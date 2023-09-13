# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2

from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections
from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import compute_number_of_face_projection_quantities

def create_Riemann_solver_call(polynomial_basis,
                               face_projections: FaceProjections):
  """
  number_of_face_projections: Integer
    How many quantities are to be projected onto the face. If you pass in one,
    this means that only the left and right values are projected onto the face.
    If you pass in two, we store the values plus the projections of the
    derivative along the normal per face.
  """

  if isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis) and face_projections==FaceProjections.Solution:
    return """solveRiemannProblem_pointwise_in_situ(
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  deltaQ,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, x, t, dt, normal, F);
          {% endif %}
      }, //ncp
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q, x, t, dt, normal);
      }, // maxEigenvalue
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value,
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.value
    );
"""
  elif isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis) and number_of_face_projections==2:
    return """solveRiemannProblem_pointwise_in_situ_with_gradient_projection(
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  deltaQ,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, x, t, dt, normal, F);
          {% endif %}          
      }, //ncp
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q, x, t, dt, normal);
      }, // maxEigenvalue
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value,
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.value
    );
"""
  else:
    assert False, "not implemented"
    return "#not implemented"
