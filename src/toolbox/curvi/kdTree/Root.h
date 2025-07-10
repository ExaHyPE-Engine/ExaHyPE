// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/kdTree/InnerNode.h"
#include "toolbox/curvi/kdTree/Leaf.h"
#include "toolbox/curvi/kdTree/Node.h"

namespace toolbox {
  namespace curvi {
    class Root {
    public:
      Root(const YAML::Node& node, double a_offset[3], double a_size[3], uint a_grid[3]);

      ~Root();

      void init(uint modes[3]);

      BlockBoundary traverse(int a_index[3]);

      BlockBoundary traverseProjection(double coords[3]);

      Block traverse();

      Node* getChild() const { return child; }

      void checkInit();

      std::string toString(int depth = 0);

      void multiplyToTopography(std::function<double(double, double)> factor);


    private:
      bool isInit;

      Coordinate coords[2];
      uint       grid[3];
      double     size[3];
      double     offset[3];
      bool       top;

      easi::Component*                      model;
      std::function<double(double, double)> topo;
      std::function<double(double, double)> final_topo;

      Node* child;
    };
  } // namespace curvi
} // namespace toolbox