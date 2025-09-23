// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/PatchUtils.h"

#include "tarch/la/Vector.h"
#include "peano4/utils/Loop.h"
#include "peano4/utils/Globals.h"

#include <functional>

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
    /**
     * Apply boundary conditions. Works only for a halo size of 1.
     *
     *
     * @param faceNumber Is usually taken from marker.getSelectedFaceNumber() and
     *  is thus a number between 0 and 2d-1.
     *
     */
    template<typename T>
    void applyBoundaryConditions(
      std::function< void(
        const T* __restrict__                        Qinside,
        {%- for i in range(ORDER) %}
        T* __restrict__                              Qoutside{{i}},
        {%- endfor %}
        const tarch::la::Vector<DIMENSIONS,double>&  faceCentre,
        const tarch::la::Vector<DIMENSIONS,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   boundaryCondition,
      const tarch::la::Vector<DIMENSIONS,double>&  faceCentre,
      const tarch::la::Vector<DIMENSIONS,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          overlap,
      int                                          unknowns,
      int                                          faceNumber,
      T* __restrict__                              Q
    );
{% for item in NAMESPACE -%}
}
{%- endfor %}

#include "BoundaryConditions.cpph"
