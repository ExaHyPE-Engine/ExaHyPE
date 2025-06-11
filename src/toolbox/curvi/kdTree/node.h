// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <sstream>
#include "easi/YAMLParser.h"

#include "toolbox/curvi/geometry/block.h"
#include "toolbox/curvi/geometry/blockBoundary.h"
#include "toolbox/curvi/geometry/faceBoundary.h"
#include "toolbox/curvi/geometry/face.h"
#include "toolbox/curvi/coordinate.h"

class Node {
public:
  virtual BlockBoundary traverse(int index[3]) = 0;
  virtual BlockBoundary traverseProjection(double coordinate[3]) = 0;  
  virtual Block traverse() = 0;

  virtual void init(BlockBoundary& boundary, uint modes[3]) = 0;
  virtual std::string toString(int depth = 0) = 0;

};
