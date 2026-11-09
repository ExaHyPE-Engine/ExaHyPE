// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/limiting/SubcellScheme.h"

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/limiting/LimiterStatus.h"
#include "exahype2/fv/FiniteVolumeScheme.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/Scratch.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/KernelTestData.h"
#include "unittests/exahype2/dg/limiting/LimiterTestData.h"
#include "unittests/tarch/accelerator/ChunkedMemoryPoolTestData.h"

namespace {
  using Layout      = exahype2::dg::limiting::unittests::StubLayout;
  using Setup       = exahype2::dg::unittests::KernelTestSetup;
  using Projections = exahype2::dg::limiting::unittests::SubcellProjections;
  using Scheme      = exahype2::dg::limiting::SubcellScheme<exahype2::dg::limiting::unittests::StubSolver, double>;
  using FixedScheme =
    exahype2::dg::limiting::SubcellScheme<exahype2::dg::limiting::unittests::FixedStepStubSolver, double>;

  using RuntimeParameters = exahype2::dg::limiting::unittests::RuntimeParameters;

  TARGET_DECLARE
  constexpr int HaloUnknownsPerFace = Scheme::HaloUnknownsPerFace;
  constexpr int HaloAuxVarsPerFace  = Scheme::HaloAuxVarsPerFace;
  constexpr int HaloUnknownsPerCell = peano4::FacesPerCell * HaloUnknownsPerFace;
  constexpr int HaloAuxVarsPerCell  = peano4::FacesPerCell * HaloAuxVarsPerFace;

