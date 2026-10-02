// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/Snapshot.h"
#include "peano4/Constants.h"
#include "tarch/Span.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/Format.h"

namespace peano4 {
  class Partition;
} // namespace peano4

namespace exahype2::plotter {
  struct DataField;
  class PatchPlotter;

  /**
   * Corner positions of a plotted quadrangle and hexahedron in the order the plot file expects.
   */
  constexpr std::array<std::array<int, 3>, 8> CellCorners{
    {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}
  };
} // namespace exahype2::plotter

struct exahype2::plotter::DataField final {
  std::string name{};
  int         components{1};
};

/**
 * Writes the data of the grid cells of a rank, one patch per grid cell, as one piece of a snapshot
 * file. plotVolumes() writes one cell of the piece per finite volume with cell data, and the piece
 * holds every volume corner once. plotNodes() writes one point per node at its quadrature position
 * with point data and connects the neighbouring nodes of a patch to cells of the piece, so the
 * points of a patch belong to that patch alone.
 */
class exahype2::plotter::PatchPlotter final {
public:
  PatchPlotter(std::string filename, std::vector<DataField> fields);
  PatchPlotter(const PatchPlotter&)            = delete;
  PatchPlotter(PatchPlotter&&)                 = delete;
  PatchPlotter& operator=(const PatchPlotter&) = delete;
  PatchPlotter& operator=(PatchPlotter&&)      = delete;
  ~PatchPlotter()                              = default;

  template <int VolumesPerAxis, int ValuesPerVolume, typename Precision>
  void plotVolumes(
    const peano4::Partition& partition,
    tarch::Span<const int>   cells,
    const Precision*         values,
    int                      valuesPerSlot,
    double                   timeStamp,
    tarch::plotter::Format   format,
    const std::string&       outputDirectory
  );

  template <int NodesPerAxis, int ValuesPerNode, typename Precision>
  void plotNodes(
    const peano4::Partition& partition,
    tarch::Span<const int>   cells,
    const Precision*         values,
    int                      valuesPerSlot,
    const double*            nodePositions,
    double                   timeStamp,
    tarch::plotter::Format   format,
    const std::string&       outputDirectory
  );

  template <int NodesPerAxis, int ValuesPerNode, int SubcellsPerAxis, typename Precision>
  void plotNodes(
    const peano4::Partition&               partition,
    tarch::Span<const int>                 cells,
    const Precision*                       values,
    int                                    valuesPerSlot,
    const double*                          nodePositions,
    const std::type_identity_t<Precision>* subcellValues,
    int                                    subcellValuesPerSlot,
    const int*                             statuses,
    int                                    statusEntriesPerSlot,
    int                                    subcellStatus,
    double                                 timeStamp,
    tarch::plotter::Format                 format,
    const std::string&                     outputDirectory
  );

  template <int NodesPerAxis, int SubcellsPerAxis>
  [[nodiscard]] static std::array<int, peano4::power(NodesPerAxis, DIMENSIONS)> containingSubcells(
    const double* nodePositions
  );

private:
  static constexpr const char* OutputSubdirectory = "patch/";

  /**
   * The positions of the grid cell fields among the cell fields behind the cell data fields.
   */
  static constexpr std::size_t RefinementLevelField = 0;
  static constexpr std::size_t CurveIndexField      = 2;

  /**
   * A volume corner of a patch and its index among the corners on the patch faces or among the interior corners.
   */
  struct PatchCorner final {
    bool onFace{false};
    int  index{0};
  };

  [[nodiscard]] static std::vector<PatchCorner> classifyCorners(int cornersPerAxis);

  template <int VolumesPerAxis>
  [[nodiscard]] static std::vector<std::int32_t> appendFaceCorners(
    const peano4::Partition&                      partition,
    tarch::Span<const int>                        cells,
    const std::vector<PatchCorner>&               corners,
    tarch::plotter::FileWriter::SubdomainBuffers& piece
  );

  [[nodiscard]] int openPiece(
    int                    numberOfPatches,
    int                    cellsPerPatch,
    std::size_t            numberOfPoints,
    bool                   writesPointData,
    bool                   writesStatus,
    double                 timeStamp,
    tarch::plotter::Format format,
    const std::string&     outputDirectory
  );

  template <int PointsPerAxis, typename PointOfCorner>
  static void connectPatch(
    const PointOfCorner&                          pointOfCorner,
    std::size_t                                   firstCell,
    tarch::plotter::FileWriter::SubdomainBuffers& piece
  );

  template <typename Precision>
  void copyState(
    const Precision*                                    state,
    int                                                 valuesPerState,
    std::size_t                                         entity,
    std::vector<tarch::plotter::FileWriter::DataField>& fields
  ) const;

  std::string            _filename;
  std::vector<DataField> _fields;
  Snapshot               _snapshot{};
};

#include "exahype2/plotter/PatchPlotter.cpph"
