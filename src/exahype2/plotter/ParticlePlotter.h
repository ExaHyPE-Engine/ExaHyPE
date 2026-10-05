// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/Format.h"

namespace exahype2::plotter {
  class ParticlePlotter;
} // namespace exahype2::plotter

/**
 * Writes the particles of a rank into the snapshots of a plot series, one plot point per particle.
 */
class exahype2::plotter::ParticlePlotter final {
public:
  INLINE_METHOD explicit ParticlePlotter(std::string filename);
  ParticlePlotter(const ParticlePlotter& other)            = delete;
  ParticlePlotter(ParticlePlotter&& other)                 = delete;
  ParticlePlotter& operator=(const ParticlePlotter& other) = delete;
  ParticlePlotter& operator=(ParticlePlotter&& other)      = delete;
  ~ParticlePlotter()                                       = default;

  template <typename Particles>
  INLINE_METHOD void plot(
    Particles&             particles,
    double                 timeStamp,
    tarch::plotter::Format format,
    const std::string&     outputDirectory
  );

private:
  static constexpr const char* OutputSubdirectory = "particle/";

  std::string                                 _filename;
  std::unique_ptr<tarch::plotter::FileWriter> _fileWriter{};
};

#include "exahype2/plotter/ParticlePlotter.cpph"
