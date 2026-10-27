// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/Format.h"

namespace peano4 {
  class CellConnectivity;
} // namespace peano4

namespace tarch::accelerator {
  template <typename T>
  class ChunkedMemoryPool;
} // namespace tarch::accelerator

namespace exahype2::plotter {
  struct DataField;
  class PatchPlotter;

  /**
   * Lists the corner positions of a quadrangle and a hexahedron in the corner order of VTK.
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
 * Writes the patches of the grid cells of a rank into the snapshots of one plot series. The function
 * plotVolumes() writes one plot cell per finite volume and every volume corner once. The function
 * plotNodes() writes nodal values at the plot points of every patch. The function plotPatch() writes
 * the solution that a solver evaluates at equidistant points from face to face of every element of
 * a patch.
 */
class exahype2::plotter::PatchPlotter final {
public:
  PatchPlotter(std::string filename, std::vector<DataField> fields);
  PatchPlotter(const PatchPlotter& other)            = delete;
  PatchPlotter(PatchPlotter&& other)                 = delete;
  PatchPlotter& operator=(const PatchPlotter& other) = delete;
  PatchPlotter& operator=(PatchPlotter&& other)      = delete;
  ~PatchPlotter()                                    = default;

  template <int VolumesPerAxis, int ValuesPerVolume, typename Precision>
  INLINE_METHOD void plotVolumes(
    const peano4::CellConnectivity&                         cellConnectivity,
    tarch::Span<const int>                                  cells,
    const tarch::accelerator::ChunkedMemoryPool<Precision>& values,
    double                                                  timeStamp,
    tarch::plotter::Format                                  format,
    const std::string&                                      outputDirectory,
    const tarch::accelerator::ChunkedMemoryPool<int>*       volumeStatuses = nullptr,
    const char*                                             statusName     = nullptr
  );

  template <int NodesPerAxis, int ValuesPerNode, typename Precision>
  INLINE_METHOD void plotNodes(
    const peano4::CellConnectivity&                         cellConnectivity,
    tarch::Span<const int>                                  cells,
    const tarch::accelerator::ChunkedMemoryPool<Precision>& values,
    const double*                                           nodePositions,
    double                                                  timeStamp,
    tarch::plotter::Format                                  format,
    const std::string&                                      outputDirectory
  );

  template <int NodesPerAxis, int ValuesPerNode, int SubcellsPerAxis, typename Precision>
  INLINE_METHOD void plotNodes(
    const peano4::CellConnectivity&                                               cellConnectivity,
    tarch::Span<const int>                                                        cells,
    const tarch::accelerator::ChunkedMemoryPool<Precision>&                       values,
    const double*                                                                 nodePositions,
    const tarch::accelerator::ChunkedMemoryPool<std::type_identity_t<Precision>>* subcellValues,
    const tarch::accelerator::ChunkedMemoryPool<int>*                             statuses,
    int                                                                           subcellStatus,
    const char*                                                                   statusName,
    double                                                                        timeStamp,
    tarch::plotter::Format                                                        format,
    const std::string&                                                            outputDirectory
  );

  template <
    int ElementsPerAxis,
    int PointsPerAxis,
    int ValuesPerPoint,
    typename Evaluate,
    typename Status = std::nullptr_t>
  INLINE_METHOD void plotPatch(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::Span<const int>          cells,
    const Evaluate&                 evaluate,
    double                          timeStamp,
    tarch::plotter::Format          format,
    const std::string&              outputDirectory,
    const Status&                   status         = nullptr,
    const char*                     statusName     = nullptr,
    const double*                   pointPositions = nullptr
  );

  template <int NodesPerAxis, int SubcellsPerAxis>
  [[nodiscard]] static INLINE_METHOD std::array<int, peano4::power(NodesPerAxis, DIMENSIONS)> containingSubcells(
    const double* nodePositions
  );

  template <int PointsPerAxis, int NodesPerAxis>
  [[nodiscard]] static INLINE_METHOD std::vector<double> getTensorProductMatrix(const double* matrix1D);

  template <int PointsPerAxis, int NodesPerAxis, int Values, typename Precision>
  static INLINE_METHOD void evaluateNodes(
    const std::vector<double>& tensorProductMatrix,
    const Precision*           nodalValues,
    double*                    pointValues
  );

private:
  static constexpr const char* OutputSubdirectory = "patch/";

  static constexpr std::size_t RefinementLevelField = 0;
  static constexpr std::size_t SubdomainField       = 1;
  static constexpr std::size_t CurveIndexField      = 2;

  struct PatchCorner final {
    bool onFace{false};
    int  index{0};
  };

  struct FaceCornerPoints final {
    std::vector<std::int32_t> pointOfFaceCorner{};
    std::vector<std::int32_t> firstPointOfPatch{};
  };

  [[nodiscard]] static std::vector<PatchCorner> classifyCorners(int cornersPerAxis);

  template <int VolumesPerAxis>
  [[nodiscard]] static INLINE_METHOD FaceCornerPoints numberFaceCorners(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::Span<const int>          cells,
    const std::vector<PatchCorner>& corners
  );

  [[nodiscard]] static std::uint64_t hashLatticeKey(const std::array<std::int64_t, DIMENSIONS>& key);

  [[nodiscard]] tarch::plotter::Mesh createMesh(
    int         numberOfPatches,
    int         cellsPerPatch,
    std::size_t numberOfPoints,
    bool        writesPointData,
    const char* statusName
  ) const;

  void write(
    const tarch::plotter::Mesh& mesh,
    double                      timeStamp,
    tarch::plotter::Format      format,
    const std::string&          outputDirectory
  );

  template <int PointsPerAxis, typename PointOfCorner>
  static INLINE_METHOD void connectPatch(
    const PointOfCorner&  pointOfCorner,
    std::size_t           firstCell,
    tarch::plotter::Mesh& mesh
  );

  template <typename Precision>
  INLINE_METHOD void copyState(
    const Precision*                          state,
    int                                       valuesPerState,
    std::size_t                               entity,
    std::vector<tarch::plotter::Mesh::Field>& fields
  ) const;

  std::string                                 _filename;
  std::vector<DataField>                      _fields;
  std::unique_ptr<tarch::plotter::FileWriter> _fileWriter{};
};

#include "exahype2/plotter/PatchPlotter.cpph"
