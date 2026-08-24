// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarks-main.h"

#include "AbstractAderSolver.h"
#include "AderSolver.h"
#include "Constants.h"
#include "DataRepository.h"
#include "FusedDataRepository.h"
#include "FusedTasks.h"

#include "exahype2/dg/DGUtils.h"
#include "peano4/Globals.h"
#include "peano4/Init.h"
#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "runner/RepositoryCallbacks.h"
#include "tarch/Init.h"
#include "tarch/MemoryUsage.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Measurement.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/plotter/HDF5FileWriter.h"
#include "tarch/plotter/XDMFTimeSeriesWriter.h"

namespace {
  constexpr int NumberOfDofsPerAxis = benchmarks::exahype2::kernelbenchmarks::AbstractAderSolver::Order + 1;
  constexpr int NumberOfData        = benchmarks::exahype2::kernelbenchmarks::AbstractAderSolver::NumberOfUnknowns
    + benchmarks::exahype2::kernelbenchmarks::AbstractAderSolver::NumberOfAuxiliaryVariables;
  constexpr int PatchCardinality = BasePowerExponent<NumberOfDofsPerAxis, DIMENSIONS>::value * NumberOfData;

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
    const tarch::instrumentation::Measurement& predictorMeasurement,
    const tarch::instrumentation::Measurement& riemannMeasurement,
    const tarch::instrumentation::Measurement& correctorMeasurement
  ) {
    if (boundaryMeasurement.getNumberOfMeasurements() > 0) {
      logInfo(formatStatistic("Boundary Handling", formatMeasurementTotal(boundaryMeasurement, "s")));
    }
    if (predictorMeasurement.getNumberOfMeasurements() > 0) {
      logInfo(formatStatistic("Predictor", formatMeasurementTotal(predictorMeasurement, "s")));
    }
    if (riemannMeasurement.getNumberOfMeasurements() > 0) {
      logInfo(formatStatistic("Riemann Solver", formatMeasurementTotal(riemannMeasurement, "s")));
    }
    if (correctorMeasurement.getNumberOfMeasurements() > 0) {
      logInfo(formatStatistic("Corrector", formatMeasurementTotal(correctorMeasurement, "s")));
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
    const std::string solutionFileName = "AderSolver";

    const double plotTimeStamp = 0.5
      * (benchmarks::exahype2::kernelbenchmarks::repositories::getMinTimeStamp()
         + benchmarks::exahype2::kernelbenchmarks::repositories::getMaxTimeStamp());

    static tarch::plotter::XDMFTimeSeriesWriter timeSeriesWriter;
    tarch::plotter::HDF5FileWriter              fileWriter(outputDirectory, 1);

    auto cellPlotter       = fileWriter.createCellPlotter(0);
    auto vertexPlotter     = fileWriter.createVertexPlotter(0);
    auto vertexDataPlotter = fileWriter.createVertexDataPlotter(
      0,
      "AderSolverQ",
      benchmarks::exahype2::kernelbenchmarks::AbstractAderSolver::NumberOfUnknowns
    );

    for (const auto& marker : cells) {
      if (not marker.hasBeenRefined()) {
        const auto* const data =
          benchmarks::exahype2::kernelbenchmarks::DataRepository::getInstance().getCellQ(marker.getX(), marker.getH());

        constexpr int TotalPoints = BasePowerExponent<NumberOfDofsPerAxis, DIMENSIONS>::value;

        int vertexIndices[TotalPoints];

        int counter         = 0;
        int linearisedIndex = 0;
        peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
          vertexIndices[counter] = vertexPlotter->plotVertex(
            exahype2::dg::getQuadraturePoint(
              marker.getX(),
              marker.getH(),
              index,
              NumberOfDofsPerAxis,
              benchmarks::exahype2::kernelbenchmarks::kernels::AderSolver::Quadrature<SolverStoragePrecision>::nodes
            )
          );

          double values[benchmarks::exahype2::kernelbenchmarks::AbstractAderSolver::NumberOfUnknowns];
          for (int unknown = 0; unknown < benchmarks::exahype2::kernelbenchmarks::AbstractAderSolver::NumberOfUnknowns;
               unknown++) {
            values[unknown] = data[linearisedIndex + unknown];
          }
          vertexDataPlotter->plotVertexData(
            vertexIndices[counter],
            values,
            benchmarks::exahype2::kernelbenchmarks::AbstractAderSolver::NumberOfUnknowns
          );

          counter++;
          linearisedIndex += NumberOfData;
        });

#if DIMENSIONS == 3
        for (int k = 0; k < NumberOfDofsPerAxis - 1; k++) {
#endif
          for (int j = 0; j < NumberOfDofsPerAxis - 1; j++) {
            for (int i = 0; i < NumberOfDofsPerAxis - 1; i++) {
              int idx = i + j * NumberOfDofsPerAxis
#if DIMENSIONS == 3
                + k * NumberOfDofsPerAxis * NumberOfDofsPerAxis
#endif
                ;

              int s_x = 1;
              int s_y = NumberOfDofsPerAxis;

#if DIMENSIONS == 2
              int v[4];
              v[0]                                 = vertexIndices[idx];
              v[1]                                 = vertexIndices[idx + s_x];
              v[2]                                 = vertexIndices[idx + s_x + s_y];
              v[3]                                 = vertexIndices[idx + s_y];
              [[maybe_unused]] const int cellIndex = cellPlotter->plotQuadrangle(v);
#elif DIMENSIONS == 3
            int s_z = NumberOfDofsPerAxis * NumberOfDofsPerAxis;
            int v[8];
            v[0]                                 = vertexIndices[idx];
            v[1]                                 = vertexIndices[idx + s_x];
            v[2]                                 = vertexIndices[idx + s_x + s_y];
            v[3]                                 = vertexIndices[idx + s_y];
            v[4]                                 = vertexIndices[idx + s_z];
            v[5]                                 = vertexIndices[idx + s_x + s_z];
            v[6]                                 = vertexIndices[idx + s_x + s_y + s_z];
            v[7]                                 = vertexIndices[idx + s_y + s_z];
            [[maybe_unused]] const int cellIndex = cellPlotter->plotHexahedron(v);
#endif
            }
          }
#if DIMENSIONS == 3
        }
#endif
      }
    }

    timeSeriesWriter.writeSnapshotFile(solutionFileName, plotTimeStamp, fileWriter);
    timeSeriesWriter.writeTimeSeriesMetaFile(outputDirectory, solutionFileName);
    benchmarks::exahype2::kernelbenchmarks::repositories::finishPlottingStep();
  }

  void applyInitialConditions() {
    for (const auto& marker : cells) {
      if (not marker.hasBeenRefined()) {
        auto* const patchData =
          benchmarks::exahype2::kernelbenchmarks::DataRepository::getInstance().getCellQ(marker.getX(), marker.getH());
        int linearisedIndex = 0;
        peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
          benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver.initialCondition(
            patchData + linearisedIndex,
            exahype2::dg::getQuadraturePoint(
              marker.getX(),
              marker.getH(),
              index,
              NumberOfDofsPerAxis,
              benchmarks::exahype2::kernelbenchmarks::kernels::AderSolver::Quadrature<SolverStoragePrecision>::nodes
            ),
            marker.getH()
          );
          linearisedIndex += NumberOfData;
        });
      }
    }
  }

  peano4::datamanagement::CellMarker createMarkerFromIndex(const tarch::la::Vector<DIMENSIONS, int>& index) {
    const auto                            cellSize  = getCellSize();
    tarch::la::Vector<DIMENSIONS, double> cellIndex = tarch::la::convertScalar<double>(index);
    tarch::la::Vector<DIMENSIONS, double> cellPositionBase =
      benchmarks::exahype2::kernelbenchmarks::DomainOffset + cellSize / 2.0;
    tarch::la::Vector<DIMENSIONS, double> cellPosition =
      cellPositionBase + tarch::la::multiplyComponents(cellIndex, cellSize);
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

  tarch::instrumentation::Measurement kernelMeasurement(1.0);
  tarch::instrumentation::Measurement boundaryMeasurement(1.0);
  tarch::instrumentation::Measurement predictorMeasurement(1.0);
  tarch::instrumentation::Measurement riemannMeasurement(1.0);
  tarch::instrumentation::Measurement correctorMeasurement(1.0);
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
        and (benchmarks::exahype2::kernelbenchmarks::repositories::getMinTimeStamp() >= nextMinPlotTimeStamp or benchmarks::exahype2::kernelbenchmarks::repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp)
        and benchmarks::exahype2::kernelbenchmarks::repositories::mayPlot()) {
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
    timeStepWatch.start();
    benchmarks::exahype2::kernelbenchmarks::repositories::startTimeStep();

    const double timeStepSize =
      benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver.getAdmissibleTimeStepSize();

    tarch::instrumentation::Watch kernelWatch("", "", false);
    kernelWatch.start();

    tarch::instrumentation::Watch predictorWatch("", "", false);
    predictorWatch.start();
    benchmarks::exahype2::kernelbenchmarks::predictorFused(
      benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver,
      fusedGrid.getQ(),
      fusedGrid.getQOld(),
      fusedGrid.getFaceEstimatesPool(),
      fusedGrid.getFaceFluxesPool(),
      fusedGrid.getFaceIndices(),
      timeStamp,
      timeStepSize,
      fusedGrid.getCellCentres()[0].data(),
      fusedGrid.getCellSizes()[0].data(),
      fusedGrid.getNumberFusedCells(),
      0
    );
    predictorWatch.stop();

    tarch::instrumentation::Watch boundaryWatch("", "", false);
    boundaryWatch.start();
    benchmarks::exahype2::kernelbenchmarks::handleBoundaryFused(
      benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver,
      fusedGrid.getFaceEstimatesPool(),
      fusedGrid.getFaceFluxesPool(),
      fusedGrid.getBoundaryFaceIndices(),
      fusedGrid.getBoundaryFaceNumbers(),
      fusedGrid.getBoundaryCellIndices(),
      fusedGrid.getCellCentres(),
      fusedGrid.getCellSizes(),
      timeStamp,
      timeStepSize,
      fusedGrid.getNumberOfBoundaryFaces(),
      fusedGrid.getNumberFusedCells(),
      0
    );
    boundaryWatch.stop();

    tarch::instrumentation::Watch riemannWatch("", "", false);
    riemannWatch.start();
    benchmarks::exahype2::kernelbenchmarks::riemannFused(
      benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver,
      fusedGrid.getFaceEstimatesPool(),
      fusedGrid.getFaceFluxesPool(),
      fusedGrid.getUniqueFaceIndices(),
      fusedGrid.getFaceNumbers(),
      fusedGrid.getFaceCellIndices(),
      timeStamp,
      timeStepSize,
      fusedGrid.getCellCentres()[0].data(),
      fusedGrid.getCellSizes()[0].data(),
      fusedGrid.getNumberOfFaces(),
      0
    );
    riemannWatch.stop();

    tarch::instrumentation::Watch correctorWatch("", "", false);
    correctorWatch.start();
    benchmarks::exahype2::kernelbenchmarks::correctorFused(
      benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver,
      fusedGrid.getQ(),
      fusedGrid.getFaceFluxesPool(),
      fusedGrid.getFaceIndices(),
      timeStamp,
      timeStepSize,
      fusedGrid.getCellCentres()[0].data(),
      fusedGrid.getCellSizes()[0].data(),
      fusedGrid.getNumberFusedCells(),
      0
    );
    correctorWatch.stop();
    kernelWatch.stop();

    const auto newTimeStepSize =
      benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver.getAdmissibleTimeStepSize();
    benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver
      .update(newTimeStepSize, timeStamp + timeStepSize, fusedGrid.getMinVolumeH(), fusedGrid.getMaxVolumeH(), false);

    timeStamp += benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfAderSolver.getAdmissibleTimeStepSize();
    benchmarks::exahype2::kernelbenchmarks::repositories::finishTimeStep();

    timeStepWatch.stop();

    if (benchmarks::exahype2::kernelbenchmarks::repositories::mayPlot()) {
      kernelMeasurement.setValue(kernelWatch.getCalendarTime());
      boundaryMeasurement.setValue(boundaryWatch.getCalendarTime());
      predictorMeasurement.setValue(predictorWatch.getCalendarTime());
      riemannMeasurement.setValue(riemannWatch.getCalendarTime());
      correctorMeasurement.setValue(correctorWatch.getCalendarTime());
      timeStepMeasurement.setValue(timeStepWatch.getCalendarTime());
      numberOfCompletedTimeSteps++;

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
      logError("Non-critical assertion has been triggered in code. Dump final state and terminate.");
    }

  } // continueToSolve

  if constexpr (benchmarks::exahype2::kernelbenchmarks::ShouldPlot) {
    tarch::instrumentation::Watch plotWatch("", "", false);
    plotWatch.start();
    fusedGrid.syncCellsDeviceToHost();
    plotGrid();
    plotWatch.stop();
    plotMeasurement.setValue(plotWatch.getCalendarTime());
    logInfo(formatPlot(plotMeasurement));
  }
  benchmarks::exahype2::kernelbenchmarks::repositories::finishSimulation();

  if (haveReceivedNonCriticalAssertion) {
    logWarning("Terminated gracefully after noncritical assertion");
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
    logInfo("Terminated successfully");
    returnCode = EXIT_SUCCESS;
  }

  logFinalStatistics(
    kernelMeasurement,
    boundaryMeasurement,
    predictorMeasurement,
    riemannMeasurement,
    correctorMeasurement
  );

  fusedGrid.clear();
  peano4::shutdown();
  tarch::shutdown();
  return returnCode;
}
