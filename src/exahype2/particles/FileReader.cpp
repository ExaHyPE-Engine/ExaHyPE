// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileReader.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Info.h"

/**
 * Appends the coordinates in filename, multiplied by scale. Every non-empty line contains exactly
 * DIMENSIONS values.
 */
void
exahype2::particles::FileReader::readDatFile(const std::string& filename, const double scale) {
  assertion(not filename.empty());
  assertion(std::isfinite(scale), scale);
  std::fstream input(filename, std::ios::in);
  if (not input) {
    tarch::terminateWithError("The particle coordinate file cannot be opened: " + filename);
  }

  std::string line;
  int         lineNumber = 0;
  while (std::getline(input, line)) {
    lineNumber++;
    if (line.find_first_not_of(" \t\r") == std::string::npos) {
      continue;
    }
    std::stringstream                     currentLine(line);
    tarch::la::Vector<DIMENSIONS, double> newEntry{};
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      if (not(currentLine >> newEntry(dimension))) {
        tarch::terminateWithError("Particle coordinate line " + std::to_string(lineNumber) + " has too few values.");
      }
      newEntry(dimension) *= scale;
      assertion(std::isfinite(newEntry(dimension)), lineNumber, dimension, newEntry(dimension));
    }
    std::string trailingValue;
    if (currentLine >> trailingValue) {
      tarch::terminateWithError("Particle coordinate line " + std::to_string(lineNumber) + " has too many values.");
    }
    _data.push_back(newEntry);
  }
  logInfo("Read " << _data.size() << " particle positions from file " << filename);
}

bool
exahype2::particles::FileReader::empty() const {
  return _data.empty();
}

/**
 * Returns the coordinates inside the closed voxel of centre x and width h, and erases them from the
 * reader if remove is set.
 */
std::list<tarch::la::Vector<DIMENSIONS, double>>
exahype2::particles::FileReader::getParticlesWithinVoxel(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const bool                                   remove
) {
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    assertion(std::isfinite(x(dimension)), dimension, x);
    assertion(std::isfinite(h(dimension)), dimension, h);
    assertion(h(dimension) > 0.0, dimension, h);
  }
  std::list<tarch::la::Vector<DIMENSIONS, double>> result;

  auto particlePosition = _data.begin();
  while (particlePosition != _data.end()) {
    bool overlaps = true;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      overlaps =
        overlaps and tarch::la::smallerEquals(x(dimension) - h(dimension) / 2.0, (*particlePosition)(dimension));
      overlaps =
        overlaps and tarch::la::greaterEquals(x(dimension) + h(dimension) / 2.0, (*particlePosition)(dimension));
    }
    if (overlaps) {
      result.push_back(*particlePosition);
      if (remove) {
        particlePosition = _data.erase(particlePosition);
      } else {
        particlePosition++;
      }
    } else {
      particlePosition++;
    }
  }
  return result;
}
