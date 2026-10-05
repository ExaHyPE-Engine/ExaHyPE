// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticlePlotterTest.h"

#include "exahype2/particles/ParticleSet.h"
#include "exahype2/plotter/ParticlePlotter.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Format.h"

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

exahype2::plotter::tests::ParticlePlotterTest::ParticlePlotterTest() :
  tarch::tests::TestCase("exahype2::plotter::tests::ParticlePlotterTest") {}

void
exahype2::plotter::tests::ParticlePlotterTest::run() {
  testMethod(testEveryParticleBecomesOnePointWithItsNumberAndData);
}

void
exahype2::plotter::tests::ParticlePlotterTest::testEveryParticleBecomesOnePointWithItsNumberAndData() {
  const int rank = tarch::mpi::Rank::getInstance().getRank();
  peano4::tests::buildRegularGrid(Depth);
  const auto& partition = peano4::Spacetree::getInstance().getPartition();

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
  particles.insert(partition, candidates);

  const std::string outputDirectory = (tarch::tests::getWritableTestDirectory() / "ParticlePlotterTest").string() + "/";
  std::filesystem::create_directories(outputDirectory + "particle/");
  exahype2::plotter::ParticlePlotter plotter("ParticlePlotterTest");
  plotter.plot(particles, 0.25, tarch::plotter::Format::VTU, outputDirectory);

  const int                   plottedParticles = peano4::tests::getGlobalSumOf(particles.getSize());
  const std::filesystem::path vtuPath          = std::filesystem::path(outputDirectory) / "particle"
    / ("ParticlePlotterTest-0_250000-rank-" + std::to_string(rank) + ".vtu");
  const std::string content = getFileContent(vtuPath);
  deleteOutputDirectory(outputDirectory);

  validateEquals(plottedParticles, NumberOfParticles);
  validateWithParams1(
    content.find("NumberOfPoints=\"" + std::to_string(particles.getSize()) + "\"") != std::string::npos,
    vtuPath.string()
  );
  validateWithParams1(
    content.find("NumberOfCells=\"" + std::to_string(particles.getSize()) + "\"") != std::string::npos,
    vtuPath.string()
  );
  validate(content.find("Name=\"Number\"") != std::string::npos);
  validate(content.find("Name=\"Data\"") != std::string::npos);
  validate(content.find("Name=\"RefinementLevel\"") != std::string::npos);
  validate(content.find("Name=\"Subdomain\"") != std::string::npos);
}
