// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PatchPlotterTest.h"

#include "exahype2/plotter/PatchPlotter.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Format.h"

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
} // namespace

exahype2::plotter::tests::PatchPlotterTest::PatchPlotterTest() :
  tarch::tests::TestCase("exahype2::plotter::tests::PatchPlotterTest") {}

void
exahype2::plotter::tests::PatchPlotterTest::run() {
  testMethod(testEveryVolumeOfEveryPatchBecomesOneCell);
  testMethod(testEveryNodeOfEveryPatchBecomesOnePoint);
  testMethod(testOneNodePerPatchBecomesOneCell);
  testMethod(testEveryNodeReadsTheSubcellThatContainsIt);
}

/**
 * A node on a subcell boundary reads the upper subcell.
 */
void
exahype2::plotter::tests::PatchPlotterTest::testEveryNodeReadsTheSubcellThatContainsIt() {
  constexpr int                   NodesPerAxis    = 3;
  constexpr int                   SubcellsPerAxis = 5;
  constexpr std::array<double, 3> GaussLegendreNodes{0.1127016653792583, 0.5, 0.8872983346207417};
  constexpr std::array<double, 3> BoundaryNodes{0.0, 0.4, 1.0};
  constexpr std::array<int, 3>    SubcellsAlongAxis{0, 2, 4};
  const auto                      gaussLegendre =
    exahype2::plotter::PatchPlotter::containingSubcells<NodesPerAxis, SubcellsPerAxis>(GaussLegendreNodes.data());
  const auto boundary =
    exahype2::plotter::PatchPlotter::containingSubcells<NodesPerAxis, SubcellsPerAxis>(BoundaryNodes.data());
  for (int node = 0; node < peano4::power(NodesPerAxis, DIMENSIONS); node++) {
    const auto nodeIndex = peano4::dDelinearised(node, NodesPerAxis);
    int        stride    = 1;
    int        expected  = 0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      expected += SubcellsAlongAxis[static_cast<std::size_t>(nodeIndex(axis))] * stride;
      stride *= SubcellsPerAxis;
    }
    validateEqualsWithParams1(gaussLegendre[static_cast<std::size_t>(node)], expected, node);
    validateEqualsWithParams1(boundary[static_cast<std::size_t>(node)], expected, node);
  }
}

void
exahype2::plotter::tests::PatchPlotterTest::testEveryNodeOfEveryPatchBecomesOnePoint() {
  const int rank = tarch::mpi::Rank::getInstance().getRank();
  peano4::tests::buildRegularGrid(Depth);
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  const auto  cells            = cellConnectivity.getCells();

  constexpr int                   SubcellsPerAxis = 3;
  constexpr int                   StatusEntries   = 4;
  constexpr int                   SubcellStatus   = 2;
  constexpr std::array<double, 2> Nodes{0.25, 0.75};
  constexpr int                   SubcellValuesPerCell = peano4::power(SubcellsPerAxis, DIMENSIONS) * ValuesPerVolume;
  const auto                      values = getFilledPool(cellConnectivity, VolumesPerPatch * ValuesPerVolume, 1.0);
  const auto                      subcellValues = getFilledPool(cellConnectivity, SubcellValuesPerCell, 2.0);
  const auto                      statuses      = getFilledPool(cellConnectivity, StatusEntries, SubcellStatus);
  const std::string               outputDirectory =
    (tarch::tests::getWritableTestDirectory() / "PatchPlotterNodalTest").string() + "/";
  std::filesystem::create_directories(outputDirectory + "patch/");
  exahype2::plotter::PatchPlotter plotter("PatchPlotterNodalTest", {{"scalar", 1}, {"pair", 2}});
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

  const std::filesystem::path vtuPath = std::filesystem::path(outputDirectory) / "patch"
    / ("PatchPlotterNodalTest-0_500000-rank-" + std::to_string(rank) + ".vtu");
  validateWithParams1(std::filesystem::exists(vtuPath), vtuPath.string());
  const std::string content        = getFileContent(vtuPath);
  const int         numberOfCells  = cells.size * peano4::power(VolumesPerAxis - 1, DIMENSIONS);
  const int         numberOfPoints = cells.size * VolumesPerPatch;
  validateWithParams1(
    content.find("NumberOfCells=\"" + std::to_string(numberOfCells) + "\"") != std::string::npos,
    vtuPath.string()
  );
  validateWithParams1(
    content.find("NumberOfPoints=\"" + std::to_string(numberOfPoints) + "\"") != std::string::npos,
    vtuPath.string()
  );
  validate(content.find("<PointData") != std::string::npos);
  validate(content.find("Name=\"pair\"") != std::string::npos);
  validate(content.find("Name=\"CurveIndex\"") != std::string::npos);
  validate(content.find("Name=\"LimiterStatus\"") != std::string::npos);
  deleteOutputDirectory(outputDirectory);
}

