// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannTest.h"

#include "TestUtils.h"

#include "../DGUtils.h"
#include "../Riemann.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "toolbox/blockstructured/Enumeration.h"

namespace {
  // A two-point Gauss-Legendre basis on the unit interval: the matrices below are what
  // exahype2.solvers.LagrangeBasisWithDiagonalMassMatrix generates for polynomial order one.
  constexpr int    NodesPerAxis = 2;
  constexpr double Node0        = 0.5 - 0.5 / 1.7320508075688772;
  constexpr double Node1        = 0.5 + 0.5 / 1.7320508075688772;

  double shapeFunction(const int node, const double x) {
    return node == 0 ? (Node1 - x) / (Node1 - Node0) : (x - Node0) / (Node1 - Node0);
  }

  std::array<double, 3 * NodesPerAxis * NodesPerAxis> interpolationMatrix() {
    std::array<double, 3 * NodesPerAxis * NodesPerAxis> result{};
    for (int block = 0; block < 3; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / 3.0;
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[block * NodesPerAxis * NodesPerAxis + fine * NodesPerAxis + coarse] = shapeFunction(coarse, fineNode);
        }
      }
    }
    return result;
  }

  std::array<double, 3 * NodesPerAxis * NodesPerAxis> restrictionMatrix() {
    std::array<double, 3 * NodesPerAxis * NodesPerAxis> result{};
    for (int block = 0; block < 3; block++) {
      for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
        for (int fine = 0; fine < NodesPerAxis; fine++) {
          const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / 3.0;
          result[block * NodesPerAxis * NodesPerAxis + coarse * NodesPerAxis + fine] =
            shapeFunction(coarse, fineNode) / 3.0;
        }
      }
    }
    return result;
  }

  constexpr std::array<double, NodesPerAxis> QuadratureWeights = {0.5, 0.5};

  double nodePosition(const int node) { return node == 0 ? Node0 : Node1; }

  // An affine trace over the tangential axes of a face is representable in the order-one
  // basis, so the face transfers have to reproduce it exactly up to roundoff.
  double affineTrace(const tarch::la::Vector<DIMENSIONS, double>& tangentialPosition, const int normal) {
    double result = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      if (d != normal) {
        result += (2.0 + d) * tangentialPosition(d);
      }
    }
    return result;
  }
} // namespace

exahype2::dg::tests::RiemannTest::RiemannTest() :
  tarch::tests::TestCase("exahype2::dg::tests::RiemannTest") {}

void exahype2::dg::tests::RiemannTest::run() {
  testMethod(testProjectVolumetricDataOntoFacesForConstantSolutionOrder3);
  testMethod(testProjectVolumetricDataOntoFacesForConstantSolutionOrder0);
  testMethod(testInterpolateProjectedFacePolynomialReproducesPolynomials);
  testMethod(testRestrictAndAccumulateProjectedFacePolynomialReproducesPolynomials);
  testMethod(testInterpolateRiemannSolutionReproducesPolynomials);
}

/**
 * The coarse-to-fine trace interpolation reproduces the coarse polynomial
 *
 * The interpolation evaluates the coarse face polynomial at the child faces'
 * nodes on the half the fine cells cannot supply, so a field the coarse basis
 * represents is reproduced exactly on every child while the fine cells' own
 * half stays untouched.
 */
