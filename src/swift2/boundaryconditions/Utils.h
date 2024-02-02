// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "peano4/datamanagement/VertexMarker.h"

namespace swift2 {
  namespace boundaryconditions {
    bool isVertexOnGlobalBoundary(
      const peano4::datamanagement::VertexMarker&   marker,
      const tarch::la::Vector<Dimensions, double>&  domainOffset,
      const tarch::la::Vector<Dimensions, double>&  domainSize
    );
  }
}

