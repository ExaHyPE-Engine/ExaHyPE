// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelsOnDeviceTest.h"

#include "exahype2/dg/ader/PointSources.h"
#include "exahype2/dg/ader/SpaceTimePredictor.h"
#include "exahype2/dg/ader/tests/DeviceKernelFixtures.h"
#include "exahype2/dg/tests/DeviceKernelFixtures.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  using Setup            = exahype2::dg::ader::tests::DeviceKernelSetup;
  using NonlinearScratch = exahype2::dg::ader::NonlinearPredictorScratch<
    Setup::DofsPerAxis,
    Setup::Unknowns,
    Setup::AuxiliaryVariables,
    true,
    true,
    true,
    true,
    true>;
  using LinearScratch = exahype2::dg::ader::
    LinearPredictorScratch<Setup::DofsPerAxis, Setup::Unknowns, Setup::AuxiliaryVariables, true, true, true>;

  TARGET_DECLARE
  /**
   * Points a cell's face arrays into the flat buffers, one block per cell and face.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void collectFacePointers(
    const int     cell,
    double* const faceStates,
    double* const faceFluxes,
    double* (&faceState)[peano4::FacesPerCell],
    double* (&faceFlux)[peano4::FacesPerCell]
  ) {
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      const int slot        = cell * peano4::FacesPerCell + face;
      const int stateOffset = slot * Setup::FaceStateCardinality;
      const int fluxOffset  = slot * Setup::FaceFluxCardinality;
      faceState[face]       = faceStates + stateOffset;
      faceFlux[face]        = faceFluxes + fluxOffset;
    }
  }
  TARGET_DECLARE_END

  /**
   * Puts source zero at the centre of cell one and source one on the right face of cell four.
   */
  std::vector<double> makePointSourceLocations(const exahype2::dg::tests::CellChain& geometry) {
    constexpr int SourceZeroCell = 1;
    constexpr int SourceOneCell  = 4;
    constexpr int ZeroOffset     = SourceZeroCell * DIMENSIONS;
    constexpr int OneOffset      = SourceOneCell * DIMENSIONS;

    std::vector<double> locations(Setup::LocationCardinality);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      locations[axis]              = geometry.cellCentres[ZeroOffset + axis];
      locations[DIMENSIONS + axis] = geometry.cellCentres[OneOffset + axis];
    }
    locations[DIMENSIONS] += 0.5 * geometry.cellSizes[OneOffset];
    return locations;
  }
} // namespace

exahype2::dg::ader::tests::KernelsOnDeviceTest::KernelsOnDeviceTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::KernelsOnDeviceTest") {}

void exahype2::dg::ader::tests::KernelsOnDeviceTest::run() {
  testMethod(testSelectPointSources);
  testMethod(testNonlinearSpaceTimePredictor);
  testMethod(testLinearSpaceTimePredictor);
}

void exahype2::dg::ader::tests::KernelsOnDeviceTest::validateBuffersMatch(
  const std::vector<double>& deviceResult,
  const std::vector<double>& hostResult,
  const std::string&         bufferName
) {
  validateEquals(deviceResult.size(), hostResult.size());
  for (std::size_t entry = 0; entry < deviceResult.size(); entry++) {
    validateNumericalEqualsWithParams2(deviceResult[entry], hostResult[entry], bufferName, entry);
  }
}

