// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/Piece.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorVectorOperations.h"

namespace exahype2::plotter {
  class CellPatchPlotter;
  class NodalPatchPlotter;
} // namespace exahype2::plotter

/**
 * Writes one plot point per node of a patch and connects the neighbouring nodes of a patch to
 * plot cells. Two patches keep two traces of their common boundary, since the plot points of a
 * patch are its own.
 */
class exahype2::plotter::NodalPatchPlotter final {
public:
  NodalPatchPlotter()                                    = default;
  NodalPatchPlotter(const NodalPatchPlotter&)            = delete;
  NodalPatchPlotter(NodalPatchPlotter&&)                 = delete;
  NodalPatchPlotter& operator=(const NodalPatchPlotter&) = delete;
  NodalPatchPlotter& operator=(NodalPatchPlotter&&)      = delete;
  ~NodalPatchPlotter()                                   = default;

  template <typename CreateCellDataPlotters>
  INLINE_METHOD void open(
    Snapshot&                     snapshot,
    int                           treeId,
    const std::vector<DataField>& fields,
    CreateCellDataPlotters        createCellDataPlotters
  );

  [[nodiscard]] std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> createCellDataPlotter(
    const std::string& name,
    int                components
  );

  template <
    int PointsPerAxis,
    int UnknownsPerPoint,
    int AlternativePointsPerAxis,
    typename Precision,
    typename CellDataPlots>
  INLINE_METHOD void plotCell(
    const peano4::dataview::CellMarker&          marker,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const std::array<double, PointsPerAxis>&     mappingAlongAxis,
    const Precision*                             patch,
    bool                                         useAlternativePatch,
    const std::type_identity_t<Precision>*       alternativePatch,
    CellDataPlots                                cellDataPlots
  );

private:
  struct FieldPlotter final {
    std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> plotter{};
    int                                                            components{1};
    int                                                            offset{0};
  };

  Piece                                                        _piece{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _curvePositionPlotter{};
  std::vector<FieldPlotter>                                    _fields{};
};

/**
 * Writes one plot cell per finite volume of a patch and one value per volume and field.
 */
class exahype2::plotter::CellPatchPlotter final {
public:
  CellPatchPlotter()                                   = default;
  CellPatchPlotter(const CellPatchPlotter&)            = delete;
  CellPatchPlotter(CellPatchPlotter&&)                 = delete;
  CellPatchPlotter& operator=(const CellPatchPlotter&) = delete;
  CellPatchPlotter& operator=(CellPatchPlotter&&)      = delete;
  ~CellPatchPlotter()                                  = default;

  template <typename CreateCellDataPlotters>
  INLINE_METHOD void open(
    Snapshot&                     snapshot,
    int                           treeId,
    const std::vector<DataField>& fields,
    CreateCellDataPlotters        createCellDataPlotters
  );

  [[nodiscard]] std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> createCellDataPlotter(
    const std::string& name,
    int                components
  );

  template <int VolumesPerAxis, int UnknownsPerVolume, typename Precision, typename CellDataPlots>
  INLINE_METHOD void plotCell(
    const peano4::dataview::CellMarker&          marker,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const Precision*                             patch,
    CellDataPlots                                cellDataPlots
  );

private:
  struct FieldPlotter final {
    std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> plotter{};
    int                                                          components{1};
    int                                                          offset{0};
  };

  Piece                                                        _piece{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _curvePositionPlotter{};
  std::vector<FieldPlotter>                                    _fields{};
};

#include "exahype2/plotter/PatchPlotter.cpph"