void exahype2::dg::tests::RiemannTest::testInterpolateProjectedFacePolynomialReproducesPolynomials() {
  constexpr int Order              = NodesPerAxis - 1;
  constexpr int Unknowns           = 2;
  constexpr int AuxiliaryVariables = 1;
  constexpr int Stride             = Unknowns + AuxiliaryVariables;

#if DIMENSIONS == 2
  constexpr int FaceSize = NodesPerAxis * 2 * Stride;
#else
  constexpr int FaceSize = NodesPerAxis * NodesPerAxis * 2 * Stride;
#endif

  const auto interpolation = interpolationMatrix();

  for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
    const int normal     = faceNumber % DIMENSIONS;
    const int coarseSide = faceNumber < DIMENSIONS ? 0 : 1;

    std::array<double, FaceSize> coarseGridFaceQ{};
    peano4::dForEachExclusive(NodesPerAxis, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
      tarch::la::Vector<DIMENSIONS, double> position(0.0);
      for (int d = 0; d < DIMENSIONS; d++) {
        if (d != normal) {
          position(d) = nodePosition(dof(d));
        }
      }
      for (int side = 0; side < 2; side++) {
        auto shiftedDoF    = dof;
        shiftedDoF(normal) = side;
        const int offset =
          toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedDoF, NodesPerAxis, 1, normal) * Stride;
        coarseGridFaceQ[offset]     = affineTrace(position, normal) + side;
        coarseGridFaceQ[offset + 1] = 4.0 - side;
        coarseGridFaceQ[offset + 2] = -1.0;
      }
    });

    peano4::grid::GridTraversalEvent event;
    peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
      if (subCellIndex(normal) != (faceNumber < DIMENSIONS ? 0 : 2)) {
        return;
      }
      event.setRelativePositionToFather(subCellIndex);
      auto faceMarker = event.getFaceMarker();
      faceMarker.select(faceNumber);

      std::array<double, FaceSize> fineGridFaceQ{};
      fineGridFaceQ.fill(7.0);
      interpolateProjectedFacePolynomial(
        faceMarker,
        Order,
        1,
        Unknowns,
        AuxiliaryVariables,
        interpolation.data(),
        coarseGridFaceQ.data(),
        fineGridFaceQ.data()
      );

      peano4::dForEachExclusive(NodesPerAxis, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
        tarch::la::Vector<DIMENSIONS, double> position(0.0);
        for (int d = 0; d < DIMENSIONS; d++) {
          if (d != normal) {
            position(d) = (nodePosition(dof(d)) + subCellIndex(d)) / 3.0;
          }
        }
        for (int side = 0; side < 2; side++) {
          auto shiftedDoF    = dof;
          shiftedDoF(normal) = side;
          const int offset =
            toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedDoF, NodesPerAxis, 1, normal) * Stride;
          if (side == coarseSide) {
            validateNumericalEqualsWithParams3(
              fineGridFaceQ[offset],
              affineTrace(position, normal) + side,
              faceNumber,
              side,
              dof
            );
            validateNumericalEqualsWithParams3(fineGridFaceQ[offset + 1], 4.0 - side, faceNumber, side, dof);
            validateNumericalEqualsWithParams3(fineGridFaceQ[offset + 2], -1.0, faceNumber, side, dof);
          } else {
            // The fine cell's own half carries its projection and stays untouched.
            validateNumericalEqualsWithParams3(fineGridFaceQ[offset], 7.0, faceNumber, side, dof);
          }
        }
      });
    });
  }
}

/**
 * The transition flux a fine cell consumes reproduces the coarse polynomial
 *
 * The flux has to be the coarse face's flux polynomial evaluated at the hanging
 * face's nodes, written into the fine side's layer while the coarse side's
 * layer stays untouched.
 */
