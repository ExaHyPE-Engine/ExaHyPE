# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org


import exahype2



def create_Riemann_solver_call(polynomial_basis):
  if isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis):
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
  else:
    return "#not implemented"
