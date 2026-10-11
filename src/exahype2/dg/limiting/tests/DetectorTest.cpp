// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DetectorTest.h"

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/limiting/Detector.h"
#include "exahype2/dg/limiting/LimiterStatus.h"
#include "exahype2/dg/limiting/tests/LimiterTestData.h"
#include "exahype2/dg/tests/KernelTestData.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/HaloExchange.h"
#include "peano4/Loop.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"

namespace {
  using Layout       = exahype2::dg::limiting::tests::StubLayout;
  using Setup        = exahype2::dg::tests::KernelTestSetup;
  using Judge        = exahype2::dg::limiting::Detector<exahype2::dg::limiting::tests::StubSolver, double>;
  using FixedJudge   = exahype2::dg::limiting::Detector<exahype2::dg::limiting::tests::FixedStepStubSolver, double>;
  using RelaxedJudge = exahype2::dg::limiting::Detector<exahype2::dg::limiting::tests::RelaxedStubSolver, double>;

  TARGET_DECLARE
  constexpr int Regular  = static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular);
  constexpr int Halo     = static_cast<int>(exahype2::dg::limiting::LimiterStatus::Halo);
  constexpr int Limited  = static_cast<int>(exahype2::dg::limiting::LimiterStatus::Limited);
  constexpr int Troubled = static_cast<int>(exahype2::dg::limiting::LimiterStatus::Troubled);

  constexpr int StatusIndex            = static_cast<int>(exahype2::dg::limiting::StatusEntry::Status);
  constexpr int PreviousStatusIndex    = static_cast<int>(exahype2::dg::limiting::StatusEntry::PreviousStatus);
  constexpr int WasTroubledIndex       = static_cast<int>(exahype2::dg::limiting::StatusEntry::WasTroubled);
  constexpr int PredictorDivergedIndex = static_cast<int>(exahype2::dg::limiting::StatusEntry::PredictorDiverged);

  constexpr double NoRelaxation = 0.0;
  constexpr double NoScaling    = 0.0;
  TARGET_DECLARE_END

  constexpr double Tolerance        = 1.0e-12;
  constexpr double PositiveInfinity = std::numeric_limits<double>::infinity();

  std::vector<double> getConstantNodes(const double unknown, const double auxiliary) {
    std::vector<double> nodes(Layout::ValuesPerCell);
    for (int node = 0; node < Layout::NodesPerCell; node++) {
      nodes[static_cast<std::size_t>(node) * Layout::NumberOfData]     = unknown;
      nodes[static_cast<std::size_t>(node) * Layout::NumberOfData + 1] = auxiliary;
    }
    return nodes;
  }

  std::vector<double> getConstantSubgrid(const double unknown, const double auxiliary) {
    std::vector<double> subgrid(Layout::SubcellValuesPerCell);
    for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
      subgrid[static_cast<std::size_t>(subcell) * Layout::NumberOfData]     = unknown;
      subgrid[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1] = auxiliary;
    }
    return subgrid;
  }

  /**
   * Returns nodes whose first value grows along the first axis from one at the left face to three at the right face.
   */
  std::vector<double> getSteepNodes(const std::vector<double>& quadraturePoints) {
    std::vector<double> nodes(Layout::ValuesPerCell, 0.0);
    peano4::dForEach(Layout::NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      const auto index                    = static_cast<std::size_t>(peano4::dLinearised(node, Layout::NodesPerAxis));
      nodes[index * Layout::NumberOfData] = 1.0 + 2.0 * quadraturePoints[static_cast<std::size_t>(node(0))];
    });
    return nodes;
  }

  template <typename Value>
  std::array<Value*, peano4::FacesPerCell> getAddressesOf(Value* const halves) {
    std::array<Value*, peano4::FacesPerCell> addresses{};
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      addresses[static_cast<std::size_t>(faceNumber)] = halves + faceNumber;
    }
    return addresses;
  }

  /**
   * CellStatus holds the status of one cell and the face halves that judgeCandidate() and
   * spreadStatus() read and write.
   */
  struct CellStatus final {
    CellStatus()                             = default;
    CellStatus(const CellStatus&)            = delete;
    CellStatus(CellStatus&&)                 = delete;
    CellStatus& operator=(const CellStatus&) = delete;
    CellStatus& operator=(CellStatus&&)      = delete;
    ~CellStatus()                            = default;

    int    status[exahype2::dg::limiting::NumberOfStatusEntries]{Regular, Regular, 0, 0};
    double candidateExtrema[Layout::ExtremaPerCell]{};
    int    faceHalves[peano4::FacesPerCell]{};
    int    neighbourHalves[peano4::FacesPerCell]{};
    std::array<int*, peano4::FacesPerCell>       ownStatusHalves{getAddressesOf(faceHalves)};
    std::array<const int*, peano4::FacesPerCell> neighbourStatusHalves{getAddressesOf<const int>(neighbourHalves)};
  };

  TARGET_DECLARE
  struct JudgeCandidateOfCell final {
    int*          statuses{nullptr};
    int*          faceStatuses{nullptr};
    const double* candidates{nullptr};
    const double* referenceExtrema{nullptr};
    const double* nodesToLobatto{nullptr};
    const double* nodesToSubcells{nullptr};
    const double* quadraturePoints{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                              cell,
      exahype2::dg::MaxEigenvalueStatistics& accumulator
    ) const {
      int* ownStatusHalves[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        ownStatusHalves[faceNumber] =
          faceStatuses + static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber;
      }
      const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(cellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(cellSizes + coordinateOffset);
      double                                      candidateExtrema[Layout::ExtremaPerCell];
      accumulator = exahype2::dg::MaxEigenvalueStatistics::combine(
        accumulator,
        Judge::judgeCandidate(
          statuses + static_cast<std::ptrdiff_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries,
          ownStatusHalves,
          candidates + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
          referenceExtrema + static_cast<std::ptrdiff_t>(cell) * Layout::ExtremaPerCell,
          nodesToLobatto,
          nodesToSubcells,
          quadraturePoints,
          NoRelaxation,
          NoScaling,
          cellCentre,
          cellSize,
          Layout::TimeStamp,
          Layout::TimeStepSize,
          candidateExtrema
        )
      );
    }
  };
  TARGET_DECLARE_END

  exahype2::dg::MaxEigenvalueStatistics judgeOnDevice(
    const exahype2::dg::limiting::tests::SubcellProjections& tables,
    const exahype2::dg::tests::CellChain&                    geometry,
    std::vector<int>&                                        statuses,
    std::vector<int>&                                        faceStatuses,
    const std::vector<double>&                               candidates,
    const std::vector<double>&                               referenceExtrema
  ) {
    const exahype2::dg::tests::DeviceBuffer<int>    deviceStatuses(statuses);
    const exahype2::dg::tests::DeviceBuffer<int>    deviceFaceStatuses(faceStatuses);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCandidates(candidates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceReferenceExtrema(referenceExtrema);
    const exahype2::dg::tests::DeviceBuffer<double> deviceNodesToLobatto(tables.nodesToLobatto);
    const exahype2::dg::tests::DeviceBuffer<double> deviceNodesToSubcells(tables.nodesToSubcells);
    const exahype2::dg::tests::DeviceBuffer<double> deviceQuadraturePoints(tables.quadraturePoints);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

    exahype2::dg::MaxEigenvalueStatistics statistics;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(Setup::NumberOfCells),
      JudgeCandidateOfCell{
        deviceStatuses.getData(),
        deviceFaceStatuses.getData(),
        deviceCandidates.getData(),
        deviceReferenceExtrema.getData(),
        deviceNodesToLobatto.getData(),
        deviceNodesToSubcells.getData(),
        deviceQuadraturePoints.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData()
      },
      tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{statistics}
    );

    deviceStatuses.copyBack(statuses);
    deviceFaceStatuses.copyBack(faceStatuses);
    return statistics;
  }
} // namespace