void
exahype2::plotter::tests::PatchPlotterTest::testOneNodePerPatchBecomesOneCell() {
  const int rank = tarch::mpi::Rank::getInstance().getRank();
  peano4::tests::buildRegularGrid(Depth);
  const auto&                     cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  const auto                      cells            = cellConnectivity.getCells();
  const auto                      values           = getFilledPool(cellConnectivity, ValuesPerVolume, 1.0);
  constexpr std::array<double, 1> Nodes{0.5};
  const std::string               outputDirectory =
    (tarch::tests::getWritableTestDirectory() / "PatchPlotterSingleNodeTest").string() + "/";
  std::filesystem::create_directories(outputDirectory + "patch/");
  exahype2::plotter::PatchPlotter plotter("PatchPlotterSingleNodeTest", {{"scalar", 1}, {"pair", 2}});
  plotter.plotNodes<1, ValuesPerVolume>(
    cellConnectivity,
    cells,
    values,
    Nodes.data(),
    0.5,
    tarch::plotter::Format::VTU,
    outputDirectory
  );

  const std::filesystem::path vtuPath = std::filesystem::path(outputDirectory) / "patch"
    / ("PatchPlotterSingleNodeTest-0_500000-rank-" + std::to_string(rank) + ".vtu");
  validateWithParams1(std::filesystem::exists(vtuPath), vtuPath.string());
  const std::string content = getFileContent(vtuPath);
  validateWithParams1(
    content.find("NumberOfCells=\"" + std::to_string(cells.size) + "\"") != std::string::npos,
    vtuPath.string()
  );
  validateWithParams1(
    content.find("NumberOfPoints=\"" + std::to_string(cells.size * peano4::VerticesPerCell) + "\"")
      != std::string::npos,
    vtuPath.string()
  );
  validate(content.find("<PointData") != std::string::npos);
  validate(content.find("Name=\"pair\"") != std::string::npos);
  validate(content.find("Name=\"RefinementLevel\"") != std::string::npos);
  validate(content.find("Name=\"CurveIndex\"") != std::string::npos);
  deleteOutputDirectory(outputDirectory);
}

void
exahype2::plotter::tests::PatchPlotterTest::testEveryVolumeOfEveryPatchBecomesOneCell() {
  const int rank          = tarch::mpi::Rank::getInstance().getRank();
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  peano4::tests::buildRegularGrid(Depth);
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  const auto  cells            = cellConnectivity.getCells();

  auto values = getFilledPool(cellConnectivity, VolumesPerPatch * ValuesPerVolume, 0.0);
  for (const int cell : cells) {
    double* const volumes = values.getChunk(cell);
    for (int volume = 0; volume < VolumesPerPatch; volume++) {
      for (int value = 0; value < ValuesPerVolume; value++) {
        volumes[volume * ValuesPerVolume + value] = 100.0 * cell + 10.0 * volume + value;
      }
    }
  }

  const std::string outputDirectory = (tarch::tests::getWritableTestDirectory() / "PatchPlotterTest").string() + "/";
  std::filesystem::create_directories(outputDirectory + "patch/");
  exahype2::plotter::PatchPlotter plotter("PatchPlotterTest", {{"scalar", 1}, {"pair", 2}});
  plotter.plotVolumes<VolumesPerAxis, ValuesPerVolume, double>(
    cellConnectivity,
    cells,
    values,
    0.25,
    tarch::plotter::Format::VTU,
    outputDirectory
  );

  const std::filesystem::path vtuPath = std::filesystem::path(outputDirectory) / "patch"
    / ("PatchPlotterTest-0_250000-rank-" + std::to_string(rank) + ".vtu");
  validateWithParams1(std::filesystem::exists(vtuPath), vtuPath.string());
  const std::string content       = getFileContent(vtuPath);
  const int         numberOfCells = cells.size * VolumesPerPatch;
  validateWithParams1(
    content.find("NumberOfCells=\"" + std::to_string(numberOfCells) + "\"") != std::string::npos,
    vtuPath.string()
  );
  if (numberOfRanks == 1) {
    const int volumesPerDomainAxis = peano4::power(peano4::SubdivisionFactor, Depth) * VolumesPerAxis;
    const int numberOfPoints       = peano4::power(volumesPerDomainAxis + 1, DIMENSIONS);
    validateWithParams1(
      content.find("NumberOfPoints=\"" + std::to_string(numberOfPoints) + "\"") != std::string::npos,
      vtuPath.string()
    );
  }
  validate(content.find("Name=\"scalar\"") != std::string::npos);
  validate(content.find("Name=\"pair\"") != std::string::npos);
  validate(content.find("Name=\"RefinementLevel\"") != std::string::npos);
  validate(content.find("Name=\"Subdomain\"") != std::string::npos);
  validate(content.find("Name=\"CurveIndex\"") != std::string::npos);

  // The global master writes the time series file and reads it back.
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::filesystem::path series = std::filesystem::path(outputDirectory) / "patch" / "PatchPlotterTest.pvd";
    validateWithParams1(std::filesystem::exists(series), series.string());
    validate(getFileContent(series).find("PatchPlotterTest-0_250000") != std::string::npos);
  }
  deleteOutputDirectory(outputDirectory);
}