void exahype2::dg::tests::RiemannTest::testInterpolateRiemannSolutionReproducesPolynomials() {
  constexpr int    Order              = NodesPerAxis - 1;
  constexpr int    Unknowns           = 2;
  constexpr int    AuxiliaryVariables = 1;
  constexpr int    ProjectionStride   = Unknowns + AuxiliaryVariables;
  constexpr double MaxEigenvalue      = 2.0;
  constexpr double RestrictedTrace    = 3.0;
  constexpr double Perturbation       = 0.25;

#if DIMENSIONS == 2
  constexpr int NodesPerFaceLayer = NodesPerAxis;
#else
  constexpr int NodesPerFaceLayer = NodesPerAxis * NodesPerAxis;
#endif
  constexpr int FluxFaceSize       = NodesPerFaceLayer * 2 * Unknowns;
  constexpr int ProjectionFaceSize = NodesPerFaceLayer * 2 * ProjectionStride;

  const auto interpolation = interpolationMatrix();

  for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
    const int    normal             = faceNumber % DIMENSIONS;
    const int    coarseSideLayer    = faceNumber < DIMENSIONS ? 0 : 1;
    const double penaltyOrientation = faceNumber < DIMENSIONS ? 1.0 : -1.0;

    // The coarse flux polynomial lives in the layer the coarse neighbour's face
    // integral consumes; the opposite layer carries unrelated data. The coarse
    // projection's fine-side layer holds the restricted fine trace, here a
    // constant, whose interpolation is the same constant.
    std::array<double, FluxFaceSize>       coarseRiemannSolution{};
    std::array<double, ProjectionFaceSize> coarseProjection{};
    peano4::dForEachExclusive(NodesPerAxis, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
      tarch::la::Vector<DIMENSIONS, double> position(0.0);
      for (int d = 0; d < DIMENSIONS; d++) {
        if (d != normal) {
          position(d) = nodePosition(dof(d));
        }
      }
      for (int side = 0; side < 2; side++) {
        auto shiftedDoF    = dof;
        shiftedDoF(normal) = side;
        const int slot = toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedDoF, NodesPerAxis, 1, normal);
        coarseRiemannSolution[slot * Unknowns]        = side == coarseSideLayer ? affineTrace(position, normal) : 9.0;
        coarseRiemannSolution[slot * Unknowns + 1]    = side == coarseSideLayer ? 4.0 : 9.0;
        coarseProjection[slot * ProjectionStride]     = RestrictedTrace;
        coarseProjection[slot * ProjectionStride + 1] = RestrictedTrace;
        coarseProjection[slot * ProjectionStride + 2] = -1.0;
      }
    });

    peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
      if (subCellIndex(normal) != (faceNumber < DIMENSIONS ? 0 : 2)) {
        return;
      }

      // The fine trace matches the restriction's image except at the first node
      // of the first unknown, whose perturbation only the penalty can pick up.
      const int                              fineSideLayer = 1 - coarseSideLayer;
      std::array<double, ProjectionFaceSize> fineProjection{};
      for (int node = 0; node < 2 * NodesPerFaceLayer; node++) {
        fineProjection[node * ProjectionStride]     = RestrictedTrace;
        fineProjection[node * ProjectionStride + 1] = RestrictedTrace;
        fineProjection[node * ProjectionStride + 2] = -1.0;
      }
      tarch::la::Vector<DIMENSIONS, int> perturbedDoF(0);
      perturbedDoF(normal) = fineSideLayer;
      const int perturbedSlot =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(perturbedDoF, NodesPerAxis, 1, normal);
      fineProjection[perturbedSlot * ProjectionStride] += Perturbation;

      std::array<double, FluxFaceSize> fineRiemannSolution{};
      fineRiemannSolution.fill(7.0);
      interpolateRiemannSolution(
        faceNumber,
        subCellIndex,
        Order,
        Unknowns,
        AuxiliaryVariables,
        MaxEigenvalue,
        interpolation.data(),
        coarseRiemannSolution.data(),
        coarseProjection.data(),
        fineProjection.data(),
        fineRiemannSolution.data()
      );

      peano4::dForEachExclusive(NodesPerAxis, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
        tarch::la::Vector<DIMENSIONS, double> position(0.0);
        for (int d = 0; d < DIMENSIONS; d++) {
          if (d != normal) {
            position(d) = (nodePosition(dof(d)) + subCellIndex(d)) / 3.0;
          }
        }
        for (int side = 0; side < 2; side++) {
          auto shiftedDoF    = dof;
          shiftedDoF(normal) = side;
          const int slot = toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedDoF, NodesPerAxis, 1, normal);
          if (side == coarseSideLayer) {
            // The coarse side's layer of the hanging face stays untouched.
            validateNumericalEqualsWithParams3(fineRiemannSolution[slot * Unknowns], 7.0, faceNumber, side, dof);
            validateNumericalEqualsWithParams3(fineRiemannSolution[slot * Unknowns + 1], 7.0, faceNumber, side, dof);
          } else {
            const double penalty =
              slot == perturbedSlot ? -penaltyOrientation * 0.5 * MaxEigenvalue * Perturbation : 0.0;
            validateNumericalEqualsWithParams3(
              fineRiemannSolution[slot * Unknowns],
              affineTrace(position, normal) + penalty,
              faceNumber,
              side,
              dof
            );
            validateNumericalEqualsWithParams3(fineRiemannSolution[slot * Unknowns + 1], 4.0, faceNumber, side, dof);
          }
        }
      });
    });
  }
}

/**
 * The fine-to-coarse trace restriction reproduces the coarse polynomial
 *
 * The restriction of the projected face polynomials is the mass-lumped L2
 * projection: accumulated over the three child faces it reproduces every
 * polynomial the coarse face basis represents, the constant state included.
 */
