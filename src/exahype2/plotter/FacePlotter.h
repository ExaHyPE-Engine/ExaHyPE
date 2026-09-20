// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/Piece.h"
#include "peano4/Constants.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorScalarOperations.h"
#include "tarch/la/VectorVectorOperations.h"

namespace exahype2::plotter {
  class FacePlotter;
} // namespace exahype2::plotter

/**
 * Writes the two traces of a face. A face holds the values of the cell on its left and of the
 * cell on its right.
 */
class exahype2::plotter::FacePlotter final {
public:
  FacePlotter()                              = default;
  FacePlotter(const FacePlotter&)            = delete;
  FacePlotter(FacePlotter&&)                 = delete;
  FacePlotter& operator=(const FacePlotter&) = delete;
  FacePlotter& operator=(FacePlotter&&)      = delete;
  ~FacePlotter()                             = default;

  void open(Snapshot& snapshot, int treeId, const std::vector<DataField>& fields);

  template <int PointsPerAxis, int UnknownsPerPoint, typename Precision>
  INLINE_METHOD void plotFace(
    const peano4::dataview::FaceMarker&      marker,
    const std::array<double, PointsPerAxis>& mappingAlongAxis,
    const Precision*                         face
  );

private:
  struct FieldPlotter final {
    std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> leftPlotter{};
    std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> rightPlotter{};
    int                                                            components{1};
    int                                                            offset{0};
  };

  Piece                     _piece{};
  std::vector<FieldPlotter> _fields{};
};

#include "exahype2/plotter/FacePlotter.cpph"
