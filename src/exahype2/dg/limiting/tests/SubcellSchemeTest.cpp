// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SubcellSchemeTest.h"

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/limiting/LimiterStatus.h"
#include "exahype2/dg/limiting/SubcellScheme.h"
#include "exahype2/dg/limiting/tests/LimiterTestData.h"
#include "exahype2/dg/tests/KernelTestData.h"
#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/singlestep/SubcellStep.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/MaxEigenvalue.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"

namespace {
  using Layout      = exahype2::dg::limiting::tests::StubLayout;
  using Setup       = exahype2::dg::tests::KernelTestSetup;
  using Scheme      = exahype2::dg::limiting::SubcellScheme<exahype2::dg::limiting::tests::StubSolver, double>;
  using FixedScheme = exahype2::dg::limiting::SubcellScheme<exahype2::dg::limiting::tests::FixedStepStubSolver, double>;

  TARGET_DECLARE
  constexpr int HaloValuesPerFace = Scheme::HaloValuesPerFace;
  constexpr int HaloValuesPerCell = peano4::FacesPerCell * HaloValuesPerFace;

  /**
   * Advances the subgrid of one cell with the single-step subcell family and projects it onto the
   * nodes, as the limiter recomputes a cell.
   */
  template <typename Solver>
  INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics recomputeSubgrid(
    double* const                                nodes,
    double* const                                subcells,
    double* const                                evolvedSubcells,
    const double* const                          haloHalves[peano4::FacesPerCell],
    const double* const                          subcellsToNodes,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const int                                    boundaryFaces,
    const double                                 timeStamp,
    const double                                 timeStepSize
  ) {
    const double* const noAppliedFluxes[peano4::FacesPerCell]{};
    exahype2::fv::singlestep::SubcellStep<Solver, double, double>::advanceSubgrid(
      subcells,
      evolvedSubcells,
      haloHalves,
      cellCentre,
      cellSize,
      boundaryFaces,
      timeStamp,
      timeStepSize,
      noAppliedFluxes,
      nullptr
    );
    return exahype2::dg::limiting::SubcellScheme<Solver, double>::projectRecomputedSubgrid(
      nodes,
      subcells,
      subcellsToNodes,
      cellCentre,
      cellSize,
      timeStamp,
      timeStepSize
    );
  }
  TARGET_DECLARE_END

  constexpr double Tolerance = 1.0e-12;

  /**
   * The arguments of every visit of the subcell loop in the order of the visits.
   */
  struct Visits final {
    int                                   numberOfVisits{0};
    const double*                         values[Layout::SubcellsPerCell]{};
    tarch::la::Vector<DIMENSIONS, double> centres[Layout::SubcellsPerCell]{};
    tarch::la::Vector<DIMENSIONS, double> sizes[Layout::SubcellsPerCell]{};
  };

