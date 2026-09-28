// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FacePlotterTest.h"

#include "exahype2/plotter/FacePlotter.h"
#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Format.h"

namespace {
  constexpr int Depth         = 1;
  constexpr int NodesPerAxis  = 3;
  constexpr int ValuesPerNode = 2;
  constexpr int NodesPerFace  = peano4::power(NodesPerAxis, DIMENSIONS - 1);
  constexpr int CellsPerFace  = peano4::power(NodesPerAxis - 1, DIMENSIONS - 1);
  constexpr int CellsPerAxis  = peano4::SubdivisionFactor;

  constexpr double QuadratureNodes[NodesPerAxis] = {0.0, 0.5, 1.0};

  std::string getFileContent(const std::filesystem::path& path) {
    const std::ifstream stream(path);
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    return buffer.str();
  }

  /**
   * Deletes the output directory on the global master once every rank has read its piece back.
   */
  void deleteOutputDirectory(const std::string& outputDirectory) {
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      std::filesystem::remove_all(outputDirectory);
    }
  }
} // namespace

exahype2::plotter::tests::FacePlotterTest::FacePlotterTest() :
  tarch::tests::TestCase("exahype2::plotter::tests::FacePlotterTest") {}

void
exahype2::plotter::tests::FacePlotterTest::run() {
  testMethod(testEverySelectedFaceBecomesItsNodesAndBothTraces);
  testMethod(testEveryFaceOfAnAdaptiveGridIsWrittenByOneRank);
}

/**
 * The faces of one plane across the ranks plot once each with one point per node, the cells
 * between the nodes, both halves of every field, the level and the rank.
 */
void
exahype2::plotter::tests::FacePlotterTest::testEverySelectedFaceBecomesItsNodesAndBothTraces() {
  const int rank = tarch::mpi::Rank::getInstance().getRank();
  peano4::tests::buildRegularGrid(Depth);
  const auto& partition = peano4::Spacetree::getInstance().getPartition();

  const int           valuesPerFace = 2 * NodesPerFace * ValuesPerNode;
  std::vector<double> values(static_cast<std::size_t>(partition.getNumberOfFaces()) * valuesPerFace, 0.0);
  for (int face = 0; face < partition.getNumberOfFaces(); face++) {
    for (int value = 0; value < valuesPerFace; value++) {
      values[static_cast<std::size_t>(face * valuesPerFace + value)] = 100.0 * face + value;
    }
  }

  const double plane        = 1.0 / CellsPerAxis;
  int          plottedFaces = 0;
  const auto   onPlane      = [&](
                                const tarch::la::Vector<DIMENSIONS, double>&                  faceCentre,
                                [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellWidth,
                                const int                                                     normal
                              ) {
    const bool selected = normal == 0 and std::abs(faceCentre(0) - plane) < 1.0e-12;
    if (selected) {
      plottedFaces++;
    }
    return selected;
  };

  const std::string outputDirectory = (tarch::tests::getWritableTestDirectory() / "FacePlotterTest").string() + "/";
  std::filesystem::create_directories(outputDirectory + "face/");
  exahype2::plotter::FacePlotter plotter("FacePlotterTest", {{"q", 1}, {"r", 1}});
  plotter.plot<NodesPerAxis, ValuesPerNode, double>(
    partition,
    values.data(),
    valuesPerFace,
    QuadratureNodes,
    0.25,
    tarch::plotter::Format::VTU,
    outputDirectory,
    onPlane
  );

  validateEquals(peano4::tests::getGlobalSumOf(plottedFaces), peano4::power(CellsPerAxis, DIMENSIONS - 1));
  const std::filesystem::path piece = std::filesystem::path(outputDirectory) / "face"
    / ("FacePlotterTest-0_250000-rank-" + std::to_string(rank) + ".vtu");
  validateWithParams1(std::filesystem::exists(piece), piece.string());
  const std::string content = getFileContent(piece);
  validateWithParams1(
    content.find("NumberOfCells=\"" + std::to_string(plottedFaces * CellsPerFace) + "\"") != std::string::npos,
    piece.string()
  );
  validateWithParams1(
    content.find("NumberOfPoints=\"" + std::to_string(plottedFaces * NodesPerFace) + "\"") != std::string::npos,
    piece.string()
  );
  validate(content.find("Name=\"qL\"") != std::string::npos);
  validate(content.find("Name=\"qR\"") != std::string::npos);
  validate(content.find("Name=\"rL\"") != std::string::npos);
  validate(content.find("Name=\"rR\"") != std::string::npos);
  validate(content.find("Name=\"RefinementLevel\"") != std::string::npos);
  validate(content.find("Name=\"Subdomain\"") != std::string::npos);
  deleteOutputDirectory(outputDirectory);
}

/**
 * Every interior, boundary and hanging face of a grid with level jumps next to the rank boundaries
 * is written by exactly one rank.
 */
void
exahype2::plotter::tests::FacePlotterTest::testEveryFaceOfAnAdaptiveGridIsWrittenByOneRank() {
  constexpr int AdaptiveDepth = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;
  peano4::tests::buildRegularGrid(AdaptiveDepth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(AdaptiveDepth)});
  const auto& partition = peano4::Spacetree::getInstance().getPartition();

  int heldFaces = 0;
  for (const auto dependence : {peano4::Dependence::Local, peano4::Dependence::Remote}) {
    for (const auto kind : {peano4::FaceKind::Interior, peano4::FaceKind::Boundary, peano4::FaceKind::Hanging}) {
      heldFaces += partition.getFaces(dependence, kind).size;
    }
  }
  const int exchangedFaces = partition.getFaces(peano4::Dependence::Remote, peano4::FaceKind::Interior).size;

  const int                 valuesPerFace = 2 * NodesPerFace * ValuesPerNode;
  const std::vector<double> values(static_cast<std::size_t>(partition.getNumberOfFaces()) * valuesPerFace, 0.0);
  int                       plottedFaces = 0;
  const auto                countFace    = [&plottedFaces](
                                             [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
                                             [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellWidth,
                                             [[maybe_unused]] const int                                    normal
                                           ) {
    plottedFaces++;
    return true;
  };

  const std::string outputDirectory =
    (tarch::tests::getWritableTestDirectory() / "FacePlotterTestAdaptive").string() + "/";
  std::filesystem::create_directories(outputDirectory + "face/");
  exahype2::plotter::FacePlotter plotter("FacePlotterTestAdaptive", {{"q", 1}, {"r", 1}});
  plotter.plot<NodesPerAxis, ValuesPerNode, double>(
    partition,
    values.data(),
    valuesPerFace,
    QuadratureNodes,
    0.5,
    tarch::plotter::Format::VTU,
    outputDirectory,
    countFace
  );

  const int globalExchangedFaces = peano4::tests::getGlobalSumOf(exchangedFaces);
  const int globalHeldFaces      = peano4::tests::getGlobalSumOf(heldFaces);
  const int globalPlottedFaces   = peano4::tests::getGlobalSumOf(plottedFaces);
  deleteOutputDirectory(outputDirectory);
  validateEquals(globalExchangedFaces % 2, 0);
  validateEquals(globalPlottedFaces, globalHeldFaces - globalExchangedFaces / 2);
}
