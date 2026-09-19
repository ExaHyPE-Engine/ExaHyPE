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
     * Sets the initial value of one node.
     *
     * @param Q Unknowns and auxiliary variables of the node.
     * @param x Position of the node.
     * @param h Size of the cell the node lies in.
     */
    virtual void initialCondition(
      {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h
    ) = 0;
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    /**
     * Returns the refinement command of one cell. The minimum and maximum mesh sizes bound the
     * mesh, so a command outside them has no effect.
     *
     * @param Q The cell's (DGOrder+1)^DIMENSIONS nodes of NumberOfUnknowns+NumberOfAuxiliaryVariables
     *  values each, or nullptr throughout grid construction.
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
""",
            undefined=jinja2.DebugUndefined,
        )

        return Template.render(**self.d)