  std::vector<double> getConstantSubgrid(const double unknown, const double auxiliary) {
    std::vector<double> subgrid(Layout::SubcellValuesPerCell);
    for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
      subgrid[static_cast<std::size_t>(subcell) * Layout::NumberOfData]     = unknown;
      subgrid[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1] = auxiliary;
    }
    return subgrid;
  }

  std::vector<double> getConstantNodes(const double unknown, const double auxiliary) {
    std::vector<double> nodes(Layout::ValuesPerCell);
    for (int node = 0; node < Layout::NodesPerCell; node++) {
      nodes[static_cast<std::size_t>(node) * Layout::NumberOfData]     = unknown;
      nodes[static_cast<std::size_t>(node) * Layout::NumberOfData + 1] = auxiliary;
    }
    return nodes;
  }

  std::vector<double> getConstantHalo(const double unknown, const double auxiliary) {
    std::vector<double> halo(HaloValuesPerCell);
    for (std::size_t entry = 0; entry < halo.size(); entry += Layout::NumberOfData) {
      halo[entry]     = unknown;
      halo[entry + 1] = auxiliary;
    }
    return halo;
  }

  /**
   * Returns the number of entries of values within the tolerance of value.
   */
  int countValues(const double* const values, const int count, const double value) {
    int result = 0;
    for (int entry = 0; entry < count; entry++) {
      result += std::abs(values[entry] - value) <= Tolerance ? 1 : 0;
    }
    return result;
  }

  TARGET_DECLARE
  struct RecomputeSubgridOfCell final {
    double*       nodes{nullptr};
    double*       subcells{nullptr};
    double*       scratch{nullptr};
    const double* halo{nullptr};
    const double* subcellsToNodes{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                              cell,
      exahype2::dg::MaxEigenvalueStatistics& accumulator
    ) const {
      const double* haloHalves[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        haloHalves[faceNumber] =
          halo + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * HaloValuesPerFace;
      }
      const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(cellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(cellSizes + coordinateOffset);
      accumulator = exahype2::dg::MaxEigenvalueStatistics::combine(
        accumulator,
        recomputeSubgrid<exahype2::dg::limiting::tests::StubSolver>(
          nodes + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
          subcells + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
          scratch + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
          haloHalves,
          subcellsToNodes,
          cellCentre,
          cellSize,
          0,
          Layout::TimeStamp,
          Layout::TimeStepSize
        )
      );
    }
  };
  TARGET_DECLARE_END

  exahype2::dg::MaxEigenvalueStatistics recomputeOnDevice(
    const exahype2::dg::limiting::tests::SubcellProjections& tables,
    const exahype2::dg::tests::CellChain&                    geometry,
    std::vector<double>&                                     nodes,
    std::vector<double>&                                     subcells,
    const std::vector<double>&                               halo
  ) {
    const std::vector<double> scratch(
      static_cast<std::size_t>(Setup::NumberOfCells) * Layout::SubcellValuesPerCell,
      0.0
    );

    const exahype2::dg::tests::DeviceBuffer<double> deviceNodes(nodes);
    const exahype2::dg::tests::DeviceBuffer<double> deviceSubcells(subcells);
    const exahype2::dg::tests::DeviceBuffer<double> deviceScratch(scratch);
    const exahype2::dg::tests::DeviceBuffer<double> deviceHalo(halo);
    const exahype2::dg::tests::DeviceBuffer<double> deviceSubcellsToNodes(tables.subcellsToNodes);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

    exahype2::dg::MaxEigenvalueStatistics statistics;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(Setup::NumberOfCells),
      RecomputeSubgridOfCell{
        deviceNodes.getData(),
        deviceSubcells.getData(),
        deviceScratch.getData(),
        deviceHalo.getData(),
        deviceSubcellsToNodes.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData()
      },
      tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{statistics}
    );

    deviceNodes.copyBack(nodes);
    deviceSubcells.copyBack(subcells);
    return statistics;
  }
} // namespace

exahype2::dg::limiting::tests::SubcellSchemeTest::SubcellSchemeTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::SubcellSchemeTest") {}

void
exahype2::dg::limiting::tests::SubcellSchemeTest::run() {
  testMethod(testAConstantStateProjectsOntoConstantSubcellsAndBack);
  testMethod(testTheInitialConditionIsSampledAtTheSubcellCentres);
  testMethod(testEverySubcellIsVisitedAtItsCentre);
  testMethod(testTheSubcellWaveSpeedsScaleWithTheCellSize);
  testMethod(testTheTermsForwardOnlyWhatTheSolverOffers);
  testMethod(testTheRiemannSolverCallsTheSubcellRiemannSolver);
  testMethod(testAConstantSubgridRecomputesToTheConstantAndReportsItsWaveSpeed);
  testMethod(testAFixedRuleReportsNoWaveSpeeds);
  testMethod(testAGalerkinCellProjectsItsPreviousSolutionOntoTheSubgridAndTheFaces);
  testMethod(testEveryCellOfADeviceRangeRecomputesLikeTheHost);
  testMethod(testEveryNodeProjectsLikeTheWholeCell);
}

/**
 * Every node projected on its own equals the node of the projection of the whole cell.
 */