  /**
   * The stub solver whose initial condition writes the centre and the size of a subcell into its first values and
   * counts the visits of the subcell in its last value.
   */
  struct SamplingStubSolver final : exahype2::dg::limiting::unittests::StubSolver {
    static constexpr int NumberOfAuxiliaryVariables = 2 * DIMENSIONS;

    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const RuntimeParameters&    parameters
    ) {
      ASSERTION(Q != nullptr);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        Q[axis]              = x(axis);
        Q[DIMENSIONS + axis] = h(axis);
      }
      Q[2 * DIMENSIONS] += 1.0;
    }
  };

  /**
   * The stub solver with a subcell Riemann solver that takes the unknown of the left state as the flux on both sides
   * of a face.
   */
  struct UpwindStubSolver final : exahype2::dg::limiting::unittests::StubSolver {
    static INLINE_METHOD GPU_CALLABLE_METHOD double solveSubcellRiemannProblem(
      const double* const                                           QL,
      [[maybe_unused]] const double* const                          QR,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const bool                                   isBoundaryFace,
      [[maybe_unused]] const int                                    faceIndex,
      double* const                                                 FL,
      double* const                                                 FR,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      ASSERTION(QL != nullptr and FL != nullptr and FR != nullptr);
      FL[0] = QL[0];
      FR[0] = QL[0];
      return 1.0;
    }
  };
  TARGET_DECLARE_END

  using SamplingScheme = exahype2::dg::limiting::SubcellScheme<SamplingStubSolver, double>;
  using UpwindScheme   = exahype2::dg::limiting::SubcellScheme<UpwindStubSolver, double>;

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

  std::vector<double> getConstantHalo(const double unknown) {
    std::vector<double> halo(HaloUnknownsPerCell);
    for (std::size_t entry = 0; entry < halo.size(); entry += Layout::NumberOfUnknowns) {
      halo[entry]     = unknown;
    }
    return halo;
  }

  std::vector<double> getConstantAuxVarsHalo(const double auxiliary) {
    std::vector<double> halo(HaloAuxVarsPerCell);
    for (std::size_t entry = 0; entry < halo.size(); entry += Layout::NumberOfAuxiliaryVariables) {
      halo[entry] = auxiliary;
    }
    return halo;
  }

  /**
   * Returns the number of entries of values within the tolerance of value.
   */
  int getNumberOfEntriesNear(const double* const values, const int count, const double value) {
    int result = 0;
    for (int entry = 0; entry < count; entry++) {
      result += std::abs(values[entry] - value) <= tarch::testing::NumericalTolerance ? 1 : 0;
    }
    return result;
  }

  TARGET_DECLARE
  template <typename SchemeType>
  struct RecomputeSubgridOfCell final {
    double*       nodes{nullptr};
    double*       subcells{nullptr};
    double*       scratch{nullptr};
    const double* halo{nullptr};
    const double* auxVarsHalo{nullptr};
    const double* subcellsToNodes{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};
    double        timeStepSize{Layout::TimeStepSize};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                              cell,
      const tarch::accelerator::TeamHandle   team,
      exahype2::dg::MaxEigenvalueStatistics& accumulator
    ) const {
      const double* haloHalves[peano4::FacesPerCell];
      collectHaloHalves(cell, haloHalves);
      const double* auxVarsHaloHalves[peano4::FacesPerCell];
      collectAuxVarsHaloHalves(cell, auxVarsHaloHalves);
      tarch::accelerator::reduceOverTeam(
        team,
        SchemeType::recomputeInTeam(
          team,
          nodes + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
          subcells + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
          scratch + static_cast<std::ptrdiff_t>(cell) * SchemeType::ScratchValuesPerRecomputedCell,
          haloHalves,
          auxVarsHaloHalves,
          subcellsToNodes,
          getCellCentre(cell),
          getCellSize(cell),
          0,
          Layout::TimeStamp,
          timeStepSize,
          RuntimeParameters{}
        ),
        tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{accumulator}
      );
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void collectHaloHalves(
      const int     cell,
      const double* haloHalves[peano4::FacesPerCell]
    ) const {
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        haloHalves[faceNumber] =
          halo + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * HaloUnknownsPerFace;
      }
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void collectAuxVarsHaloHalves(
      const int     cell,
      const double* haloHalves[peano4::FacesPerCell]
    ) const {
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        haloHalves[faceNumber] =
          halo + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * HaloAuxVarsPerFace;
      }
    }

    INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getCellCentre(const int cell) const {
      return tarch::la::Vector<DIMENSIONS, double>(cellCentres + static_cast<std::ptrdiff_t>(cell) * DIMENSIONS);
    }

    INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getCellSize(const int cell) const {
      return tarch::la::Vector<DIMENSIONS, double>(cellSizes + static_cast<std::ptrdiff_t>(cell) * DIMENSIONS);
    }
  };

  struct ProjectSubgridOfCell final {
    const int*    statuses{nullptr};
    double*       subcells{nullptr};
    const double* previousSolution{nullptr};
    double*       halo{nullptr};
    const double* nodesToSubcells{nullptr};
    double*       scratch{nullptr};
    bool          subcellsStoreInitialCondition{false};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell, const tarch::accelerator::TeamHandle team) const {
      double* haloHalves[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        haloHalves[faceNumber] =
          halo + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * HaloUnknownsPerFace;
      }
      Scheme::projectSubgridOntoFacesInTeam(
        team,
        statuses + static_cast<std::ptrdiff_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries,
        subcells + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
        previousSolution + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
        haloHalves,
        nodesToSubcells,
        subcellsStoreInitialCondition,
        scratch + static_cast<std::ptrdiff_t>(cell) * Scheme::ScratchValuesPerRecomputedCell
      );
    }
  };
  TARGET_DECLARE_END

  template <typename SchemeType>
  exahype2::dg::MaxEigenvalueStatistics recomputeOnDevice(
    const tarch::accelerator::TeamRange& range,
    const Projections&                   tables,
    const std::vector<double>&           cellCentres,
    const std::vector<double>&           cellSizes,
    std::vector<double>&                 nodes,
    std::vector<double>&                 subcells,
    const std::vector<double>&           halo,
    const std::vector<double>&           auxVarsHalo,
    const double                         timeStepSize = Layout::TimeStepSize
  ) {
    const auto cells = static_cast<std::size_t>(range.n);
    ASSERTION(cellCentres.size() == cells * DIMENSIONS, cellCentres.size(), cells);
    ASSERTION(cellSizes.size() == cells * DIMENSIONS, cellSizes.size(), cells);
    ASSERTION(nodes.size() == cells * Layout::ValuesPerCell, nodes.size(), cells);
    ASSERTION(subcells.size() == cells * Layout::SubcellValuesPerCell, subcells.size(), cells);
    ASSERTION(halo.size() == cells * HaloUnknownsPerCell, halo.size(), cells);
    ASSERTION(auxVarsHalo.size() == cells * HaloAuxVarsPerCell, auxVarsHalo.size(), cells);
    const std::vector<double> scratch(cells * SchemeType::ScratchValuesPerRecomputedCell, 0.0);

    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodes(nodes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceSubcells(subcells);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceScratch(scratch);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceHalo(halo);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceAuxVarsHalo(auxVarsHalo);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceSubcellsToNodes(tables.subcellsToNodes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(cellSizes);

    exahype2::dg::MaxEigenvalueStatistics statistics;
    tarch::accelerator::parallelReduce(
      range,
      RecomputeSubgridOfCell<SchemeType>{
        deviceNodes.getData(),
        deviceSubcells.getData(),
        deviceScratch.getData(),
        deviceHalo.getData(),
        deviceAuxVarsHalo.getData(),
        deviceSubcellsToNodes.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        timeStepSize
      },
      tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{statistics}
    );

    deviceNodes.copyBack(nodes);
    deviceSubcells.copyBack(subcells);
    return statistics;
  }

  template <typename SchemeType>
  exahype2::dg::MaxEigenvalueStatistics recomputeCellOnDevice(
    const Projections&                           tables,
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    std::vector<double>&                         nodes,
    std::vector<double>&                         subcells,
    const std::vector<double>&                   halo,
    const std::vector<double>&                   auxVarsHalo,
    const double                                 timeStepSize = Layout::TimeStepSize
  ) {
    std::vector<double> cellCentres(DIMENSIONS);
    std::vector<double> cellSizes(DIMENSIONS);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellCentres[static_cast<std::size_t>(axis)] = centre(axis);
      cellSizes[static_cast<std::size_t>(axis)]   = width(axis);
    }
    return recomputeOnDevice<SchemeType>(
      tarch::accelerator::TeamRange(1, SchemeType::getTeamThreads(), 0),
      tables,
      cellCentres,
      cellSizes,
      nodes,
      subcells,
      halo,
      auxVarsHalo,
      timeStepSize
    );
  }

  void projectSubgridsOnDevice(
    const tarch::accelerator::TeamRange& range,
    const Projections&                   tables,
    const std::vector<int>&              statuses,
    const std::vector<double>&           previousSolution,
    std::vector<double>&                 subcells,
    std::vector<double>&                 halo,
    const bool                           subcellsStoreInitialCondition
  ) {
    const auto cells = static_cast<std::size_t>(range.n);
    ASSERTION(statuses.size() == cells * exahype2::dg::limiting::NumberOfStatusEntries, statuses.size(), cells);
    ASSERTION(previousSolution.size() == cells * Layout::ValuesPerCell, previousSolution.size(), cells);
    ASSERTION(subcells.size() == cells * Layout::SubcellValuesPerCell, subcells.size(), cells);
    ASSERTION(halo.size() == cells * HaloUnknownsPerCell, halo.size(), cells);
    const std::vector<double> scratch(cells * Scheme::ScratchValuesPerRecomputedCell, 0.0);

    const exahype2::dg::unittests::DeviceBuffer<int>    deviceStatuses(statuses);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceSubcells(subcells);
    const exahype2::dg::unittests::DeviceBuffer<double> devicePreviousSolution(previousSolution);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceHalo(halo);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(tables.nodesToSubcells);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceScratch(scratch);
    tarch::accelerator::parallelFor(
      range,
      ProjectSubgridOfCell{
        deviceStatuses.getData(),
        deviceSubcells.getData(),
        devicePreviousSolution.getData(),
        deviceHalo.getData(),
        deviceNodesToSubcells.getData(),
        deviceScratch.getData(),
        subcellsStoreInitialCondition
      }
    );
    deviceSubcells.copyBack(subcells);
    deviceHalo.copyBack(halo);
  }
} // namespace

