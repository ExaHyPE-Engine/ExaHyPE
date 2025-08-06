// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Coordinate.h"

std::string toolbox::curvi::coordToString(Coordinate c) {
  if (c == Coordinate::X) {
    return "X";
  } else if (c == Coordinate::Y) {
    return "Y";
  } else {
    return "Z";
  }
}

std::string toolbox::curvi::coordToLowercaseString(Coordinate c) {
  if (c == Coordinate::X) {
    return "x";
  } else if (c == Coordinate::Y) {
    return "y";
  } else {
    return "z";
  }
}

toolbox::curvi::Coordinate toolbox::curvi::stringToCoord(std::string s) {
  if (s == coordToString(Coordinate::X)) {
    return Coordinate::X;
  } else if (s == coordToString(Coordinate::Y)) {
    return Coordinate::Y;
  } else {
    return Coordinate::Z;
  }
}

toolbox::curvi::Coordinate toolbox::curvi::getNormal(Coordinate c1, Coordinate c2) {
  return static_cast<Coordinate>(3 - c1 - c2);
}

void toolbox::curvi::getPlane(Coordinate c1, Coordinate& c2, Coordinate& c3) {
  c2 = static_cast<Coordinate>((c1 + 1) % 3);
  c3 = static_cast<Coordinate>((c1 + 2) % 3);
}
