// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarks-main.h"

#include "AbstractFVSolver.h"
#include "Constants.h"
#include "DataRepository.h"
#include "EnclaveDataRepository.h"
#include "FVSolver.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/RuntimeParameters.h"
#include "exahype2/fv/BoundaryConditions.h"
#include "exahype2/fv/PatchUtils.h"
#include "kernels/FVSolver/HandleBoundaryFused.h"
#include "kernels/FVSolver/ProjectPatchFused.h"
#include "kernels/FVSolver/ReconstructPatch.h"
#include "kernels/FVSolver/ReconstructPatchFused.h"
#include "kernels/FVSolver/RiemannSolverFused.h"
#include "peano4/Globals.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/peano4.h"
#include "peano4/utils/Loop.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Measurement.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/plotter/AMRPlotter.h"
#include "tarch/plotter/PVDTimeSeriesWriter.h"
#include "tarch/plotter/VTUFileWriter.h"
#include "tarch/tarch.h"
#include "toolbox/blockstructured/Interpolation.h"
#include "toolbox/blockstructured/Projection.h"
#include "toolbox/blockstructured/Restriction.h"

#include <limits>
#include <string>

using namespace benchmarks::exahype2::kernelbenchmarks;

namespace {
  tarch::instrumentation::Measurement timeStepMeasurement;
  double                              logNextTimeStep                  = 0.0;
  bool                                haveReceivedNonCriticalAssertion = false;

  std::vector<peano4::datamanagement::CellMarker> _cells;

  int getGridLength() {
    int result = 1;
    for (int level = 0; level < ::exahype2::RuntimeParameters::getInstance().getGridLevels(); ++level) {
      result *= 3;
    }
    return result;
  }

  tarch::la::Vector<DIMENSIONS, double> getCellSize() {
    return DomainSize / static_cast<double>(getGridLength());
  }