void
exahype2::dg::limiting::tests::SubcellSchemeTest::testEveryNodeProjectsLikeTheWholeCell() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  std::vector<double>                                     subcells(Layout::SubcellValuesPerCell);
  for (std::size_t entry = 0; entry < subcells.size(); entry++) {
    subcells[entry] = 1.0 + 0.37 * static_cast<double>(entry % 11) - 0.05 * static_cast<double>(entry);
  }
  std::vector<double> whole(Layout::ValuesPerCell);
  std::vector<double> nodeByNode(Layout::ValuesPerCell);
  Scheme::projectSubcellsOntoNodes(subcells.data(), whole.data(), tables.subcellsToNodes.data());
  for (int node = 0; node < Layout::ValuesPerCell / Layout::NumberOfData; node++) {
    Scheme::projectSubcellsOntoNode(subcells.data(), nodeByNode.data(), tables.subcellsToNodes.data(), node);
  }
  validateNumericalBuffersEquals(nodeByNode, whole);
}

void
exahype2::dg::limiting::tests::SubcellSchemeTest::testAConstantStateProjectsOntoConstantSubcellsAndBack() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const std::vector<double>                               nodes = getConstantNodes(3.0, -1.5);
  std::vector<double>                                     subcells(Layout::SubcellValuesPerCell);
  Scheme::projectNodesOntoSubcells(nodes.data(), subcells.data(), tables.nodesToSubcells.data());
  for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
    validateNumericalEqualsWithParams1(
      subcells[static_cast<std::size_t>(subcell) * Layout::NumberOfData],
      3.0,
      subcell
    );
    validateNumericalEqualsWithParams1(
      subcells[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1],
      -1.5,
      subcell
    );
  }

  std::vector<double> back(Layout::ValuesPerCell);
  Scheme::projectSubcellsOntoNodes(subcells.data(), back.data(), tables.subcellsToNodes.data());
  validateNumericalBuffersEquals(back, nodes);
}

void
exahype2::dg::limiting::tests::SubcellSchemeTest::testTheInitialConditionIsSampledAtTheSubcellCentres() {
  const tarch::la::Vector<DIMENSIONS, double> centre(2.0);
  const tarch::la::Vector<DIMENSIONS, double> width(0.6);
  std::vector<double>                         subcells(Layout::SubcellValuesPerCell);
  Scheme::sampleInitialCondition(subcells.data(), centre, width);

  peano4::dForEach(Layout::SubcellsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& subcell) {
    const int    index    = peano4::dLinearised(subcell, Layout::SubcellsPerAxis);
    const double expected = centre(0) - 0.5 * width(0) + (subcell(0) + 0.5) * width(0) / Layout::SubcellsPerAxis;
    validateNumericalEqualsWithParams1(
      subcells[static_cast<std::size_t>(index) * Layout::NumberOfData],
      expected,
      index
    );
    validateNumericalEqualsWithParams1(
      subcells[static_cast<std::size_t>(index) * Layout::NumberOfData + 1],
      width(0) / Layout::SubcellsPerAxis,
      index
    );
  });
}

/**
 * forEachSubcell() visits the subcells in linearised order at their centres and with their size.
 */
void
exahype2::dg::limiting::tests::SubcellSchemeTest::testEverySubcellIsVisitedAtItsCentre() {
  std::vector<double>                         values(Layout::SubcellValuesPerCell, 0.0);
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.3);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.7);

  Visits visits{};
  Scheme::forEachSubcell(
    cellCentre,
    cellSize,
    values.data(),
    [&](
      const double* const                          value,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      const int visit = visits.numberOfVisits;
      validate(visit < Layout::SubcellsPerCell);
      visits.values[visit]  = value;
      visits.centres[visit] = x;
      visits.sizes[visit]   = h;
      visits.numberOfVisits++;
    }
  );

  validateEquals(visits.numberOfVisits, Layout::SubcellsPerCell);
  const auto expectedSize = exahype2::fv::getVolumeSize(cellSize, Layout::SubcellsPerAxis);
  for (int visit = 0; visit < Layout::SubcellsPerCell; visit++) {
    validateTrue(visits.values[visit] == values.data() + static_cast<std::ptrdiff_t>(visit) * Layout::NumberOfData);
    const auto subcell        = peano4::dDelinearised(visit, Layout::SubcellsPerAxis);
    const auto expectedCentre = exahype2::fv::getVolumeCentre(cellCentre, cellSize, Layout::SubcellsPerAxis, subcell);
    std::vector<double> measured(visits.centres[visit].getData(), visits.centres[visit].getData() + DIMENSIONS);
    std::vector<double> expected(expectedCentre.getData(), expectedCentre.getData() + DIMENSIONS);
    measured.insert(measured.end(), visits.sizes[visit].getData(), visits.sizes[visit].getData() + DIMENSIONS);
    expected.insert(expected.end(), expectedSize.getData(), expectedSize.getData() + DIMENSIONS);
    validateNumericalBuffersEquals(measured, expected);
  }
}

