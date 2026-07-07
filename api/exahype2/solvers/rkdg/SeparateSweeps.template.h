// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Abstract{{CLASSNAME}}.h"
#include "tarch/instrumentation/Log.h"

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
     * Refinement criterion
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
     * @param Q This is the (current) solution. The data is not set
     *  to a valid value throughout grid construction. In this case,
     *  it is nullptr.
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const double * NOALIAS                  Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS,double>&  volumeCentre,
      const tarch::la::Vector<DIMENSIONS,double>&  volumeH,
      double                                       t
    ) override;
    {% endif %}


    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    virtual void initialCondition(
      double * NOALIAS Q,
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      const tarch::la::Vector<DIMENSIONS,double>&  h
    ) override;
    {% endif %}

    
    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    virtual void boundaryConditions(
      const double * NOALIAS Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double * NOALIAS Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      double                                       t,
      int                                          normal
    )  override;
    {% endif %}

    {{SOLVER_USER_DECLARATIONS}}
};
