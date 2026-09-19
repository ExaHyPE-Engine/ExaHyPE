// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RecomputeSubcellsTest.h"

#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/limiting/RecomputeSubcells.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "peano4/Constants.h"

namespace {
  using AdaptiveSolver = exahype2::dg::limiting::tests::TestSolver<true>;
  using FixedSolver    = exahype2::dg::limiting::tests::TestSolver<false>;
  using Layout         = exahype2::dg::limiting::SubcellLayout<AdaptiveSolver>;

  constexpr int HaloWidth               = exahype2::fv::reconstruction::Constant::HaloSize;
  constexpr int TangentialSubcells      = peano4::power(Layout::NumberOfSubcellsPerAxis, DIMENSIONS - 1);
  constexpr int TangentialNodes         = peano4::power(Layout::NumberOfDofsPerAxis, DIMENSIONS - 1);
  constexpr int SubcellFaceCardinality  = 2 * HaloWidth * TangentialSubcells;
  constexpr int EstimateFaceCardinality = 2 * TangentialNodes;

  using FaceLabel      = exahype2::dg::limiting::tests::TestFaceLabel;
  using FaceLabels     = exahype2::dg::limiting::tests::TestEnumerator<FaceLabel, peano4::FacesPerCell>;
  using SubcellFace    = exahype2::dg::limiting::tests::TestPatch<double, SubcellFaceCardinality>;
  using SubcellFaces   = exahype2::dg::limiting::tests::TestEnumerator<SubcellFace, peano4::FacesPerCell>;
  using EstimateFace   = exahype2::dg::limiting::tests::TestPatch<double, EstimateFaceCardinality>;
  using EstimateFaces  = exahype2::dg::limiting::tests::TestEnumerator<EstimateFace, peano4::FacesPerCell>;
  using NodalPatch     = exahype2::dg::limiting::tests::TestPatch<double, Layout::NumberOfNodes>;
  using SubcellPatch   = exahype2::dg::limiting::tests::TestPatch<double, Layout::NumberOfSubcells>;
  using SingleStep     = exahype2::fv::timestepping::SingleStep;
  using ConstantScheme = exahype2::fv::reconstruction::Constant;

  // Every column sums to one, so a constant field survives both projections.
  constexpr double
    DgToFvMatrix[Layout::NumberOfDofsPerAxis * Layout::NumberOfSubcellsPerAxis]{1.0, 0.5, 0.0, 0.0, 0.5, 1.0};
  constexpr double
    FvToDgMatrix[Layout::NumberOfSubcellsPerAxis * Layout::NumberOfDofsPerAxis]{0.5, 0.0, 0.5, 0.5, 0.0, 0.5};

  constexpr double CellCentre     = 0.5;
  constexpr double CellWidth      = 1.0;
  constexpr double TimeStepSize   = 0.05;
  constexpr double UniformState   = 1.0;
  constexpr double UntouchedState = 9.0;

  /**
   * Holds the entities of one cell that the recompute reads and writes.
   */
  struct Cell final {
    exahype2::dg::limiting::tests::TestCellLabel cellLabel{};
    FaceLabel                                    faces[peano4::FacesPerCell]{};
    EstimateFace                                 estimateStorage[peano4::FacesPerCell]{};
    SubcellFace                                  oldFaceStorage[peano4::FacesPerCell]{};
    SubcellPatch                                 subcellPatch{};
    NodalPatch                                   updatedState{};
  };

  /**
   * Fills the cell with the given status, a uniform subgrid, uniform halo faces and a nodal patch
   * the recompute has not written.
   */
  void fill(Cell& cell, const exahype2::dg::LimiterStatus status, const double subgridValue, const double haloValue) {
    cell.cellLabel.setLimiterStatus(status);
    cell.cellLabel.timeStamp    = TimeStepSize;
    cell.cellLabel.timeStepSize = TimeStepSize;
    for (auto& face : cell.oldFaceStorage) {
      std::fill_n(face.data(), SubcellFaceCardinality, haloValue);
    }
    std::fill_n(cell.subcellPatch.data(), Layout::NumberOfSubcells, subgridValue);
    std::fill_n(cell.updatedState.data(), Layout::NumberOfNodes, UntouchedState);
  }

  /**
   * Runs the recompute of the cell with the Rusanov flux of the constant reconstruction.
   */
  template <typename SolverType, bool UsesAdmissibilityCriterion, typename FallbackReconstruction>
  void recompute(Cell& cell, SolverType& solver) {
    const FaceLabels    faceLabels{cell.faces};
    const EstimateFaces estimateFaces{cell.estimateStorage};
    const SubcellFaces  oldSubcellFaces{cell.oldFaceStorage};
    exahype2::dg::limiting::recomputeSubcells<
      SolverType,
      ConstantScheme,
      FallbackReconstruction,
      SingleStep,
      HaloWidth,
      UsesAdmissibilityCriterion,
      true,
      false,
      false,
      false,
      false,
      true,
      true>(
      exahype2::dg::limiting::tests::TestGeometry::cellMarker(CellCentre, CellWidth),
      solver,
      cell.cellLabel,
      faceLabels,
      estimateFaces,
      oldSubcellFaces,
      cell.subcellPatch,
      cell.updatedState,
      DgToFvMatrix,
      FvToDgMatrix
    );
  }
} // namespace