void
exahype2::dg::limiting::tests::SubcellSchemeTest::testTheSubcellWaveSpeedsScaleWithTheCellSize() {
  const tarch::la::Vector<DIMENSIONS, double> centre(0.0);
  tarch::la::Vector<DIMENSIONS, double>       width;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    width(axis) = 0.5 + 0.5 * axis;
  }
  std::vector<double> subcells(Layout::SubcellValuesPerCell);
  for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
    subcells[static_cast<std::size_t>(subcell) * Layout::NumberOfData]     = 1.0 + subcell;
    subcells[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1] = 0.0;
  }
  const auto statistics = Scheme::reduceMaxEigenvalues(subcells.data(), centre, width, 0.0, 0.1);
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, static_cast<double>(Layout::SubcellsPerCell));
  double scaled = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    scaled += Layout::SubcellsPerCell / width(axis);
  }
  validateNumericalEqualsWithEps(statistics.maxScaledEigenvalue, scaled, Tolerance);
}

/**
 * SubgridSolver has a term only where the solver declares it.
 */
void
exahype2::dg::limiting::tests::SubcellSchemeTest::testTheTermsForwardOnlyWhatTheSolverOffers() {
  using Reconstruction = exahype2::dg::limiting::tests::StubSolver::Reconstruction;
  using SubgridSolver  = Scheme::SubgridSolver<Reconstruction>;
  using RiemannSubgridSolver =
    exahype2::dg::limiting::SubcellScheme<exahype2::dg::limiting::tests::RiemannStubSolver, double>::SubgridSolver<
      Reconstruction
    >;

  validateEquals((exahype2::terms::HasFlux<SubgridSolver, double>), true);
  validateEquals((exahype2::terms::HasMaxEigenvalue<SubgridSolver, double>), true);
  validateEquals((exahype2::terms::HasSourceTerm<SubgridSolver, double>), false);
  validateEquals((exahype2::terms::HasRiemannSolver<SubgridSolver, double>), false);
  validateEquals((exahype2::terms::HasRiemannSolver<RiemannSubgridSolver, double>), true);
  validateEquals(SubgridSolver::PatchSize, Layout::SubcellsPerAxis);
  validate(SubgridSolver::TimeSteppingRule == exahype2::TimeStepping::Rule::Fixed);
}

/**
 * SubgridSolver::solveRiemannProblem() returns the wave speed of the subcell Riemann solver, and
 * the other terms call the terms of the solver.
 */
void
exahype2::dg::limiting::tests::SubcellSchemeTest::testTheRiemannSolverCallsTheSubcellRiemannSolver() {
  using Reconstruction = exahype2::dg::limiting::tests::StubSolver::Reconstruction;
  using RiemannTerms =
    exahype2::dg::limiting::SubcellScheme<exahype2::dg::limiting::tests::RiemannStubSolver, double>::SubgridSolver<
      Reconstruction
    >;
  const double                                state[Layout::NumberOfData]{3.0, 0.0};
  double                                      leftFlux[1]{};
  double                                      rightFlux[1]{};
  const tarch::la::Vector<DIMENSIONS, double> x(0.5);
  const tarch::la::Vector<DIMENSIONS, double> h(0.1);

  validateNumericalEquals(
    RiemannTerms::solveRiemannProblem(state, state, x, h, 0.0, 0.1, 1, false, 1, leftFlux, rightFlux),
    exahype2::dg::limiting::tests::RiemannStubSolver::SubcellWaveSpeed
  );
  validateNumericalEquals(RiemannTerms::maxEigenvalue(state, x, h, 0.0, 0.1, 1), 3.0);

  double flux[1]{};
  RiemannTerms::flux(state, x, h, 0.0, 0.1, 1, flux);
  validateNumericalEquals(flux[0], 6.0);
}

