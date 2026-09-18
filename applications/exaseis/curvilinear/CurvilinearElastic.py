def initial(pointwise_initial_conditions):
    return (
        """
  std::fill_n(Q, (NumberOfAuxiliaryVariables + NumberOfUnknowns) * (Order + 1) * (Order + 1) * (Order + 1), 0);

  tarch::la::Vector<DIMENSIONS, double> center = (*_curvi)(x);

  auto initUnknownsPointwise = [&](
    const tarch::la::Vector<DIMENSIONS,double>& x,
    const tarch::la::Vector<DIMENSIONS,double>& center,
    {{SOLUTION_STORAGE_PRECISION}}* Q
  ) -> void {"""
        + pointwise_initial_conditions
        + """
  };

  int linearisedIndex = 0;
  peano4::dForEach(Order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
    tarch::la::Vector<DIMENSIONS, double> curviPos = (*_curvi)(
      ::exahype2::dg::getQuadraturePoint(
        x,
        h,
        index,
        kernels::ElasticSolver::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes
      )
    );
    Q[linearisedIndex+Shortcuts::curve_grid + 0] = curviPos[0];
    Q[linearisedIndex+Shortcuts::curve_grid + 1] = curviPos[1];
    Q[linearisedIndex+Shortcuts::curve_grid + 2] = curviPos[2];

    initUnknownsPointwise(
      curviPos, center, Q+linearisedIndex
    );

    linearisedIndex += NumberOfUnknowns + NumberOfAuxiliaryVariables;
  });

  const tarch::la::Vector<DIMENSIONS, {{SOLUTION_STORAGE_PRECISION}}> invDx = convertScalar<{{SOLUTION_STORAGE_PRECISION}}>(invertEntries(h));

  constexpr int nDof    = Order+1;
  constexpr int nDofPad = {{NUMBER_OF_DOF_PADDED}};
  constexpr int strideQ = (NumberOfUnknowns+NumberOfAuxiliaryVariables);

  // x-loop
  for (int yz = 0; yz < nDof*nDof; yz++) {
    UNROLL(nDof)
    for (int row = 0; row < nDof; row++) {
      UNROLL(nDof)
      for (int column = 0; column < nDof; column++) {
        // x derivatives
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          Q[yz*nDof*strideQ + column*strideQ + Shortcuts::metric_derivative + axis] +=
            Q[yz*nDof*strideQ + row*strideQ + Shortcuts::curve_grid + axis]
            * invDx[0] * kernels::ElasticSolver::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudxTransposed[column*nDofPad + row];
        }
      }
    } // row
  } // yz

  // y-loop
  for (int z = 0; z < nDof; z++) {
    for (int x = 0; x < nDof; x++) {
      UNROLL(nDof)
      for (int row = 0; row < nDof; row++) {
        UNROLL(nDof)
        for (int column = 0; column < nDof; column++) {
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            Q[z*nDof*nDof*strideQ + x*strideQ + column*nDof*strideQ + Shortcuts::metric_derivative + DIMENSIONS + axis] +=
              Q[z*nDof*nDof*strideQ + x*strideQ + row*nDof*strideQ + Shortcuts::curve_grid + axis]
              * invDx[1] * kernels::ElasticSolver::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudxTransposed[column*nDofPad + row];
          } // axis
        } // column
      } // row
    } // x
  } // z

  // z-loop
  for (int xy = 0; xy < nDof*nDof; xy++) {
    UNROLL(nDof)
    for (int row = 0; row < nDof; row++) {
      UNROLL(nDof)
      for (int column = 0; column < nDof; column++) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          Q[xy*strideQ + column*nDof*nDof*strideQ + Shortcuts::metric_derivative + 2*DIMENSIONS + axis] +=
            Q[xy*strideQ + row*nDof*nDof*strideQ + Shortcuts::curve_grid + axis]
            * invDx[2] * kernels::ElasticSolver::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudxTransposed[column*nDofPad + row];
        } // axis
      } // column
    } // row
  } // xy

  linearisedIndex = 0;
  peano4::dForEach(Order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
    auto curveXDerivativeQ = Q[linearisedIndex+Shortcuts::metric_derivative+0];
    auto curveYDerivativeQ = Q[linearisedIndex+Shortcuts::metric_derivative+1];
    auto curveZDerivativeQ = Q[linearisedIndex+Shortcuts::metric_derivative+2];

    auto curveXDerivativeR = Q[linearisedIndex+Shortcuts::metric_derivative+DIMENSIONS+0];
    auto curveYDerivativeR = Q[linearisedIndex+Shortcuts::metric_derivative+DIMENSIONS+1];
    auto curveZDerivativeR = Q[linearisedIndex+Shortcuts::metric_derivative+DIMENSIONS+2];

    auto curveXDerivativeS = Q[linearisedIndex+Shortcuts::metric_derivative+2*DIMENSIONS+0];
    auto curveYDerivativeS = Q[linearisedIndex+Shortcuts::metric_derivative+2*DIMENSIONS+1];
    auto curveZDerivativeS = Q[linearisedIndex+Shortcuts::metric_derivative+2*DIMENSIONS+2];

    auto jacobian =   curveXDerivativeQ * (curveYDerivativeR * curveZDerivativeS - curveYDerivativeS * curveZDerivativeR)
                    - curveXDerivativeR * (curveYDerivativeQ * curveZDerivativeS - curveYDerivativeS * curveZDerivativeQ)
                    + curveXDerivativeS * (curveYDerivativeQ * curveZDerivativeR - curveYDerivativeR * curveZDerivativeQ);

    Q[linearisedIndex+Shortcuts::jacobian] = jacobian;
    Q[linearisedIndex+Shortcuts::metric_derivative+0] = (1 / jacobian) * (curveYDerivativeR * curveZDerivativeS - curveZDerivativeR * curveYDerivativeS);
    Q[linearisedIndex+Shortcuts::metric_derivative+3] = (1 / jacobian) * (curveZDerivativeQ * curveYDerivativeS - curveYDerivativeQ * curveZDerivativeS);
    Q[linearisedIndex+Shortcuts::metric_derivative+6] = (1 / jacobian) * (curveYDerivativeQ * curveZDerivativeR - curveZDerivativeQ * curveYDerivativeR);

    Q[linearisedIndex+Shortcuts::metric_derivative+1] = (1 / jacobian) * (curveZDerivativeR * curveXDerivativeS - curveXDerivativeR * curveZDerivativeS);
    Q[linearisedIndex+Shortcuts::metric_derivative+4] = (1 / jacobian) * (curveXDerivativeQ * curveZDerivativeS - curveZDerivativeQ * curveXDerivativeS);
    Q[linearisedIndex+Shortcuts::metric_derivative+7] = (1 / jacobian) * (curveZDerivativeQ * curveXDerivativeR - curveXDerivativeQ * curveZDerivativeR);

    Q[linearisedIndex+Shortcuts::metric_derivative+2] = (1 / jacobian) * (curveXDerivativeR * curveYDerivativeS - curveYDerivativeR * curveXDerivativeS);
    Q[linearisedIndex+Shortcuts::metric_derivative+5] = (1 / jacobian) * (curveYDerivativeQ * curveXDerivativeS - curveXDerivativeQ * curveYDerivativeS);
    Q[linearisedIndex+Shortcuts::metric_derivative+8] = (1 / jacobian) * (curveXDerivativeQ * curveYDerivativeR - curveYDerivativeQ * curveXDerivativeR);

    linearisedIndex += strideQ;
  });
"""
    )


