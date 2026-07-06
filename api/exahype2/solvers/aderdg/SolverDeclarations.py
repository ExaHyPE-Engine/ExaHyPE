# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class SolverDeclarations:
    def __init__(self, dictionary):
        self.d = dictionary

    def create_solver_user_declarations(self):
        Template = jinja2.Template(
            """
    {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
    static inline
    void initialCondition(
      [[maybe_unused]] {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h
    ) {
      {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{INITIAL_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void boundaryConditions(
      [[maybe_unused]] const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS        Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const int                                      normal
    ) {
      {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    ::exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t
    ) {
      {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<empty>" %}
      {{REFINEMENT_CRITERION_IMPLEMENTATION}}
      {% else %}
      auto result = ::exahype2::RefinementCommand::Keep;
      return result;
      {% endif %}
    }
    {% endif %}

    {% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
    {% if MAX_EIGENVALUE_IMPLEMENTATION!="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    double maxEigenvalue(
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const double                                   dt,
      [[maybe_unused]] const int                                      normal
    ) {
      {% if MAX_EIGENVALUE_IMPLEMENTATION!="<empty>" %}
      {{MAX_EIGENVALUE_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void flux(
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
{% if USE_DIFFUSIVE_FLUX %}
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  deltaQ[DIMENSIONS], // deltaQ[{{NUMBER_OF_UNKNOWNS}}]
{% endif %}
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const double                                   dt,
      [[maybe_unused]] const int                                      normal,
      [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if FLUX_IMPLEMENTATION!="<empty>" %}
      {{FLUX_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void nonconservativeProduct(
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const double                                   dt,
      [[maybe_unused]] const int                                      normal,
      [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if NCP_IMPLEMENTATION!="<empty>" %}
      {{NCP_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void algebraicSource(
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const double                                   dt,
      [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
      {{SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void diffusiveAlgebraicSource(
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS deltaQ,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const double                                   dt,
      [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
      {{DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if MATERIAL_PARAM_IMPLEMENTATION!="<user-defined>" and MATERIAL_PARAM_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void multiplyMaterialParameterMatrix(
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const double                                   dt,
      [[maybe_unused]] const int                                      normal,
      [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   rhs
    ) {
      {% if MATERIAL_PARAM_IMPLEMENTATION!="<empty>" %}
      {{MATERIAL_PARAM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if POINT_SOURCES_IMPLEMENTATION!="<user-defined>" and POINT_SOURCES_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void pointSource(
      [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
      [[maybe_unused]] const double* const                            x,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const double                                   dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        forceVector,
      const int                                                       n
    ) {
      {% if POINT_SOURCES_IMPLEMENTATION!="<empty>" %}
      {{POINT_SOURCES_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void riemannSolver(
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
      {% if RIEMANN_SOLVER_IMPLEMENTATION!="<empty>" %}
      {{RIEMANN_SOLVER_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}
    {% endfor %}
    
    {% if ADJUST_SOLUTION_IMPLEMENTATION!="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void adjustSolution(
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&   h,
      [[maybe_unused]] const double                                   t,
      [[maybe_unused]] const double                                   dt,
      [[maybe_unused]] {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS  Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) {
      {% if ADJUST_SOLUTION_IMPLEMENTATION!="<empty>" %}
      {{ADJUST_SOLUTION_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if INIT_POINT_SOURCES_IMPLEMENTATION!="<user-defined>" and INIT_POINT_SOURCES_IMPLEMENTATION!="<none>" %}
    void initPointSourceLocations([[maybe_unused]] double location[NumberOfPointSources][DIMENSIONS]) override {
      {% if INIT_POINT_SOURCES_IMPLEMENTATION!="<empty>" %}
      {{INIT_POINT_SOURCES_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    void initialCondition(
      {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h
    ) override final;
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    void boundaryConditions(
      const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS        Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&   x,
      const tarch::la::Vector<DIMENSIONS, double>&   h,
      const double                                   t,
      const int                                      normal
    ) override final;
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    ::exahype2::RefinementCommand refinementCriterion(
      const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&   x,
      const tarch::la::Vector<DIMENSIONS, double>&   h,
      const double                                   t
    ) override final;
    {% endif %}

    {% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    double maxEigenvalue(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&   x,
      const tarch::la::Vector<DIMENSIONS, double>&   h,
      const double                                   t,
      const double                                   dt,
      const int                                      normal
    ) override final;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    void flux(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
{% if USE_DIFFUSIVE_FLUX %}
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  deltaQ[DIMENSIONS], // deltaQ[{{NUMBER_OF_UNKNOWNS}}]
{% endif %}
      const tarch::la::Vector<DIMENSIONS, double>&   x,
      const tarch::la::Vector<DIMENSIONS, double>&   h,
      const double                                   t,
      const double                                   dt,
      const int                                      normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    void nonconservativeProduct(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&   x,
      const tarch::la::Vector<DIMENSIONS, double>&   h,
      const double                                   t,
      const double                                   dt,
      const int                                      normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    void algebraicSource(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&   x,
      const tarch::la::Vector<DIMENSIONS, double>&   h,
      const double                                   t,
      const double                                   dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    void diffusiveAlgebraicSource(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  deltaQ,
      const tarch::la::Vector<DIMENSIONS, double>&   x,
      const tarch::la::Vector<DIMENSIONS, double>&   h,
      const double                                   t,
      const double                                   dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" and MATERIAL_PARAM_IMPLEMENTATION!="<none>" %}
    void multiplyMaterialParameterMatrix(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
      const tarch::la::Vector<DIMENSIONS, double>&   x,
      const tarch::la::Vector<DIMENSIONS, double>&   h,
      const double                                   t,
      const double                                   dt,
      const int                                      normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   rhs
    ) override final;
    {% endif %}

    {% if POINT_SOURCES_IMPLEMENTATION=="<user-defined>" and POINT_SOURCES_IMPLEMENTATION!="<none>" %}
    void pointSource(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
      const double* const                            x,
      const double                                   t,
      const double                                   dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS   forceVector,
      const int                                      n
    ) override final;
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    void riemannSolver(
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS       FL, // FL[{{NUMBER_OF_UNKNOWNS}}
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS       FR, // FR[{{NUMBER_OF_UNKNOWNS}}
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double                                       t,
      const double                                       dt,
      const tarch::la::Vector<DIMENSIONS, double>&       x,
      const tarch::la::Vector<DIMENSIONS, double>&       h,
      const int                                          direction,
      const bool                                         isBoundaryFace,
      const int                                          faceIndex
    ) override final;
    {% endif %}
    {% endfor %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    void adjustSolution(
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      const double                                  t,
      const double                                  dt,
      {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS  Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) override final;
    {% endif %}

    {% if INIT_POINT_SOURCES_IMPLEMENTATION=="<user-defined>" and INIT_POINT_SOURCES_IMPLEMENTATION!="<none>" %}
    void initPointSourceLocations(double location[NumberOfPointSources][DIMENSIONS]) override final;
    {% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )

        return Template.render(**self.d)
