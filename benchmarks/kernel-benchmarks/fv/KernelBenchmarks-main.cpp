// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarks-main.h"

#include "AbstractFVSolver.h"
#include "Constants.h"
#include "DataRepository.h"
#include "FVSolver.h"
#include "FusedDataRepository.h"

#include "exahype2/fv/EigenvalueStatistics.h"
#include "exahype2/fv/FiniteVolume.h"
#include "exahype2/fv/HandleBoundary.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "peano4/Globals.h"
#include "peano4/Init.h"
#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "runner/RepositoryCallbacks.h"
#include "runner/RuntimeParameters.h"
#include "tarch/Init.h"
#include "tarch/MemoryUsage.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Measurement.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/plotter/PVDTimeSeriesWriter.h"
#include "tarch/plotter/VTUFileWriter.h"
#include "toolbox/blockstructured/AMRRoutines.h"
#include "toolbox/blockstructured/ProjectPatch.h"

namespace {
  constexpr int NumberOfVolumesPerAxis =
    benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch;
  constexpr int NumberOfData = benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfUnknowns
    + benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfAuxiliaryVariables;
  constexpr int PatchCardinality = BasePowerExponent<NumberOfVolumesPerAxis, DIMENSIONS>::value * NumberOfData;
  constexpr int FaceCardinality  = 2 * BasePowerExponent<NumberOfVolumesPerAxis, DIMENSIONS - 1>::value * NumberOfData;

  tarch::instrumentation::Measurement timeStepMeasurement;
  double                              logNextTimeStep                  = 0.0;
  bool                                haveReceivedNonCriticalAssertion = false;

  std::vector<peano4::datamanagement::CellMarker> cells;

  int getGridLength() {
    int result = 1;
    for (int level = 0; level < runner::RuntimeParameters::getInstance().getGridLevels(); ++level) {
      result *= 3;
    }
    return result;
  }

  tarch::la::Vector<DIMENSIONS, double> getCellSize() {
    return benchmarks::exahype2::kernelbenchmarks::DomainSize / static_cast<double>(getGridLength());
  }

  bool helpRequested(const int argumentCount, char* const* const arguments) {
    for (int index = 1; index < argumentCount; ++index) {
      const std::string argument = arguments[index];
      if (argument == "--help" or argument == "-h") {
        return true;
      }
    }
    return false;
  }

  template <typename Value>
  std::string toScientificNotationString(
    const Value  value,
    const int    precision    = 4,
    const bool   considerSign = false,
    const double threshold    = 1e6
  ) {
    std::ostringstream out;
    const double       numericValue = static_cast<double>(value);

    if (std::abs(numericValue) < threshold) {
      if (considerSign) {
        out << std::showpos;
      }
      out << value;
      return out.str();
    }

    out << std::scientific << std::setprecision(precision);
    if (considerSign) {
      out << std::showpos;
    }
    out << numericValue;
    return out.str();
  }

  template <typename Value>
  std::string formatStatistic(const std::string& label, const Value& value) {
    std::ostringstream result;
    result << "  " << std::left << std::setw(28) << label << value;
    return result.str();
  }

  std::string renderSolverStatistics(const std::vector<runner::SolverStatistic>& statistics) {
    std::ostringstream result;
    for (const auto& statistic : statistics) {
      if (not result.str().empty()) {
        result << "   ";
      }
      result << statistic.label << " = ";
      if (statistic.kind == runner::SolverStatistic::Kind::Count) {
        result << statistic.count;
      } else if (statistic.minimum == statistic.maximum) {
        result << toScientificNotationString(statistic.minimum);
      } else {
        result << "[" << toScientificNotationString(statistic.minimum) << ", "
               << toScientificNotationString(statistic.maximum) << "]";
      }
    }
    return result.str();
  }

  std::string formatSimulationTime() {
    std::ostringstream result;
    result << benchmarks::exahype2::kernelbenchmarks::repositories::getMinTimeStamp() << " / "
           << runner::RuntimeParameters::getInstance().getMinTerminalTime();
    return formatStatistic("Time", result.str());
  }

  std::string formatTimeStep(
    const int                                  numberOfCompletedTimeSteps,
    const tarch::instrumentation::Measurement& measurement
  ) {
    std::ostringstream result;
    result << "Time Step " << numberOfCompletedTimeSteps;
    if (measurement.getNumberOfMeasurements() > 0) {
      result << " (" << measurement.getMostRecentValue() << "s)";
    }
    return result.str();
  }

