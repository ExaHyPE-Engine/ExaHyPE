// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/kdTree/Node.h"

namespace toolbox {
  namespace curvi {
    class Leaf: public Node {
    public:
      Leaf();


      void init(BlockBoundary& a_boundary, uint modes[3]);

      std::string toString(int depth = 0);

      Block traverse();

      BlockBoundary traverse(int index[3]);

      BlockBoundary traverseProjection(double coordinate[3]);

      void checkInit();

      bool isLeaf();

    private:
      bool          isInit;
      BlockBoundary boundary;
    };
  } // namespace curvi
} // namespace toolbox