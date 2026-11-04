// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/plotter/FacePlotter.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Spacetree.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Format.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  constexpr int Depth         = 1;
  constexpr int NodesPerAxis  = 3;
  constexpr int ValuesPerNode = 2;
  constexpr int NodesPerFace  = peano4::power(NodesPerAxis, DIMENSIONS - 1);
  constexpr int ValuesPerFace = 2 * NodesPerFace * ValuesPerNode;
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
   * Deletes the output directory on the global master once every rank has read its file back.
   */
  void deleteOutputDirectory(const std::string& outputDirectory) {
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      std::filesystem::remove_all(outputDirectory);
    }
  }
} // namespace

UNIT_TEST("The faces of a plane plot once each with their nodes, both halves of every field, the level and the rank") {
  GIVEN("A regular grid over every rank, values on both sides of every face and a plotter that selects one plane") {
    const int rank = tarch::mpi::Rank::getInstance().getRank();
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    tarch::accelerator::ChunkedMemoryPool<double> values(ValuesPerFace, tarch::MemoryLocation::Heap);
    values.reserve(cellConnectivity.getNumberOfFaces());
    for (int face = 0; face < cellConnectivity.getNumberOfFaces(); face++) {
      double* const halves = values.getChunk(face);
      for (int value = 0; value < ValuesPerFace; value++) {
        halves[value] = 100.0 * face + value;
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

    const std::string outputDirectory = (tarch::testing::getWritableTestDirectory() / "FacePlotterTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "face/");
    exahype2::plotter::FacePlotter plotter("FacePlotterTest", {{"q", 1}, {"r", 1}});
    const std::filesystem::path    vtuPath = std::filesystem::path(outputDirectory) / "face"
      / ("FacePlotterTest-0_250000-rank-" + std::to_string(rank) + ".vtu");

    WHEN("The plotter writes the faces of the plane at the time one quarter and every rank reads its file back") {
      plotter.plot<NodesPerAxis, ValuesPerNode, double>(
        cellConnectivity,
        values,
        QuadratureNodes,
        0.25,
        tarch::plotter::Format::VTU,
        outputDirectory,
        onPlane
      );
      const int         globalPlottedFaces = peano4::unittests::getGlobalSumOf(plottedFaces);
      const bool        fileExists         = std::filesystem::exists(vtuPath);
      const std::string content            = getFileContent(vtuPath);
      deleteOutputDirectory(outputDirectory);

      THEN("The ranks plot each face of the plane once and the file contains its cells, its points and every field") {
        VALIDATE_EQUALS(globalPlottedFaces, peano4::power(CellsPerAxis, DIMENSIONS - 1));
        VALIDATE(fileExists, vtuPath.string());
        VALIDATE(
          content.find("NumberOfCells=\"" + std::to_string(plottedFaces * CellsPerFace) + "\"") != std::string::npos,
          vtuPath.string()
        );
        VALIDATE(
          content.find("NumberOfPoints=\"" + std::to_string(plottedFaces * NodesPerFace) + "\"") != std::string::npos,
          vtuPath.string()
        );
        VALIDATE(content.find("Name=\"qL\"") != std::string::npos);
        VALIDATE(content.find("Name=\"qR\"") != std::string::npos);
        VALIDATE(content.find("Name=\"rL\"") != std::string::npos);
        VALIDATE(content.find("Name=\"rR\"") != std::string::npos);
        VALIDATE(content.find("Name=\"RefinementLevel\"") != std::string::npos);
        VALIDATE(content.find("Name=\"Subdomain\"") != std::string::npos);
      }
    }
  }
}

UNIT_TEST("Exactly one rank writes every interior, boundary and hanging face of a grid with level jumps") {
  GIVEN("A grid over every rank refined around its centre, the faces the ranks own and a plotter of every face") {
    constexpr int AdaptiveDepth = peano4::unittests::DepthWithCoarseCellsBesideTheCentreBlock;
    peano4::unittests::buildRegularGrid(AdaptiveDepth);
    peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(AdaptiveDepth)});
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    int storedFaces = 0;
    for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
      for (const auto kind : {peano4::FaceKind::Interior, peano4::FaceKind::Boundary, peano4::FaceKind::Hanging}) {
        storedFaces += cellConnectivity.getFaces(adjacency, kind).size;
      }
    }
    const int exchangedFaces = cellConnectivity.getFaces(peano4::Adjacency::Remote, peano4::FaceKind::Interior).size;
    const int globalExchangedFaces = peano4::unittests::getGlobalSumOf(exchangedFaces);
    const int globalStoredFaces    = peano4::unittests::getGlobalSumOf(storedFaces);

    tarch::accelerator::ChunkedMemoryPool<double> values(ValuesPerFace, tarch::MemoryLocation::Heap);
    values.reserve(cellConnectivity.getNumberOfFaces());
    for (int face = 0; face < values.getCapacity(); face++) {
      std::fill_n(values.getChunk(face), ValuesPerFace, 0.0);
    }
    int        plottedFaces = 0;
    const auto countFace    = [&plottedFaces](
                                [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
                                [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellWidth,
                                [[maybe_unused]] const int                                    normal
                              ) {
      plottedFaces++;
      return true;
    };

    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "FacePlotterTestAdaptive").string() + "/";
    std::filesystem::create_directories(outputDirectory + "face/");
    exahype2::plotter::FacePlotter plotter("FacePlotterTestAdaptive", {{"q", 1}, {"r", 1}});

    WHEN("The plotter writes every face at the time one half") {
      plotter.plot<NodesPerAxis, ValuesPerNode, double>(
        cellConnectivity,
        values,
        QuadratureNodes,
        0.5,
        tarch::plotter::Format::VTU,
        outputDirectory,
        countFace
      );
      const int globalPlottedFaces = peano4::unittests::getGlobalSumOf(plottedFaces);
      deleteOutputDirectory(outputDirectory);

      THEN("The ranks write every face they own once and a face on a rank boundary on one of its two ranks") {
        VALIDATE_EQUALS(globalExchangedFaces % 2, 0);
        VALIDATE_EQUALS(globalPlottedFaces, globalStoredFaces - globalExchangedFaces / 2);
      }
    }
  }
}