  [[maybe_unused]] std::string formatPlot(const tarch::instrumentation::Measurement& measurement) {
    std::ostringstream result;
    result << "Plot Solution (" << measurement.getMostRecentValue() << "s)";
    return result.str();
  }

  std::string formatMeasurementTotal(const tarch::instrumentation::Measurement& measurement, const char* const unit) {
    std::ostringstream result;
    result << "Total=" << measurement.getAccumulatedValue() << unit
           << ", Measurements=" << measurement.getNumberOfMeasurements();
    return result.str();
  }

  std::vector<std::string> formatSolverStatistics() {
    std::vector<std::string> result;
    result.push_back(formatSimulationTime());
    for (const auto& statistic : benchmarks::exahype2::kernelbenchmarks::repositories::getSolverStatistics()) {
      result.push_back(formatStatistic(statistic.first, renderSolverStatistics(statistic.second)));
    }
    return result;
  }

  template <typename MemorySize>
  std::vector<std::string> formatKernelBenchmarkStatistics(
    const std::size_t                          globalNumberOfPatches,
    const double                               totalNumberOfDoFs,
    const MemorySize                           totalPersistentMemory,
    const tarch::instrumentation::Measurement& measurement
  ) {
    std::vector<std::string> result;

    std::ostringstream gridMessage;
    gridMessage << "Patches=" << toScientificNotationString(globalNumberOfPatches)
                << ", DoFs=" << toScientificNotationString(totalNumberOfDoFs);
    result.push_back(formatStatistic("Grid", gridMessage.str()));

    const auto         memoryFormat = tarch::chooseMemoryUsageFormat(totalPersistentMemory);
    std::ostringstream memoryMessage;
    memoryMessage << "Total=" << std::fixed << std::setprecision(2)
                  << tarch::convertMemorySize(totalPersistentMemory, memoryFormat) << " "
                  << tarch::toString(memoryFormat);
    result.push_back(formatStatistic("Memory", memoryMessage.str()));

    if (measurement.getNumberOfMeasurements() > 0) {
      const auto duration         = measurement.getMostRecentValue();
      const auto patchesPerSecond = globalNumberOfPatches / duration;
      const auto dofsPerSecond    = totalNumberOfDoFs / duration;

      std::ostringstream performanceMessage;
      performanceMessage << patchesPerSecond << " Patch/s, " << dofsPerSecond << " DoF/s";
      result.push_back(formatStatistic("Performance", performanceMessage.str()));
    }

    return result;
  }

  void logFinalStatistics(
    const tarch::instrumentation::Measurement& kernelMeasurement,
    const tarch::instrumentation::Measurement& boundaryMeasurement,
    const tarch::instrumentation::Measurement& riemannMeasurement
  ) {
    if (boundaryMeasurement.getNumberOfMeasurements() > 0) {
      logInfo(formatStatistic("Boundary Handling", formatMeasurementTotal(boundaryMeasurement, "s")));
    }
    if (riemannMeasurement.getNumberOfMeasurements() > 0) {
      logInfo(formatStatistic("Riemann Solver", formatMeasurementTotal(riemannMeasurement, "s")));
    }

    if (kernelMeasurement.getNumberOfMeasurements() > 0) {
      logInfo(formatStatistic("Time Stepping", formatMeasurementTotal(kernelMeasurement, "s")));

      const auto   globalNumberOfPatches = cells.size();
      const double totalNumberOfDoFs     = static_cast<double>(globalNumberOfPatches) * PatchCardinality;
      const auto   averageStepTime       = kernelMeasurement.getAverageValue();

      std::ostringstream performanceMessage;
      performanceMessage << "Average=" << globalNumberOfPatches / averageStepTime << " Patch/s, "
                         << totalNumberOfDoFs / averageStepTime << " DoF/s";
      logInfo(formatStatistic("Performance", performanceMessage.str()));
    }
  }