exahype2::dg::limiting::tests::DetectorTest::DetectorTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::DetectorTest") {}

void
exahype2::dg::limiting::tests::DetectorTest::run() {
  testMethod(testTheExtremaMergesKeepNaNAndSignedZero);
  testMethod(testTheDiscreteMaximumPrinciple);
  testMethod(testTheExtremumQuantisation);
  testMethod(testTheObservableExtremaCoverEveryState);
  testMethod(testTheProjectedObservableExtremaCoverTheSamples);
  testMethod(testTheCellExtremaCoverTheNodesAndTheSamples);
  testMethod(testTheRelaxationTermReplacesTheSpecifiedParameter);
  testMethod(testACandidateInsideTheReferenceExtremaIsAccepted);
  testMethod(testACandidateOutsideTheReferenceExtremaIsRejected);
  testMethod(testTheRelaxationWidensTheAcceptedRange);
  testMethod(testTheAdmissibilityCombinesNodesAndSubcells);
  testMethod(testTheAdmissibilityCoversTheLobattoSamples);
  testMethod(testWidenExtremaMergesTheMinimaAndTheMaxima);
  testMethod(testOpenInitialStatusMarksAnOscillatingInterpolant);
  testMethod(testOpenStepCopiesTheSolutionAndOpensTheStatus);
  testMethod(testJudgeCandidateMarksAnInadmissibleCellTroubledAndWritesTheFaces);
  testMethod(testJudgeCandidateKeepsATroubledCellLimited);
  testMethod(testJudgeCandidateReportsTheWaveSpeedsOfAnAcceptedCell);
  testMethod(testJudgeCandidateFailsADivergedPredictor);
  testMethod(testJudgeCandidateRejectsACandidateOutsideTheReferenceExtrema);
  testMethod(testSpreadStatusLiftsByTheNeighbours);
  testMethod(testSpreadStatusesReadsThePreviousRing);
  testMethod(testClearFacesWritesRegularAndZerosTheSubcellHalo);
  testMethod(testAFixedRuleReportsNoWaveSpeeds);
  testMethod(testEveryCellOfADeviceRangeJudgesLikeTheHost);
  testMethod(testTheCellListsHoldTheLocalCellsFirst);
}

void
exahype2::dg::limiting::tests::DetectorTest::testTheExtremaMergesKeepNaNAndSignedZero() {
  constexpr double        FiniteValue         = 4.0;
  constexpr double        SmallerFiniteValue  = -2.0;
  constexpr double        PositiveZero        = 0.0;
  constexpr double        NegativeZero        = -0.0;
  constexpr double        NegativeInfinity    = -PositiveInfinity;
  constexpr std::uint64_t NoncanonicalNaNBits = 0x7FF8000000000042ULL;
  constexpr auto          CanonicalNaNBits    = std::bit_cast<std::uint64_t>(std::numeric_limits<double>::quiet_NaN());
  constexpr auto          NoncanonicalNaN     = std::bit_cast<double>(NoncanonicalNaNBits);

  validateNumericalEquals(Judge::mergeMinimum(FiniteValue, SmallerFiniteValue), SmallerFiniteValue);
  validateNumericalEquals(Judge::mergeMinimum(SmallerFiniteValue, FiniteValue), SmallerFiniteValue);
  validateNumericalEquals(Judge::mergeMaximum(FiniteValue, SmallerFiniteValue), FiniteValue);
  validateNumericalEquals(Judge::mergeMaximum(SmallerFiniteValue, FiniteValue), FiniteValue);
  validateNumericalEquals(Judge::mergeMinimum(PositiveInfinity, FiniteValue), FiniteValue);
  validateNumericalEquals(Judge::mergeMinimum(FiniteValue, PositiveInfinity), FiniteValue);
  validateNumericalEquals(Judge::mergeMaximum(NegativeInfinity, FiniteValue), FiniteValue);
  validateNumericalEquals(Judge::mergeMaximum(FiniteValue, NegativeInfinity), FiniteValue);
  validateNumericalEquals(Judge::mergeMinimum(FiniteValue, FiniteValue), FiniteValue);
  validateNumericalEquals(Judge::mergeMaximum(FiniteValue, FiniteValue), FiniteValue);
  validate(std::isinf(Judge::mergeMinimum(PositiveInfinity, PositiveInfinity)));
  validate(not std::signbit(Judge::mergeMinimum(PositiveInfinity, PositiveInfinity)));
  validate(std::isinf(Judge::mergeMaximum(NegativeInfinity, NegativeInfinity)));
  validate(std::signbit(Judge::mergeMaximum(NegativeInfinity, NegativeInfinity)));

  const auto leftMinimumNaN  = Judge::mergeMinimum(NoncanonicalNaN, FiniteValue);
  const auto rightMinimumNaN = Judge::mergeMinimum(FiniteValue, NoncanonicalNaN);
  const auto leftMaximumNaN  = Judge::mergeMaximum(NoncanonicalNaN, FiniteValue);
  const auto rightMaximumNaN = Judge::mergeMaximum(FiniteValue, NoncanonicalNaN);
  validateEquals(std::bit_cast<std::uint64_t>(leftMinimumNaN), CanonicalNaNBits);
  validateEquals(std::bit_cast<std::uint64_t>(rightMinimumNaN), CanonicalNaNBits);
  validateEquals(std::bit_cast<std::uint64_t>(leftMaximumNaN), CanonicalNaNBits);
  validateEquals(std::bit_cast<std::uint64_t>(rightMaximumNaN), CanonicalNaNBits);
  validateEquals(
    std::bit_cast<std::uint64_t>(Judge::mergeMinimum(leftMinimumNaN, SmallerFiniteValue)),
    CanonicalNaNBits
  );
  validateEquals(std::bit_cast<std::uint64_t>(Judge::mergeMaximum(leftMaximumNaN, FiniteValue)), CanonicalNaNBits);

  validate(std::signbit(Judge::mergeMinimum(PositiveZero, NegativeZero)));
  validate(std::signbit(Judge::mergeMinimum(NegativeZero, PositiveZero)));
  validate(not std::signbit(Judge::mergeMaximum(PositiveZero, NegativeZero)));
  validate(not std::signbit(Judge::mergeMaximum(NegativeZero, PositiveZero)));
  validate(not std::signbit(Judge::mergeMinimum(PositiveZero, PositiveZero)));
  validate(std::signbit(Judge::mergeMaximum(NegativeZero, NegativeZero)));
}