def boundary():
    return """
  std::copy_n(Qinside, NumberOfUnknowns + NumberOfAuxiliaryVariables, Qoutside);
"""


def eigenvalue():
    return """
  assertion(
    not (std::isnan(Q[Shortcuts::v + 0]) or std::isnan(Q[Shortcuts::v + 1]) or std::isnan(Q[Shortcuts::v + 2])),
    "Velocity must not be NaN",
    Q[Shortcuts::v + 0],
    Q[Shortcuts::v + 1],
    Q[Shortcuts::v + 2]
  );

  auto cp  = Q[Shortcuts::cp];
  auto cs  = Q[Shortcuts::cs];
  auto qDerivativeX = Q[Shortcuts::metric_derivative + 0];
  auto qDerivativeY = Q[Shortcuts::metric_derivative + 1];
  auto qDerivativeZ = Q[Shortcuts::metric_derivative + 2];
  auto rDerivativeX = Q[Shortcuts::metric_derivative + 3];
  auto rDerivativeY = Q[Shortcuts::metric_derivative + 4];
  auto rDerivativeZ = Q[Shortcuts::metric_derivative + 5];
  auto sDerivativeX = Q[Shortcuts::metric_derivative + 6];
  auto sDerivativeY = Q[Shortcuts::metric_derivative + 7];
  auto sDerivativeZ = Q[Shortcuts::metric_derivative + 8];

  double lambda[10]{1.};

  lambda[0] = (double) (std::sqrt(qDerivativeX * qDerivativeX + qDerivativeY * qDerivativeY + qDerivativeZ * qDerivativeZ) * cp);
  lambda[1] = (double) (std::sqrt(qDerivativeX * qDerivativeX + qDerivativeY * qDerivativeY + qDerivativeZ * qDerivativeZ) * cs);
  lambda[2] = (double) (std::sqrt(qDerivativeX * qDerivativeX + qDerivativeY * qDerivativeY + qDerivativeZ * qDerivativeZ) * cs);

  lambda[3] = (double) (std::sqrt(rDerivativeX * rDerivativeX + rDerivativeY * rDerivativeY + rDerivativeZ * rDerivativeZ) * cp);
  lambda[4] = (double) (std::sqrt(rDerivativeX * rDerivativeX + rDerivativeY * rDerivativeY + rDerivativeZ * rDerivativeZ) * cs);
  lambda[5] = (double) (std::sqrt(rDerivativeX * rDerivativeX + rDerivativeY * rDerivativeY + rDerivativeZ * rDerivativeZ) * cs);

  lambda[6] = (double) (std::sqrt(sDerivativeX * sDerivativeX + sDerivativeY * sDerivativeY + sDerivativeZ * sDerivativeZ) * cp);
  lambda[7] = (double) (std::sqrt(sDerivativeX * sDerivativeX + sDerivativeY * sDerivativeY + sDerivativeZ * sDerivativeZ) * cs);
  lambda[8] = (double) (std::sqrt(sDerivativeX * sDerivativeX + sDerivativeY * sDerivativeY + sDerivativeZ * sDerivativeZ) * cs);

  lambda[9] = 1.;

  return *std::max_element(lambda, lambda + 10);
"""


