// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/Coordinate.h"
#include "toolbox/curvi/input/YAMLInput.h"
#include "toolbox/curvi/interpolation/LagrangeInterpolation.h"

namespace toolbox {
  namespace curvi {
    class Interface {
    public:
      Interface(std::string file, double a_offset[3], double a_size[3], uint a_elements[3], int a_order);

      ~Interface();

      void initTree();

      Root* getRoot();

      double invertProjection(Coordinate coord, double coords[3]);

      Block getElement(double* nodes, int ez, int ey, int ex);

    private:
      int order;

      Root* root;

      double offset[3];
      double size[3];
      uint   elements[3];
      uint   modes[3];
      uint   grid[3];
    };
  } // namespace curvi
} // namespace toolbox