  bool helpRequested(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
      const std::string argument = argv[index];
      if (argument == "--help" or argument == "-h") {
        return true;
      }
    }
    return false;
  }

  template <typename T>
  std::string toScientificNotationString(T value, int precision = 4, bool considerSign = false, double threshold = 1e6) {
    std::ostringstream out;
    double             v = static_cast<double>(value);

    if (std::abs(v) < threshold) {
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
    out << v;
    return out.str();
  }

  template <typename Value>
  std::string formatStatistic(const std::string& label, const Value& value) {
    std::ostringstream result;
    result << "  " << std::left << std::setw(28) << label << value;
    return result.str();
  }

  std::string formatSimulationTime() {
    std::ostringstream result;
    result << repositories::getMinTimeStamp() << " / " << ::exahype2::RuntimeParameters::getInstance().getMinTerminalTime();
    return formatStatistic("Time", result.str());
  }

  std::string formatTimeStep(int numberOfCompletedTimeSteps, const tarch::instrumentation::Measurement& measurement) {
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

  std::string formatMeasurementTotal(const tarch::instrumentation::Measurement& measurement, const char* unit) {
    std::ostringstream result;
    result
      << "Total=" << measurement.getAccumulatedValue() << unit
      << ", Measurements=" << measurement.getNumberOfMeasurements();
    return result.str();
  }

  std::vector<std::string> formatSolverStatistics() {
    std::vector<std::string> result;
    result.push_back(formatSimulationTime());
    for (const auto& statistic : repositories::getSolverStatistics()) {
      result.push_back(formatStatistic(statistic.first, statistic.second));
    }
    return result;
  }

  template <typename Bytes>
  std::vector<std::string> formatKernelBenchmarkStatistics(
    std::size_t                                globalNumberOfPatches,
    double                                     totalNumberOfDoFs,
    Bytes                                      totalPersistentMemory,
    const tarch::instrumentation::Measurement& measurement
  ) {
    std::vector<std::string> result;

    std::ostringstream gridMessage;
    gridMessage
      << "Patches=" << toScientificNotationString(globalNumberOfPatches)
      << ", DoFs=" << toScientificNotationString(totalNumberOfDoFs);
    result.push_back(formatStatistic("Grid", gridMessage.str()));

    const auto         memoryFormat = tarch::chooseMemoryUsageFormat(totalPersistentMemory);
    std::ostringstream memoryMessage;
    memoryMessage
      << "Total=" << std::fixed << std::setprecision(2) << tarch::convertMemorySize(totalPersistentMemory, memoryFormat)
      << " " << tarch::toString(memoryFormat);
    result.push_back(formatStatistic("Memory", memoryMessage.str()));

    if (measurement.getNumberOfMeasurements() > 0) {
      const auto dt               = measurement.getMostRecentValue();
      const auto patchesPerSecond = globalNumberOfPatches / dt;
      const auto dofsPerSecond    = totalNumberOfDoFs / dt;

      std::ostringstream performanceMessage;
      performanceMessage << patchesPerSecond << " Patch/s, " << dofsPerSecond << " DoF/s";
      result.push_back(formatStatistic("Performance", performanceMessage.str()));
    }

    return result;
  }
} // namespace

void logFinalStatistics(
  const tarch::instrumentation::Measurement& measurement,
  const tarch::instrumentation::Measurement& measurementBoundary,
  const tarch::instrumentation::Measurement& measurementReconstruct,
  const tarch::instrumentation::Measurement& measurementRiemann,
  const tarch::instrumentation::Measurement& measurementProjection
) {
  if (measurementBoundary.getNumberOfMeasurements() > 0) {
    logInfo(formatStatistic("Boundary Handling", formatMeasurementTotal(measurementBoundary, "s")));
  }
  if (measurementReconstruct.getNumberOfMeasurements() > 0) {
    logInfo(formatStatistic("Reconstruction", formatMeasurementTotal(measurementReconstruct, "s")));
  }
  if (measurementRiemann.getNumberOfMeasurements() > 0) {
    logInfo(formatStatistic("Riemann Solver", formatMeasurementTotal(measurementRiemann, "s")));
  }
  if (measurementProjection.getNumberOfMeasurements() > 0) {
    logInfo(formatStatistic("Projection", formatMeasurementTotal(measurementProjection, "s")));
  }

  if (measurement.getNumberOfMeasurements() > 0) {
    logInfo(formatStatistic("Time Stepping", formatMeasurementTotal(measurement, "s")));

    const auto globalNumberOfPatches = _cells.size();
    const auto totalNumberOfDoFs     = globalNumberOfPatches
                                   * std::pow(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS)
                                   * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables);
    const auto averageStepTime = measurement.getAverageValue();

    std::ostringstream performanceMessage;
    performanceMessage
      << "Average=" << globalNumberOfPatches / averageStepTime << " Patch/s, " << totalNumberOfDoFs / averageStepTime
      << " DoF/s";
    logInfo(formatStatistic("Performance", performanceMessage.str()));
  }
}

