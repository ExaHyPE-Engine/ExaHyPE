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
  constexpr int ValuesPerPoint  = 3;
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
   * Returns every value of the dataset path in the VTKHDF file.
   */
  std::vector<float> readFloatDataset(const std::string& path, const std::string& datasetPath) {
    const hid_t        file      = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    const hid_t        dataset   = H5Dopen(file, datasetPath.c_str(), H5P_DEFAULT);
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

    auto values = getFilledPool(cellConnectivity, VolumesPerPatch * ValuesPerPoint, 0.0);
    for (const int cell : cells) {
      double* const volumes = values.getChunk(cell);
      for (int volume = 0; volume < VolumesPerPatch; volume++) {
        for (int value = 0; value < ValuesPerPoint; value++) {
          volumes[volume * ValuesPerPoint + value] = 100.0 * cell + 10.0 * volume + value;
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
      plotter.plotVolumes<VolumesPerAxis, ValuesPerPoint, double>(
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

UNIT_TEST("A patch snapshot gives every element its own points, writes every field and the series lists it") {
  GIVEN("A regular grid over every rank and a plotter of two fields") {
    constexpr int ElementsPerAxis = 2;
    constexpr int PointsPerAxis   = 3;
    constexpr int Elements        = peano4::power(ElementsPerAxis, DIMENSIONS);
    constexpr int PointsPerPatch  = Elements * peano4::power(PointsPerAxis, DIMENSIONS);
    const int     rank            = tarch::mpi::Rank::getInstance().getRank();
    const bool    isGlobalMaster  = tarch::mpi::Rank::getInstance().isGlobalMaster();
    peano4::unittests::buildRegularGrid(Depth);
    const auto&       cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto        cells            = cellConnectivity.getCells();
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "PatchPlotterElementTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "patch/");
    exahype2::plotter::PatchPlotter plotter("PatchPlotterElementTest", {{"scalar", 1}, {"pair", 2}});
    const std::filesystem::path     vtuPath = std::filesystem::path(outputDirectory) / "patch"
      / ("PatchPlotterElementTest-0_250000-rank-" + std::to_string(rank) + ".vtu");
    const std::filesystem::path series =
      std::filesystem::path(outputDirectory) / "patch" / "PatchPlotterElementTest.pvd";
    const int numberOfCells  = cells.size * Elements * peano4::power(PointsPerAxis - 1, DIMENSIONS);
    const int numberOfPoints = cells.size * PointsPerPatch;

    WHEN("The plotter writes the elements with a status per element at one quarter and every rank reads back") {
      plotter.plotPatch<ElementsPerAxis, PointsPerAxis, ValuesPerPoint>(
        cellConnectivity,
        cells,
        [](const int, double* const values) { std::fill_n(values, PointsPerPatch * ValuesPerPoint, 1.0); },
        0.25,
        tarch::plotter::Format::VTU,
        outputDirectory,
        [](const int, const int element) { return element; },
        "Status"
      );
      const bool        fileExists    = std::filesystem::exists(vtuPath);
      const std::string content       = getFileContent(vtuPath);
      const bool        seriesExists  = isGlobalMaster and std::filesystem::exists(series);
      const std::string seriesContent = isGlobalMaster ? getFileContent(series) : std::string();
      deleteOutputDirectory(outputDirectory);

      THEN("The series lists the snapshot and no two elements share a point") {
        if (isGlobalMaster) {
          VALIDATE(seriesExists, series.string());
          VALIDATE(
            seriesContent.find("PatchPlotterElementTest-0_250000") != std::string::npos,
            "PatchPlotterElementTest-0_250000"
          );
        }
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
        VALIDATE(content.find("Name=\"scalar\"") != std::string::npos);
        VALIDATE(content.find("Name=\"pair\"") != std::string::npos);
        VALIDATE(content.find("Name=\"RefinementLevel\"") != std::string::npos);
        VALIDATE(content.find("Name=\"Subdomain\"") != std::string::npos);
        VALIDATE(content.find("Name=\"CurveIndex\"") != std::string::npos);
        VALIDATE(content.find("Name=\"Status\"") != std::string::npos);
      }
    }
  }
}

UNIT_TEST("A file without cells writes the status field whenever the solver passes a status functor") {
  GIVEN("A regular grid over every rank and a patch plotter") {
    constexpr int PointsPerAxis  = 2;
    constexpr int PointsPerPatch = peano4::power(PointsPerAxis, DIMENSIONS);
    const int     rank           = tarch::mpi::Rank::getInstance().getRank();
    peano4::unittests::buildRegularGrid(Depth);
    const auto&       cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "PatchPlotterEmptyTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "patch/");
    exahype2::plotter::PatchPlotter plotter("PatchPlotterEmptyTest", {{"scalar", 1}, {"pair", 2}});
    const std::filesystem::path     vtuPath = std::filesystem::path(outputDirectory) / "patch"
      / ("PatchPlotterEmptyTest-0_500000-rank-" + std::to_string(rank) + ".vtu");

    WHEN("The plotter writes zero cells at one half and every rank reads its file back") {
      plotter.plotPatch<1, PointsPerAxis, ValuesPerPoint>(
        cellConnectivity,
        tarch::Span<const int>{},
        [](const int, double* const values) { std::fill_n(values, PointsPerPatch * ValuesPerPoint, 0.0); },
        0.5,
        tarch::plotter::Format::VTU,
        outputDirectory,
        [](const int, const int) { return 0; },
        "Status"
      );
      const std::string content = getFileContent(vtuPath);
      deleteOutputDirectory(outputDirectory);

      THEN("The file contains zero cells and the status field") {
        VALIDATE(content.find("NumberOfCells=\"0\"") != std::string::npos, vtuPath.string());
        VALIDATE(content.find("Name=\"Status\"") != std::string::npos, vtuPath.string());
      }
    }
  }
}

#ifdef PEANO_WITH_HDF5
UNIT_TEST("A patch snapshot places equidistant points across every cell and stores every point field value") {
  GIVEN("A regular grid over every rank and a patch plotter") {
    constexpr int PointsPerAxis  = 4;
    constexpr int PointsPerPatch = peano4::power(PointsPerAxis, DIMENSIONS);
    peano4::unittests::buildRegularGrid(Depth);
    const auto&       cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto        cells            = cellConnectivity.getCells();
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "PatchPlotterPointsTest").string() + "/";
    exahype2::plotter::PatchPlotter plotter("EquidistantPoints", {{"scalar", 1}, {"pair", 2}});

    WHEN("The plotter writes a VTKHDF snapshot and the global master reads it back") {
      plotter.plotPatch<1, PointsPerAxis, ValuesPerPoint>(
        cellConnectivity,
        cells,
        [](const int, double* const values) {
          for (int point = 0; point < PointsPerPatch; point++) {
            values[point * ValuesPerPoint + 0] = static_cast<double>(point);
            values[point * ValuesPerPoint + 1] = 2.0 * point + 1.0;
            values[point * ValuesPerPoint + 2] = -static_cast<double>(point);
          }
        },
        0.5,
        tarch::plotter::Format::VTKHDF,
        outputDirectory
      );
      std::vector<float> scalar;
      std::vector<float> pair;
      std::vector<float> points;
      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        const std::string path = outputDirectory + "patch/EquidistantPoints.vtkhdf";
        scalar                 = readFloatDataset(path, "/VTKHDF/PointData/scalar");
        pair                   = readFloatDataset(path, "/VTKHDF/PointData/pair");
        points                 = readFloatDataset(path, "/VTKHDF/Points");
      }
      deleteOutputDirectory(outputDirectory);

      THEN("Every point stores the evaluated field values and sits on the equidistant lattice") {
        constexpr double  CellWidth = 1.0 / peano4::power(peano4::SubdivisionFactor, Depth);
        const std::size_t patches   = scalar.size() / static_cast<std::size_t>(PointsPerPatch);
        VALIDATE_EQUALS(scalar.size(), patches * static_cast<std::size_t>(PointsPerPatch));
        VALIDATE_EQUALS(pair.size(), scalar.size() * 3);
        VALIDATE_EQUALS(points.size(), scalar.size() * 3);
        for (std::size_t patch = 0; patch < patches; patch++) {
          const std::size_t firstPoint = patch * static_cast<std::size_t>(PointsPerPatch);
          for (int point = 0; point < PointsPerPatch; point++) {
            const std::size_t meshPoint  = firstPoint + static_cast<std::size_t>(point);
            const auto        pointIndex = peano4::dDelinearised(point, PointsPerAxis);
            VALIDATE_EQUALS(scalar[meshPoint], static_cast<float>(point), meshPoint);
            VALIDATE_EQUALS(pair[meshPoint * 3 + 0], static_cast<float>(2 * point + 1), meshPoint);
            VALIDATE_EQUALS(pair[meshPoint * 3 + 1], static_cast<float>(-point), meshPoint);
            VALIDATE_EQUALS(pair[meshPoint * 3 + 2], 0.0f, meshPoint);
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const double relative = static_cast<double>(pointIndex(axis)) / (PointsPerAxis - 1);
              const float  expected = points[firstPoint * 3 + axis] + static_cast<float>(relative * CellWidth);
              VALIDATE(
                std::abs(points[meshPoint * 3 + axis] - expected) < 1.0e-6f,
                meshPoint,
                axis,
                points[meshPoint * 3 + axis],
                expected
              );
            }
          }
        }
      }
    }
  }
}
#endif