exahype2::dg::limiting::tests::RecomputeSubcellsTest::RecomputeSubcellsTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::RecomputeSubcellsTest") {}

void exahype2::dg::limiting::tests::RecomputeSubcellsTest::run() {
  testMethod(testCellOnTheGalerkinSchemeKeepsItsSolution);
  testMethod(testLimitedCellAdvancesItsSubgridAndProjectsItBack);
  testMethod(testAFixedStepSolverReportsNoWaveSpeed);
  testMethod(testAnInadmissibleSubgridRunsTheFallback);
  testMethod(testATransitionFaceOfTheCellTakesItsRebuiltHalo);
  testMethod(testHangingFaceHaloComesFromTheOwnFaceTouchingLayer);
  testMethod(testCoarseFaceHaloComesFromTheNeighbourTrace);
  testMethod(testTheUpperCoarseFaceHaloComesFromTheUpperNeighbourHalf);
}

/**
 * Checks that a cell that does not run the subcell scheme keeps its nodal solution.
 */
void exahype2::dg::limiting::tests::RecomputeSubcellsTest::testCellOnTheGalerkinSchemeKeepsItsSolution() {
  AdaptiveSolver solver;
  Cell           cell;
  fill(cell, exahype2::dg::LimiterStatus::Halo, UniformState, UniformState);

  recompute<AdaptiveSolver, false, void>(cell, solver);

  for (int node = 0; node < Layout::NumberOfNodes; node++) {
    validateNumericalEqualsWithParams1(cell.updatedState.data()[node], UntouchedState, node);
  }
  validateNumericalEquals(solver.maxPhysicalEigenvalue, 0.0);
}

/**
 * Checks that a limited cell advances a uniform subgrid, projects it onto the nodes, keeps it and
 * reports its wave speed.
 */
void exahype2::dg::limiting::tests::RecomputeSubcellsTest::testLimitedCellAdvancesItsSubgridAndProjectsItBack() {
  AdaptiveSolver solver;
  Cell           cell;
  fill(cell, exahype2::dg::LimiterStatus::Limited, UniformState, UniformState);

  recompute<AdaptiveSolver, false, void>(cell, solver);

  for (int subcell = 0; subcell < Layout::NumberOfSubcells; subcell++) {
    validateNumericalEqualsWithParams1(cell.subcellPatch.data()[subcell], UniformState, subcell);
  }
  for (int node = 0; node < Layout::NumberOfNodes; node++) {
    validateNumericalEqualsWithParams1(cell.updatedState.data()[node], UniformState, node);
  }
  validateNumericalEquals(solver.maxPhysicalEigenvalue, AdaptiveSolver::AdvectionSpeed);
}

/**
 * Checks that a limited cell of a fixed-step solver advances its subgrid and reports no wave speed.
 */
void exahype2::dg::limiting::tests::RecomputeSubcellsTest::testAFixedStepSolverReportsNoWaveSpeed() {
  FixedSolver solver;
  Cell        cell;
  fill(cell, exahype2::dg::LimiterStatus::Troubled, UniformState, UniformState);

  recompute<FixedSolver, false, void>(cell, solver);

  for (int node = 0; node < Layout::NumberOfNodes; node++) {
    validateNumericalEqualsWithParams1(cell.updatedState.data()[node], UniformState, node);
  }
  validateNumericalEquals(solver.maxPhysicalEigenvalue, 0.0);
}

/**
 * Checks that a subgrid the admissibility term rejects advances a second time with the fallback
 * reconstruction.
 */
void exahype2::dg::limiting::tests::RecomputeSubcellsTest::testAnInadmissibleSubgridRunsTheFallback() {
  AdaptiveSolver solver;

  Cell admissibleCell;
  fill(admissibleCell, exahype2::dg::LimiterStatus::Limited, UniformState, UniformState);
  AdaptiveSolver::numberOfFluxEvaluations = 0;
  recompute<AdaptiveSolver, true, ConstantScheme>(admissibleCell, solver);
  const int fluxEvaluationsOfOneAdvance = AdaptiveSolver::numberOfFluxEvaluations;
  validateTrue(fluxEvaluationsOfOneAdvance > 0);

  Cell inadmissibleCell;
  fill(inadmissibleCell, exahype2::dg::LimiterStatus::Limited, -UniformState, -UniformState);
  AdaptiveSolver::numberOfFluxEvaluations = 0;
  recompute<AdaptiveSolver, true, ConstantScheme>(inadmissibleCell, solver);
  validateEquals(AdaptiveSolver::numberOfFluxEvaluations, 2 * fluxEvaluationsOfOneAdvance);
}

