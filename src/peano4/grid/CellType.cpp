// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellType.h"

std::ostream& operator<<(std::ostream& out, const peano4::grid::CellType& type) {
  return (out << peano4::grid::toString(type));
}

std::string peano4::grid::toString(CellType type) {
  switch (type) {
  case CellType::New:
    return "New";
  case CellType::Persistent:
    return "Persistent";
  case CellType::Delete:
    return "Delete";
  }
  return "<undef>";
}

peano4::grid::CellType peano4::grid::getCellType(const VertexType vertexTypes[TWO_POWER_D]) {
  bool allVerticesAreDelete = true;
  bool allVerticesAreNew    = true;

  for (int i = 0; i < TWO_POWER_D; i++) {
    switch (vertexTypes[i]) {
    case VertexType::Hanging:
      break;
    case VertexType::Persistent:
      allVerticesAreDelete = false;
      allVerticesAreNew    = false;
      return CellType::Persistent; // Cannot be all-new or all-delete anymore
    case VertexType::New:
      allVerticesAreDelete = false;
      break;
    case VertexType::Delete:
      allVerticesAreNew = false;
      break;
    }
  }

  assertion(not(allVerticesAreDelete and allVerticesAreNew));

  if (allVerticesAreDelete) {
    return CellType::Delete;
  } else if (allVerticesAreNew) {
    return CellType::New;
  } else {
    return CellType::Persistent;
  }
}
