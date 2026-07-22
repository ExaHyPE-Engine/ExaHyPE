// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Abstract{{CLASSNAME}}.h"

{% for item in NAMESPACE -%}
  namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};

{% for item in NAMESPACE -%}
  }
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public Abstract{{CLASSNAME}} {
  public:
    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}
    /**
     * Evaluate the refinement criterion for one cell patch.
     *
     * The configured minimum and maximum patch sizes bound the resulting mesh.
     *
     * @param Q Whole nodal patch in AoS layout, or nullptr during grid construction.
     * @param x Patch centre.
     * @param h Patch size.
     * @param t Time.
     */
    ::exahype2::RefinementCommand refinementCriterion(
      const double * NOALIAS                  Q,
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      const tarch::la::Vector<DIMENSIONS,double>&  h,
      double                                       t
    ) override;
    {% endif %}

    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    void initialCondition(
      double * NOALIAS Q,
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      const tarch::la::Vector<DIMENSIONS,double>&  h
    ) override;
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    void boundaryConditions(
      const double * NOALIAS Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double * NOALIAS Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      double                                       t,
      int                                          normal
    ) override;
    {% endif %}

    {{SOLVER_USER_DECLARATIONS}}
};
