{% include "AbstractSolverFixedTimeStepSize.template.cpp" %}



{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  Order( {{ORDER}} ),
{{BASIS_INITIALIZERS | indent(2,True) }}
  _timeStamp(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH({{MAX_H}}),
  _minH({{MIN_H}})
  {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
  _solverState = SolverState::Prediction;
}



void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  switch ( _solverState ) {
    case SolverState::GridConstruction:
      assertion(false);
      break;
    case SolverState::GridInitialisation:
      _solverState = SolverState::Prediction;
      break;
    case SolverState::Prediction:
      _solverState = SolverState::RiemannProblemSolve;
      break;
    case SolverState::RiemannProblemSolve:
      _solverState = SolverState::Correction;
      break;
    case SolverState::Correction:
      _solverState = SolverState::Prediction;
      _timeStamp += {{TIME_STEP_SIZE}};
      break;
    case SolverState::Plotting:
      assertion(false);
      break;
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::Plotting;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {
  _solverState = SolverState::Prediction;
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Prediction:
      return "prediction";
    case SolverState::RiemannProblemSolve:
      return "Riemann-problem-solve";
    case SolverState::Correction:
      return "correction";
    case SolverState::Plotting:
      return "plotting";
  }
  return "<undef>";
}


{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" %}
double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}


{% if FLUX_IMPLEMENTATION!="<none>" %}
{% if FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::flux(
 const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
 const tarch::la::Vector<Dimensions,double>&  x,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<none>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryFlux(
 const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
 const tarch::la::Vector<Dimensions,double>&  x,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  flux(Q,x,t,normal,F);
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<none>" %}
{% if NCP_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             dQdn, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}
{% endif %}


{% if SOURCES_IMPLEMENTATION!="<none>" %}
{% if SOURCES_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::algebraicSources(
    const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
    const tarch::la::Vector<Dimensions,double>&  x,
    double                                       t,
    double * __restrict__ S
) {
  {{SOURCES_IMPLEMENTATION}}
}
{% endif %}
{% endif %}
