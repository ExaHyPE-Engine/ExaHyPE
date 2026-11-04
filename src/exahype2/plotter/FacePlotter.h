// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/PatchPlotter.h"
#include "tarch/CompilerSpecificSettings.h"
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
  class FacePlotter;
} // namespace exahype2::plotter

/**
 * Writes the two halves of every selected face of a rank into the snapshots of a plot series.
 * Every field becomes one point field of the lower and one of the upper cell.
 */
class exahype2::plotter::FacePlotter final {
public:
  FacePlotter(std::string filename, std::vector<DataField> fields);
  FacePlotter(const FacePlotter& other)            = delete;
  FacePlotter(FacePlotter&& other)                 = delete;
  FacePlotter& operator=(const FacePlotter& other) = delete;
  FacePlotter& operator=(FacePlotter&& other)      = delete;
  ~FacePlotter()                                   = default;

  template <int NodesPerAxis, int ValuesPerNode, typename Precision, typename Predicate>
  INLINE_METHOD void plot(
    const peano4::CellConnectivity&                         cellConnectivity,
    const tarch::accelerator::ChunkedMemoryPool<Precision>& values,
    const double*                                           quadratureNodes,
    double                                                  timeStamp,
    tarch::plotter::Format                                  format,
    const std::string&                                      outputDirectory,
    const Predicate&                                        plotsFace
  );

private:
  static constexpr const char* OutputSubdirectory = "face/";

  [[nodiscard]] static std::vector<int> getPlottedFaces(const peano4::CellConnectivity& cellConnectivity);

  std::string                                 _filename;
  std::vector<DataField>                      _fields;
  std::unique_ptr<tarch::plotter::FileWriter> _fileWriter{};
};

#include "exahype2/plotter/FacePlotter.cpph"
