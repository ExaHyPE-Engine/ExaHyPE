// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/plotter/PatchPlotter.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Spacetree.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Format.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

#ifdef PEANO_WITH_HDF5
#include <hdf5.h>
#endif

namespace {
  constexpr int Depth           = 1;
  constexpr int VolumesPerAxis  = 2;
  constexpr int ValuesPerVolume = 3;
  constexpr int VolumesPerPatch = peano4::power(VolumesPerAxis, DIMENSIONS);

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

  /**
   * Returns a pool in host memory with one chunk of chunkSize entries per slot of cellConnectivity
   * and value in every entry.
   */
  template <typename T>
  tarch::accelerator::ChunkedMemoryPool<T> getFilledPool(
    const peano4::CellConnectivity& cellConnectivity,
    const int                       chunkSize,
    const T                         value
  ) {
    tarch::accelerator::ChunkedMemoryPool<T> pool(chunkSize, tarch::MemoryLocation::Heap);
    pool.reserve(cellConnectivity.getNumberOfSlots());
    for (int slot = 0; slot < pool.getCapacity(); slot++) {
      std::fill_n(pool.getChunk(slot), chunkSize, value);
    }
    return pool;
  }

#ifdef PEANO_WITH_HDF5
  /**
   * Returns every value of the point field name in the VTKHDF file path.
   */
  std::vector<float> readPointField(const std::string& path, const std::string& name) {
    const hid_t        file      = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    const hid_t        dataset   = H5Dopen(file, ("/VTKHDF/PointData/" + name).c_str(), H5P_DEFAULT);
    const hid_t        dataSpace = H5Dget_space(dataset);
    std::vector<float> values(static_cast<std::size_t>(H5Sget_simple_extent_npoints(dataSpace)));
    H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    H5Fclose(file);
    return values;
  }
#endif
} // namespace

UNIT_TEST("A volume snapshot writes a cell per volume, every field, the level and the rank and the series lists it") {
  GIVEN("A regular grid over every rank, the values of every volume and a patch plotter") {
    const int  rank           = tarch::mpi::Rank::getInstance().getRank();
    const int  numberOfRanks  = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const bool isGlobalMaster = tarch::mpi::Rank::getInstance().isGlobalMaster();
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto  cells            = cellConnectivity.getCells();
    const int   numberOfCells    = cells.size * VolumesPerPatch;

    constexpr int VolumesPerDomainAxis     = peano4::power(peano4::SubdivisionFactor, Depth) * VolumesPerAxis;
    constexpr int VolumeCornersOfTheDomain = peano4::power(VolumesPerDomainAxis + 1, DIMENSIONS);

    auto values = getFilledPool(cellConnectivity, VolumesPerPatch * ValuesPerVolume, 0.0);
    for (const int cell : cells) {
      double* const volumes = values.getChunk(cell);
      for (int volume = 0; volume < VolumesPerPatch; volume++) {
        for (int value = 0; value < ValuesPerVolume; value++) {
          volumes[volume * ValuesPerVolume + value] = 100.0 * cell + 10.0 * volume + value;
        }
      }
    }

    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "PatchPlotterTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "patch/");
    exahype2::plotter::PatchPlotter plotter("PatchPlotterTest", {{"scalar", 1}, {"pair", 2}});
    const std::filesystem::path     vtuPath = std::filesystem::path(outputDirectory) / "patch"
      / ("PatchPlotterTest-0_250000-rank-" + std::to_string(rank) + ".vtu");
    const std::filesystem::path series = std::filesystem::path(outputDirectory) / "patch" / "PatchPlotterTest.pvd";

    WHEN("The plotter writes the volumes at the time one quarter and the ranks read the files back") {
      plotter.plotVolumes<VolumesPerAxis, ValuesPerVolume, double>(
        cellConnectivity,
        cells,
        values,
        0.25,
        tarch::plotter::Format::VTU,
        outputDirectory
      );
      const bool        fileExists = std::filesystem::exists(vtuPath);
      const std::string content    = getFileContent(vtuPath);
      // The global master writes the time series file and reads it back.
      const bool        seriesExists  = isGlobalMaster and std::filesystem::exists(series);
      const std::string seriesContent = isGlobalMaster ? getFileContent(series) : std::string();
      deleteOutputDirectory(outputDirectory);

      THEN("The series lists the snapshot and the file contains a cell per volume, its corners and every field") {
        if (isGlobalMaster) {
          VALIDATE(seriesExists, series.string());
          VALIDATE(seriesContent.find("PatchPlotterTest-0_250000") != std::string::npos, "PatchPlotterTest-0_250000");
        }
        VALIDATE(fileExists, vtuPath.string());
        VALIDATE(
          content.find("NumberOfCells=\"" + std::to_string(numberOfCells) + "\"") != std::string::npos,
          vtuPath.string()
        );
        if (numberOfRanks == 1) {
          VALIDATE(
            content.find("NumberOfPoints=\"" + std::to_string(VolumeCornersOfTheDomain) + "\"") != std::string::npos,
            vtuPath.string()
          );
        }
        VALIDATE(content.find("Name=\"scalar\"") != std::string::npos);
        VALIDATE(content.find("Name=\"pair\"") != std::string::npos);
        VALIDATE(content.find("Name=\"RefinementLevel\"") != std::string::npos);
        VALIDATE(content.find("Name=\"Subdomain\"") != std::string::npos);
        VALIDATE(content.find("Name=\"CurveIndex\"") != std::string::npos);
      }
    }
  }
}