void
exahype2::dg::limiting::tests::SubcellSchemeTest::testAConstantSubgridRecomputesToTheConstantAndReportsItsWaveSpeed() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  tarch::la::Vector<DIMENSIONS, double>                   width;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    width(axis) = 0.5 + 0.5 * axis;
  }
  std::vector<double>       subcells = getConstantSubgrid(2.0, 0.5);
  const std::vector<double> halo     = getConstantHalo(2.0, 0.5);
  std::vector<double>       nodes(Layout::ValuesPerCell, 0.0);
  std::vector<double>       evolved(Layout::SubcellValuesPerCell, 0.0);
  const double*             haloHalves[peano4::FacesPerCell];
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    haloHalves[faceNumber] = halo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloValuesPerFace;
  }

  const auto statistics = recomputeSubgrid<exahype2::dg::limiting::tests::StubSolver>(
    nodes.data(),
    subcells.data(),
    evolved.data(),
    haloHalves,
    tables.subcellsToNodes.data(),
    centre,
    width,
    0,
    Layout::TimeStamp,
    Layout::TimeStepSize
  );

  validateNumericalBuffersEquals(subcells, getConstantSubgrid(2.0, 0.5));
  validateNumericalBuffersEquals(nodes, getConstantNodes(2.0, 0.5));
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 2.0);
  double scaled = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    scaled += 2.0 / width(axis);
  }
  validateNumericalEqualsWithEps(statistics.maxScaledEigenvalue, scaled, Tolerance);
}

void
exahype2::dg::limiting::tests::SubcellSchemeTest::testAFixedRuleReportsNoWaveSpeeds() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  std::vector<double>                                     subcells = getConstantSubgrid(2.0, 0.5);
  const std::vector<double>                               halo     = getConstantHalo(2.0, 0.5);
  std::vector<double>                                     nodes(Layout::ValuesPerCell, 0.0);
  std::vector<double>                                     evolved(Layout::SubcellValuesPerCell, 0.0);
  const double*                                           haloHalves[peano4::FacesPerCell];
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    haloHalves[faceNumber] = halo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloValuesPerFace;
  }

  const auto statistics = recomputeSubgrid<exahype2::dg::limiting::tests::FixedStepStubSolver>(
    nodes.data(),
    subcells.data(),
    evolved.data(),
    haloHalves,
    tables.subcellsToNodes.data(),
    centre,
    width,
    0,
    Layout::TimeStamp,
    Layout::TimeStepSize
  );

  validateNumericalBuffersEquals(nodes, getConstantNodes(2.0, 0.5));
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, 0.0);
}

/**
 * A cell projects its previous solution onto its subgrid only after a Galerkin step, and never in
 * the first step.
 */
