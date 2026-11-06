// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/plotter/ParticlePlotter.h"

#include "exahype2/particles/ParticleSet.h"
#include "peano4/Spacetree.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Format.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  constexpr int Depth             = 2;
  constexpr int NumberOfData      = 3;
  constexpr int NumberOfParticles = 4;

  std::string getFileContent(const std::filesystem::path& path) {
    const std::ifstream stream(path);
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    return buffer.str();
  }

  /**
   * Deletes the output directory on the global master once every rank has read its file back.
   */
  void deleteOutputDirectory(const std::string& outputDirectory) {
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      std::filesystem::remove_all(outputDirectory);
    }
  }
} // namespace

UNIT_TEST("The ranks together write every particle once as one point with the fields Number and Data") {
  GIVEN("Four particles with three data entries in a regular grid over every rank and a particle plotter") {
    const int rank = tarch::mpi::Rank::getInstance().getRank();
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    std::vector<double> candidates;
    for (int particle = 0; particle < NumberOfParticles; particle++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        candidates.push_back(0.1 + 0.2 * particle + 0.01 * axis);
      }
      for (int entry = 0; entry < NumberOfData; entry++) {
        candidates.push_back(10.0 * particle + entry);
      }
    }
    exahype2::particles::ParticleSet<NumberOfData> particles;
    particles.insert(cellConnectivity, candidates);

    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "ParticlePlotterTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "particle/");
    exahype2::plotter::ParticlePlotter plotter("ParticlePlotterTest");
    const std::filesystem::path        vtuPath = std::filesystem::path(outputDirectory) / "particle"
      / ("ParticlePlotterTest-0_250000-rank-" + std::to_string(rank) + ".vtu");

    WHEN("The plotter writes the particles at the time one quarter and every rank reads its file back") {
      plotter.plot(particles, 0.25, tarch::plotter::Format::VTU, outputDirectory);
      const int         plottedParticles = peano4::unittests::getGlobalSumOf(particles.getSize());
      const std::string content          = getFileContent(vtuPath);
      deleteOutputDirectory(outputDirectory);

      THEN("The ranks own every particle and the file contains a point and a cell per particle and every field") {
        VALIDATE_EQUALS(plottedParticles, NumberOfParticles);
        VALIDATE(
          content.find("NumberOfPoints=\"" + std::to_string(particles.getSize()) + "\"") != std::string::npos,
          vtuPath.string()
        );
        VALIDATE(
          content.find("NumberOfCells=\"" + std::to_string(particles.getSize()) + "\"") != std::string::npos,
          vtuPath.string()
        );
        VALIDATE(content.find("Name=\"Number\"") != std::string::npos);
        VALIDATE(content.find("Name=\"Data\"") != std::string::npos);
        VALIDATE(content.find("Name=\"RefinementLevel\"") != std::string::npos);
        VALIDATE(content.find("Name=\"Subdomain\"") != std::string::npos);
      }
    }
  }
}
