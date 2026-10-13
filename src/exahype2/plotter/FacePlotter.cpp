// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FacePlotter.h"

exahype2::plotter::FacePlotter::FacePlotter(std::string filename, std::vector<DataField> fields) :
  _filename(std::move(filename)),
  _fields(std::move(fields)) {
  assertion(not _filename.empty());
}

/**
 * Returns the sorted faces this rank writes. The rank of the lower cell writes a face, and the rank
 * of the upper cell writes a face whose lower cell is on no other rank. Only a remote interior face
 * has its lower cell on another rank.
 */
std::vector<int>
exahype2::plotter::FacePlotter::getPlottedFaces(const peano4::CellConnectivity& cellConnectivity) {
  std::vector<int> faces;
  for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
    for (const auto kind : {peano4::FaceKind::Interior, peano4::FaceKind::Boundary, peano4::FaceKind::Hanging}) {
      for (const int face : cellConnectivity.getFaces(adjacency, kind)) {
        const bool lowerCellLocal  = cellConnectivity.getFaceCell(face, 0) >= 0;
        const bool upperCellLocal  = cellConnectivity.getFaceCell(face, 1) >= 0;
        const bool lowerCellRemote = kind == peano4::FaceKind::Interior and adjacency == peano4::Adjacency::Remote;
        if (lowerCellLocal or (upperCellLocal and not lowerCellRemote)) {
          faces.push_back(face);
        }
      }
    }
  }
  std::sort(faces.begin(), faces.end());
  return faces;
}