/**
 * Checks that the recompute rebuilds the outer halo layer of a hanging face of the cell from the
 * cell's own face-touching layer and leaves the other faces alone.
 */
void exahype2::dg::limiting::tests::RecomputeSubcellsTest::testATransitionFaceOfTheCellTakesItsRebuiltHalo() {
  AdaptiveSolver solver;
  Cell           cell;
  fill(cell, exahype2::dg::LimiterStatus::Limited, UniformState, UntouchedState);
  cell.faces[0].isHanging = true;

  recompute<AdaptiveSolver, false, void>(cell, solver);

  for (int tangential = 0; tangential < TangentialSubcells; tangential++) {
    validateNumericalEqualsWithParams1(
      cell.oldFaceStorage[0].data()[tangential * 2 * HaloWidth],
      UniformState,
      tangential
    );
    validateNumericalEqualsWithParams1(
      cell.oldFaceStorage[0].data()[tangential * 2 * HaloWidth + HaloWidth],
      UntouchedState,
      tangential
    );
  }
  for (int entry = 0; entry < SubcellFaceCardinality; entry++) {
    validateNumericalEqualsWithParams1(cell.oldFaceStorage[1].data()[entry], UntouchedState, entry);
  }
}

/**
 * Checks that the outer halo of a hanging face repeats the cell's own face-touching subcell layer.
 */
void exahype2::dg::limiting::tests::RecomputeSubcellsTest::testHangingFaceHaloComesFromTheOwnFaceTouchingLayer() {
  constexpr int FaceNumber = 0;
  EstimateFace  estimateFace;
  SubcellPatch  subcellPatch;
  SubcellFace   transitionFace;
  for (int tangential = 0; tangential < TangentialSubcells; tangential++) {
    subcellPatch.data()[tangential * Layout::NumberOfSubcellsPerAxis] = 11.0 + static_cast<double>(tangential);
  }

  exahype2::dg::limiting::rebuildTransitionFaceHalo<AdaptiveSolver, HaloWidth>(
    FaceNumber,
    false,
    true,
    estimateFace.data(),
    subcellPatch.data(),
    transitionFace.data(),
    DgToFvMatrix
  );

  for (int tangential = 0; tangential < TangentialSubcells; tangential++) {
    validateNumericalEqualsWithParams1(
      transitionFace.data()[tangential * 2 * HaloWidth],
      11.0 + static_cast<double>(tangential),
      tangential
    );
  }
}

/**
 * Checks that the outer halo of a lower coarse face samples the neighbour's Galerkin face trace.
 */
void exahype2::dg::limiting::tests::RecomputeSubcellsTest::testCoarseFaceHaloComesFromTheNeighbourTrace() {
  constexpr int    FaceNumber     = 0;
  constexpr double NeighbourTrace = 4.0;
  EstimateFace     estimateFace;
  std::fill_n(estimateFace.data(), EstimateFaceCardinality, NeighbourTrace);
  SubcellPatch subcellPatch;
  SubcellFace  transitionFace;

  exahype2::dg::limiting::rebuildTransitionFaceHalo<AdaptiveSolver, HaloWidth>(
    FaceNumber,
    true,
    false,
    estimateFace.data(),
    subcellPatch.data(),
    transitionFace.data(),
    DgToFvMatrix
  );

  for (int tangential = 0; tangential < TangentialSubcells; tangential++) {
    validateNumericalEqualsWithParams1(transitionFace.data()[tangential * 2 * HaloWidth], NeighbourTrace, tangential);
  }
}

/**
 * Checks that the outer halo of an upper coarse face samples the upper half of the estimate face
 * and fills the upper layers of the face.
 */
void exahype2::dg::limiting::tests::RecomputeSubcellsTest::testTheUpperCoarseFaceHaloComesFromTheUpperNeighbourHalf() {
  constexpr int    FaceNumber = DIMENSIONS;
  constexpr double LowerTrace = 2.0;
  constexpr double UpperTrace = 4.0;
  EstimateFace     estimateFace;
  std::fill_n(estimateFace.data(), TangentialNodes, LowerTrace);
  std::fill_n(estimateFace.data() + TangentialNodes, TangentialNodes, UpperTrace);
  SubcellPatch subcellPatch;
  SubcellFace  transitionFace;

  exahype2::dg::limiting::rebuildTransitionFaceHalo<AdaptiveSolver, HaloWidth>(
    FaceNumber,
    true,
    false,
    estimateFace.data(),
    subcellPatch.data(),
    transitionFace.data(),
    DgToFvMatrix
  );

  for (int tangential = 0; tangential < TangentialSubcells; tangential++) {
    validateNumericalEqualsWithParams1(
      transitionFace.data()[tangential * 2 * HaloWidth + HaloWidth],
      UpperTrace,
      tangential
    );
    validateNumericalEqualsWithParams1(transitionFace.data()[tangential * 2 * HaloWidth], 0.0, tangential);
  }
}
