// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::particles {
  class FileReader;
} // namespace exahype2::particles

/** Stores particle coordinates read from a whitespace-separated text file. */
class exahype2::particles::FileReader final {
public:
  void readDatFile(const std::string& filename, double scale);

  std::list<tarch::la::Vector<DIMENSIONS, double>> getParticlesWithinVoxel(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    bool                                         remove
  );

  bool empty() const;

private:
  std::list<tarch::la::Vector<DIMENSIONS, double>> _data{};
};