UNIT_TEST("A nodal snapshot writes a point per node, the data as point fields and the limiter status as a cell field") {
  GIVEN("A regular grid over every rank, the values, subcell values and statuses of every patch and a patch plotter") {
    const int rank = tarch::mpi::Rank::getInstance().getRank();
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto  cells            = cellConnectivity.getCells();

    constexpr int SubcellsPerAxis      = 3;
    constexpr int StatusEntries        = 4;
    constexpr int SubcellStatus        = 2;
    constexpr int SubcellValuesPerCell = peano4::power(SubcellsPerAxis, DIMENSIONS) * ValuesPerVolume;
    static constexpr std::array<double, 2> Nodes{0.25, 0.75};
    const auto        values        = getFilledPool(cellConnectivity, VolumesPerPatch * ValuesPerVolume, 1.0);
    const auto        subcellValues = getFilledPool(cellConnectivity, SubcellValuesPerCell, 2.0);
    const auto        statuses      = getFilledPool(cellConnectivity, StatusEntries, SubcellStatus);
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "PatchPlotterNodalTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "patch/");
    exahype2::plotter::PatchPlotter plotter("PatchPlotterNodalTest", {{"scalar", 1}, {"pair", 2}});
    const std::filesystem::path     vtuPath = std::filesystem::path(outputDirectory) / "patch"
      / ("PatchPlotterNodalTest-0_500000-rank-" + std::to_string(rank) + ".vtu");
    const int numberOfCells  = cells.size * peano4::power(VolumesPerAxis - 1, DIMENSIONS);
    const int numberOfPoints = cells.size * VolumesPerPatch;

    WHEN("The plotter writes the nodes at the time one half and every rank reads its file back") {
      plotter.plotNodes<VolumesPerAxis, ValuesPerVolume, SubcellsPerAxis, double>(
        cellConnectivity,
        cells,
        values,
        Nodes.data(),
        &subcellValues,
        &statuses,
        SubcellStatus,
        0.5,
        tarch::plotter::Format::VTU,
        outputDirectory
      );
      const bool        fileExists = std::filesystem::exists(vtuPath);
      const std::string content    = getFileContent(vtuPath);
      deleteOutputDirectory(outputDirectory);

      THEN("The file contains a cell between adjacent nodes, a point per node, the point data and the status field") {
        VALIDATE(fileExists, vtuPath.string());
        VALIDATE(
          content.find("NumberOfCells=\"" + std::to_string(numberOfCells) + "\"") != std::string::npos,
          vtuPath.string()
        );
        VALIDATE(
          content.find("NumberOfPoints=\"" + std::to_string(numberOfPoints) + "\"") != std::string::npos,
          vtuPath.string()
        );
        VALIDATE(content.find("<PointData") != std::string::npos);
        VALIDATE(content.find("Name=\"pair\"") != std::string::npos);
        VALIDATE(content.find("Name=\"CurveIndex\"") != std::string::npos);
        VALIDATE(content.find("Name=\"LimiterStatus\"") != std::string::npos);
      }
    }
  }
}