  void plotGrid() {
    benchmarks::exahype2::kernelbenchmarks::repositories::startPlottingStep();

    const std::string outputDirectory  = "output";
    const std::string solutionFileName = "FVSolver";

    const double plotTimeStamp = 0.5
      * (benchmarks::exahype2::kernelbenchmarks::repositories::getMinTimeStamp()
         + benchmarks::exahype2::kernelbenchmarks::repositories::getMaxTimeStamp());

    static tarch::plotter::PVDTimeSeriesWriter timeSeriesWriter;
    tarch::plotter::VTUFileWriter              fileWriter(outputDirectory, 1);

    auto vertexPlotter   = fileWriter.createVertexPlotter(0);
    auto cellPlotter     = fileWriter.createCellPlotter(0);
    auto cellDataPlotter = fileWriter.createCellDataPlotter(
      0,
      "FVSolverQ",
      benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfUnknowns
    );

    for (const auto& marker : cells) {
      if (not marker.hasBeenRefined()) {
        const auto& patchSize   = marker.getH();
        const auto  patchOffset = marker.getX() - patchSize * 0.5;

        const auto        cellSize = patchSize / static_cast<double>(NumberOfVolumesPerAxis);
        const auto* const qOut     = benchmarks::exahype2::kernelbenchmarks::DataRepository::getInstance().getCellQNew(
          marker.getX(),
          marker.getH()
        );

        int currentDegreeOfFreedom = 0;
        peano4::dForEach(NumberOfVolumesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
#if DIMENSIONS == 2
          tarch::la::Vector<2, double> cellOffset;
          cellOffset(0) = patchOffset(0) + static_cast<double>(volume(0)) * cellSize(0);
          cellOffset(1) = patchOffset(1) + static_cast<double>(volume(1)) * cellSize(1);
          int vertices[4];
          vertices[0] = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{cellOffset(0), cellOffset(1)});
          vertices[1] =
            vertexPlotter->plotVertex(tarch::la::Vector<2, double>{cellOffset(0) + cellSize(0), cellOffset(1)});
          vertices[2] = vertexPlotter->plotVertex(
            tarch::la::Vector<2, double>{cellOffset(0) + cellSize(0), cellOffset(1) + cellSize(1)}
          );
          vertices[3] =
            vertexPlotter->plotVertex(tarch::la::Vector<2, double>{cellOffset(0), cellOffset(1) + cellSize(1)});
          const auto cellIndex = cellPlotter->plotQuadrangle(vertices);
#else
          tarch::la::Vector<3, double> cellOffset;
          cellOffset(0) = patchOffset(0) + static_cast<double>(volume(0)) * cellSize(0);
          cellOffset(1) = patchOffset(1) + static_cast<double>(volume(1)) * cellSize(1);
          cellOffset(2) = patchOffset(2) + static_cast<double>(volume(2)) * cellSize(2);
          int vertices[8];
          vertices[0] = vertexPlotter->plotVertex(
            tarch::la::Vector<3, double>{cellOffset(0), cellOffset(1), cellOffset(2)}
          );
          vertices[1] = vertexPlotter->plotVertex(
            tarch::la::Vector<3, double>{cellOffset(0) + cellSize(0), cellOffset(1), cellOffset(2)}
          );
          vertices[2] = vertexPlotter->plotVertex(
            tarch::la::Vector<3, double>{cellOffset(0) + cellSize(0), cellOffset(1) + cellSize(1), cellOffset(2)}
          );
          vertices[3] = vertexPlotter->plotVertex(
            tarch::la::Vector<3, double>{cellOffset(0), cellOffset(1) + cellSize(1), cellOffset(2)}
          );
          vertices[4] = vertexPlotter->plotVertex(
            tarch::la::Vector<3, double>{cellOffset(0), cellOffset(1), cellOffset(2) + cellSize(2)}
          );
          vertices[5] = vertexPlotter->plotVertex(
            tarch::la::Vector<3, double>{cellOffset(0) + cellSize(0), cellOffset(1), cellOffset(2) + cellSize(2)}
          );
          vertices[6] = vertexPlotter->plotVertex(
            tarch::la::Vector<3, double>{
              cellOffset(0) + cellSize(0),
              cellOffset(1) + cellSize(1),
              cellOffset(2) + cellSize(2)
            }
          );
          vertices[7] = vertexPlotter->plotVertex(
            tarch::la::Vector<3, double>{cellOffset(0), cellOffset(1) + cellSize(1), cellOffset(2) + cellSize(2)}
          );
          const auto cellIndex = cellPlotter->plotHexahedron(vertices);
#endif

          const auto* const values = qOut + currentDegreeOfFreedom;
          cellDataPlotter->plotCellData(
            cellIndex,
            values,
            benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfUnknowns
          );

          currentDegreeOfFreedom += NumberOfData;
        });
      }
    }

    timeSeriesWriter.writeSnapshotFile(solutionFileName, plotTimeStamp, fileWriter);
    timeSeriesWriter.writeTimeSeriesMetaFile(outputDirectory, solutionFileName);

    benchmarks::exahype2::kernelbenchmarks::repositories::finishPlottingStep();
  }

  void applyInitialConditions() {
    for (const auto& marker : cells) {
      if (not marker.hasBeenRefined()) {
        auto* const qOut = benchmarks::exahype2::kernelbenchmarks::DataRepository::getInstance().getCellQNew(
          marker.getX(),
          marker.getH()
        );
        peano4::dForEach(NumberOfVolumesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          const auto linearIndex = peano4::dLinearised(volume, NumberOfVolumesPerAxis) * NumberOfData;
          benchmarks::exahype2::kernelbenchmarks::FVSolver::initialCondition(
            qOut + linearIndex,
            exahype2::fv::getVolumeCentre(marker.getX(), marker.getH(), NumberOfVolumesPerAxis, volume),
            exahype2::fv::getVolumeSize(marker.getH(), NumberOfVolumesPerAxis)
          );
        });
      }
    }
  }

  peano4::datamanagement::CellMarker createMarkerFromIndex(const tarch::la::Vector<DIMENSIONS, int>& index) {
    const auto cellSize         = getCellSize();
    const auto cellIndex        = tarch::la::convertScalar<double>(index);
    const auto cellPositionBase = benchmarks::exahype2::kernelbenchmarks::DomainOffset + cellSize / 2.0;
    const auto cellPosition     = cellPositionBase + tarch::la::multiplyComponents(cellIndex, cellSize);
    peano4::datamanagement::CellMarker marker;
    marker.h      = cellSize;
    marker.centre = cellPosition;
    return marker;
  }

  void createGrid() {
    benchmarks::exahype2::kernelbenchmarks::repositories::startGridConstructionStep();
    const int gridLength = getGridLength();
    cells.reserve(tarch::la::aPowI(DIMENSIONS, gridLength));
    peano4::dForEach(gridLength, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      cells.push_back(createMarkerFromIndex(volume));
    });
    benchmarks::exahype2::kernelbenchmarks::repositories::finishGridConstructionStep();
  }

  void initGrid() {
    benchmarks::exahype2::kernelbenchmarks::repositories::startGridInitialisationStep();
    applyInitialConditions();
    benchmarks::exahype2::kernelbenchmarks::repositories::finishGridInitialisationStep();
  }
} // namespace