void
exahype2::dg::limiting::tests::DetectorTest::testTheDiscreteMaximumPrinciple() {
  validate(Judge::satisfiesDiscreteMaximumPrinciple(0.4, 0.6, 0.0, 1.0, NoRelaxation, NoScaling));
  validate(not Judge::satisfiesDiscreteMaximumPrinciple(-0.1, 0.6, 0.0, 1.0, NoRelaxation, NoScaling));
  validate(not Judge::satisfiesDiscreteMaximumPrinciple(0.4, 1.1, 0.0, 1.0, NoRelaxation, NoScaling));

  validate(Judge::satisfiesDiscreteMaximumPrinciple(-0.05, 0.6, 0.0, 1.0, 0.1, NoScaling));
  validate(Judge::satisfiesDiscreteMaximumPrinciple(-0.05, 0.6, 0.0, 1.0, NoRelaxation, 0.1));
  validate(Judge::satisfiesDiscreteMaximumPrinciple(-0.15, 0.6, 0.0, 1.0, 0.2, 0.1));
  validate(not Judge::satisfiesDiscreteMaximumPrinciple(-0.25, 0.6, 0.0, 1.0, 0.2, 0.1));

  constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
  validate(not Judge::satisfiesDiscreteMaximumPrinciple(NaN, 0.6, 0.0, 1.0, NoRelaxation, NoScaling));
  validate(
    not Judge::satisfiesDiscreteMaximumPrinciple(0.4, 0.6, PositiveInfinity, -PositiveInfinity, NoRelaxation, NoScaling)
  );
}

void
exahype2::dg::limiting::tests::DetectorTest::testTheExtremumQuantisation() {
  constexpr double Value        = 0.6427813001938251;
  const double     lastBitNoise = std::nextafter(Value, 1.0);

  validateNumericalEquals(Judge::quantiseObservableExtremum(Value), Judge::quantiseObservableExtremum(lastBitNoise));
  validateNumericalEquals(
    Judge::quantiseObservableExtremum(Judge::quantiseObservableExtremum(Value)),
    Judge::quantiseObservableExtremum(Value)
  );
  validateNumericalEquals(Judge::quantiseObservableExtremum(-Value), -Judge::quantiseObservableExtremum(Value));
  validate(std::abs(Judge::quantiseObservableExtremum(Value) - Value) < 1.0e-11 * Value);
  validateNumericalEquals(Judge::quantiseObservableExtremum(0.0), 0.0);
  validate(std::isinf(Judge::quantiseObservableExtremum(PositiveInfinity)));
  validate(std::isnan(Judge::quantiseObservableExtremum(std::numeric_limits<double>::quiet_NaN())));
}

void
exahype2::dg::limiting::tests::DetectorTest::testTheObservableExtremaCoverEveryState() {
  constexpr int NumberOfStates                                = 3;
  const double  states[NumberOfStates * Layout::NumberOfData] = {1.0, 2.0, 3.0, 4.0, 0.0, 5.0};
  double        extrema[Layout::ExtremaPerCell];
  Judge::clearExtrema(extrema);
  Judge::reduceObservableExtrema(extrema, states, NumberOfStates);
  validateNumericalEquals(extrema[0], 0.0);
  validateNumericalEquals(extrema[1], 3.0);
}

