// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/Piece.h"
#include "peano4/dataview/VertexMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/la/Vector.h"

namespace exahype2::plotter {
  class ParticlePlotter;

  INLINE_METHOD bool isAtUpperPeriodicBoundary(
    const peano4::dataview::VertexMarker&        marker,
    const std::bitset<DIMENSIONS>&               periodicBoundaries,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth
  );
} // namespace exahype2::plotter

/**
 * Writes one plot point per particle of a vertex. Each attribute of a particle becomes one cell
 * field, and the caller writes its values through the plotter the attribute's index names.
 */
class exahype2::plotter::ParticlePlotter final {
public:
  ParticlePlotter()                                  = default;
  ParticlePlotter(const ParticlePlotter&)            = delete;
  ParticlePlotter(ParticlePlotter&&)                 = delete;
  ParticlePlotter& operator=(const ParticlePlotter&) = delete;
  ParticlePlotter& operator=(ParticlePlotter&&)      = delete;
  ~ParticlePlotter()                                 = default;

  void open(Snapshot& snapshot, int treeId, const std::vector<DataField>& attributes);

  [[nodiscard]] INLINE_METHOD tarch::plotter::FileWriter::CellDataPlotter& getAttributePlotter(int index);

  template <typename ParticleSet, typename AttributePlots>
  INLINE_METHOD void plotParticles(
    const peano4::dataview::VertexMarker&        marker,
    ParticleSet&                                 particles,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
    const std::bitset<DIMENSIONS>&               periodicBoundaries,
    AttributePlots                               attributePlots
  );

private:
  Piece                                                                     _piece{};
  std::vector<std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter>> _attributePlotters{};
};

#include "exahype2/plotter/ParticlePlotter.cpph"