UNIT_TEST("A constant state projects onto constant subcells and back onto constant nodes") {
  GIVEN("A cell of unit size whose nodes, subgrid and halos store the unknown three and the auxiliary -1.5") {

    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    const std::vector<double>                   nodes       = getConstantNodes(3.0, -1.5);
    std::vector<double>                         subcells    = getConstantSubgrid(3.0, -1.5);
    const std::vector<double>                   halo        = getConstantHalo(3.0);
    const std::vector<double>                   auxVarsHalo = getConstantAuxVarsHalo(-1.5);


    WHEN("The nodes project onto the subcells") {
      std::vector<double> projected(Layout::SubcellValuesPerCell);
      Scheme::projectNodesOntoSubcells(nodes.data(), projected.data(), tables.nodesToSubcells.data());

      THEN("Every subcell equals the state") {
        for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
          VALIDATE_NUMERICAL_EQUALS(projected[static_cast<std::size_t>(subcell) * Layout::NumberOfData], 3.0, subcell);
          VALIDATE_NUMERICAL_EQUALS(
            projected[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1],
            -1.5,
            subcell
          );
        }
      }
    }

    WHEN("The subgrid recomputes with the halos and projects back onto the nodes") {
      std::vector<double> recomputedNodes(Layout::ValuesPerCell, 0.0);
      static_cast<void>(recomputeCellOnDevice<Scheme>(tables, centre, width, recomputedNodes, subcells, halo, auxVarsHalo));

      THEN("The nodes equal the state") { VALIDATE_NUMERICAL_BUFFERS_EQUALS(recomputedNodes, nodes); }
    }
  }
}