void exahype2::dg::ader::tests::KernelsOnDeviceTest::testSelectPointSources() {
  const exahype2::dg::tests::CellChain geometry;
  const std::vector<double>            locations = makePointSourceLocations(geometry);
  const std::vector<int>               indices(Setup::AllIndicesCardinality, -1);
  const std::vector<int>               counts(Setup::NumberOfCells, -1);

  const exahype2::dg::tests::DeviceBuffer<double> deviceLocations(locations);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<int>    deviceIndices(indices);
  const exahype2::dg::tests::DeviceBuffer<int>    deviceCounts(counts);

  const double* const onDeviceLocations   = deviceLocations.data();
  const double* const onDeviceCellCentres = deviceCellCentres.data();
  const double* const onDeviceCellSizes   = deviceCellSizes.data();
  int* const          onDeviceIndices     = deviceIndices.data();
  int* const          onDeviceCounts      = deviceCounts.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell) {
      const int                                   coordinateOffset = cell * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(onDeviceCellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(onDeviceCellSizes + coordinateOffset);
      int                                         pointSourceIndices[Setup::NumberOfPointSources];
      for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
        pointSourceIndices[entry] = -1;
      }
      onDeviceCounts[cell] = exahype2::dg::ader::selectPointSources<Setup::NumberOfPointSources>(
        pointSourceIndices,
        onDeviceLocations,
        cellCentre,
        cellSize
      );
      const int indexOffset = cell * Setup::NumberOfPointSources;
      for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
        onDeviceIndices[indexOffset + entry] = pointSourceIndices[entry];
      }
    }
  );

  std::vector<int> deviceIndicesResult(indices.size());
  deviceIndices.copyBack(deviceIndicesResult);
  std::vector<int> deviceCountsResult(counts.size());
  deviceCounts.copyBack(deviceCountsResult);

  int selectedSources = 0;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const int                                   coordinateOffset = cell * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    int                                         pointSourceIndices[Setup::NumberOfPointSources];
    for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
      pointSourceIndices[entry] = -1;
    }
    const int count = exahype2::dg::ader::selectPointSources<Setup::NumberOfPointSources>(
      pointSourceIndices,
      locations.data(),
      cellCentre,
      cellSize
    );
    selectedSources += count;
    validateEqualsWithParams1(deviceCountsResult[cell], count, cell);
    const int indexOffset = cell * Setup::NumberOfPointSources;
    for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
      validateEqualsWithParams2(deviceIndicesResult[indexOffset + entry], pointSourceIndices[entry], cell, entry);
    }
  }
  validateEquals(selectedSources, Setup::NumberOfPointSources);
}

