// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/PatchPlotter.h"
#include "exahype2/plotter/Snapshot.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/plotter/Format.h"

namespace peano4 {
  class Partition;
} // namespace peano4

namespace exahype2::plotter {
  class FacePlotter;
} // namespace exahype2::plotter

/**
 * Writes the two halves of every selected face of a rank into one piece of a snapshot file. Every
 * field becomes one point field of the lower and one of the upper cell.
 */
class exahype2::plotter::FacePlotter final {
public:
  FacePlotter(std::string filename, std::vector<DataField> fields);
  FacePlotter(const FacePlotter&)            = delete;
  FacePlotter(FacePlotter&&)                 = delete;
  FacePlotter& operator=(const FacePlotter&) = delete;
  FacePlotter& operator=(FacePlotter&&)      = delete;
  ~FacePlotter()                             = default;

  template <int NodesPerAxis, int ValuesPerNode, typename Precision, typename Predicate>
  INLINE_METHOD void plot(
    const peano4::Partition& partition,
    const Precision*         values,
    int                      valuesPerFace,
    const double*            quadratureNodes,
    double                   timeStamp,
    tarch::plotter::Format   format,
    const std::string&       outputDirectory,
    const Predicate&         plotsFace
  );

private:
  static constexpr const char* OutputSubdirectory = "face/";

  [[nodiscard]] static std::vector<int> getPlottedFaces(const peano4::Partition& partition);

  std::string            _filename;
  std::vector<DataField> _fields;
  Snapshot               _snapshot{};
};

#include "exahype2/plotter/FacePlotter.cpph"