void plotGrid() {
  repositories::startPlottingStep();

  const std::string outputDirectory  = "solutions";
  const std::string solutionFileName = "FVSolver";

  const double t = 0.5 * (repositories::getMinTimeStamp() + repositories::getMaxTimeStamp());

  static tarch::plotter::PVDTimeSeriesWriter timeSeriesWriter;
  tarch::plotter::VTUFileWriter              fileWriter(outputDirectory, 1);
  tarch::plotter::AMRPlotter                 amrPlotter(fileWriter, 0, 0.5, 0.0);

  auto cellDataPlotter = fileWriter.createCellDataPlotter(0, "FVSolverQ", AbstractFVSolver::NumberOfUnknowns);

  for (auto& marker : _cells) {
    if (not marker.hasBeenRefined()) {
      const auto& patchSize   = marker.getH();
      const auto& patchOffset = marker.getX() - patchSize * 0.5;

      const auto  cellSize = patchSize / static_cast<double>(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch);
      const auto* QOut     = DataRepository::getInstance().getCellQ(marker.getX(), marker.getH());

      int currentDoF = 0;
      dfor(k, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch) {
#if DIMENSIONS == 2
        tarch::la::Vector<2, double> cellOffset;
        cellOffset(0)       = patchOffset(0) + static_cast<double>(k(0)) * cellSize(0);
        cellOffset(1)       = patchOffset(1) + static_cast<double>(k(1)) * cellSize(1);
        const int cellIndex = amrPlotter.plotCell2D(cellOffset, cellSize, 0);
#else
        tarch::la::Vector<3, double> cellOffset;
        cellOffset(0)       = patchOffset(0) + static_cast<double>(k(0)) * cellSize(0);
        cellOffset(1)       = patchOffset(1) + static_cast<double>(k(1)) * cellSize(1);
        cellOffset(2)       = patchOffset(2) + static_cast<double>(k(2)) * cellSize(2);
        const int cellIndex = amrPlotter.plotCell3D(cellOffset, cellSize, 0);
#endif

        const auto* values = QOut + currentDoF;
        cellDataPlotter->plotCellData(cellIndex, values, AbstractFVSolver::NumberOfUnknowns);

        currentDoF += AbstractFVSolver::NumberOfUnknowns;
      }
    }
  }

  timeSeriesWriter.writeSnapshotFile(solutionFileName, t, fileWriter);
  timeSeriesWriter.writeTimeSeriesMetaFile(outputDirectory, solutionFileName);

  repositories::finishPlottingStep();
}

void interpolateBetweenAMRBorder() {
  const auto cellSize = getCellSize();
  for (auto& marker : _cells) {
    if (marker.getH()[0] < cellSize[0]) { // Check if cell is in AMR region
      auto parentSize   = marker.getH() * 3.0;
      auto parentCenter = marker.getInvokingParentCellsCentre();

      for (int f = 0; f < TWO_TIMES_D; f++) {
        int normal = f % DIMENSIONS;
        if ((f >= DIMENSIONS and marker.getRelativePositionWithinFatherCell()[normal] == 2)
            or (f < DIMENSIONS and marker.getRelativePositionWithinFatherCell()[normal] == 0)) {
          peano4::datamanagement::FaceMarker faceMarker{};
          faceMarker.relativePositionWithinFatherCell = marker.getRelativePositionWithinFatherCell();
          faceMarker.selectedFace                     = f;
          double* fineGridFace   = DataRepository::getInstance().getFaceQNew(marker.getX(), marker.getH(), f);
          double* coarseGridFace = DataRepository::getInstance().getFaceQNew(parentCenter, parentSize, f);
          toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_inject(
            faceMarker,
            AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
            1, // Halo size
            AbstractFVSolver::NumberOfUnknowns,
            fineGridFace,
            coarseGridFace,
            false
          );
        }
      }
    }
  }
  for (auto& marker : _cells) {
    if (marker.getH()[0] < cellSize[0]) { // Check if cell is in AMR region
      auto parentCenter = marker.getInvokingParentCellsCentre();
      auto parentSize   = marker.getH() * 3.0;

      for (int f = 0; f < TWO_TIMES_D; f++) {
        int normal = f % DIMENSIONS;
        if ((f >= DIMENSIONS and marker.getRelativePositionWithinFatherCell()[normal] == 2)
            or (f < DIMENSIONS and marker.getRelativePositionWithinFatherCell()[normal] == 0)) {
          peano4::datamanagement::FaceMarker faceMarker{};
          faceMarker.relativePositionWithinFatherCell = marker.getRelativePositionWithinFatherCell();
          faceMarker.selectedFace                     = f;
          double* fineGridFace   = DataRepository::getInstance().getFaceQNew(marker.getX(), marker.getH(), f);
          double* coarseGridFace = DataRepository::getInstance().getFaceQNew(parentCenter, parentSize, f);
          toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
            faceMarker,
            AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
            1, // Halo size
            AbstractFVSolver::NumberOfUnknowns,
            coarseGridFace,
            fineGridFace
          );
        }
      }
    }
  }
}

