# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class AbstractSolverDeclarations:
    def create_abstract_solver_user_declarations(self):
        template = """
    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    virtual void initialCondition(
      {{STORAGE_PRECISION}}* const NOALIAS          Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
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
      const {{STORAGE_PRECISION}}* const NOALIAS    Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {{STORAGE_PRECISION}}* const NOALIAS          Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
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
     * @param Q This is the current patch solution with
     *  NumberOfFiniteVolumesPerAxisPerPatch^DIMENSIONS volumes of
     *  NumberOfUnknowns+NumberOfAuxiliaryVariables values each. The data is
     *  not set to a valid value throughout grid construction. In this case,
     *  it is nullptr.
     * @param x Patch centre.
     * @param h Patch size.
     * @param t Time stamp of the patch.
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const {{STORAGE_PRECISION}}* const NOALIAS    Q, // Q[patch-size * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})]
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      const double                                  t
    ) = 0;
    {% endif %}
"""

        return template