UNIT_TEST("The scheme samples the initial condition at the subcell centres") {
  GIVEN("A cell at the centre two of width 0.6") {
    const tarch::la::Vector<DIMENSIONS, double> centre(2.0);
    const tarch::la::Vector<DIMENSIONS, double> width(0.6);
    std::vector<double>                         subcells(Layout::SubcellValuesPerCell);
    std::vector<double>                         auxVarsSubfaces(HaloAuxVarsPerFace * peano4::FacesPerCell, 0.);

    double* auxVarsHaloHalves[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      auxVarsHaloHalves[faceNumber] =
        auxVarsSubfaces.data() + faceNumber * HaloAuxVarsPerFace;
    }

    WHEN("The scheme samples the initial condition of the stub solver that stores the first coordinate and the width") {
      Scheme::sampleInitialCondition(subcells.data(), auxVarsHaloHalves, centre, width, RuntimeParameters{});

      THEN("Every subcell stores the first coordinate of its centre and the width of a subcell") {
        peano4::dForEach(Layout::SubcellsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& subcell) {
          const int    index    = peano4::dLinearised(subcell, Layout::SubcellsPerAxis);
          const double expected = centre(0) - 0.5 * width(0) + (subcell(0) + 0.5) * width(0) / Layout::SubcellsPerAxis;
          VALIDATE_NUMERICAL_EQUALS(subcells[static_cast<std::size_t>(index) * Layout::NumberOfData], expected, index);
          VALIDATE_NUMERICAL_EQUALS(
            subcells[static_cast<std::size_t>(index) * Layout::NumberOfData + 1],
            width(0) / Layout::SubcellsPerAxis,
            index
          );
        });

        std::array<int, peano4::FacesPerCell> auxiliariesOnFace{};
        std::array<int, peano4::FacesPerCell> zerosOnFace{};
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          const auto face         = static_cast<std::size_t>(faceNumber);
          auxiliariesOnFace[face] = getNumberOfEntriesNear(auxVarsHaloHalves[faceNumber], HaloAuxVarsPerFace, width(0) / Layout::SubcellsPerAxis);
          zerosOnFace[face]       = getNumberOfEntriesNear(auxVarsHaloHalves[faceNumber], HaloAuxVarsPerFace, 0.0);
        }
        for (std::size_t face = 0; face < auxiliariesOnFace.size(); face++) {
          VALIDATE_EQUALS(auxiliariesOnFace[face], HaloAuxVarsPerFace/2, face);
          VALIDATE_EQUALS(zerosOnFace[face], HaloAuxVarsPerFace-auxiliariesOnFace[face], face);
        }

      }
    }
  }
}

UNIT_TEST("Sampling visits every subcell once at the centre and with the size of the volume of the same index") {
  GIVEN("A cell at the centre 0.3 of size 0.7 and a zero subgrid of the stub solver that records its samples") {
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.3);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(0.7);
    std::vector<double>                         subcells(SamplingScheme::SubcellValuesPerCell, 0.0);
    std::vector<double>                         auxVarsSubfaces(SamplingScheme::HaloAuxVarsPerFace * peano4::FacesPerCell, 0.0);

    double* auxVarsHaloHalves[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      auxVarsHaloHalves[faceNumber] =
        auxVarsSubfaces.data() + faceNumber * HaloAuxVarsPerFace;
    }

    const auto          expectedSize = exahype2::fv::getVolumeSize(cellSize, Layout::SubcellsPerAxis);
    std::vector<double> expectedCentresAndSizes;
    for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
      const auto expectedCentre = exahype2::fv::getVolumeCentre(
        cellCentre,
        cellSize,
        Layout::SubcellsPerAxis,
        peano4::dDelinearised(subcell, Layout::SubcellsPerAxis)
      );
      expectedCentresAndSizes
        .insert(expectedCentresAndSizes.end(), expectedCentre.getData(), expectedCentre.getData() + DIMENSIONS);
      expectedCentresAndSizes
        .insert(expectedCentresAndSizes.end(), expectedSize.getData(), expectedSize.getData() + DIMENSIONS);
    }

    WHEN("The scheme samples the initial condition") {
      SamplingScheme::sampleInitialCondition(subcells.data(), auxVarsHaloHalves, cellCentre, cellSize, RuntimeParameters{});
      std::vector<double> centresAndSizes;
      std::vector<double> visits;
      for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
        const auto first = subcells.begin() + static_cast<std::ptrdiff_t>(subcell) * SamplingScheme::NumberOfData;
        centresAndSizes.insert(centresAndSizes.end(), first, first + 2 * DIMENSIONS);
        visits.push_back(first[2 * DIMENSIONS]);
      }

      THEN("The scheme visits every subcell once and each stores the centre and size of the volume of its index") {
        for (std::size_t subcell = 0; subcell < visits.size(); subcell++) {
          VALIDATE_EQUALS(visits[subcell], 1.0, subcell);
        }
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(centresAndSizes, expectedCentresAndSizes);
      }
    }
  }
}