/**
 * The projected extrema equal the nodal extrema for an identity sampling matrix and the constant of
 * a constant field for a column-stochastic matrix.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testTheProjectedObservableExtremaCoverTheSamples() {
  const double        identity[Layout::NodesPerAxis * Layout::NodesPerAxis] = {1.0, 0.0, 0.0, 1.0};
  std::vector<double> nodes(Layout::ValuesPerCell, 0.0);
  for (int node = 0; node < Layout::NodesPerCell; node++) {
    nodes[static_cast<std::size_t>(node) * Layout::NumberOfData] = -2.0 + node;
  }
  double extrema[Layout::ExtremaPerCell];
  Judge::clearExtrema(extrema);
  Judge::reduceProjectedObservableExtrema<Layout::NodesPerAxis>(extrema, nodes.data(), identity);
  validateNumericalEquals(extrema[0], -2.0);
  validateNumericalEquals(extrema[1], -2.0 + Layout::NodesPerCell - 1);

  constexpr double Constant                                                     = 7.0;
  const double columnStochastic[Layout::NodesPerAxis * Layout::SubcellsPerAxis] = {0.25, 0.5, 0.75, 0.75, 0.5, 0.25};
  const std::vector<double> constantNodes                                       = getConstantNodes(Constant, 0.0);
  Judge::clearExtrema(extrema);
  Judge::reduceProjectedObservableExtrema<Layout::SubcellsPerAxis>(extrema, constantNodes.data(), columnStochastic);
  validateNumericalEquals(extrema[0], Constant);
  validateNumericalEquals(extrema[1], Constant);
}

/**
 * A profile linear along the first axis reaches its extrema at the Lobatto samples outside the nodes.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testTheCellExtremaCoverTheNodesAndTheSamples() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const std::vector<double>                               nodes = getSteepNodes(tables.quadraturePoints);
  double                                                  extrema[Layout::ExtremaPerCell];
  Judge::reduceExtrema(extrema, nodes.data(), tables.nodesToLobatto.data(), tables.nodesToSubcells.data());
  validateNumericalEquals(extrema[0], 1.0);
  validateNumericalEquals(extrema[1], 3.0);
}

void
exahype2::dg::limiting::tests::DetectorTest::testTheRelaxationTermReplacesTheSpecifiedParameter() {
  validateNumericalEquals(Judge::getRelaxationParameterOfObservable(0, 0.25), 0.25);
  validateNumericalEquals(RelaxedJudge::getRelaxationParameterOfObservable(0, 0.25), 0.5);
}

void
exahype2::dg::limiting::tests::DetectorTest::testACandidateInsideTheReferenceExtremaIsAccepted() {
  const double candidateExtrema[Layout::ExtremaPerCell]{1.0, 9.0};
  const double referenceExtrema[Layout::ExtremaPerCell]{0.0, 10.0};
  validateTrue(Judge::satisfiesRelaxedMaximumPrinciple(candidateExtrema, referenceExtrema, NoRelaxation, NoScaling));
}

void
exahype2::dg::limiting::tests::DetectorTest::testACandidateOutsideTheReferenceExtremaIsRejected() {
  const double candidateExtrema[Layout::ExtremaPerCell]{-0.5, 8.0};
  const double referenceExtrema[Layout::ExtremaPerCell]{0.0, 10.0};
  validateFalse(Judge::satisfiesRelaxedMaximumPrinciple(candidateExtrema, referenceExtrema, NoRelaxation, NoScaling));
}

/**
 * The relaxation term accepts a candidate the specified parameter alone rejects.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testTheRelaxationWidensTheAcceptedRange() {
  const double     candidateExtrema[Layout::ExtremaPerCell]{-0.5, 9.0};
  const double     referenceExtrema[Layout::ExtremaPerCell]{0.0, 10.0};
  constexpr double SpecifiedRelaxation = 0.25;
  validateFalse(
    Judge::satisfiesRelaxedMaximumPrinciple(candidateExtrema, referenceExtrema, SpecifiedRelaxation, NoScaling)
  );
  validateTrue(
    RelaxedJudge::satisfiesRelaxedMaximumPrinciple(candidateExtrema, referenceExtrema, SpecifiedRelaxation, NoScaling)
  );
}

void
exahype2::dg::limiting::tests::DetectorTest::testTheAdmissibilityCombinesNodesAndSubcells() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.0);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  std::vector<double>                                     nodes    = getConstantNodes(1.0, 1.0);
  std::vector<double>                                     subcells = getConstantSubgrid(1.0, 1.0);
  const double* const                                     lobatto  = tables.nodesToLobatto.data();
  validate(Judge::isPhysicallyAdmissible(nodes.data(), subcells.data(), lobatto, centre, width));
  subcells[static_cast<std::size_t>(Layout::SubcellsPerCell - 1) * Layout::NumberOfData] = -1.0;
  validate(not Judge::isPhysicallyAdmissible(nodes.data(), subcells.data(), lobatto, centre, width));
  subcells[static_cast<std::size_t>(Layout::SubcellsPerCell - 1) * Layout::NumberOfData] = 1.0;
  nodes[0]                                                                               = 0.0;
  validate(not Judge::isPhysicallyAdmissible(nodes.data(), subcells.data(), lobatto, centre, width));
}

/**
 * A candidate with admissible nodes and subcells that extrapolates to an inadmissible state at a
 * face fails the admissibility check.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testTheAdmissibilityCoversTheLobattoSamples() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.0);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  const std::vector<double>                               subcells = getConstantSubgrid(1.0, 1.0);
  const double* const                                     lobatto  = tables.nodesToLobatto.data();

  // steeperNodes stays positive at the nodes. Its interpolant reaches -0.5 at the left face.
  const std::vector<double> steepNodes = getSteepNodes(tables.quadraturePoints);
  std::vector<double>       steeperNodes(steepNodes);
  for (int node = 0; node < Layout::NodesPerCell; node++) {
    const auto index        = static_cast<std::size_t>(node) * Layout::NumberOfData;
    steeperNodes[index]     = 2.0 * steepNodes[index] - 2.5;
    steeperNodes[index + 1] = 1.0;
  }
  validate(Judge::isPhysicallyAdmissible(steepNodes.data(), subcells.data(), lobatto, centre, width));
  for (int node = 0; node < Layout::NodesPerCell; node++) {
    validateWithParams1(steeperNodes[static_cast<std::size_t>(node) * Layout::NumberOfData] > 0.0, node);
  }
  validate(not Judge::isPhysicallyAdmissible(steeperNodes.data(), subcells.data(), lobatto, centre, width));
}

void
exahype2::dg::limiting::tests::DetectorTest::testWidenExtremaMergesTheMinimaAndTheMaxima() {
  double extrema[Layout::ExtremaPerCell];
  Judge::clearExtrema(extrema);
  validate(std::isinf(extrema[0]) and not std::signbit(extrema[0]));
  validate(std::isinf(extrema[1]) and std::signbit(extrema[1]));

  const double first[Layout::ExtremaPerCell]{2.0, 5.0};
  Judge::widenExtrema(extrema, first);
  validateNumericalEquals(extrema[0], 2.0);
  validateNumericalEquals(extrema[1], 5.0);

  const double inner[Layout::ExtremaPerCell]{3.0, 4.0};
  Judge::widenExtrema(extrema, inner);
  validateNumericalEquals(extrema[0], 2.0);
  validateNumericalEquals(extrema[1], 5.0);

  const double outer[Layout::ExtremaPerCell]{1.0, 6.0};
  Judge::widenExtrema(extrema, outer);
  validateNumericalEquals(extrema[0], 1.0);
  validateNumericalEquals(extrema[1], 6.0);
}

/**
 * Of two interpolants over a constant subgrid only the steep one leaves the sampled range.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testOpenInitialStatusMarksAnOscillatingInterpolant() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const std::vector<double>                               subcells = getConstantSubgrid(2.0, 0.0);
  int    status[exahype2::dg::limiting::NumberOfStatusEntries]{Troubled, Troubled, 1, 1};
  double sampledExtrema[Judge::ExtremaPerCell];
  Judge::reduceSampledExtrema(sampledExtrema, subcells.data());

  const std::vector<double> steepNodes = getSteepNodes(tables.quadraturePoints);
  Judge::openInitialStatus(
    status,
    steepNodes.data(),
    sampledExtrema,
    tables.nodesToLobatto.data(),
    tables.nodesToSubcells.data(),
    NoRelaxation,
    NoScaling
  );
  validateEquals(status[StatusIndex], Regular);
  validateEquals(status[PreviousStatusIndex], Regular);
  validateEquals(status[WasTroubledIndex], 1);
  validateEquals(status[PredictorDivergedIndex], 0);

  const std::vector<double> constantNodes = getConstantNodes(2.0, 0.0);
  Judge::openInitialStatus(
    status,
    constantNodes.data(),
    sampledExtrema,
    tables.nodesToLobatto.data(),
    tables.nodesToSubcells.data(),
    NoRelaxation,
    NoScaling
  );
  validateEquals(status[WasTroubledIndex], 0);
}

void
exahype2::dg::limiting::tests::DetectorTest::testOpenStepCopiesTheSolutionAndOpensTheStatus() {
  const std::vector<double> solution = getConstantNodes(4.0, 0.5);
  std::vector<double>       previous(Layout::ValuesPerCell, 0.0);

  int troubledStatus[exahype2::dg::limiting::NumberOfStatusEntries]{Troubled, Regular, 1, 0};
  Judge::openStep(troubledStatus, solution.data(), previous.data());
  validateNumericalBuffersEquals(previous, solution);
  validateEquals(troubledStatus[StatusIndex], Limited);
  validateEquals(troubledStatus[PreviousStatusIndex], Troubled);
  validateEquals(troubledStatus[WasTroubledIndex], 1);

  int haloStatus[exahype2::dg::limiting::NumberOfStatusEntries]{Halo, Limited, 0, 0};
  Judge::openStep(haloStatus, solution.data(), previous.data());
  validateEquals(haloStatus[StatusIndex], Regular);
  validateEquals(haloStatus[PreviousStatusIndex], Halo);
}

void
exahype2::dg::limiting::tests::DetectorTest::testJudgeCandidateMarksAnInadmissibleCellTroubledAndWritesTheFaces() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  const std::vector<double>                               candidate = getConstantNodes(-1.0, 0.0);
  const double                                            referenceExtrema[Layout::ExtremaPerCell]{-5.0, 5.0};
  CellStatus                                              cell;

  const auto statistics = Judge::judgeCandidate(
    cell.status,
    cell.ownStatusHalves.data(),
    candidate.data(),
    referenceExtrema,
    tables.nodesToLobatto.data(),
    tables.nodesToSubcells.data(),
    tables.quadraturePoints.data(),
    NoRelaxation,
    NoScaling,
    centre,
    width,
    Layout::TimeStamp,
    Layout::TimeStepSize,
    cell.candidateExtrema
  );

  validateEquals(cell.status[StatusIndex], Troubled);
  validateEquals(cell.status[WasTroubledIndex], 1);
  validateEquals(cell.status[PredictorDivergedIndex], 0);
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    validateEqualsWithParams1(cell.faceHalves[faceNumber], Troubled, faceNumber);
  }
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, 0.0);
}

/**
 * A cell that was troubled in the previous step stays on the subcell scheme without wave speeds.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testJudgeCandidateKeepsATroubledCellLimited() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  const std::vector<double>                               candidate = getConstantNodes(2.0, 0.0);
  const double                                            referenceExtrema[Layout::ExtremaPerCell]{0.5, 3.0};
  CellStatus                                              cell;
  cell.status[StatusIndex]         = Limited;
  cell.status[PreviousStatusIndex] = Troubled;
  cell.status[WasTroubledIndex]    = 1;

  const auto statistics = Judge::judgeCandidate(
    cell.status,
    cell.ownStatusHalves.data(),
    candidate.data(),
    referenceExtrema,
    tables.nodesToLobatto.data(),
    tables.nodesToSubcells.data(),
    tables.quadraturePoints.data(),
    NoRelaxation,
    NoScaling,
    centre,
    width,
    Layout::TimeStamp,
    Layout::TimeStepSize,
    cell.candidateExtrema
  );

  validateEquals(cell.status[StatusIndex], Limited);
  validateEquals(cell.status[WasTroubledIndex], 0);
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    validateEqualsWithParams1(cell.faceHalves[faceNumber], Limited, faceNumber);
  }
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 0.0);
}

void
exahype2::dg::limiting::tests::DetectorTest::testJudgeCandidateReportsTheWaveSpeedsOfAnAcceptedCell() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  tarch::la::Vector<DIMENSIONS, double>                   width;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    width(axis) = 0.5 + 0.5 * axis;
  }
  const std::vector<double> candidate = getConstantNodes(2.0, 0.0);
  const double              referenceExtrema[Layout::ExtremaPerCell]{0.5, 3.0};
  CellStatus                cell;

  const auto statistics = Judge::judgeCandidate(
    cell.status,
    cell.ownStatusHalves.data(),
    candidate.data(),
    referenceExtrema,
    tables.nodesToLobatto.data(),
    tables.nodesToSubcells.data(),
    tables.quadraturePoints.data(),
    NoRelaxation,
    NoScaling,
    centre,
    width,
    Layout::TimeStamp,
    Layout::TimeStepSize,
    cell.candidateExtrema
  );

  validateEquals(cell.status[StatusIndex], Regular);
  validateEquals(cell.status[WasTroubledIndex], 0);
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    validateEqualsWithParams1(cell.faceHalves[faceNumber], Regular, faceNumber);
  }
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 2.0);
  double scaled = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    scaled += 2.0 / width(axis);
  }
  validateNumericalEqualsWithEps(statistics.maxScaledEigenvalue, scaled, Tolerance);
}

void
exahype2::dg::limiting::tests::DetectorTest::testJudgeCandidateFailsADivergedPredictor() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  const std::vector<double>                               candidate = getConstantNodes(2.0, 0.0);
  const double                                            referenceExtrema[Layout::ExtremaPerCell]{0.5, 3.0};
  CellStatus                                              cell;
  cell.status[PredictorDivergedIndex] = 1;

  static_cast<void>(Judge::judgeCandidate(
    cell.status,
    cell.ownStatusHalves.data(),
    candidate.data(),
    referenceExtrema,
    tables.nodesToLobatto.data(),
    tables.nodesToSubcells.data(),
    tables.quadraturePoints.data(),
    NoRelaxation,
    NoScaling,
    centre,
    width,
    Layout::TimeStamp,
    Layout::TimeStepSize,
    cell.candidateExtrema
  ));

  validateEquals(cell.status[StatusIndex], Troubled);
  validateEquals(cell.status[WasTroubledIndex], 1);
  validateEquals(cell.status[PredictorDivergedIndex], 0);
}

void
exahype2::dg::limiting::tests::DetectorTest::testJudgeCandidateRejectsACandidateOutsideTheReferenceExtrema() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  const std::vector<double>                               candidate = getConstantNodes(2.0, 0.0);
  const double                                            referenceExtrema[Layout::ExtremaPerCell]{0.5, 1.0};
  CellStatus                                              cell;

  static_cast<void>(Judge::judgeCandidate(
    cell.status,
    cell.ownStatusHalves.data(),
    candidate.data(),
    referenceExtrema,
    tables.nodesToLobatto.data(),
    tables.nodesToSubcells.data(),
    tables.quadraturePoints.data(),
    NoRelaxation,
    NoScaling,
    centre,
    width,
    Layout::TimeStamp,
    Layout::TimeStepSize,
    cell.candidateExtrema
  ));

  validateEquals(cell.status[StatusIndex], Troubled);
  validateEquals(cell.status[WasTroubledIndex], 1);
}

/**
 * A cell ends with the more limited of its own status and the status that its most limited
 * neighbour spreads.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testSpreadStatusLiftsByTheNeighbours() {
  CellStatus cell;
  cell.neighbourHalves[0] = Troubled;
  Judge::spreadStatus(cell.status, cell.neighbourStatusHalves.data());
  validateEquals(cell.status[StatusIndex], Limited);
  validateEquals(cell.status[WasTroubledIndex], 1);

  CellStatus haloCell;
  haloCell.neighbourHalves[1] = Limited;
  Judge::spreadStatus(haloCell.status, haloCell.neighbourStatusHalves.data());
  validateEquals(haloCell.status[StatusIndex], Halo);
  validateEquals(haloCell.status[WasTroubledIndex], 0);

  CellStatus troubledCell;
  troubledCell.status[StatusIndex] = Troubled;
  Judge::spreadStatus(troubledCell.status, troubledCell.neighbourStatusHalves.data());
  validateEquals(troubledCell.status[StatusIndex], Troubled);
}

/**
 * A troubled corner cell adds one ring of Limited cells and one ring of Halo cells.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testSpreadStatusesReadsThePreviousRing() {
  constexpr int Depth = 1;
  peano4::tests::buildRegularGrid(Depth);
  const auto&                                cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  const auto                                 cells            = cellConnectivity.getCells();
  tarch::accelerator::ChunkedMemoryPool<int> statuses(exahype2::dg::limiting::NumberOfStatusEntries);
  tarch::accelerator::ChunkedMemoryPool<int> faceStatuses(Judge::StatusValuesPerFace);
  statuses.reserve(cellConnectivity.getNumberOfSlots());
  faceStatuses.reserve(cellConnectivity.getNumberOfFaces());
  std::vector<int> statusValues;
  std::vector<int> faceValues;
  statuses.copyToHost(statusValues);
  faceStatuses.copyToHost(faceValues);
  std::fill(statusValues.begin(), statusValues.end(), 0);
  std::fill(faceValues.begin(), faceValues.end(), Regular);
  const auto paddedStatus = static_cast<std::size_t>(statuses.getPaddedChunkSize());
  const auto paddedFace   = static_cast<std::size_t>(faceStatuses.getPaddedChunkSize());
  for (int entry = 0; entry < cells.size; entry++) {
    const int  slot    = cells[entry];
    const auto lattice = cellConnectivity.getCellLattice(slot);
    int        sum     = 0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      sum += lattice(axis);
    }
    const int status                                                               = sum == 0 ? Troubled : Regular;
    statusValues[static_cast<std::size_t>(slot) * paddedStatus + StatusIndex]      = status;
    statusValues[static_cast<std::size_t>(slot) * paddedStatus + WasTroubledIndex] = sum == 0 ? 1 : 0;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const auto face = static_cast<std::size_t>(cellConnectivity.getCellFace(slot, faceNumber));
      faceValues[face * paddedFace + exahype2::dg::getOwnHalf(faceNumber)] = status;
    }
  }
  statuses.copyFromHost(statusValues);
  faceStatuses.copyFromHost(faceValues);
  peano4::HaloExchange<int> exchange(faceStatuses);
  exchange.rebuild(cellConnectivity);
  std::array<std::vector<int>, exahype2::dg::limiting::NumberOfSpreadRings> statusesAfterSpread;
  std::array<bool, exahype2::dg::limiting::NumberOfSpreadRings>             facesUnchanged{};

  for (int ring = 0; ring < exahype2::dg::limiting::NumberOfSpreadRings; ring++) {
    exchange.post();
    exchange.wait();
    faceStatuses.copyToHost(faceValues);
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(cells.size),
      exahype2::dg::limiting::SpreadStatuses<exahype2::dg::limiting::tests::StubSolver, double>{
        cellConnectivity.onDevice(),
        cellConnectivity.onDevice().getCells(),
        statuses.getView(),
        std::as_const(faceStatuses).getView()
      }
    );
    std::vector<int> facesAfterSpread;
    faceStatuses.copyToHost(facesAfterSpread);
    facesUnchanged[ring] = facesAfterSpread == faceValues;
    statuses.copyToHost(statusesAfterSpread[ring]);
    if (ring + 1 < exahype2::dg::limiting::NumberOfSpreadRings) {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(cells.size),
        exahype2::dg::limiting::WriteStatusesOntoFaces{
          cellConnectivity.onDevice(),
          std::as_const(statuses).getView(),
          faceStatuses.getView()
        }
      );
    }
  }
  for (int ring = 0; ring < exahype2::dg::limiting::NumberOfSpreadRings; ring++) {
    validateWithParams1(facesUnchanged[ring], ring);
    for (int entry = 0; entry < cells.size; entry++) {
      const int  slot     = cells[entry];
      const auto lattice  = cellConnectivity.getCellLattice(slot);
      int        distance = 0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        distance += lattice(axis);
      }
      const int expected = distance <= ring + 1 ? std::max(Regular, Troubled - distance) : Regular;
      validateEqualsWithParams2(
        statusesAfterSpread[ring][static_cast<std::size_t>(slot) * paddedStatus + StatusIndex],
        expected,
        slot,
        ring
      );
      validateEqualsWithParams2(
        statusesAfterSpread[ring][static_cast<std::size_t>(slot) * paddedStatus + WasTroubledIndex],
        distance <= 1 ? 1 : 0,
        slot,
        ring
      );
    }
  }
}

/**
 * ClearFaces writes Regular into both status halves and zeros into the whole subcell halo.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testClearFacesWritesRegularAndZerosTheSubcellHalo() {
  constexpr int NumberOfFaces     = 5;
  constexpr int HaloValuesPerFace = 7;
  using Clear = exahype2::dg::limiting::ClearFaces<exahype2::dg::limiting::tests::StubSolver, double, double>;

  tarch::accelerator::ChunkedMemoryPool<int>    faceStatuses(Judge::StatusValuesPerFace);
  tarch::accelerator::ChunkedMemoryPool<double> extremaFaces(Judge::ExtremaValuesPerFace);
  tarch::accelerator::ChunkedMemoryPool<double> subcellHalo(HaloValuesPerFace);
  faceStatuses.reserve(NumberOfFaces);
  extremaFaces.reserve(NumberOfFaces);
  subcellHalo.reserve(NumberOfFaces);
  std::vector<double> haloOnHost;
  subcellHalo.copyToHost(haloOnHost);
  std::fill(haloOnHost.begin(), haloOnHost.end(), std::numeric_limits<double>::quiet_NaN());
  subcellHalo.copyFromHost(haloOnHost);

  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(NumberOfFaces),
    Clear{faceStatuses.getView(), extremaFaces.getView(), subcellHalo.getView()}
  );

  std::vector<int> statusesOnHost;
  faceStatuses.copyToHost(statusesOnHost);
  subcellHalo.copyToHost(haloOnHost);
  const auto paddedStatus = static_cast<std::size_t>(faceStatuses.getPaddedChunkSize());
  const auto paddedHalo   = static_cast<std::size_t>(subcellHalo.getPaddedChunkSize());
  for (int face = 0; face < NumberOfFaces; face++) {
    validateEqualsWithParams1(statusesOnHost[static_cast<std::size_t>(face) * paddedStatus], Regular, face);
    validateEqualsWithParams1(statusesOnHost[static_cast<std::size_t>(face) * paddedStatus + 1], Regular, face);
    for (std::size_t value = 0; value < paddedHalo; value++) {
      validateEqualsWithParams2(haloOnHost[static_cast<std::size_t>(face) * paddedHalo + value], 0.0, face, value);
    }
  }
}

void
exahype2::dg::limiting::tests::DetectorTest::testAFixedRuleReportsNoWaveSpeeds() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  const std::vector<double>                               candidate = getConstantNodes(2.0, 0.0);
  const double                                            referenceExtrema[Layout::ExtremaPerCell]{0.5, 3.0};
  CellStatus                                              cell;

  const auto statistics = FixedJudge::judgeCandidate(
    cell.status,
    cell.ownStatusHalves.data(),
    candidate.data(),
    referenceExtrema,
    tables.nodesToLobatto.data(),
    tables.nodesToSubcells.data(),
    tables.quadraturePoints.data(),
    NoRelaxation,
    NoScaling,
    centre,
    width,
    Layout::TimeStamp,
    Layout::TimeStepSize,
    cell.candidateExtrema
  );

  validateEquals(cell.status[StatusIndex], Regular);
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, 0.0);
}

/**
 * The candidate check of every cell of a device range reproduces the serial host loop.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testEveryCellOfADeviceRangeJudgesLikeTheHost() {
  const exahype2::dg::limiting::tests::SubcellProjections tables;
  const exahype2::dg::tests::CellChain                    geometry;
  const std::vector<double>                               candidates =
    exahype2::dg::tests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ValuesPerCell, 1.0, 0.01);
  std::vector<int> statuses(
    static_cast<std::size_t>(Setup::NumberOfCells) * exahype2::dg::limiting::NumberOfStatusEntries,
    0
  );
  std::vector<double> referenceExtrema(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ExtremaPerCell, 0.0);
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const auto statusOffset  = static_cast<std::size_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries;
    const auto extremaOffset = static_cast<std::size_t>(cell) * Layout::ExtremaPerCell;
    statuses[statusOffset + WasTroubledIndex]       = cell % 3 == 0 ? 1 : 0;
    statuses[statusOffset + PredictorDivergedIndex] = cell == 4 ? 1 : 0;
    referenceExtrema[extremaOffset + 1]             = cell % 2 == 0 ? 100.0 : 0.5;
  }
  const std::vector<int> faceStatuses(static_cast<std::size_t>(Setup::NumberOfCells) * peano4::FacesPerCell, 0);

  std::vector<int> deviceStatuses     = statuses;
  std::vector<int> deviceFaceStatuses = faceStatuses;
  const auto       deviceStatistics =
    judgeOnDevice(tables, geometry, deviceStatuses, deviceFaceStatuses, candidates, referenceExtrema);

  std::vector<int>                      hostStatuses     = statuses;
  std::vector<int>                      hostFaceStatuses = faceStatuses;
  exahype2::dg::MaxEigenvalueStatistics hostStatistics;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    int* ownStatusHalves[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      ownStatusHalves[faceNumber] =
        hostFaceStatuses.data() + static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber;
    }
    const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    double                                      candidateExtrema[Layout::ExtremaPerCell];
    hostStatistics = exahype2::dg::MaxEigenvalueStatistics::combine(
      hostStatistics,
      Judge::judgeCandidate(
        hostStatuses.data() + static_cast<std::ptrdiff_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries,
        ownStatusHalves,
        candidates.data() + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
        referenceExtrema.data() + static_cast<std::ptrdiff_t>(cell) * Layout::ExtremaPerCell,
        tables.nodesToLobatto.data(),
        tables.nodesToSubcells.data(),
        tables.quadraturePoints.data(),
        NoRelaxation,
        NoScaling,
        cellCentre,
        cellSize,
        Layout::TimeStamp,
        Layout::TimeStepSize,
        candidateExtrema
      )
    );
  }

  int troubled = 0;
  for (std::size_t entry = 0; entry < hostStatuses.size(); entry++) {
    validateEqualsWithParams1(deviceStatuses[entry], hostStatuses[entry], entry);
    troubled += entry % exahype2::dg::limiting::NumberOfStatusEntries == 0 and hostStatuses[entry] == Troubled ? 1 : 0;
  }
  validateEquals(troubled, Setup::NumberOfCells / 2 + 1);
  for (std::size_t entry = 0; entry < hostFaceStatuses.size(); entry++) {
    validateEqualsWithParams1(deviceFaceStatuses[entry], hostFaceStatuses[entry], entry);
  }
  validate(hostStatistics.maxPhysicalEigenvalue > 0.0);
  validateNumericalEquals(deviceStatistics.maxPhysicalEigenvalue, hostStatistics.maxPhysicalEigenvalue);
  validateNumericalEquals(deviceStatistics.maxScaledEigenvalue, hostStatistics.maxScaledEigenvalue);
}

/**
 * The block kernels count and list the cells on the subcell scheme in the order of the cell
 * connectivity with the local cells first.
 */
