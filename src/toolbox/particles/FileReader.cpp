// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileReader.h"

#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Info.h"

void toolbox::particles::FileReader::readDatFile(const std::string& filename, const double scale) {
  std::fstream fin;

  fin.open(filename, std::ios::in);
  std::string line;

  if (not fin) {
    logError("Was not able to open file " << filename);
    std::abort();
  }

  while (std::getline(fin, line)) {
    std::stringstream                     currentLine(line);
    tarch::la::Vector<DIMENSIONS, double> newEntry;

    int         entry = 0;
    std::string token;
    while (std::getline(currentLine, token, ' ') and entry < DIMENSIONS) {
      if (token != "") {
        newEntry(entry) = scale * std::atof(token.c_str());
        entry++;
      }
    }
    _data.push_back(newEntry);
  }

  fin.close();

  logInfo("Read " << _data.size() << " particle positions from file " << filename);
}

void toolbox::particles::FileReader::clear() { _data.clear(); }

bool toolbox::particles::FileReader::empty() const { return _data.empty(); }

bool toolbox::particles::FileReader::getNumberOfCoordinates() const { return _data.size(); }

std::list<tarch::la::Vector<DIMENSIONS, double>> toolbox::particles::FileReader::getParticlesWithinVoxel(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  bool                                         remove
) {
  std::list<tarch::la::Vector<DIMENSIONS, double>> result;

  auto particle = _data.begin();
  while (particle != _data.end()) {
    bool overlaps = true;

    for (int d = 0; d < DIMENSIONS; d++) {
      overlaps &= tarch::la::smallerEquals(x(d) - h(d) / 2.0, (*particle)(d));
      overlaps &= tarch::la::greaterEquals(x(d) + h(d) / 2.0, (*particle)(d));
    }

    if (overlaps) {
      result.push_back(*particle);
      if (remove) {
        particle = _data.erase(particle);
      } else {
        particle++;
      }
    } else {
      particle++;
    }
  }

  return result;
}