void exahype2::dg::tests::RiemannTest::testRestrictAndAccumulateProjectedFacePolynomialReproducesPolynomials() {
  constexpr int Order              = NodesPerAxis - 1;
  constexpr int Unknowns           = 2;
  constexpr int AuxiliaryVariables = 1;
  constexpr int Stride             = Unknowns + AuxiliaryVariables;

#if DIMENSIONS == 2
  constexpr int FaceSize = NodesPerAxis * 2 * Stride;
#else
  constexpr int FaceSize = NodesPerAxis * NodesPerAxis * 2 * Stride;
#endif

  const auto restriction = restrictionMatrix();

  for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
    const int normal = faceNumber % DIMENSIONS;
    // The restriction fills the half of the coarse face the fine cells project onto.
    const int restrictedSide = faceNumber < DIMENSIONS ? 1 : 0;

    std::array<double, FaceSize> coarseGridFaceQ{};

    peano4::grid::GridTraversalEvent event;
    peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
      if (subCellIndex(normal) != (faceNumber < DIMENSIONS ? 0 : 2)) {
        return;
      }
      event.setRelativePositionToFather(subCellIndex);
      auto faceMarker = event.getFaceMarker();
      faceMarker.select(faceNumber);

      std::array<double, FaceSize> fineGridFaceQ{};
      peano4::dForEachExclusive(NodesPerAxis, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
        tarch::la::Vector<DIMENSIONS, double> position(0.0);
        for (int d = 0; d < DIMENSIONS; d++) {
          if (d != normal) {
            position(d) = (nodePosition(dof(d)) + subCellIndex(d)) / 3.0;
          }
        }
        auto shiftedDoF    = dof;
        shiftedDoF(normal) = restrictedSide;
        const int offset =
          toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedDoF, NodesPerAxis, 1, normal) * Stride;
        fineGridFaceQ[offset]     = affineTrace(position, normal);
        fineGridFaceQ[offset + 1] = 4.0;
        fineGridFaceQ[offset + 2] = -1.0;
      });

      restrictAndAccumulateProjectedFacePolynomial(
        faceMarker,
        Order,
        1,
        Unknowns,
        AuxiliaryVariables,
        restriction.data(),
        QuadratureWeights.data(),
        fineGridFaceQ.data(),
        coarseGridFaceQ.data()
      );
    });

    peano4::dForEachExclusive(NodesPerAxis, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
      tarch::la::Vector<DIMENSIONS, double> position(0.0);
      for (int d = 0; d < DIMENSIONS; d++) {
        if (d != normal) {
          position(d) = nodePosition(dof(d));
        }
      }
      for (int side = 0; side < 2; side++) {
        auto shiftedDoF    = dof;
        shiftedDoF(normal) = side;
        const int offset =
          toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedDoF, NodesPerAxis, 1, normal) * Stride;
        if (side == restrictedSide) {
          validateNumericalEqualsWithParams3(
            coarseGridFaceQ[offset],
            affineTrace(position, normal),
            faceNumber,
            side,
            dof
          );
          validateNumericalEqualsWithParams3(coarseGridFaceQ[offset + 1], 4.0, faceNumber, side, dof);
          validateNumericalEqualsWithParams3(coarseGridFaceQ[offset + 2], -1.0, faceNumber, side, dof);
        } else {
          validateNumericalEqualsWithParams3(coarseGridFaceQ[offset], 0.0, faceNumber, side, dof);
        }
      }
    });
  }
}

/**
 * Project a constant order-3 solution onto the faces
 *
 * We construct a cell patch which hosts one unknown and one auxiliary variable
 * per degree of freedom. The unknown is set to 1 and the auxiliary variable to
 * -1. We then project onto the 2d faces and look at their values. All is
 * hard-coded to order 3, i.e. we need four quadrature points per coordinate
 * axis, and the projected values have to be the same constant.
 *
 * For the left face of the cell, we have
 *
 *         ... | ...         ... | ...
 *         4,5 | 6,7    ->   0,0 | 1,-1
 *         0,1 | 2,3         0,0 | 1,-1
 *
 * where the commas separate the unknown from the auxiliary variable.
 * For the right face, we have the same enumeration, but the values
 * are mirrored along the vertical line.
 *
 * For the bottom face, we have the enumeration
 *
 *          8,9   10,11  12,13  14,15
 *         --------------------------
 *          0,1    2,3    4,5    6,7
 *
 * which translates into values
 *
 *         1,-1   1,-1   1,-1   1,-1
 *         --------------------------
 *          0,0    0,0    0,0    0,0
 *
 * In a 3d setup, both patterns are repeated in the z-direction, so
 * the same pattern restarts at index 16. The 3d case therefore means that the
 * 4*4*2=32 first entries of the left side are all 0. The remaining entries
 * then exhibit the 1,-1 pattern.
 */
