# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class SolverDefinitions:
    def create_solver_user_definitions(self):
        template = """
{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::initialCondition(
  [[maybe_unused]] {{STORAGE_PRECISION}}* const NOALIAS         Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}

{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::boundaryConditions(
  [[maybe_unused]] const {{STORAGE_PRECISION}}* const NOALIAS   Qinside, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] {{STORAGE_PRECISION}}* const NOALIAS         Qoutside, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}

{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
::exahype2::RefinementCommand {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::refinementCriterion(
  [[maybe_unused]] const {{STORAGE_PRECISION}}* const NOALIAS   Q, // [{{NUMBER_OF_VOLUMES_PER_AXIS}}^DIMENSIONS * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}

{% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         F // [{{NUMBER_OF_UNKNOWNS}}]
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   deltaQ, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         BTimesDeltaQ // [{{NUMBER_OF_UNKNOWNS}}]
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         S // [{{NUMBER_OF_UNKNOWNS}}]
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}

{% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::diffusiveSourceTerm(
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   deltaQ, // [{{DIMENSIONS}} * {{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         S // [{{NUMBER_OF_UNKNOWNS}}]
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::solveRiemannProblem(
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QL, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QR, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         FL, // [{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         FR  // [{{NUMBER_OF_UNKNOWNS}}]
) {
  throw std::runtime_error("Not implemented.");
}
{% endif %}
"""

        return template