void exahype2::dg::ader::tests::KernelsOnDeviceTest::testNonlinearSpaceTimePredictor() {
  const exahype2::dg::ader::tests::BasisTableSet     tables;
  const exahype2::dg::ader::tests::DeviceBasisTables deviceTables(tables);
  const exahype2::dg::tests::CellChain               geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::makeRamp(Setup::AllCellStatesCardinality, 1.0, 0.003);
  const std::vector<double> faceStates(Setup::AllFaceStatesCardinality, 0.0);
  const std::vector<double> faceFluxes(Setup::AllFaceFluxesCardinality, 0.0);
  const std::vector<double> timeAveragedScratch(
    static_cast<std::size_t>(Setup::NumberOfCells) * NonlinearScratch::TimeAveragedSize,
    0.0
  );
  const std::vector<double> picardScratch(
    static_cast<std::size_t>(Setup::NumberOfCells) * NonlinearScratch::PicardSize,
    0.0
  );
  const std::vector<int> sweeps(Setup::NumberOfCells, 0);

  const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceFaceStates(faceStates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceFaceFluxes(faceFluxes);
  const exahype2::dg::tests::DeviceBuffer<double> deviceTimeAveragedScratch(timeAveragedScratch);
  const exahype2::dg::tests::DeviceBuffer<double> devicePicardScratch(picardScratch);
  const exahype2::dg::tests::DeviceBuffer<int>    deviceSweeps(sweeps);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

  const exahype2::dg::ader::BasisTables<double> onDeviceTables              = deviceTables.view();
  double* const                                 onDeviceCellStates          = deviceCellStates.data();
  double* const                                 onDeviceFaceStates          = deviceFaceStates.data();
  double* const                                 onDeviceFaceFluxes          = deviceFaceFluxes.data();
  double* const                                 onDeviceTimeAveragedScratch = deviceTimeAveragedScratch.data();
  double* const                                 onDevicePicardScratch       = devicePicardScratch.data();
  int* const                                    onDeviceSweeps              = deviceSweeps.data();
  const double* const                           onDeviceCellCentres         = deviceCellCentres.data();
  const double* const                           onDeviceCellSizes           = deviceCellSizes.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell) {
      double* faceState[peano4::FacesPerCell];
      double* faceFlux[peano4::FacesPerCell];
      collectFacePointers(cell, onDeviceFaceStates, onDeviceFaceFluxes, faceState, faceFlux);
      const int                                   coordinateOffset = cell * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(onDeviceCellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(onDeviceCellSizes + coordinateOffset);
      const int                                   stateOffset        = cell * Setup::CellStateCardinality;
      const int                                   timeAveragedOffset = cell * NonlinearScratch::TimeAveragedSize;
      const int                                   picardOffset       = cell * NonlinearScratch::PicardSize;
      onDeviceSweeps[cell]                                           = exahype2::dg::ader::spaceTimePredictor<
                                                  exahype2::dg::ader::tests::StubSolver,
                                                  Setup::DofsPerAxis,
                                                  Setup::Unknowns,
                                                  Setup::AuxiliaryVariables,
                                                  true,
                                                  true,
                                                  true,
                                                  true,
                                                  true,
                                                  double,
                                                  double,
                                                  double,
                                                  double>(
        faceState,
        faceFlux,
        onDeviceCellStates + stateOffset,
        onDeviceTimeAveragedScratch + timeAveragedOffset,
        onDevicePicardScratch + picardOffset,
        onDeviceTables,
        onDeviceTables,
        cellCentre,
        cellSize,
        Setup::TimeStamp,
        Setup::TimeStepSize,
        Setup::PicardTolerance
      );
    }
  );

  std::vector<double> deviceCellResult(cellStates.size());
  deviceCellStates.copyBack(deviceCellResult);
  std::vector<double> deviceFaceStateResult(faceStates.size());
  deviceFaceStates.copyBack(deviceFaceStateResult);
  std::vector<double> deviceFaceFluxResult(faceFluxes.size());
  deviceFaceFluxes.copyBack(deviceFaceFluxResult);
  std::vector<int> deviceSweepResult(sweeps.size());
  deviceSweeps.copyBack(deviceSweepResult);

  std::vector<double>                           hostCellStates          = cellStates;
  std::vector<double>                           hostFaceStates          = faceStates;
  std::vector<double>                           hostFaceFluxes          = faceFluxes;
  std::vector<double>                           hostTimeAveragedScratch = timeAveragedScratch;
  std::vector<double>                           hostPicardScratch       = picardScratch;
  const exahype2::dg::ader::BasisTables<double> hostTables              = tables.view();
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    double* faceState[peano4::FacesPerCell];
    double* faceFlux[peano4::FacesPerCell];
    collectFacePointers(cell, hostFaceStates.data(), hostFaceFluxes.data(), faceState, faceFlux);
    const int                                   coordinateOffset = cell * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    const int                                   stateOffset        = cell * Setup::CellStateCardinality;
    const int                                   timeAveragedOffset = cell * NonlinearScratch::TimeAveragedSize;
    const int                                   picardOffset       = cell * NonlinearScratch::PicardSize;
    const int                                   hostSweeps         = exahype2::dg::ader::spaceTimePredictor<
                                                exahype2::dg::ader::tests::StubSolver,
                                                Setup::DofsPerAxis,
                                                Setup::Unknowns,
                                                Setup::AuxiliaryVariables,
                                                true,
                                                true,
                                                true,
                                                true,
                                                true,
                                                double,
                                                double,
                                                double,
                                                double>(
      faceState,
      faceFlux,
      hostCellStates.data() + stateOffset,
      hostTimeAveragedScratch.data() + timeAveragedOffset,
      hostPicardScratch.data() + picardOffset,
      hostTables,
      hostTables,
      cellCentre,
      cellSize,
      Setup::TimeStamp,
      Setup::TimeStepSize,
      Setup::PicardTolerance
    );
    validateWithParams2(hostSweeps > 0, cell, hostSweeps);
    validateEqualsWithParams1(deviceSweepResult[cell], hostSweeps, cell);
  }

  validateBuffersMatch(deviceCellResult, hostCellStates, "cellStates");
  validateBuffersMatch(deviceFaceStateResult, hostFaceStates, "faceStates");
  validateBuffersMatch(deviceFaceFluxResult, hostFaceFluxes, "faceFluxes");
}