UNIT_TEST("The subcell wave speeds scale with the cell size") {
  GIVEN("A cell whose width grows along the axes and subcells whose unknown rises from one by one per subcell") {
    const Projections                           tables;
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
    const std::vector<double> halo        = getConstantHalo(1.0);
    const std::vector<double> auxVarsHalo = getConstantAuxVarsHalo(0.0);
    double                    scaled = 0.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      scaled += Layout::SubcellsPerCell / width(axis);
    }

    WHEN("The subgrid recomputes with the time step size zero") {
      std::vector<double> nodes(Layout::ValuesPerCell, 0.0);
      const auto          statistics = recomputeCellOnDevice<Scheme>(tables, centre, width, nodes, subcells, halo, auxVarsHalo, 0.0);

      THEN("The largest wave speed is the number of subcells and the scheme reports it plain and per width") {
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, static_cast<double>(Layout::SubcellsPerCell));
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, scaled);
      }
    }
  }
}

UNIT_TEST("The subgrid update of a cell equals the finite-volume update of its solver") {
  GIVEN("A cell of unit size with a subgrid and halos that rise over their values") {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    const std::vector<double>                   initialSubcells =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Layout::SubcellValuesPerCell), 1.0, 0.002);
    const std::vector<double> halo =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(HaloUnknownsPerCell), 1.5, 0.003);
    const std::vector<double> auxVarsHalo =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(HaloAuxVarsPerCell), 1.5, 0.003);
    const double* haloHalves[peano4::FacesPerCell];
    const double* auxVarsHaloHalves[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      haloHalves[faceNumber]        = halo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloUnknownsPerFace;
      auxVarsHaloHalves[faceNumber] = auxVarsHalo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloAuxVarsPerFace;
    }

    WHEN("The subgrid recomputes and the finite-volume scheme of the stub solver updates the same subgrid") {
      std::vector<double> subcells = initialSubcells;
      std::vector<double> nodes(Layout::ValuesPerCell, 0.0);
      static_cast<void>(recomputeCellOnDevice<Scheme>(tables, centre, width, nodes, subcells, halo, auxVarsHalo));
      std::vector<double> updated(Layout::SubcellValuesPerCell, 0.0);
      static_cast<void>(exahype2::fv::FiniteVolumeScheme<exahype2::dg::limiting::unittests::StubSolver, double>::update(
        initialSubcells.data(),
        updated.data(),
        haloHalves,
        auxVarsHaloHalves,
        centre,
        width,
        Layout::TimeStamp,
        Layout::TimeStepSize,
        0,
        RuntimeParameters{}
      ));

      THEN("Both give the same subgrid") { VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, updated); }
    }
  }
}

UNIT_TEST("The subgrid update of a solver with a subcell Riemann solver takes the fluxes of that Riemann solver") {
  GIVEN(
    "A cell of unit size with a subgrid of two and halos of two whose outer layer on the lower face along the "
    "first axis stores three"
  ) {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    std::vector<double>                         subcells = getConstantSubgrid(2.0, 0.5);
    std::vector<double>                         halo        = getConstantHalo(2.0);
    const std::vector<double>                   auxVarsHalo = getConstantAuxVarsHalo(0.5);
    for (int volume = 0; volume < Scheme::SubcellsPerFace; volume++) {
      halo[static_cast<std::size_t>(volume) * Layout::NumberOfUnknowns] = 3.0;
    }
    const double lowestLayerUnknown = 2.0 + Layout::TimeStepSize * Layout::SubcellsPerAxis / width(0) * (3.0 - 2.0);

    std::vector<double> expected = getConstantSubgrid(2.0, 0.5);
    for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
      if (peano4::dDelinearised(subcell, Layout::SubcellsPerAxis)(0) == 0) {
        expected[static_cast<std::size_t>(subcell) * Layout::NumberOfData] = lowestLayerUnknown;
      }
    }

    WHEN("The subgrid of the stub solver with the upwind Riemann solver recomputes") {
      std::vector<double> nodes(Layout::ValuesPerCell, 0.0);
      static_cast<void>(recomputeCellOnDevice<UpwindScheme>(tables, centre, width, nodes, subcells, halo, auxVarsHalo));

      THEN("The lowest layer along the first axis gains the flux difference one times the time step over the width") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, expected);
      }
    }
  }
}