void applyInitialConditions() {
  for (auto& marker : _cells) {
    if (not marker.hasBeenRefined()) {
      double* QOut = DataRepository::getInstance().getCellQ(marker.getX(), marker.getH());
      // Initialise cell data with initial conditions.
      dfor(volume, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch) {
        int linearIndex = peano4::utils::dLinearised(volume, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch)
                          * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables);
        FVSolver::initialCondition(
          QOut + linearIndex,
          ::exahype2::fv::getVolumeCentre(
            marker.getX(),
            marker.getH(),
            AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
            volume
          ),
          ::exahype2::fv::getVolumeSize(marker.getH(), AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch)
        );
      }
    }
  }
}

peano4::datamanagement::CellMarker createMarkerFromIndex(const tarch ::la ::Vector<DIMENSIONS, int>& index) {
  const auto cellSize = getCellSize();
  tarch::la::Vector<DIMENSIONS, double> cellIndex        = tarch::la::convertScalar<double>(index);
  tarch::la::Vector<DIMENSIONS, double> cellPositionBase = DomainOffset + cellSize / 2.0;
  tarch::la::Vector<DIMENSIONS, double>
    cellPosition = cellPositionBase + tarch::la::multiplyComponents(cellIndex, cellSize);
  peano4::datamanagement::CellMarker marker;
  marker.h      = cellSize;
  marker.centre = cellPosition;
  return marker;
}

std::vector<peano4::datamanagement::CellMarker> createRefinedMarkersFromIndex(
  const tarch ::la ::Vector<DIMENSIONS, int>& index
) {
  const auto cellSize = getCellSize();
  tarch::la::Vector<DIMENSIONS, double> cellIndex              = tarch::la::convertScalar<double>(index);
  tarch::la::Vector<DIMENSIONS, double> deeperCellIndex        = cellIndex * 3.0;
  tarch::la::Vector<DIMENSIONS, double> deeperCellSize         = cellSize / 3.0;
  tarch::la::Vector<DIMENSIONS, double> cellPositionBase       = DomainOffset + cellSize / 2.0;
  tarch::la::Vector<DIMENSIONS, double> deepercellPositionBase = DomainOffset + deeperCellSize / 2.0;

  std::vector<peano4::datamanagement::CellMarker> result;

  tarch::la::Vector<DIMENSIONS, double>
    cellPosition = cellPositionBase + tarch::la::multiplyComponents(cellIndex, cellSize);
  peano4::datamanagement::CellMarker marker;
  marker.centre = cellPosition;
  marker.h      = cellSize;
  marker.flags |= peano4::datamanagement::CellMarker::HasBeenRefined;
  result.push_back(marker);

  dfor(offset, 3) {
    tarch::la::Vector<DIMENSIONS, double> deepCellOffset = tarch::la::convertScalar<double>(offset);
    tarch::la::Vector<DIMENSIONS, double> localCellIndex = deepCellOffset + deeperCellIndex;
    tarch::la::Vector<DIMENSIONS, double>
      deeperCellPosition = deepercellPositionBase + tarch::la::multiplyComponents(localCellIndex, deeperCellSize);
    peano4::datamanagement::CellMarker marker;
    marker.centre                           = deeperCellPosition;
    marker.h                                = deeperCellSize;
    marker.relativePositionWithinFatherCell = offset;
    result.push_back(marker);
  }
  return result;
}