void exahype2::dg::ader::tests::KernelsOnDeviceTest::testLinearSpaceTimePredictor() {
  const exahype2::dg::ader::tests::BasisTableSet     tables;
  const exahype2::dg::ader::tests::DeviceBasisTables deviceTables(tables);
  const exahype2::dg::tests::CellChain               geometry;
  const std::vector<double>                          locations = makePointSourceLocations(geometry);
  const std::vector<double> cellStates = exahype2::dg::tests::makeRamp(Setup::AllCellStatesCardinality, 0.5, 0.002);
  const std::vector<double> faceStates(Setup::AllFaceStatesCardinality, 0.0);
  const std::vector<double> faceFluxes(Setup::AllFaceFluxesCardinality, 0.0);
  const std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * LinearScratch::Size, 0.0);

  const exahype2::dg::tests::DeviceBuffer<double> deviceLocations(locations);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceFaceStates(faceStates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceFaceFluxes(faceFluxes);
  const exahype2::dg::tests::DeviceBuffer<double> deviceScratch(scratch);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

  const exahype2::dg::ader::BasisTables<double> onDeviceTables      = deviceTables.view();
  const double* const                           onDeviceLocations   = deviceLocations.data();
  double* const                                 onDeviceCellStates  = deviceCellStates.data();
  double* const                                 onDeviceFaceStates  = deviceFaceStates.data();
  double* const                                 onDeviceFaceFluxes  = deviceFaceFluxes.data();
  double* const                                 onDeviceScratch     = deviceScratch.data();
  const double* const                           onDeviceCellCentres = deviceCellCentres.data();
  const double* const                           onDeviceCellSizes   = deviceCellSizes.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell) {
      double* faceState[peano4::FacesPerCell];
      double* faceFlux[peano4::FacesPerCell];
      collectFacePointers(cell, onDeviceFaceStates, onDeviceFaceFluxes, faceState, faceFlux);
      const int                                   coordinateOffset = cell * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(onDeviceCellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(onDeviceCellSizes + coordinateOffset);
      const int                                   stateOffset   = cell * Setup::CellStateCardinality;
      const int                                   scratchOffset = cell * LinearScratch::Size;
      int                                         pointSourceIndices[Setup::NumberOfPointSources];
      const int numberOfPointSources = exahype2::dg::ader::selectPointSources<Setup::NumberOfPointSources>(
        pointSourceIndices,
        onDeviceLocations,
        cellCentre,
        cellSize
      );
      exahype2::dg::ader::linearSpaceTimePredictor<
        exahype2::dg::ader::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::Unknowns,
        Setup::AuxiliaryVariables,
        true,
        true,
        true,
        true,
        true,
        double,
        double,
        double>(
        faceState,
        faceFlux,
        onDeviceCellStates + stateOffset,
        onDeviceScratch + scratchOffset,
        pointSourceIndices,
        numberOfPointSources,
        onDeviceLocations,
        onDeviceTables,
        cellCentre,
        cellSize,
        Setup::TimeStamp,
        Setup::TimeStepSize
      );
    }
  );

  std::vector<double> deviceCellResult(cellStates.size());
  deviceCellStates.copyBack(deviceCellResult);
  std::vector<double> deviceFaceStateResult(faceStates.size());
  deviceFaceStates.copyBack(deviceFaceStateResult);
  std::vector<double> deviceFaceFluxResult(faceFluxes.size());
  deviceFaceFluxes.copyBack(deviceFaceFluxResult);

  std::vector<double>                           hostCellStates = cellStates;
  std::vector<double>                           hostFaceStates = faceStates;
  std::vector<double>                           hostFaceFluxes = faceFluxes;
  std::vector<double>                           hostScratch    = scratch;
  const exahype2::dg::ader::BasisTables<double> hostTables     = tables.view();
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    double* faceState[peano4::FacesPerCell];
    double* faceFlux[peano4::FacesPerCell];
    collectFacePointers(cell, hostFaceStates.data(), hostFaceFluxes.data(), faceState, faceFlux);
    const int                                   coordinateOffset = cell * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    const int                                   stateOffset   = cell * Setup::CellStateCardinality;
    const int                                   scratchOffset = cell * LinearScratch::Size;
    int                                         pointSourceIndices[Setup::NumberOfPointSources];
    const int numberOfPointSources = exahype2::dg::ader::selectPointSources<Setup::NumberOfPointSources>(
      pointSourceIndices,
      locations.data(),
      cellCentre,
      cellSize
    );
    exahype2::dg::ader::linearSpaceTimePredictor<
      exahype2::dg::ader::tests::StubSolver,
      Setup::DofsPerAxis,
      Setup::Unknowns,
      Setup::AuxiliaryVariables,
      true,
      true,
      true,
      true,
      true,
      double,
      double,
      double>(
      faceState,
      faceFlux,
      hostCellStates.data() + stateOffset,
      hostScratch.data() + scratchOffset,
      pointSourceIndices,
      numberOfPointSources,
      locations.data(),
      hostTables,
      cellCentre,
      cellSize,
      Setup::TimeStamp,
      Setup::TimeStepSize
    );
  }

  validateBuffersMatch(deviceCellResult, hostCellStates, "cellStates");
  validateBuffersMatch(deviceFaceStateResult, hostFaceStates, "faceStates");
  validateBuffersMatch(deviceFaceFluxResult, hostFaceFluxes, "faceFluxes");
}