void exahype2::dg::tests::RiemannTest::testProjectVolumetricDataOntoFacesForConstantSolutionOrder3() {
  const int order        = 3;
  const int nodesPerAxis = order + 1;
#if DIMENSIONS == 2
  const int nodesPerCell = nodesPerAxis * nodesPerAxis;
#else
  const int nodesPerCell = nodesPerAxis * nodesPerAxis * nodesPerAxis;
#endif
  const int nodesPerFace = nodesPerCell / nodesPerAxis;

  // 1 unknown, 1 auxiliary variable
  double cellQin[2 * nodesPerCell];
  for (int i = 0; i < 2 * nodesPerCell; i++) {
    cellQin[i] = i % 2 == 0 ? 1.0 : -1.0;
  }

  // Values on the faces: left and right, one unknown, one auxiliary variable
  double QxL[2 * nodesPerFace * 2];
  double QxR[2 * nodesPerFace * 2];
  double QyL[2 * nodesPerFace * 2];
  double QyR[2 * nodesPerFace * 2];
#if DIMENSIONS == 3
  double QzL[2 * nodesPerFace * 2];
  double QzR[2 * nodesPerFace * 2];
#endif

  for (int i = 0; i < 2 * nodesPerFace * 2; i++) {
    QxL[i] = 0.0;
    QxR[i] = 0.0;
    QyL[i] = 0.0;
    QyR[i] = 0.0;
#if DIMENSIONS == 3
    QzL[i] = 0.0;
    QzR[i] = 0.0;
#endif
  }

  projectVolumetricDataOntoFaces(
    cellQin,
    order,
    1, // unknowns
    1, // auxiliary variables
    BasisFunctionValuesLeftP3,
    QxL,
    QxR,
    QyL,
    QyR
#if DIMENSIONS == 3
    ,
    QzL,
    QzR
#endif
  );

  validateNumericalEquals(QxL[0], 0.0);
  validateNumericalEquals(QxL[1], 0.0);
  validateNumericalEquals(QxL[2], 1.0);
  validateNumericalEquals(QxL[3], -1.0);
  validateNumericalEquals(QxL[4], 0.0);
  validateNumericalEquals(QxL[5], 0.0);
  validateNumericalEquals(QxL[6], 1.0);
  validateNumericalEquals(QxL[7], -1.0);
  validateNumericalEquals(QxL[8], 0.0);
  validateNumericalEquals(QxL[9], 0.0);
#if DIMENSIONS == 3
  validateNumericalEquals(QxL[20], 0.0);
  validateNumericalEquals(QxL[21], 0.0);
  validateNumericalEquals(QxL[22], 1.0);
  validateNumericalEquals(QxL[23], -1.0);
#endif

  validateNumericalEquals(QxR[0], 1.0);
  validateNumericalEquals(QxR[1], -1.0);
  validateNumericalEquals(QxR[2], 0.0);
  validateNumericalEquals(QxR[3], 0.0);
  validateNumericalEquals(QxR[4], 1.0);
  validateNumericalEquals(QxR[5], -1.0);
  validateNumericalEquals(QxR[6], 0.0);
  validateNumericalEquals(QxR[7], 0.0);
  validateNumericalEquals(QxR[8], 1.0);
  validateNumericalEquals(QxR[9], -1.0);

  validateNumericalEquals(QyL[0], 0.0);
  validateNumericalEquals(QyL[1], 0.0);
  validateNumericalEquals(QyL[2], 0.0);
  validateNumericalEquals(QyL[3], 0.0);
  validateNumericalEquals(QyL[4], 0.0);
  validateNumericalEquals(QyL[5], 0.0);
  validateNumericalEquals(QyL[10], 1.0);
  validateNumericalEquals(QyL[11], -1.0);
  validateNumericalEquals(QyL[12], 1.0);
  validateNumericalEquals(QyL[13], -1.0);
#if DIMENSIONS == 3
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({0, 0, 0}, 3 + 1, 1, 1) * 2 + 0, 0);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({0, 0, 0}, 3 + 1, 1, 1) * 2 + 1, 1);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({1, 0, 0}, 3 + 1, 1, 1) * 2 + 0, 2);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({1, 0, 0}, 3 + 1, 1, 1) * 2 + 1, 3);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({0, 1, 0}, 3 + 1, 1, 1) * 2 + 0, 8);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({0, 1, 0}, 3 + 1, 1, 1) * 2 + 1, 9);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({1, 1, 0}, 3 + 1, 1, 1) * 2 + 0, 10);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({1, 1, 0}, 3 + 1, 1, 1) * 2 + 1, 11);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({2, 1, 0}, 3 + 1, 1, 1) * 2 + 0, 12);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({2, 1, 0}, 3 + 1, 1, 1) * 2 + 1, 13);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({3, 1, 0}, 3 + 1, 1, 1) * 2 + 0, 14);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({3, 1, 0}, 3 + 1, 1, 1) * 2 + 1, 15);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({0, 0, 1}, 3 + 1, 1, 1) * 2 + 0, 16);
  validateEquals(toolbox::blockstructured::serialiseVoxelIndexInOverlap({0, 0, 1}, 3 + 1, 1, 1) * 2 + 1, 17);

  validateNumericalEquals(QyL[16], 0.0);
  validateNumericalEquals(QyL[17], 0.0);
  validateNumericalEquals(QyL[18], 0.0);
  validateNumericalEquals(QyL[19], 0.0);
  validateNumericalEquals(QyL[20], 0.0);
  validateNumericalEquals(QyL[21], 0.0);
  validateNumericalEquals(QyL[24], 1.0);
  validateNumericalEquals(QyL[25], -1.0);
  validateNumericalEquals(QyL[26], 1.0);
  validateNumericalEquals(QyL[27], -1.0);