def flux():
    return """
  auto sigmaXX = Q[Shortcuts::sigma + 0];
  auto sigmaYY = Q[Shortcuts::sigma + 1];
  auto sigmaZZ = Q[Shortcuts::sigma + 2];
  auto sigmaXY = Q[Shortcuts::sigma + 3];
  auto sigmaXZ = Q[Shortcuts::sigma + 4];
  auto sigmaYZ = Q[Shortcuts::sigma + 5];

  auto jacobian = Q[Shortcuts::jacobian];

  std::fill_n(F, NumberOfUnknowns, 0.);

  switch (normal) {
    case 0: {
      auto qDerivativeX = Q[Shortcuts::metric_derivative + 0];
      auto qDerivativeY = Q[Shortcuts::metric_derivative + 1];
      auto qDerivativeZ = Q[Shortcuts::metric_derivative + 2];
      F[Shortcuts::v+0] = -jacobian * (qDerivativeX * sigmaXX + qDerivativeY * sigmaXY + qDerivativeZ * sigmaXZ);
      F[Shortcuts::v+1] = -jacobian * (qDerivativeX * sigmaXY + qDerivativeY * sigmaYY + qDerivativeZ * sigmaYZ);
      F[Shortcuts::v+2] = -jacobian * (qDerivativeX * sigmaXZ + qDerivativeY * sigmaYZ + qDerivativeZ * sigmaZZ);
    } break;
    case 1: {
      auto rDerivativeX = Q[Shortcuts::metric_derivative + 3];
      auto rDerivativeY = Q[Shortcuts::metric_derivative + 4];
      auto rDerivativeZ = Q[Shortcuts::metric_derivative + 5];
      F[Shortcuts::v+0] = -jacobian * (rDerivativeX * sigmaXX + rDerivativeY * sigmaXY + rDerivativeZ * sigmaXZ);
      F[Shortcuts::v+1] = -jacobian * (rDerivativeX * sigmaXY + rDerivativeY * sigmaYY + rDerivativeZ * sigmaYZ);
      F[Shortcuts::v+2] = -jacobian * (rDerivativeX * sigmaXZ + rDerivativeY * sigmaYZ + rDerivativeZ * sigmaZZ);
    } break;
    case 2: {
      auto sDerivativeX = Q[Shortcuts::metric_derivative + 6];
      auto sDerivativeY = Q[Shortcuts::metric_derivative + 7];
      auto sDerivativeZ = Q[Shortcuts::metric_derivative + 8];
      F[Shortcuts::v+0] = -jacobian * (sDerivativeX * sigmaXX + sDerivativeY * sigmaXY + sDerivativeZ * sigmaXZ);
      F[Shortcuts::v+1] = -jacobian * (sDerivativeX * sigmaXY + sDerivativeY * sigmaYY + sDerivativeZ * sigmaYZ);
      F[Shortcuts::v+2] = -jacobian * (sDerivativeX * sigmaXZ + sDerivativeY * sigmaYZ + sDerivativeZ * sigmaZZ);
    }
  }
"""


