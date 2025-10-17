# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class SolverDefinitions:
    def __init__(self, dictionary):
        self.d = dictionary

    def create_solver_user_definitions(self):
        Template = jinja2.Template(
            """
{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::initialCondition(
  [[maybe_unused]] {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const bool                                   gridIsConstructed
) {
  // @todo Implement
}
{% endif %}

{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::boundaryConditions(
  [[maybe_unused]] const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS   Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS         Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  // @todo Implement
}
{% endif %}

{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
::exahype2::RefinementCommand {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::refinementCriterion(
  [[maybe_unused]] const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t
) {
  auto result = ::exahype2::RefinementCommand::Keep;
  // @todo Implement
  return result;
}
{% endif %}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
{% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal
) {
  // @todo Implement
  return 0.0;
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo Implement
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo Implement
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo Implement
}
{% endif %}

{% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::diffusiveAlgebraicSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   deltaQ,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo Implement
}
{% endif %}

{% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" and MATERIAL_PARAM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
  [[maybe_unused]] const double                                   t,
  [[maybe_unused]] const double                                   dt,
  [[maybe_unused]] const int                                      normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   rhs
) {
  // @todo Implement
}
{% endif %}

{% if POINT_SOURCES_IMPLEMENTATION=="<user-defined>" and POINT_SOURCES_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
  [[maybe_unused]] const double* const                            x,
  [[maybe_unused]] const double                                   t,
  [[maybe_unused]] const double                                   dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   forceVector,
  [[maybe_unused]] const int                                      n
) {
  // @todo Implement
}
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::riemannSolver(
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS       FL, // FL[{{NUMBER_OF_UNKNOWNS}}
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS       FR, // FR[{{NUMBER_OF_UNKNOWNS}}
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double                                       t,
  [[maybe_unused]] const double                                       dt,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&       x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&       h,
  [[maybe_unused]] const int                                          direction,
  [[maybe_unused]] const bool                                         isBoundaryFace,
  [[maybe_unused]] const int                                          faceIndex
) {
  // @todo Implement
}
{% endif %}
{% endfor %}

{% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::adjustSolution(
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
  [[maybe_unused]] const double                                   t,
  [[maybe_unused]] const double                                   dt,
  [[maybe_unused]] {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS  Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
) {
  // @todo Implement
}
{% endif %}

{% if INIT_POINT_SOURCES_IMPLEMENTATION=="<user-defined>" and INIT_POINT_SOURCES_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::initPointSourceLocations([[maybe_unused]] double location[NumberOfPointSources][DIMENSIONS]) {
  // @todo Implement
}
{% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )

        return Template.render(**self.d)
