// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

namespace toolbox {
  namespace curvi {

    enum Coordinate { Z = 0, Y = 1, X = 2 };

#define NCOORDS 3

    std::string coordToString(Coordinate c);
    std::string coordToLowercaseString(Coordinate c);

    Coordinate stringToCoord(std::string c);
    Coordinate getNormal(Coordinate c1, Coordinate c2);
    void       getPlane(Coordinate c1, Coordinate& c2, Coordinate& c3);

  } // namespace curvi
} // namespace toolbox