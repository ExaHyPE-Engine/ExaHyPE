// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "easi/YAMLParser.h"
#include "toolbox/curvi/Coordinate.h"
#include "toolbox/curvi/geometry/Block.h"
#include "toolbox/curvi/geometry/BlockBoundary.h"
#include "toolbox/curvi/geometry/Face.h"
#include "toolbox/curvi/geometry/FaceBoundary.h"

#include <sstream>

namespace toolbox {
  namespace curvi {
    class Node {
    public:
      virtual BlockBoundary traverse(int index[3])                   = 0;
      virtual BlockBoundary traverseProjection(double coordinate[3]) = 0;
      virtual Block         traverse()                               = 0;

      virtual void        init(BlockBoundary& boundary, uint modes[3]) = 0;
      virtual std::string toString(int depth = 0)                      = 0;
    };
  } // namespace curvi
} // namespace toolbox