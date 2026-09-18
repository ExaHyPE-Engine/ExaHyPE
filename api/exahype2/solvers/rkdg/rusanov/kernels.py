# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2

from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections


def create_Riemann_solver_call(polynomial_basis,
                               face_projections: FaceProjections):
  """
  Returns the C++ call of the Riemann solver.

  face_projections: FaceProjections
    Solution projects the left and the right value onto the face.
    SolutionAndGradient adds the projection of the derivative along the normal.
  """

  if isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis) and face_projections==FaceProjections.Solution:
    return """solveRiemannProblemPointwiseInSitu(
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
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1D,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data()
    );
"""
  elif isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis) and face_projections==FaceProjections.SolutionAndGradient:
    return """solveRiemannProblemPointwiseInSituWithGradientProjection(
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
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1D,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data()
    );
"""
  else:
    raise NotImplementedError(
      "The Riemann solver call supports a Gauss-Legendre basis with the Solution "
      "or the SolutionAndGradient face projections only."
    )