UNIT_TEST("A constant subgrid recomputes to the constant and reports its wave speed") {
  GIVEN("A cell whose width grows along the axes and a subgrid and halos of the unknown two and the auxiliary 0.5") {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    tarch::la::Vector<DIMENSIONS, double>       width;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      width(axis) = 0.5 + 0.5 * axis;
    }
    std::vector<double>       subcells    = getConstantSubgrid(2.0, 0.5);
    const std::vector<double> halo        = getConstantHalo(2.0);
    const std::vector<double> auxVarsHalo = getConstantAuxVarsHalo(0.5);
    double                    scaled   = 0.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      scaled += 2.0 / width(axis);
    }

    WHEN("The subgrid recomputes") {
      std::vector<double> nodes(Layout::ValuesPerCell, 0.0);
      const auto          statistics = recomputeCellOnDevice<Scheme>(tables, centre, width, nodes, subcells, halo, auxVarsHalo);

      THEN("The state stays and the scheme reports the wave speed two plain and per width") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, getConstantSubgrid(2.0, 0.5));
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(nodes, getConstantNodes(2.0, 0.5));
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 2.0);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, scaled);
      }
    }
  }
}

UNIT_TEST("A subgrid of a solver with a fixed time step rule reports the wave speed zero") {
  GIVEN("A cell of unit size and a subgrid and halos of the unknown two and the auxiliary 0.5") {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    std::vector<double>                         subcells    = getConstantSubgrid(2.0, 0.5);
    const std::vector<double>                   halo        = getConstantHalo(2.0);
    const std::vector<double>                   auxVarsHalo = getConstantAuxVarsHalo(0.5);

    WHEN("The subgrid of the solver with a fixed rule recomputes") {
      std::vector<double> nodes(Layout::ValuesPerCell, 0.0);
      const auto          statistics = recomputeCellOnDevice<FixedScheme>(tables, centre, width, nodes, subcells, halo, auxVarsHalo);

      THEN("The nodes keep the state and the scheme reports the wave speed zero") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(nodes, getConstantNodes(2.0, 0.5));
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 0.0);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, 0.0);
      }
    }
  }
}

UNIT_TEST(
  "A cell projects its previous solution onto its subgrid after a Galerkin step and writes the subgrid onto "
  "its faces"
) {
  GIVEN("A previous solution of three, a subgrid of five, zero halos and the status of a regular cell") {
    const Projections         tables;
    const std::vector<double> previous = getConstantNodes(3.0, 0.25);
    std::vector<double>       subcells = getConstantSubgrid(5.0, 0.125);
    std::vector<double>       halo(HaloUnknownsPerCell, 0.0);
    double*                   haloHalves[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      haloHalves[faceNumber] = halo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloUnknownsPerFace;
    }
    std::vector<int> status{
      static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular),
      static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular),
      0,
      0
    };
    const tarch::accelerator::TeamRange oneTeam(1, Scheme::getTeamThreads(), 0);

    WHEN("The cell that ran the Galerkin scheme in the previous step writes its subgrid after the first step") {
      projectSubgridsOnDevice(oneTeam, tables, status, previous, subcells, halo, false);
      std::array<int, peano4::FacesPerCell> unknownsOnFace{};
      std::array<int, peano4::FacesPerCell> zerosOnFace{};
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto face         = static_cast<std::size_t>(faceNumber);
        unknownsOnFace[face]    = getNumberOfEntriesNear(haloHalves[faceNumber], HaloUnknownsPerFace, 3.0);
        zerosOnFace[face]       = getNumberOfEntriesNear(haloHalves[faceNumber], HaloUnknownsPerFace, 0.0);
      }

      THEN("The subgrid takes the projection of the previous solution and every face receives its boundary layer") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, getConstantSubgrid(3.0, 0.25));
        for (std::size_t face = 0; face < unknownsOnFace.size(); face++) {
          VALIDATE(unknownsOnFace[face] > 0, face, unknownsOnFace[face]);
          VALIDATE_EQUALS(zerosOnFace[face], HaloUnknownsPerFace - unknownsOnFace[face], face);
        }
      }

      AND_WHEN("The cell then ran the subcell scheme and writes a fresh subgrid of five onto its faces") {
        status[static_cast<int>(exahype2::dg::limiting::StatusEntry::PreviousStatus)] =
          static_cast<int>(exahype2::dg::limiting::LimiterStatus::Limited);
        subcells = getConstantSubgrid(5.0, 0.125);
        projectSubgridsOnDevice(oneTeam, tables, status, previous, subcells, halo, false);
        std::array<int, peano4::FacesPerCell> previousUnknownsOnFace{};
        std::array<int, peano4::FacesPerCell> subgridUnknownsOnFace{};
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          const auto face              = static_cast<std::size_t>(faceNumber);
          previousUnknownsOnFace[face] = getNumberOfEntriesNear(haloHalves[faceNumber], HaloUnknownsPerFace, 3.0);
          subgridUnknownsOnFace[face]  = getNumberOfEntriesNear(haloHalves[faceNumber], HaloUnknownsPerFace, 5.0);
        }

        THEN("The subgrid stays and every face receives the values of the subgrid only") {
          VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, getConstantSubgrid(5.0, 0.125));
          for (std::size_t face = 0; face < previousUnknownsOnFace.size(); face++) {
            VALIDATE_EQUALS(previousUnknownsOnFace[face], 0, face);
            VALIDATE(subgridUnknownsOnFace[face] > 0, face);
          }
        }
      }
    }

    WHEN("The cell writes its subgrid onto its faces in the first step") {
      projectSubgridsOnDevice(oneTeam, tables, status, previous, subcells, halo, true);

      THEN("The subgrid keeps its sampled values") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, getConstantSubgrid(5.0, 0.125));
      }
    }
  }
}

