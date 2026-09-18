// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Abstract{{CLASSNAME}}.h"
#include "exahype2/RefinementControl.h"
#include "tarch/instrumentation/Log.h"

{% for item in NAMESPACE -%}
  namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
  }
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public {{NAMESPACE | join("::")}}::Abstract{{CLASSNAME}} {
  public:
    /**
     * Evaluates the refinement criterion for one cell patch.
     *
     * The configured minimum and maximum patch sizes bound the resulting mesh.
     *
     * @param Q Whole nodal patch in AoS layout, or nullptr during grid construction.
     * @param x Patch centre.
     * @param h Patch size.
     * @param t Time.
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const double* NOALIAS Q,
      const tarch::la::Vector<DIMENSIONS,double>& x,
      const tarch::la::Vector<DIMENSIONS,double>& h,
      double t
    ) {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}final{% endif %};

    /**
     * Sets the initial value of one nodal degree of freedom.
     *
     * @param Q Nodal state, the auxiliary variables included.
     * @param x Position of the node.
     * @param h Size of the cell the node lies in.
     */
    virtual void initialCondition(
      double* NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>& x,
      const tarch::la::Vector<DIMENSIONS,double>& h
    ) {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}final{% endif %};

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    /**
     * Writes the outside values of a boundary face node from the inside
     * values, and the Riemann solver reads both afterwards through the flux
     * and eigenvalue callbacks.
     */
    static void boundaryConditions(
      const double* NOALIAS Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double* NOALIAS Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>& x,
      const tarch::la::Vector<DIMENSIONS,double>& h,
      double t,
      int normal
    );
    {% elif BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    /**
     * Writes the outside values of a boundary face node from the inside
     * values, and the Riemann solver reads both afterwards through the flux
     * and eigenvalue callbacks.
     */
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void boundaryConditions(
      [[maybe_unused]] const double* const NOALIAS Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] double* const NOALIAS Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const int normal
    ) {
      {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {{SOLVER_USER_DECLARATIONS}}
};
