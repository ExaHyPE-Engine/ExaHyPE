# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class AbstractSolverDeclarations:
    def __init__(self, dictionary):
        self.d = dictionary

    def create_abstract_solver_user_declarations(self):
        Template = jinja2.Template(
            """
    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    /**
     * Set the initial condition of one degree of freedom.
     *
     * @param Q Unknowns and auxiliary variables of the degree of freedom.
     * @param x Position of the quadrature point within the computational domain.
     * @param h Size of the cell within which the quadrature point is placed.
     */
    virtual void initialCondition(
      {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h
    ) = 0;
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    /**
     * Apply boundary conditions. You can overwrite both the inside and the
     * outside values, though most boundary conditions only modify the outside
     * ones. The values set here are afterwards subject to the Riemann solver,
     * i.e. to the flux and the eigenvalues.
     */
    virtual void boundaryConditions(
      const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS    Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS          Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      const double                                  t,
      const int                                     normal
    ) = 0;
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    /**
     * Refinement criterion.
     *
     * ExaHyPE 2 is guided by a maximum and minimum mesh (patch) size.
     * All (dynamic) AMR is constrained by these values, i.e. if your
     * mesh is coarser than the maximum mesh size, ExaHyPE 2 will
     * automatically refine. If you try to refine further than the
     * minimum mesh size, ExaHyPE 2 will ignore any refinement.
     *
     * Consequently, you are fine if you work with a regular mesh:
     * You set the maximum mesh size, and you leave everything else
     * to Peano 4/ExaHyPE 2. If you want to have an adaptive mesh,
     * use this routine to implement the refinement pattern.
     *
     * @param Q This is the current cell solution with (Order+1)^DIMENSIONS
     *  degrees of freedom of NumberOfUnknowns+NumberOfAuxiliaryVariables values
     *  each. The data is not set to a valid value throughout grid construction.
     *  In this case, it is nullptr.
     * @param x Cell centre.
     * @param h Cell size.
     * @param t Time stamp of the cell.
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS    Q, // Q[({{ORDER}}+1)^DIMENSIONS * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})]
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      const double                                  t
    ) = 0;
    {% endif %}

    {% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    virtual double maxEigenvalue(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                    x,
      const tarch::la::Vector<DIMENSIONS, double>&                    h,
      const double                                                    t,
      const double                                                    dt,
      const int                                                       normal
    ) = 0;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                    x,
      const tarch::la::Vector<DIMENSIONS, double>&                    h,
      const double                                                    t,
      const double                                                    dt,
      const int                                                       normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) = 0;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                    x,
      const tarch::la::Vector<DIMENSIONS, double>&                    h,
      const double                                                    t,
      const double                                                    dt,
      const int                                                       normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) = 0;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void algebraicSource(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
      const tarch::la::Vector<DIMENSIONS, double>&                    x,
      const tarch::la::Vector<DIMENSIONS, double>&                    h,
      const double                                                    t,
      const double                                                    dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        S
    ) = 0;
    {% endif %}

    {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void diffusiveAlgebraicSource(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  deltaQ,
      const tarch::la::Vector<DIMENSIONS, double>&                    x,
      const tarch::la::Vector<DIMENSIONS, double>&                    h,
      const double                                                    t,
      const double                                                    dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        S
    ) = 0;
    {% endif %}

    {% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" and MATERIAL_PARAM_IMPLEMENTATION!="<none>" %}
    virtual void multiplyMaterialParameterMatrix(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
      const tarch::la::Vector<DIMENSIONS, double>&                    x,
      const tarch::la::Vector<DIMENSIONS, double>&                    h,
      const double                                                    t,
      const double                                                    dt,
      const int                                                       normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        rhs
    ) = 0;
    {% endif %}

    {% if POINT_SOURCES_IMPLEMENTATION=="<user-defined>" and POINT_SOURCES_IMPLEMENTATION!="<none>" %}
    virtual void pointSource(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  Q,
      const double* const                                             x,
      const double                                                    t,
      const double                                                    dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        forceVector,
      const int                                                       n
    ) = 0;
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    virtual void riemannSolver(
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS        FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS  QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double                                                    t,
      const double                                                    dt,
      const tarch::la::Vector<DIMENSIONS, double>&                    x,
      const tarch::la::Vector<DIMENSIONS, double>&                    h,
      const int                                                       direction,
      const bool                                                      isBoundaryFace,
      const int                                                       faceIndex
    ) = 0;
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    virtual void adjustSolution(
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      const double                                  t,
      const double                                  dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const NOALIAS Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) = 0;
    {% endif %}
    {% endfor %}
""",
            undefined=jinja2.DebugUndefined,
        )

        return Template.render(**self.d)