UNIT_TEST("Every team of a device range recomputes the subgrid of its cell like a team of one thread") {
  GIVEN("A chain of cells whose subgrids and halos rise over their values") {
    const Projections                        tables;
    const exahype2::dg::unittests::CellChain geometry;
    const std::vector<double>                subcells = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * Layout::SubcellValuesPerCell,
      1.0,
      0.002
    );
    const std::vector<double> halo =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * HaloUnknownsPerCell, 1.5, 0.003);
   const std::vector<double> auxVarsHalo =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * HaloAuxVarsPerCell, 1.5, 0.003);
      const std::vector<double> nodes(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ValuesPerCell, 0.0);

    WHEN("Every cell recomputes its subgrid once in a team of the device and once in a team of one thread") {
      std::vector<double> teamNodes      = nodes;
      std::vector<double> teamSubcells   = subcells;
      const auto          teamStatistics = recomputeOnDevice<Scheme>(
        tarch::accelerator::TeamRange(Setup::NumberOfCells, Scheme::getTeamThreads(), 0),
        tables,
        geometry.cellCentres,
        geometry.cellSizes,
        teamNodes,
        teamSubcells,
        halo,
        auxVarsHalo
      );
      std::vector<double> serialNodes      = nodes;
      std::vector<double> serialSubcells   = subcells;
      const auto          serialStatistics = recomputeOnDevice<Scheme>(
        tarch::accelerator::TeamRange(Setup::NumberOfCells, 1, 0),
        tables,
        geometry.cellCentres,
        geometry.cellSizes,
        serialNodes,
        serialSubcells,
        halo,
        auxVarsHalo
      );

      THEN("Both give the same subgrids, nodes and wave speeds") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(teamSubcells, serialSubcells);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(teamNodes, serialNodes);
        VALIDATE_NUMERICAL_EQUALS(teamStatistics.maxPhysicalEigenvalue, serialStatistics.maxPhysicalEigenvalue);
        VALIDATE_NUMERICAL_EQUALS(teamStatistics.maxScaledEigenvalue, serialStatistics.maxScaledEigenvalue);
      }
    }
  }
}

UNIT_TEST("Every team of a device projects the subgrid of its cell onto the faces like a team of one thread") {
  GIVEN("Rising previous solutions and subgrids of cells that ran the Galerkin and the subcell scheme in turn") {
    const Projections         tables;
    const std::vector<double> previousSolution = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ValuesPerCell,
      1.0,
      0.002
    );
    const std::vector<double> subcells = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * Layout::SubcellValuesPerCell,
      2.0,
      0.003
    );
    const std::vector<double> halo(static_cast<std::size_t>(Setup::NumberOfCells) * HaloUnknownsPerCell, 0.0);
    std::vector<int>          statuses(
      static_cast<std::size_t>(Setup::NumberOfCells) * exahype2::dg::limiting::NumberOfStatusEntries,
      0
    );
    for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
      const auto previousStatus =
        cell % 2 == 0 ? exahype2::dg::limiting::LimiterStatus::Regular : exahype2::dg::limiting::LimiterStatus::Limited;
      statuses
        [static_cast<std::size_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries
         + static_cast<std::size_t>(exahype2::dg::limiting::StatusEntry::PreviousStatus)] =
          static_cast<int>(previousStatus);
    }

    WHEN("Teams of the device and teams of one thread project every subgrid onto the faces") {
      std::vector<double> teamSubcells = subcells;
      std::vector<double> teamHalo     = halo;
      projectSubgridsOnDevice(
        tarch::accelerator::TeamRange(Setup::NumberOfCells, Scheme::getTeamThreads(), 0),
        tables,
        statuses,
        previousSolution,
        teamSubcells,
        teamHalo,
        false
      );
      std::vector<double> serialSubcells = subcells;
      std::vector<double> serialHalo     = halo;
      projectSubgridsOnDevice(
        tarch::accelerator::TeamRange(Setup::NumberOfCells, 1, 0),
        tables,
        statuses,
        previousSolution,
        serialSubcells,
        serialHalo,
        false
      );

      THEN("Both write the same subgrids and halo halves within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(teamSubcells, serialSubcells);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(teamHalo, serialHalo);
      }
    }
  }
}