#endif

#if DIMENSIONS == 3
  validateNumericalEquals(QzL[0], 0.0);
  validateNumericalEquals(QzL[1], 0.0);
  validateNumericalEquals(QzL[2], 0.0);
  validateNumericalEquals(QzL[3], 0.0);
  validateNumericalEquals(QzL[4], 0.0);
  validateNumericalEquals(QzL[5], 0.0);
  validateNumericalEquals(QzL[32], 1.0);
  validateNumericalEquals(QzL[33], -1.0);

  validateNumericalEquals(QzR[0], 1.0);
  validateNumericalEquals(QzR[1], -1.0);
#endif
}

/**
 * Project a constant solution onto the faces for the order-0 Euler setup
 *
 * This is the projection that is used if DG degenerates to a Finite Volumes
 * scheme for the Euler equation, i.e. for 5 unknowns.
 */
void exahype2::dg::tests::RiemannTest::testProjectVolumetricDataOntoFacesForConstantSolutionOrder0() {
  // const int order        = 0;

  double QIn[]     = {0.1, 0.2, 0.3, 0.4, 0.5};
  double leftQ[]   = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double rightQ[]  = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double bottomQ[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double topQ[]    = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
#if DIMENSIONS == 3
  double frontQ[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double backQ[]  = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
#endif

#if DIMENSIONS == 2
  ::exahype2::dg::projectVolumetricDataOntoFaces(QIn, 0, 5, 0, BasisFunctionValuesLeftP0, leftQ, rightQ, bottomQ, topQ);
#endif

#if DIMENSIONS == 3
  ::exahype2::dg::projectVolumetricDataOntoFaces(
    QIn,
    0,
    5,
    0,
    BasisFunctionValuesLeftP0,
    leftQ,
    rightQ,
    bottomQ,
    topQ,
    frontQ,
    backQ
  );
#endif

  validateNumericalEquals(leftQ[0], 0.0);
  validateNumericalEquals(leftQ[1], 0.0);
  validateNumericalEquals(leftQ[2], 0.0);
  validateNumericalEquals(leftQ[3], 0.0);
  validateNumericalEquals(leftQ[4], 0.0);
  validateNumericalEquals(leftQ[5], 0.1);
}