void
exahype2::dg::limiting::tests::SubcellSchemeTest::
  testAGalerkinCellProjectsItsPreviousSolutionOntoTheSubgridAndTheFaces() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const std::vector<double>                               previous = getConstantNodes(3.0, 0.25);
  std::vector<double>                                     subcells = getConstantSubgrid(5.0, 0.125);
  std::vector<double>                                     halo(HaloValuesPerCell, 0.0);
  double*                                                 haloHalves[peano4::FacesPerCell];
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    haloHalves[faceNumber] = halo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloValuesPerFace;
  }
  int status[exahype2::dg::limiting::NumberOfStatusEntries] = {
    static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular),
    static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular),
    0,
    0
  };

  Scheme::projectSubgridOntoFaces(
    status,
    subcells.data(),
    previous.data(),
    haloHalves,
    tables.nodesToSubcells.data(),
    false
  );
  validateNumericalBuffersEquals(subcells, getConstantSubgrid(3.0, 0.25));
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    const int unknowns    = countValues(haloHalves[faceNumber], HaloValuesPerFace, 3.0);
    const int auxiliaries = countValues(haloHalves[faceNumber], HaloValuesPerFace, 0.25);
    const int zeros       = countValues(haloHalves[faceNumber], HaloValuesPerFace, 0.0);
    validateWithParams2(unknowns > 0, faceNumber, unknowns);
    validateEqualsWithParams1(auxiliaries, unknowns, faceNumber);
    validateEqualsWithParams1(zeros, HaloValuesPerFace - 2 * unknowns, faceNumber);
  }

  status[static_cast<int>(exahype2::dg::limiting::StatusEntry::PreviousStatus)] =
    static_cast<int>(exahype2::dg::limiting::LimiterStatus::Limited);
  subcells = getConstantSubgrid(5.0, 0.125);
  Scheme::projectSubgridOntoFaces(
    status,
    subcells.data(),
    previous.data(),
    haloHalves,
    tables.nodesToSubcells.data(),
    false
  );
  validateNumericalBuffersEquals(subcells, getConstantSubgrid(5.0, 0.125));
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    validateEqualsWithParams1(countValues(haloHalves[faceNumber], HaloValuesPerFace, 3.0), 0, faceNumber);
    validateWithParams1(countValues(haloHalves[faceNumber], HaloValuesPerFace, 5.0) > 0, faceNumber);
  }

  status[static_cast<int>(exahype2::dg::limiting::StatusEntry::PreviousStatus)] =
    static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular);
  Scheme::projectSubgridOntoFaces(
    status,
    subcells.data(),
    previous.data(),
    haloHalves,
    tables.nodesToSubcells.data(),
    true
  );
  validateNumericalBuffersEquals(subcells, getConstantSubgrid(5.0, 0.125));
}

void
exahype2::dg::limiting::tests::SubcellSchemeTest::testEveryCellOfADeviceRangeRecomputesLikeTheHost() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const exahype2::dg::tests::CellChain                    geometry;
  const std::vector<double>                               subcells = exahype2::dg::tests::getRamp(
    static_cast<std::size_t>(Setup::NumberOfCells) * Layout::SubcellValuesPerCell,
    1.0,
    0.002
  );
  const std::vector<double> halo =
    exahype2::dg::tests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * HaloValuesPerCell, 1.5, 0.003);
  const std::vector<double> nodes(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ValuesPerCell, 0.0);

  std::vector<double> deviceNodes      = nodes;
  std::vector<double> deviceSubcells   = subcells;
  const auto          deviceStatistics = recomputeOnDevice(tables, geometry, deviceNodes, deviceSubcells, halo);

  std::vector<double>                   hostNodes    = nodes;
  std::vector<double>                   hostSubcells = subcells;
  std::vector<double>                   evolved(Layout::SubcellValuesPerCell, 0.0);
  exahype2::dg::MaxEigenvalueStatistics hostStatistics;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const double* haloHalves[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      haloHalves[faceNumber] =
        halo.data() + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * HaloValuesPerFace;
    }
    const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    hostStatistics = exahype2::dg::MaxEigenvalueStatistics::combine(
      hostStatistics,
      recomputeSubgrid<exahype2::dg::limiting::tests::StubSolver>(
        hostNodes.data() + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
        hostSubcells.data() + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
        evolved.data(),
        haloHalves,
        tables.subcellsToNodes.data(),
        cellCentre,
        cellSize,
        0,
        Layout::TimeStamp,
        Layout::TimeStepSize
      )
    );
  }

  validateNumericalBuffersEquals(deviceSubcells, hostSubcells);
  validateNumericalBuffersEquals(deviceNodes, hostNodes);
  validateNumericalEquals(deviceStatistics.maxPhysicalEigenvalue, hostStatistics.maxPhysicalEigenvalue);
  validateNumericalEquals(deviceStatistics.maxScaledEigenvalue, hostStatistics.maxScaledEigenvalue);
}