int main(int argumentCount, char** arguments) {
  int returnCode = EXIT_SUCCESS;

  auto& runtimeParameters = runner::RuntimeParameters::getInstance();
  runtimeParameters.setGeneratedDefaults(
    benchmarks::exahype2::kernelbenchmarks::BenchmarkMinTerminalTime,
    benchmarks::exahype2::kernelbenchmarks::BenchmarkMaxTerminalTime,
    benchmarks::exahype2::kernelbenchmarks::BenchmarkGridLevels,
    benchmarks::exahype2::kernelbenchmarks::BenchmarkMaxGridLevels,
    "KernelBenchmarks"
  );
  if (helpRequested(argumentCount, arguments)) {
    runtimeParameters.printUsage("KernelBenchmarks");
    return EXIT_SUCCESS;
  }
  tarch::init(
    &argumentCount,
    &arguments,
    {runtimeParameters.getMPITimeoutWarning(),
     runtimeParameters.getMPIDeadlockTimeout(),
     runtimeParameters.isMPIRequestTrackingEnabled()}
  );
  runtimeParameters.configure(argumentCount, arguments);

  peano4::init(
    benchmarks::exahype2::kernelbenchmarks::DomainOffset,
    benchmarks::exahype2::kernelbenchmarks::DomainSize
  );

  benchmarks::exahype2::kernelbenchmarks::repositories::startSimulation();

  createGrid();
  initGrid();

  auto& fusedGrid = benchmarks::exahype2::kernelbenchmarks::FusedDataRepository::getInstance();
  fusedGrid.startCollection();
  for (const auto& marker : cells) {
    if (not marker.hasBeenRefined()) {
      fusedGrid.bookmarkCell(marker.getX(), marker.getH());
    }
  }

  fusedGrid.buildFromBookmarks();

  {
    const int                  numberOfPatches = fusedGrid.getNumberFusedCells();
    double* const              qNewFlattened   = fusedGrid.getQNew();
    double* const              facesPool       = fusedGrid.getFaceQNewPool();
    const std::uint64_t* const faceIndices     = fusedGrid.getFaceIndices();
    tarch::parallelFor(tarch::Range(numberOfPatches), [=] GPU_CALLABLE_LAMBDA(const int patchIndex) {
      const double* const qIn = &qNewFlattened[patchIndex * PatchCardinality];
      double*             faces[TWO_TIMES_D];
      for (int faceNumber = 0; faceNumber < TWO_TIMES_D; ++faceNumber) {
        const auto faceIndex = faceIndices[patchIndex * TWO_TIMES_D + faceNumber];
        faces[faceNumber]    = &facesPool[faceIndex * FaceCardinality];
      }
      toolbox::blockstructured::projectPatch<NumberOfVolumesPerAxis, 1, NumberOfData, double>(qIn, faces);
    });
  }

  tarch::instrumentation::Measurement kernelMeasurement(1.0);
  tarch::instrumentation::Measurement boundaryMeasurement(1.0);
  tarch::instrumentation::Measurement riemannMeasurement(1.0);
  tarch::instrumentation::Measurement plotMeasurement(1.0);
  tarch::instrumentation::Watch       timeStepWatch("::", "main()", false);
  double                              timeStamp                            = 0.0;
  double                              nextMinPlotTimeStamp                 = runtimeParameters.getFirstPlotTimeStamp();
  double                              nextMaxPlotTimeStamp                 = runtimeParameters.getFirstPlotTimeStamp();
  bool                                continueToSolve                      = true;
  int                                 sampleCount                          = 0;
  int                                 numberOfCompletedTimeSteps           = 0;
  int                                 lastLoggedNumberOfCompletedTimeSteps = 0;
  const double                        minTerminalTime                      = runtimeParameters.getMinTerminalTime();
  const double                        maxTerminalTime                      = runtimeParameters.getMaxTerminalTime();
  const double                        plottingEndTime =
    maxTerminalTime < std::numeric_limits<double>::max() / 10.0 ? maxTerminalTime : minTerminalTime;
  const int    numberOfSnapshots  = runtimeParameters.getNumberOfSnapshots();
  const double timeInBetweenPlots = numberOfSnapshots <= 0 ? 0.0 : plottingEndTime / numberOfSnapshots;

  while (continueToSolve) {
    if (timeInBetweenPlots > 0.0
        and (benchmarks::exahype2::kernelbenchmarks::repositories::getMinTimeStamp() >= nextMinPlotTimeStamp or benchmarks::exahype2::kernelbenchmarks::repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp)) {
      if (benchmarks::exahype2::kernelbenchmarks::repositories::getMinTimeStamp() >= nextMinPlotTimeStamp) {
        nextMinPlotTimeStamp += timeInBetweenPlots;
      }
      if (benchmarks::exahype2::kernelbenchmarks::repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp) {
        nextMaxPlotTimeStamp += timeInBetweenPlots;
      }

      if constexpr (benchmarks::exahype2::kernelbenchmarks::ShouldPlot) {
        tarch::instrumentation::Watch plotWatch("", "", false);
        plotWatch.start();
        fusedGrid.syncCellsDeviceToHost();
        plotGrid();
        plotWatch.stop();
        plotMeasurement.setValue(plotWatch.getCalendarTime());
        logInfo(formatPlot(plotMeasurement));
      }
    }
    fusedGrid.swapQNewAndQOld();

    timeStepWatch.start();
    benchmarks::exahype2::kernelbenchmarks::repositories::startTimeStep();

    const double timeStepSize =
      benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();

    tarch::instrumentation::Watch kernelWatch("", "", false);
    kernelWatch.start();

    tarch::instrumentation::Watch boundaryWatch("", "", false);
    boundaryWatch.start();
    {
      const int                                          numberOfBoundaryFaces = fusedGrid.getNumberOfBoundaryFaces();
      double* const                                      facesPool             = fusedGrid.getFaceQOldPool();
      const std::uint64_t* const                         boundaryFaceIndices   = fusedGrid.getBoundaryFaceIndices();
      const std::uint8_t* const                          boundaryFaceNumbers   = fusedGrid.getBoundaryFaceNumbers();
      const std::uint64_t* const                         boundaryCellIndices   = fusedGrid.getBoundaryCellIndices();
      const tarch::la::Vector<DIMENSIONS, double>* const cellCentres           = fusedGrid.getCellCentres();
      const tarch::la::Vector<DIMENSIONS, double>* const cellSizes             = fusedGrid.getCellSizes();
      tarch::parallelFor(tarch::Range(numberOfBoundaryFaces), [=] GPU_CALLABLE_LAMBDA(const int boundaryFaceIndex) {
        const auto faceIndex  = boundaryFaceIndices[boundaryFaceIndex];
        const auto faceNumber = static_cast<int>(boundaryFaceNumbers[boundaryFaceIndex]);
        const auto cellIndex  = boundaryCellIndices[boundaryFaceIndex];
        exahype2::fv::handleBoundary<
          benchmarks::exahype2::kernelbenchmarks::FVSolver,
          NumberOfVolumesPerAxis,
          1,
          benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfUnknowns,
          benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfAuxiliaryVariables,
          double>(
          benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVSolver,
          cellCentres[cellIndex],
          cellSizes[cellIndex],
          timeStamp,
          faceNumber,
          &facesPool[faceIndex * FaceCardinality]
        );
      });
    }
    boundaryWatch.stop();

    tarch::instrumentation::Watch riemannWatch("", "", false);
    riemannWatch.start();
    double maxEigenvalue = 0.0;
    {
      const int                                          numberOfPatches = fusedGrid.getNumberFusedCells();
      const double* const                                qOldFlattened   = fusedGrid.getQOld();
      double* const                                      qNewFlattened   = fusedGrid.getQNew();
      const double* const                                facesOldPool    = fusedGrid.getFaceQOldPool();
      double* const                                      facesNewPool    = fusedGrid.getFaceQNewPool();
      const std::uint64_t* const                         faceIndices     = fusedGrid.getFaceIndices();
      const tarch::la::Vector<DIMENSIONS, double>* const cellCentres     = fusedGrid.getCellCentres();
      const tarch::la::Vector<DIMENSIONS, double>* const cellSizes       = fusedGrid.getCellSizes();
      tarch::parallelReduce(
        tarch::TeamRange(numberOfPatches),
        [=] GPU_CALLABLE_LAMBDA(
          const int                                       patchIndex,
          [[maybe_unused]] tarch::accelerator::TeamHandle teamHandle,
          double&                                         reducedPatchEigenvalue
        ) {
          const double* const qIn         = &qOldFlattened[patchIndex * PatchCardinality];
          double* const       qOut        = &qNewFlattened[patchIndex * PatchCardinality];
          const auto          patchCentre = cellCentres[patchIndex];
          const auto          patchSize   = cellSizes[patchIndex];
          const double*       faceQOld[TWO_TIMES_D];
          double*             faceQNew[TWO_TIMES_D];
          for (int faceNumber = 0; faceNumber < TWO_TIMES_D; ++faceNumber) {
            const auto faceIndex = faceIndices[patchIndex * TWO_TIMES_D + faceNumber];
            faceQOld[faceNumber] = &facesOldPool[faceIndex * FaceCardinality];
            faceQNew[faceNumber] = &facesNewPool[faceIndex * FaceCardinality];
          }
          const double patchEigenvalue = exahype2::fv::applyFiniteVolume<
            exahype2::fv::reconstruction::Constant,
            exahype2::fv::timestepping::SingleStep,
            benchmarks::exahype2::kernelbenchmarks::FVSolver,
            NumberOfVolumesPerAxis,
            benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfUnknowns,
            benchmarks::exahype2::kernelbenchmarks::AbstractFVSolver::NumberOfAuxiliaryVariables,
            true,
            false,
            false,
            false,
            false,
            true,
            true,
            double,
            double>(qIn, qOut, faceQOld, faceQNew, patchCentre, patchSize, timeStamp, timeStepSize);
          // The maximum reduction seeds its accumulator with the lowest double, which lies
          // outside the non-negative range exahype2::fv::accumulateMaxEigenvalue requires,
          // so the patch maxima combine here directly.
          reducedPatchEigenvalue = patchEigenvalue > reducedPatchEigenvalue ? patchEigenvalue : reducedPatchEigenvalue;
        },
        tarch::Max<double>{maxEigenvalue}
      );
    }
    riemannWatch.stop();
    kernelWatch.stop();

    benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVSolver.setMaxEigenvalue(maxEigenvalue);
    const auto newTimeStepSize =
      benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();
    benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVSolver
      .update(newTimeStepSize, timeStamp + timeStepSize, fusedGrid.getMinVolumeH(), fusedGrid.getMaxVolumeH());

    timeStamp += benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();
    benchmarks::exahype2::kernelbenchmarks::repositories::finishTimeStep();

    timeStepWatch.stop();

    if (benchmarks::exahype2::kernelbenchmarks::repositories::mayPlot()) {
      kernelMeasurement.setValue(kernelWatch.getCalendarTime());
      boundaryMeasurement.setValue(boundaryWatch.getCalendarTime());
      riemannMeasurement.setValue(riemannWatch.getCalendarTime());
      timeStepMeasurement.setValue(timeStepWatch.getCalendarTime());
      ++numberOfCompletedTimeSteps;

      if (timeStepMeasurement.getAccumulatedValue() >= logNextTimeStep) {
        const auto totalPersistentMemory =
          benchmarks::exahype2::kernelbenchmarks::DataRepository::getInstance().getMemoryUsageInBytes();

        const auto   globalNumberOfPatches = cells.size();
        const double totalNumberOfDoFs     = static_cast<double>(globalNumberOfPatches) * PatchCardinality;

        logInfo(formatTimeStep(numberOfCompletedTimeSteps, timeStepMeasurement));
        for (const auto& statistic : formatSolverStatistics()) {
          logInfo(statistic);
        }
        for (const auto& statistic : formatKernelBenchmarkStatistics(
               globalNumberOfPatches,
               totalNumberOfDoFs,
               totalPersistentMemory,
               kernelMeasurement
             )) {
          logInfo(statistic);
        }
        lastLoggedNumberOfCompletedTimeSteps = numberOfCompletedTimeSteps;
        logNextTimeStep += runtimeParameters.getTimeStepLogInterval();
      }

      if (benchmarks::exahype2::kernelbenchmarks::Samples > 0
          and ++sampleCount >= benchmarks::exahype2::kernelbenchmarks::Samples) {
        continueToSolve = false;
      }
    }

    if (benchmarks::exahype2::kernelbenchmarks::repositories::getMinTimeStamp() >= minTerminalTime
        and benchmarks::exahype2::kernelbenchmarks::repositories::getMaxTimeStamp() >= maxTerminalTime) {
      continueToSolve = false;
    }

    if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNonCriticalAssertion) {
      continueToSolve                  = false;
      haveReceivedNonCriticalAssertion = true;
      logError("A non-critical assertion was triggered. Dump the final state and terminate.");
    }
  }

  if (timeInBetweenPlots > 0.0) {
    if constexpr (benchmarks::exahype2::kernelbenchmarks::ShouldPlot) {
      tarch::instrumentation::Watch plotWatch("", "", false);
      plotWatch.start();
      fusedGrid.syncCellsDeviceToHost();
      plotGrid();
      plotWatch.stop();
      plotMeasurement.setValue(plotWatch.getCalendarTime());
      logInfo(formatPlot(plotMeasurement));
    }
  }
  benchmarks::exahype2::kernelbenchmarks::repositories::finishSimulation();

  if (haveReceivedNonCriticalAssertion) {
    logWarning("Terminated gracefully after a non-critical assertion.");
    returnCode = EXIT_FAILURE;
  } else {
    if (numberOfCompletedTimeSteps > lastLoggedNumberOfCompletedTimeSteps) {
      logInfo(formatTimeStep(numberOfCompletedTimeSteps, timeStepMeasurement));
      for (const auto& statistic : formatSolverStatistics()) {
        logInfo(statistic);
      }
      const auto totalPersistentMemory =
        benchmarks::exahype2::kernelbenchmarks::DataRepository::getInstance().getMemoryUsageInBytes();
      const auto   globalNumberOfPatches = cells.size();
      const double totalNumberOfDoFs     = static_cast<double>(globalNumberOfPatches) * PatchCardinality;
      for (const auto& statistic : formatKernelBenchmarkStatistics(
             globalNumberOfPatches,
             totalNumberOfDoFs,
             totalPersistentMemory,
             kernelMeasurement
           )) {
        logInfo(statistic);
      }
    }
    logInfo("Terminated successfully.");
    returnCode = EXIT_SUCCESS;
  }

  logFinalStatistics(kernelMeasurement, boundaryMeasurement, riemannMeasurement);

  fusedGrid.clear();

  peano4::shutdown();
  tarch::shutdown();

  return returnCode;
}