def ncp():
    return """
  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0.0);

  switch (normal) {
    case 0: {
      auto velocityJumpX = deltaQ[Shortcuts::v + 0];
      auto velocityJumpY = deltaQ[Shortcuts::v + 1];
      auto velocityJumpZ = deltaQ[Shortcuts::v + 2];
      auto qDerivativeX = Q[Shortcuts::metric_derivative + 0];
      auto qDerivativeY = Q[Shortcuts::metric_derivative + 1];
      auto qDerivativeZ = Q[Shortcuts::metric_derivative + 2];
      BTimesDeltaQ[Shortcuts::sigma + 0] = -qDerivativeX*velocityJumpX;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -qDerivativeY*velocityJumpY;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -qDerivativeZ*velocityJumpZ;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -(qDerivativeY*velocityJumpX+qDerivativeX*velocityJumpY); //sigmaXY
      BTimesDeltaQ[Shortcuts::sigma + 4] = -(qDerivativeZ*velocityJumpX+qDerivativeX*velocityJumpZ); //sigmaXZ
      BTimesDeltaQ[Shortcuts::sigma + 5] = -(qDerivativeZ*velocityJumpY+qDerivativeY*velocityJumpZ); //sigmaYZ
    } break;
    case 1: {
      auto velocityJumpX = deltaQ[Shortcuts::v + 0];
      auto velocityJumpY = deltaQ[Shortcuts::v + 1];
      auto velocityJumpZ = deltaQ[Shortcuts::v + 2];
      auto rDerivativeX = Q[Shortcuts::metric_derivative + 3];
      auto rDerivativeY = Q[Shortcuts::metric_derivative + 4];
      auto rDerivativeZ = Q[Shortcuts::metric_derivative + 5];
      BTimesDeltaQ[Shortcuts::sigma + 0] = -rDerivativeX*velocityJumpX;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -rDerivativeY*velocityJumpY;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -rDerivativeZ*velocityJumpZ;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -(rDerivativeY*velocityJumpX+rDerivativeX*velocityJumpY); //sigmaXY
      BTimesDeltaQ[Shortcuts::sigma + 4] = -(rDerivativeZ*velocityJumpX+rDerivativeX*velocityJumpZ); //sigmaXZ
      BTimesDeltaQ[Shortcuts::sigma + 5] = -(rDerivativeZ*velocityJumpY+rDerivativeY*velocityJumpZ); //sigmaYZ
    } break;
    case 2: {
      auto velocityJumpX = deltaQ[Shortcuts::v + 0];
      auto velocityJumpY = deltaQ[Shortcuts::v + 1];
      auto velocityJumpZ = deltaQ[Shortcuts::v + 2];
      auto sDerivativeX = Q[Shortcuts::metric_derivative + 6];
      auto sDerivativeY = Q[Shortcuts::metric_derivative + 7];
      auto sDerivativeZ = Q[Shortcuts::metric_derivative + 8];
      BTimesDeltaQ[Shortcuts::sigma + 0] = -sDerivativeX*velocityJumpX;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -sDerivativeY*velocityJumpY;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -sDerivativeZ*velocityJumpZ;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -(sDerivativeY*velocityJumpX+sDerivativeX*velocityJumpY); //sigmaXY
      BTimesDeltaQ[Shortcuts::sigma + 4] = -(sDerivativeZ*velocityJumpX+sDerivativeX*velocityJumpZ); //sigmaXZ
      BTimesDeltaQ[Shortcuts::sigma + 5] = -(sDerivativeZ*velocityJumpY+sDerivativeY*velocityJumpZ); //sigmaYZ
    }
  }
"""