UNIT_TEST("A nodal snapshot with one node per patch writes one cell per patch and the node at every patch corner") {
  GIVEN("A regular grid over every rank, the values of one node in every patch and a patch plotter") {
    const int rank = tarch::mpi::Rank::getInstance().getRank();
    peano4::unittests::buildRegularGrid(Depth);
    const auto&                            cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto                             cells            = cellConnectivity.getCells();
    const auto                             values           = getFilledPool(cellConnectivity, ValuesPerVolume, 1.0);
    static constexpr std::array<double, 1> Nodes{0.5};
    const std::string                      outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "PatchPlotterSingleNodeTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "patch/");
    exahype2::plotter::PatchPlotter plotter("PatchPlotterSingleNodeTest", {{"scalar", 1}, {"pair", 2}});
    const std::filesystem::path     vtuPath = std::filesystem::path(outputDirectory) / "patch"
      / ("PatchPlotterSingleNodeTest-0_500000-rank-" + std::to_string(rank) + ".vtu");

    WHEN("The plotter writes the nodes at the time one half and every rank reads its file back") {
      plotter.plotNodes<1, ValuesPerVolume>(
        cellConnectivity,
        cells,
        values,
        Nodes.data(),
        0.5,
        tarch::plotter::Format::VTU,
        outputDirectory
      );
      const bool        fileExists = std::filesystem::exists(vtuPath);
      const std::string content    = getFileContent(vtuPath);
      deleteOutputDirectory(outputDirectory);

      THEN("The file contains a cell per patch, a point per patch corner, the point data and the grid fields") {
        VALIDATE(fileExists, vtuPath.string());
        VALIDATE(
          content.find("NumberOfCells=\"" + std::to_string(cells.size) + "\"") != std::string::npos,
          vtuPath.string()
        );
        VALIDATE(
          content.find("NumberOfPoints=\"" + std::to_string(cells.size * peano4::VerticesPerCell) + "\"")
            != std::string::npos,
          vtuPath.string()
        );
        VALIDATE(content.find("<PointData") != std::string::npos);
        VALIDATE(content.find("Name=\"pair\"") != std::string::npos);
        VALIDATE(content.find("Name=\"RefinementLevel\"") != std::string::npos);
        VALIDATE(content.find("Name=\"CurveIndex\"") != std::string::npos);
      }
    }
  }
}

UNIT_TEST("A file without cells writes the limiter status field whenever the solver passes its statuses") {
  GIVEN("A regular grid over every rank, the values, subcell values and statuses of every patch and a patch plotter") {
    const int rank = tarch::mpi::Rank::getInstance().getRank();
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    constexpr int SubcellsPerAxis      = 3;
    constexpr int StatusEntries        = 4;
    constexpr int SubcellStatus        = 2;
    constexpr int SubcellValuesPerCell = peano4::power(SubcellsPerAxis, DIMENSIONS) * ValuesPerVolume;
    static constexpr std::array<double, 2> Nodes{0.25, 0.75};
    const auto        values        = getFilledPool(cellConnectivity, VolumesPerPatch * ValuesPerVolume, 1.0);
    const auto        subcellValues = getFilledPool(cellConnectivity, SubcellValuesPerCell, 2.0);
    const auto        statuses      = getFilledPool(cellConnectivity, StatusEntries, SubcellStatus);
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "PatchPlotterEmptyTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "patch/");
    exahype2::plotter::PatchPlotter plotter("PatchPlotterEmptyTest", {{"scalar", 1}, {"pair", 2}});
    const std::filesystem::path     vtuPath = std::filesystem::path(outputDirectory) / "patch"
      / ("PatchPlotterEmptyTest-0_500000-rank-" + std::to_string(rank) + ".vtu");

    WHEN("The plotter writes the nodes of zero cells at the time one half and every rank reads its file back") {
      plotter.plotNodes<VolumesPerAxis, ValuesPerVolume, SubcellsPerAxis, double>(
        cellConnectivity,
        tarch::Span<const int>{},
        values,
        Nodes.data(),
        &subcellValues,
        &statuses,
        SubcellStatus,
        0.5,
        tarch::plotter::Format::VTU,
        outputDirectory
      );
      const std::string content = getFileContent(vtuPath);
      deleteOutputDirectory(outputDirectory);

      THEN("The file contains zero cells and the limiter status field") {
        VALIDATE(content.find("NumberOfCells=\"0\"") != std::string::npos, vtuPath.string());
        VALIDATE(content.find("Name=\"LimiterStatus\"") != std::string::npos, vtuPath.string());
      }
    }
  }
}

