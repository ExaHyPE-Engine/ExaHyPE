// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FacePlotter.h"

exahype2::plotter::FacePlotter::FacePlotter(std::string filename, std::vector<DataField> fields) :
  _filename(std::move(filename)),
  _fields(std::move(fields)) {
  assertion(not _filename.empty());
}

/**
 * Returns the faces this rank writes, in the order of their indices: The rank of the lower cell
 * writes a face, the rank of the upper cell writes a boundary face and a local face without a
 * lower cell, and a coarse or an intermediate face of a level jump is never written.
 */
std::vector<int>
exahype2::plotter::FacePlotter::getPlottedFaces(const peano4::Partition& partition) {
  std::vector<int> faces;
  for (const auto dependence : {peano4::Dependence::Local, peano4::Dependence::Remote}) {
    for (const auto kind : {peano4::FaceKind::Interior, peano4::FaceKind::Boundary, peano4::FaceKind::Hanging}) {
      for (const int face : partition.getFaces(dependence, kind)) {
        const bool lowerCellLocal = partition.getFaceCell(face, 0) >= 0;
        const bool upperCellLocal = partition.getFaceCell(face, 1) >= 0;
        const bool upperCellWrites =
          upperCellLocal and (kind == peano4::FaceKind::Boundary or dependence == peano4::Dependence::Local);
        if (lowerCellLocal or upperCellWrites) {
          faces.push_back(face);
        }
      }
    }
  }
  std::sort(faces.begin(), faces.end());
  return faces;
}