void
exahype2::dg::limiting::tests::DetectorTest::testTheCellListsHoldTheLocalCellsFirst() {
  constexpr int                Depth          = 1;
  constexpr int                NumberOfBlocks = 2;
  constexpr std::array<int, 4> StatusPattern{Regular, Halo, Troubled, Limited};
  peano4::tests::buildRegularGrid(Depth);
  const auto& cellConnectivity   = peano4::Spacetree::getInstance().getCellConnectivity();
  const auto  cells              = cellConnectivity.getCells();
  const int   numberOfSlots      = cellConnectivity.getNumberOfSlots();
  const int   numberOfLocalCells = cellConnectivity.getCells(peano4::Adjacency::Local).size;

  tarch::accelerator::ChunkedMemoryPool<int> statuses(exahype2::dg::limiting::NumberOfStatusEntries);
  statuses.reserve(numberOfSlots);
  std::vector<int> chunks;
  statuses.copyToHost(chunks);
  const auto                                  paddedChunkSize = static_cast<std::size_t>(statuses.getPaddedChunkSize());
  std::vector<int>                            expectedCellsOnScheme;
  std::vector<int>                            expectedRecomputedCells;
  exahype2::dg::limiting::SubcellSchemeCounts expected{};
  for (int entry = 0; entry < cells.size; entry++) {
    const int cell   = cells[entry];
    const int status = StatusPattern[static_cast<std::size_t>(entry) % StatusPattern.size()];
    chunks[static_cast<std::size_t>(cell) * paddedChunkSize + static_cast<std::size_t>(StatusIndex)] = status;
    if (status != Regular) {
      expectedCellsOnScheme.push_back(cell);
      expected.cellsOnScheme++;
    }
    if (status == Troubled or status == Limited) {
      expectedRecomputedCells.push_back(cell);
      expected.recomputedCells++;
      expected.localRecomputedCells += entry < numberOfLocalCells ? 1 : 0;
    }
    expected.troubledCells += status == Troubled ? 1 : 0;
  }
  statuses.copyFromHost(chunks);

  tarch::accelerator::Buffer<exahype2::dg::limiting::SubcellSchemeCounts> countsOfBlocks(
    NumberOfBlocks,
    tarch::MemoryLocation::Device
  );
  exahype2::dg::limiting::SubcellSchemeCounts counts{};
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfBlocks),
    exahype2::dg::limiting::CountCellsOnSubcellScheme{
      cellConnectivity.onDevice(),
      std::as_const(statuses).getView(),
      NumberOfBlocks,
      numberOfLocalCells,
      countsOfBlocks.getData()
    },
    tarch::accelerator::Custom<exahype2::dg::limiting::SubcellSchemeCountsReduction>{counts}
  );
  validateEquals(counts.cellsOnScheme, expected.cellsOnScheme);
  validateEquals(counts.recomputedCells, expected.recomputedCells);
  validateEquals(counts.localRecomputedCells, expected.localRecomputedCells);
  validateEquals(counts.troubledCells, expected.troubledCells);

  tarch::accelerator::Buffer<int> cellsOnScheme(numberOfSlots, tarch::MemoryLocation::Device);
  tarch::accelerator::Buffer<int> recomputedCells(numberOfSlots, tarch::MemoryLocation::Device);
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(NumberOfBlocks),
    exahype2::dg::limiting::ListCellsOnSubcellScheme{
      cellConnectivity.onDevice(),
      std::as_const(statuses).getView(),
      NumberOfBlocks,
      std::as_const(countsOfBlocks).getData(),
      cellsOnScheme.getData(),
      recomputedCells.getData()
    }
  );
  const std::vector<int> listedCellsOnScheme   = cellsOnScheme.getValues();
  const std::vector<int> listedRecomputedCells = recomputedCells.getValues();
  for (std::size_t entry = 0; entry < expectedCellsOnScheme.size(); entry++) {
    validateEqualsWithParams1(listedCellsOnScheme[entry], expectedCellsOnScheme[entry], entry);
  }
  for (std::size_t entry = 0; entry < expectedRecomputedCells.size(); entry++) {
    validateEqualsWithParams1(listedRecomputedCells[entry], expectedRecomputedCells[entry], entry);
  }
}