#ifdef PEANO_WITH_HDF5
UNIT_TEST("A troubled patch plots every node with its containing subcell and a boundary node with the upper one") {
  GIVEN("A regular grid over every rank, troubled patches whose subcells store their index and two patch plotters") {
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto  cells            = cellConnectivity.getCells();

    constexpr int                          NodesPerAxis    = 3;
    constexpr int                          SubcellsPerAxis = 5;
    constexpr int                          NodesPerPatch   = peano4::power(NodesPerAxis, DIMENSIONS);
    constexpr int                          Subcells        = peano4::power(SubcellsPerAxis, DIMENSIONS);
    constexpr int                          StatusEntries   = 4;
    constexpr int                          SubcellStatus   = 2;
    static constexpr std::array<double, 3> GaussLegendreNodes{0.1127016653792583, 0.5, 0.8872983346207417};
    static constexpr std::array<double, 3> BoundaryNodes{0.0, 0.4, 1.0};
    static constexpr std::array<int, 3>    SubcellsAlongAxis{0, 2, 4};

    std::array<int, NodesPerPatch> expectedSubcellOfNode{};
    for (int node = 0; node < NodesPerPatch; node++) {
      const auto nodeIndex = peano4::dDelinearised(node, NodesPerAxis);
      int        stride    = 1;
      int        expected  = 0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        expected += SubcellsAlongAxis[static_cast<std::size_t>(nodeIndex(axis))] * stride;
        stride *= SubcellsPerAxis;
      }
      expectedSubcellOfNode[static_cast<std::size_t>(node)] = expected;
    }
    const auto values        = getFilledPool(cellConnectivity, NodesPerPatch, -1.0);
    auto       subcellValues = getFilledPool(cellConnectivity, Subcells, 0.0);
    for (int slot = 0; slot < subcellValues.getCapacity(); slot++) {
      for (int subcell = 0; subcell < Subcells; subcell++) {
        subcellValues.getChunk(slot)[subcell] = subcell;
      }
    }
    const auto        statuses = getFilledPool(cellConnectivity, StatusEntries, SubcellStatus);
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "PatchPlotterSubcellTest").string() + "/";
    exahype2::plotter::PatchPlotter gaussLegendrePlotter("GaussLegendreNodes", {{"subcell", 1}});
    exahype2::plotter::PatchPlotter boundaryPlotter("BoundaryNodes", {{"subcell", 1}});

    WHEN("Both plotters write their nodes and the global master reads both files back") {
      gaussLegendrePlotter.plotNodes<NodesPerAxis, 1, SubcellsPerAxis, double>(
        cellConnectivity,
        cells,
        values,
        GaussLegendreNodes.data(),
        &subcellValues,
        &statuses,
        SubcellStatus,
        0.5,
        tarch::plotter::Format::VTKHDF,
        outputDirectory
      );
      boundaryPlotter.plotNodes<NodesPerAxis, 1, SubcellsPerAxis, double>(
        cellConnectivity,
        cells,
        values,
        BoundaryNodes.data(),
        &subcellValues,
        &statuses,
        SubcellStatus,
        0.5,
        tarch::plotter::Format::VTKHDF,
        outputDirectory
      );
      std::vector<float> gaussLegendre;
      std::vector<float> boundary;
      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        gaussLegendre = readPointField(outputDirectory + "patch/GaussLegendreNodes.vtkhdf", "subcell");
        boundary      = readPointField(outputDirectory + "patch/BoundaryNodes.vtkhdf", "subcell");
      }
      deleteOutputDirectory(outputDirectory);

      THEN("Every point of both files stores the subcell of its node") {
        VALIDATE_EQUALS(gaussLegendre.size(), boundary.size());
        for (std::size_t point = 0; point < gaussLegendre.size(); point++) {
          const int expected = expectedSubcellOfNode[point % static_cast<std::size_t>(NodesPerPatch)];
          VALIDATE_EQUALS(static_cast<int>(gaussLegendre[point]), expected, point);
          VALIDATE_EQUALS(static_cast<int>(boundary[point]), expected, point);
        }
      }
    }
  }
}
#endif
