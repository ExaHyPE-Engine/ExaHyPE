# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2

from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections


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
        const double * NOALIAS                  Q,
        const tarch::la::Vector<DIMENSIONS,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * NOALIAS                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * NOALIAS                  Q,
          const double * NOALIAS                  deltaQ,
          const tarch::la::Vector<DIMENSIONS,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * NOALIAS                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, x, t, dt, normal, F);
          {% endif %}
      }, //ncp
      [&](
          const double * NOALIAS                  Q,
          const tarch::la::Vector<DIMENSIONS,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q, x, t, dt, normal);
      }, // maxEigenvalue
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data()
    );
"""
  elif isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis) and face_projections==FaceProjections.SolutionAndGradient:
    return """solveRiemannProblem_pointwise_in_situ_with_gradient_projection(
      [&](
        const double * NOALIAS                  Q,
        const tarch::la::Vector<DIMENSIONS,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * NOALIAS                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * NOALIAS                  Q,
          const double * NOALIAS                  deltaQ,
          const tarch::la::Vector<DIMENSIONS,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * NOALIAS                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, x, t, dt, normal, F);
          {% endif %}          
      }, //ncp
      [&](
          const double * NOALIAS                  Q,
          const tarch::la::Vector<DIMENSIONS,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q, x, t, dt, normal);
      }, // maxEigenvalue
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data()
    );
"""
  else:
    assert False, "not implemented"
    return "#not implemented"
