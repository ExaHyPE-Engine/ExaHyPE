// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "coordinate.h"

std::string coordToString(Coordinate c){
  if(c == Coordinate::X){
    return "X";
  }
  if(c == Coordinate::Y){
    return "Y";
  }
  if(c == Coordinate::Z){
    return "Z";
  }
}

std::string coordToLowercaseString(Coordinate c){
  if(c == Coordinate::X){
    return "x";
  }
  if(c == Coordinate::Y){
    return "y";
  }
  if(c == Coordinate::Z){
    return "z";
  }
}

Coordinate stringToCoord(std::string s){
  if(s == coordToString(Coordinate::X)){
    return Coordinate::X;
  }
  if(s == coordToString(Coordinate::Y)){
    return Coordinate::Y;
  }
  if(s == coordToString(Coordinate::Z)){
    return Coordinate::Z;
  }
}

Coordinate getNormal(Coordinate c1, Coordinate c2){
  return static_cast<Coordinate>( 3 - c1 - c2);
}

void getPlane(Coordinate c1, Coordinate& c2, Coordinate& c3){
  c2 = static_cast<Coordinate>( (c1+1) % 3);
  c3 = static_cast<Coordinate>( (c1+2) % 3);

}