void createGrid() {
  repositories::startGridConstructionStep();
  const int gridLength = getGridLength();
  _cells.reserve(std::pow(gridLength, DIMENSIONS));
  tarch::la::Vector<DIMENSIONS, double> DomainCenter = DomainOffset + DomainSize / 2.0;
  dfor(volume, gridLength) {
    peano4::datamanagement::CellMarker marker = createMarkerFromIndex(volume);
    if (UseAMR and tarch::la::norm2(marker.getX() - DomainCenter) < 0.1) {
      auto markers = createRefinedMarkersFromIndex(volume);
      _cells.insert(std::end(_cells), std::begin(markers), std::end(markers));
    } else {
      _cells.push_back(marker);
    }
  }
  repositories::finishGridConstructionStep();
}

void initGrid() {
  repositories::startGridInitialisationStep();
  applyInitialConditions();
  repositories::finishGridInitialisationStep();
}

int main(int argc, char** argv) {
  int returnCode = EXIT_SUCCESS;

  auto& runtimeParameters = ::exahype2::RuntimeParameters::getInstance();
  runtimeParameters.setGeneratedDefaults(
    BenchmarkMinTerminalTime,
    BenchmarkMaxTerminalTime,
    BenchmarkGridLevels,
    BenchmarkMaxGridLevels
  );
  if (helpRequested(argc, argv)) {
    runtimeParameters.printUsage("KernelBenchmarks");
    return EXIT_SUCCESS;
  }
  runtimeParameters.configure(argc, argv);

  peano4::init(&argc, &argv, DomainOffset, DomainSize, PeriodicBC);

  repositories::loadBalancer.enable(false);
  repositories::startSimulation();

  createGrid();
  initGrid();

  EnclaveDataRepository& enclaveGrid = EnclaveDataRepository::getInstance();
  enclaveGrid.startCollection();
  for (auto& marker : _cells) {
    if (not marker.hasBeenRefined()) {
      enclaveGrid.bookmarkCell(marker.getX(), marker.getH());
    }
  }

  enclaveGrid.buildFromBookmarks();

  benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::projectPatchFused(
    enclaveGrid.getQ(),
    enclaveGrid.getFacePool(),
    enclaveGrid.getFaceIndices(),
    enclaveGrid.getNumberOfEnclaveCells(),
    0
  );

  tarch::instrumentation::Measurement measurement(1.0);
  tarch::instrumentation::Measurement measurementBoundary(1.0);
  tarch::instrumentation::Measurement measurementReconstruct(1.0);
  tarch::instrumentation::Measurement measurementRiemann(1.0);
  tarch::instrumentation::Measurement measurementProjection(1.0);
  tarch::instrumentation::Measurement plotMeasurement(1.0);
  tarch::instrumentation::Watch       timeStepWatch("::", "main()", false);
  double                              timeStamp                            = 0.0;
  double                              nextMinPlotTimeStamp                 = runtimeParameters.getFirstPlotTimeStamp();
  double                              nextMaxPlotTimeStamp                 = runtimeParameters.getFirstPlotTimeStamp();
  bool                                continueToSolve                      = true;
  int                                 count                                = 0;
  int                                 numberOfCompletedTimeSteps           = 0;
  int                                 lastLoggedNumberOfCompletedTimeSteps = 0;
  const double                        minTerminalTime                      = runtimeParameters.getMinTerminalTime();
  const double                        maxTerminalTime                      = runtimeParameters.getMaxTerminalTime();
  const double                        plottingEndTime                      = maxTerminalTime < std::numeric_limits<double>::max() / 10.0 ? maxTerminalTime : minTerminalTime;
  const int                           numberOfSnapshots                    = runtimeParameters.getNumberOfSnapshots();
  const double                        timeInBetweenPlots                   = numberOfSnapshots <= 0 ? 0.0 : plottingEndTime / numberOfSnapshots;

  while (continueToSolve) {
    if (
      timeInBetweenPlots > 0.0
      and (repositories::getMinTimeStamp() >= nextMinPlotTimeStamp
           or repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp)
    ) {
      if (repositories::getMinTimeStamp() >= nextMinPlotTimeStamp) {
        nextMinPlotTimeStamp += timeInBetweenPlots;
      }
      if (repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp) {
        nextMaxPlotTimeStamp += timeInBetweenPlots;
      }

      if constexpr (ShouldPlot) {
        tarch::instrumentation::Watch plotWatch("", "", false);
        plotWatch.start();
        enclaveGrid.syncCellsDeviceToHost();
        plotGrid();
        plotWatch.stop();
        plotMeasurement.setValue(plotWatch.getCalendarTime());
        logInfo(formatPlot(plotMeasurement));
      }
    }
    if (UseAMR) {
      enclaveGrid.syncFacesDeviceToHost();
      interpolateBetweenAMRBorder();
      enclaveGrid.syncFacesHostToDevice();
      // Hack to make static AMR work in the mini-app. The full-app uses face updates that get interpolated between
      // coarse and fine faces. In the mini-app we do interpolation a bit different because we only use one face type to
      // keep things simple. We therefore first restrict the fine grid faces into the coarse grid faces, we do this for
      // all faces that have been refined. We then interpolate these coarse grid faces onto the fine grid faces. This is
      // needed at the AMR boundary, but not at the internal faces of the refined region. I have currently implemented
      // no way to distinguish between internal faces in the AMR region and those at the boundary. The interpolation
      // step therefore introduces a loss of precision at the internal faces of the AMR region. We can circumvent this
      // by reprojecting the cell data onto the faces for all cells. This is less than ideal and should be improved.
      benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::projectPatchFused(
        enclaveGrid.getQ(),
        enclaveGrid.getFacePool(),
        enclaveGrid.getFaceIndices(),
        enclaveGrid.getNumberOfEnclaveCells(),
        0
      );
    }

    timeStepWatch.start();
    repositories::startTimeStep();

    double dt = repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();

    tarch::instrumentation::Watch watch("", "", false);
    watch.start();

    tarch::instrumentation::Watch watchBoundary("", "", false);
    watchBoundary.start();
    benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::handleBoundaryFused(
      repositories::instanceOfFVSolver,
      enclaveGrid.getFacePool(),
      enclaveGrid.getBoundaryFaceIndices(),
      enclaveGrid.getBoundaryFaceNumbers(),
      enclaveGrid.getBoundaryCellIndices(),
      enclaveGrid.getCellCentres(),
      enclaveGrid.getCellSizes(),
      timeStamp,
      dt,
      enclaveGrid.getNumberOfBoundaryFaces(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchBoundary.stop();

    tarch::instrumentation::Watch watchReconstruct("", "", false);
    watchReconstruct.start();
    benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::reconstructPatchFused(
      enclaveGrid.getQ(),
      enclaveGrid.getFacePool(),
      enclaveGrid.getFaceIndices(),
      enclaveGrid.getQReconstructed(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchReconstruct.stop();

    tarch::instrumentation::Watch watchRiemann("", "", false);
    watchRiemann.start();
    const auto maxEigenvalue = benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::riemannSolverFused(
      repositories::instanceOfFVSolver,
      enclaveGrid.getQReconstructed(),
      enclaveGrid.getQ(),
      timeStamp,
      dt,
      enclaveGrid.getCellCentres()[0].data(),
      enclaveGrid.getCellSizes()[0].data(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchRiemann.stop();

    tarch::instrumentation::Watch watchProjection("", "", false);
    watchProjection.start();
    benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::projectPatchFused(
      enclaveGrid.getQ(),
      enclaveGrid.getFacePool(),
      enclaveGrid.getFaceIndices(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchProjection.stop();
    watch.stop();

    repositories::instanceOfFVSolver.setMaxEigenvalue(maxEigenvalue);
    const auto newTimeStepSize = repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();
    repositories::instanceOfFVSolver
      .update(newTimeStepSize, timeStamp + dt, enclaveGrid.getMinVolumeH(), enclaveGrid.getMaxVolumeH());

    timeStamp += repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();
    repositories::finishTimeStep();

    timeStepWatch.stop();

    if (repositories::mayPlot()) {
      measurement.setValue(watch.getCalendarTime());
      measurementBoundary.setValue(watchBoundary.getCalendarTime());
      measurementReconstruct.setValue(watchReconstruct.getCalendarTime());
      measurementRiemann.setValue(watchRiemann.getCalendarTime());
      measurementProjection.setValue(watchProjection.getCalendarTime());
      timeStepMeasurement.setValue(timeStepWatch.getCalendarTime());
      numberOfCompletedTimeSteps++;

      if (timeStepMeasurement.getAccumulatedValue() >= logNextTimeStep) {
        const auto totalPersistentMemory = DataRepository::getInstance().getMemoryUsageInBytes();

        const auto globalNumberOfPatches = _cells.size();
        const auto totalNumberOfDoFs
          = globalNumberOfPatches * std::pow(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS)
            * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables);

        logInfo(formatTimeStep(numberOfCompletedTimeSteps, timeStepMeasurement));
        for (const auto& statistic : formatSolverStatistics()) {
          logInfo(statistic);
        }
        for (const auto& statistic : formatKernelBenchmarkStatistics(
               globalNumberOfPatches,
               totalNumberOfDoFs,
               totalPersistentMemory,
               measurement
             )) {
          logInfo(statistic);
        }
        lastLoggedNumberOfCompletedTimeSteps = numberOfCompletedTimeSteps;
        logNextTimeStep += runtimeParameters.getLogIntervalInSeconds();
      }

      if (Samples > 0 and ++count >= Samples) {
        continueToSolve = false;
      }
    }

    if (repositories::getMinTimeStamp() >= minTerminalTime and repositories::getMaxTimeStamp() >= maxTerminalTime) {
      continueToSolve = false;
    }

    if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNonCriticalAssertion) {
      continueToSolve                  = false;
      haveReceivedNonCriticalAssertion = true;
      logError("Noncritical assertion has been triggered in code. Dump final state and terminate.");
    }
  }

  // Plot last timestep
  if constexpr (ShouldPlot) {
    tarch::instrumentation::Watch plotWatch("", "", false);
    plotWatch.start();
    enclaveGrid.syncCellsDeviceToHost();
    plotGrid();
    plotWatch.stop();
    plotMeasurement.setValue(plotWatch.getCalendarTime());
    logInfo(formatPlot(plotMeasurement));
  }
  repositories::finishSimulation();

  if (haveReceivedNonCriticalAssertion) {
    logWarning("Terminated gracefully after noncritical assertion");
    returnCode = EXIT_FAILURE;
  } else {
    // Log final timestep information
    if (numberOfCompletedTimeSteps > lastLoggedNumberOfCompletedTimeSteps) {
      logInfo(formatTimeStep(numberOfCompletedTimeSteps, timeStepMeasurement));
      for (const auto& statistic : formatSolverStatistics()) {
        logInfo(statistic);
      }
      const auto totalPersistentMemory = DataRepository::getInstance().getMemoryUsageInBytes();
      const auto globalNumberOfPatches = _cells.size();
      const auto totalNumberOfDoFs
        = globalNumberOfPatches * std::pow(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS)
          * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables);
      for (const auto& statistic : formatKernelBenchmarkStatistics(
             globalNumberOfPatches,
             totalNumberOfDoFs,
             totalPersistentMemory,
             measurement
           )) {
        logInfo(statistic);
      }
    }
    logInfo("Terminated successfully");
    returnCode = EXIT_SUCCESS;
  }

  logFinalStatistics(
    measurement,
    measurementBoundary,
    measurementReconstruct,
    measurementRiemann,
    measurementProjection
  );

  enclaveGrid.clear();

  peano4::shutdown();

  return returnCode;
}