UNIT_TEST(
  "Every listed cell on the Galerkin scheme projects its nodes onto its subgrid and every other subgrid stays as it is"
) {
  GIVEN("Pools of rising nodes and subgrids, cells on both schemes in turn and every slot listed but slot two") {
    constexpr int     NumberOfTeams = 2;
    constexpr int     UnlistedSlot  = 2;
    constexpr int     StatusIndex   = static_cast<int>(exahype2::dg::limiting::StatusEntry::Status);
    const Projections tables;
    tarch::accelerator::ChunkedMemoryPool<double> nodes(Layout::ValuesPerCell);
    tarch::accelerator::ChunkedMemoryPool<double> subcells(Layout::SubcellValuesPerCell);
    tarch::accelerator::ChunkedMemoryPool<int>    statuses(exahype2::dg::limiting::NumberOfStatusEntries);
    nodes.reserve(Setup::NumberOfCells);
    subcells.reserve(Setup::NumberOfCells);
    statuses.reserve(Setup::NumberOfCells);
    std::vector<double> nodeValues    = tarch::accelerator::unittests::getChunksOf(nodes);
    std::vector<double> subcellValues = tarch::accelerator::unittests::getChunksOf(subcells);
    std::vector<int>    statusValues  = tarch::accelerator::unittests::getChunksOf(statuses);
    nodeValues                        = exahype2::dg::unittests::getRamp(nodeValues.size(), 1.0, 0.002);
    subcellValues                     = exahype2::dg::unittests::getRamp(subcellValues.size(), -2.0, 0.003);
    std::fill(statusValues.begin(), statusValues.end(), 0);
    const auto       paddedNodes    = static_cast<std::size_t>(nodes.getPaddedChunkSize());
    const auto       paddedSubcells = static_cast<std::size_t>(subcells.getPaddedChunkSize());
    const auto       paddedStatus   = static_cast<std::size_t>(statuses.getPaddedChunkSize());
    std::vector<int> listedSlots;
    for (int slot = 0; slot < Setup::NumberOfCells; slot++) {
      const auto status =
        slot % 2 == 0 ? exahype2::dg::limiting::LimiterStatus::Regular : exahype2::dg::limiting::LimiterStatus::Limited;
      statusValues[static_cast<std::size_t>(slot) * paddedStatus + StatusIndex] = static_cast<int>(status);
      if (slot != UnlistedSlot) {
        listedSlots.push_back(slot);
      }
    }
    tarch::accelerator::unittests::setChunksOf(nodes, nodeValues);
    tarch::accelerator::unittests::setChunksOf(subcells, subcellValues);
    tarch::accelerator::unittests::setChunksOf(statuses, statusValues);

    WHEN("Two teams project the nodes of the listed cells onto their subgrids") {
      const exahype2::dg::unittests::DeviceBuffer<int>    deviceSlots(listedSlots);
      const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(tables.nodesToSubcells);
      tarch::accelerator::Buffer<double>                  scratch(
        static_cast<std::size_t>(tarch::accelerator::getNumberOfScratchChunks(NumberOfTeams))
          * Scheme::SubcellProjectionScratchValues,
        tarch::MemoryLocation::Device
      );
      tarch::accelerator::parallelFor(
        tarch::accelerator::TeamRange(NumberOfTeams, Scheme::getTeamThreads(), 0),
        exahype2::dg::limiting::ProjectNodesOntoSubgrids<exahype2::dg::limiting::unittests::StubSolver, double, double>{
          tarch::Span<const int>{deviceSlots.getData(), static_cast<int>(listedSlots.size())},
          NumberOfTeams,
          std::as_const(nodes).getView(),
          subcells.getView(),
          std::as_const(statuses).getView(),
          deviceNodesToSubcells.getData(),
          scratch.getData()
        }
      );
      const std::vector<double> projectedSubcells = tarch::accelerator::unittests::getChunksOf(subcells);

      THEN("Every listed Galerkin cell stores the projection of its nodes and every other subgrid its old values") {
        std::vector<double> expectedSubcells = subcellValues;
        for (const int slot : listedSlots) {
          if (slot % 2 == 0) {
            const auto slotOffset = static_cast<std::ptrdiff_t>(slot);
            Scheme::projectNodesOntoSubcells(
              nodeValues.data() + slotOffset * static_cast<std::ptrdiff_t>(paddedNodes),
              expectedSubcells.data() + slotOffset * static_cast<std::ptrdiff_t>(paddedSubcells),
              tables.nodesToSubcells.data()
            );
          }
        }
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(projectedSubcells, expectedSubcells);
      }
    }
  }
}
