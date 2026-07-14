// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceType.h"

std::ostream& operator<<(std::ostream& out, const peano4::grid::FaceType& type) {
  return (out << peano4::grid::toString(type));
}

std::string peano4::grid::toString(FaceType type) {
  switch (type) {
  case FaceType::New:
    return "New";
  case FaceType::Hanging:
    return "Hanging";
  case FaceType::Persistent:
    return "Persistent";
  case FaceType::Delete:
    return "Delete";
  }
  return "<undef>";
}

peano4::grid::FaceType peano4::grid::getFaceType(const VertexType vertexTypes[TWO_POWER_D], int faceNumber) {
  bool allVerticesAreHanging         = true;
  bool allVerticesAreDeleteOrHanging = true;
  bool allVerticesAreNewOrHanging    = true;

  const int normal = faceNumber % DIMENSIONS;
  const int side   = (faceNumber >= DIMENSIONS) ? 1 : 0;

  for (int i = 0; i < TWO_POWER_D; i++) {
    // Only keep indices on the requested face (skip the opposite half).
    if (((i >> normal) & 1) != side)
      continue;
    switch (vertexTypes[i]) {
    case VertexType::Hanging:
      break;
    case VertexType::New:
      allVerticesAreDeleteOrHanging = false;
      allVerticesAreHanging         = false;
      break;
    case VertexType::Persistent:
      allVerticesAreHanging         = false;
      allVerticesAreDeleteOrHanging = false;
      allVerticesAreNewOrHanging    = false;
      return FaceType::Persistent; // Can't be Hanging/New/Delete anymore.
    case VertexType::Delete:
      allVerticesAreHanging      = false;
      allVerticesAreNewOrHanging = false;
      break;
    }
  }

  FaceType result = FaceType::Persistent;
  if (allVerticesAreHanging) {
    result = FaceType::Hanging;
  } else if (allVerticesAreNewOrHanging) {
    result = FaceType::New;
  } else if (allVerticesAreDeleteOrHanging) {
    result = FaceType::Delete;
  }

  return result;
}
