// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/Snapshot.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/plotter/Format.h"

namespace exahype2::plotter {
  class ParticlePlotter;
} // namespace exahype2::plotter

/**
 * Writes the particles of a rank into one piece of a snapshot file, one plot point per particle.
 */
class exahype2::plotter::ParticlePlotter final {
public:
  INLINE_METHOD explicit ParticlePlotter(std::string filename);
  ParticlePlotter(const ParticlePlotter&)            = delete;
  ParticlePlotter(ParticlePlotter&&)                 = delete;
  ParticlePlotter& operator=(const ParticlePlotter&) = delete;
  ParticlePlotter& operator=(ParticlePlotter&&)      = delete;
  ~ParticlePlotter()                                 = default;

  template <typename Particles>
  INLINE_METHOD void plot(
    Particles&             particles,
    double                 timeStamp,
    tarch::plotter::Format format,
    const std::string&     outputDirectory
  );

private:
  static constexpr const char* OutputSubdirectory = "particle/";

  std::string _filename;
  Snapshot    _snapshot{};
};

#include "exahype2/plotter/ParticlePlotter.cpph"
