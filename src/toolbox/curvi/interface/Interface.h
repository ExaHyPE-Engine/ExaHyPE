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

      //For a given coordinate in the physical space, this returns the coordinate in direction coord of
      // the point in the reference (cartesian) space which will be projected to that position.
      // i.e. if you want the x-coordinate of the point which will be mapped to position (x,y,z)
      // (e.g for a tracer or a point source) then you hand this function the parameters
      // Coordinate::X and the coordinates in the curvi order [z,y,x].
      double invertProjection(Coordinate coord, double coords[3]);

      Block getElement(double* nodes, int ex, int ey, int ez);

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