def multiplyMaterialParameterMatrix():
    return """
  auto rho = Q[Shortcuts::rho];
  auto cp  = Q[Shortcuts::cp];
  auto cs  = Q[Shortcuts::cs];
  auto jacobian = Q[Shortcuts::jacobian];

  auto mu     = rho * cs * cs;
  auto lambda = rho * cp * cp - 2 * mu;
  auto inverseDensityTimesJacobian = 1/(rho*jacobian);

  // The right-hand side uses the same formula for every axis.

  rhs[Shortcuts::v + 0] = inverseDensityTimesJacobian * rhs[Shortcuts::v + 0];
  rhs[Shortcuts::v + 1] = inverseDensityTimesJacobian * rhs[Shortcuts::v + 1];
  rhs[Shortcuts::v + 2] = inverseDensityTimesJacobian * rhs[Shortcuts::v + 2];

  auto lambdaTimesStressTrace = lambda * (rhs[Shortcuts::sigma+0] + rhs[Shortcuts::sigma+1] + rhs[Shortcuts::sigma+2]);

  rhs[Shortcuts::sigma+0] = (2*mu) * rhs[Shortcuts::sigma+0] + lambdaTimesStressTrace;
  rhs[Shortcuts::sigma+1] = (2*mu) * rhs[Shortcuts::sigma+1] + lambdaTimesStressTrace;
  rhs[Shortcuts::sigma+2] = (2*mu) * rhs[Shortcuts::sigma+2] + lambdaTimesStressTrace;

  rhs[Shortcuts::sigma+3] = mu*rhs[Shortcuts::sigma+3];
  rhs[Shortcuts::sigma+4] = mu*rhs[Shortcuts::sigma+4];
  rhs[Shortcuts::sigma+5] = mu*rhs[Shortcuts::sigma+5];
"""


def refinement_criterion():
    return """
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
"""


def riemann_solver():
    return """
  applications::exahype2::exaseis::riemannSolverIsotropic<VariableShortcuts{{SOLVER_NAME}}, {{CORRECTOR_COMPUTATION_PRECISION}}, Order + 1, NumberOfUnknowns, (NumberOfUnknowns + NumberOfAuxiliaryVariables), 1>(
    FL, FR, QL, QR, t, dt, direction, isBoundaryFace, faceIndex
  );
"""


def userIncludes():
    return """
#include <memory>

#include "../riemannSolvers/curvilinearRoutines.h"
#include "../riemannSolvers/riemannsolverIsotropic.h"

#include "exahype2/Curvi.h"
#include "exahype2/dg/DGUtils.h"

#include "peano4/sfc/ActiveCurve.h"

#include "tarch/reader/NetCDFFieldSampler.h"
"""


def abstractUserDefinitions():
    return """
std::unique_ptr<::exahype2::Curvi> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_curvi;

tarch::la::Vector<DIMENSIONS,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::invertProjection(const tarch::la::Vector<DIMENSIONS,double> coordinates){

  return _curvi->invertProjection(coordinates);

}
"""


def abstractDeclarations():
    return """
public:
  tarch::la::Vector<DIMENSIONS,double> invertProjection(const tarch::la::Vector<DIMENSIONS,double> coordinates);

protected:
  static std::unique_ptr<::exahype2::Curvi> _curvi;
"""


def init_grid_step_implementation(scenario_perturbations):
    return (
        """
  // Width of a cell on the finest grid level.
  tarch::la::Vector<DIMENSIONS, double> minCellSize = DomainSize;
  const double minAdmissiblePatchSize = ::runner::RuntimeParameters::getInstance().getMinAdmissiblePatchSize(DomainSize);
  while (min(minCellSize) > minAdmissiblePatchSize) {
    minCellSize /= static_cast<double>(peano4::sfc::ActiveCurve::SubdivisionFactor);
  }

  // Grid initialisation is re-run once per round of static AMR, so this hook fires more than once.
  // Assigning to the unique_ptr releases the Curvi the earlier round built, and the perturbations
  // below then rebuild it from scratch.
  _curvi = std::make_unique<::exahype2::Curvi>(
    DomainOffset,
    DomainSize,
    minCellSize
  );

"""
        + scenario_perturbations
    )
