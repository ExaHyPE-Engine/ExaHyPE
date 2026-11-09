# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def initial(pointwise_initial_conditions):
    return (
        """
  constexpr int NodesPerAxis = DGOrder + 1;
  constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  std::fill_n(Q, NumberOfData * NodesPerAxis * NodesPerAxis * NodesPerAxis, 0);

  const tarch::la::Vector<DIMENSIONS, double> cellCentre = (*_curvi)(x);

  const auto initialiseNode = [&](
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    {{STORAGE_PRECISION}}* const Q
  ) -> void {"""
        + pointwise_initial_conditions
        + """
  };

  int nodeOffset = 0;
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
    const tarch::la::Vector<DIMENSIONS, double> curvilinearPosition = (*_curvi)(::exahype2::dg::getQuadraturePoint(
      x,
      h,
      index,
      Quadrature<{{STORAGE_PRECISION}}>::getQuadraturePoints1D().data()
    ));
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      Q[nodeOffset + Shortcuts::curveGrid + axis] = curvilinearPosition[axis];
    }
    initialiseNode(curvilinearPosition, cellCentre, Q + nodeOffset);
    nodeOffset += NumberOfData;
  });

  // Differentiates the curvilinear positions along each reference axis into the metric entries.
  const tarch::la::Vector<DIMENSIONS, {{STORAGE_PRECISION}}> inverseCellSize =
    convertScalar<{{STORAGE_PRECISION}}>(invertEntries(h));
  const auto& derivativeOperator = DGMatrices<{{STORAGE_PRECISION}}>::getDerivativeOperator1D();
  for (int yz = 0; yz < NodesPerAxis * NodesPerAxis; yz++) {
    UNROLL(NodesPerAxis)
    for (int row = 0; row < NodesPerAxis; row++) {
      UNROLL(NodesPerAxis)
      for (int column = 0; column < NodesPerAxis; column++) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          Q[yz * NodesPerAxis * NumberOfData + column * NumberOfData + Shortcuts::metricDerivative + axis] +=
            Q[yz * NodesPerAxis * NumberOfData + row * NumberOfData + Shortcuts::curveGrid + axis]
            * inverseCellSize[0] * derivativeOperator[column * NodesPerAxis + row];
        }
      }
    }
  }
  for (int zNode = 0; zNode < NodesPerAxis; zNode++) {
    for (int xNode = 0; xNode < NodesPerAxis; xNode++) {
      const int lineOffset = zNode * NodesPerAxis * NodesPerAxis * NumberOfData + xNode * NumberOfData;
      UNROLL(NodesPerAxis)
      for (int row = 0; row < NodesPerAxis; row++) {
        UNROLL(NodesPerAxis)
        for (int column = 0; column < NodesPerAxis; column++) {
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            Q[lineOffset + column * NodesPerAxis * NumberOfData + Shortcuts::metricDerivative + DIMENSIONS + axis] +=
              Q[lineOffset + row * NodesPerAxis * NumberOfData + Shortcuts::curveGrid + axis]
              * inverseCellSize[1] * derivativeOperator[column * NodesPerAxis + row];
          }
        }
      }
    }
  }
  for (int xy = 0; xy < NodesPerAxis * NodesPerAxis; xy++) {
    UNROLL(NodesPerAxis)
    for (int row = 0; row < NodesPerAxis; row++) {
      UNROLL(NodesPerAxis)
      for (int column = 0; column < NodesPerAxis; column++) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          Q[xy * NumberOfData + column * NodesPerAxis * NodesPerAxis * NumberOfData + Shortcuts::metricDerivative
            + 2 * DIMENSIONS + axis] +=
            Q[xy * NumberOfData + row * NodesPerAxis * NodesPerAxis * NumberOfData + Shortcuts::curveGrid + axis]
            * inverseCellSize[2] * derivativeOperator[column * NodesPerAxis + row];
        }
      }
    }
  }

  // Replaces the position derivatives of every node by the inverse metric and stores their
  // determinant as the Jacobian.
  nodeOffset = 0;
  peano4::dForEach(NodesPerAxis, [&]([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, int>& index) {
    {{STORAGE_PRECISION}}* const metric = Q + nodeOffset + Shortcuts::metricDerivative;
    const auto curveXDerivativeQ = metric[0];
    const auto curveYDerivativeQ = metric[1];
    const auto curveZDerivativeQ = metric[2];
    const auto curveXDerivativeR = metric[DIMENSIONS + 0];
    const auto curveYDerivativeR = metric[DIMENSIONS + 1];
    const auto curveZDerivativeR = metric[DIMENSIONS + 2];
    const auto curveXDerivativeS = metric[2 * DIMENSIONS + 0];
    const auto curveYDerivativeS = metric[2 * DIMENSIONS + 1];
    const auto curveZDerivativeS = metric[2 * DIMENSIONS + 2];

    const auto jacobian = curveXDerivativeQ * (curveYDerivativeR * curveZDerivativeS - curveYDerivativeS * curveZDerivativeR)
                        - curveXDerivativeR * (curveYDerivativeQ * curveZDerivativeS - curveYDerivativeS * curveZDerivativeQ)
                        + curveXDerivativeS * (curveYDerivativeQ * curveZDerivativeR - curveYDerivativeR * curveZDerivativeQ);
    ASSERTION(jacobian != 0, jacobian);

    Q[nodeOffset + Shortcuts::jacobian] = jacobian;
    metric[0] = (1 / jacobian) * (curveYDerivativeR * curveZDerivativeS - curveZDerivativeR * curveYDerivativeS);
    metric[3] = (1 / jacobian) * (curveZDerivativeQ * curveYDerivativeS - curveYDerivativeQ * curveZDerivativeS);
    metric[6] = (1 / jacobian) * (curveYDerivativeQ * curveZDerivativeR - curveZDerivativeQ * curveYDerivativeR);
    metric[1] = (1 / jacobian) * (curveZDerivativeR * curveXDerivativeS - curveXDerivativeR * curveZDerivativeS);
    metric[4] = (1 / jacobian) * (curveXDerivativeQ * curveZDerivativeS - curveZDerivativeQ * curveXDerivativeS);
    metric[7] = (1 / jacobian) * (curveZDerivativeQ * curveXDerivativeR - curveXDerivativeQ * curveZDerivativeR);
    metric[2] = (1 / jacobian) * (curveXDerivativeR * curveYDerivativeS - curveYDerivativeR * curveXDerivativeS);
    metric[5] = (1 / jacobian) * (curveYDerivativeQ * curveXDerivativeS - curveXDerivativeQ * curveYDerivativeS);
    metric[8] = (1 / jacobian) * (curveXDerivativeQ * curveYDerivativeR - curveYDerivativeQ * curveXDerivativeR);

    nodeOffset += NumberOfData;
  });
"""
    )


def boundary():
    return """
  for (int value = 0; value < NumberOfUnknowns + NumberOfAuxiliaryVariables; value++) {
    Qoutside[value] = Qinside[value];
  }
"""


def eigenvalue():
    return """
  ASSERTION(
    not (std::isnan(Q[Shortcuts::v + 0]) or std::isnan(Q[Shortcuts::v + 1]) or std::isnan(Q[Shortcuts::v + 2])),
    Q[Shortcuts::v + 0],
    Q[Shortcuts::v + 1],
    Q[Shortcuts::v + 2]
  );
  double result = 1.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const auto* const metric     = Q + Shortcuts::metricDerivative + static_cast<std::ptrdiff_t>(axis) * DIMENSIONS;
    const auto        metricNorm = std::sqrt(metric[0] * metric[0] + metric[1] * metric[1] + metric[2] * metric[2]);
    result = std::max(result, static_cast<double>(metricNorm * Q[Shortcuts::cp]));
    result = std::max(result, static_cast<double>(metricNorm * Q[Shortcuts::cs]));
  }
  return result;
"""


def flux():
    return """
  const auto* const metric   = Q + Shortcuts::metricDerivative + static_cast<std::ptrdiff_t>(normal) * DIMENSIONS;
  const auto* const stress   = Q + Shortcuts::sigma;
  const auto        jacobian = Q[Shortcuts::jacobian];
  std::fill_n(F, NumberOfUnknowns, 0.0);
  F[Shortcuts::v + 0] = -jacobian * (metric[0] * stress[0] + metric[1] * stress[3] + metric[2] * stress[4]);
  F[Shortcuts::v + 1] = -jacobian * (metric[0] * stress[3] + metric[1] * stress[1] + metric[2] * stress[5]);
  F[Shortcuts::v + 2] = -jacobian * (metric[0] * stress[4] + metric[1] * stress[5] + metric[2] * stress[2]);
"""


def ncp():
    return """
  const auto* const metric       = Q + Shortcuts::metricDerivative + static_cast<std::ptrdiff_t>(normal) * DIMENSIONS;
  const auto* const velocityJump = deltaQ + Shortcuts::v;
  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0.0);
  BTimesDeltaQ[Shortcuts::sigma + 0] = -metric[0] * velocityJump[0];
  BTimesDeltaQ[Shortcuts::sigma + 1] = -metric[1] * velocityJump[1];
  BTimesDeltaQ[Shortcuts::sigma + 2] = -metric[2] * velocityJump[2];
  BTimesDeltaQ[Shortcuts::sigma + 3] = -(metric[1] * velocityJump[0] + metric[0] * velocityJump[1]);
  BTimesDeltaQ[Shortcuts::sigma + 4] = -(metric[2] * velocityJump[0] + metric[0] * velocityJump[2]);
  BTimesDeltaQ[Shortcuts::sigma + 5] = -(metric[2] * velocityJump[1] + metric[1] * velocityJump[2]);
"""


def multiplyMaterialParameterMatrix():
    return """
  const auto density                     = Q[Shortcuts::rho];
  const auto shearModulus                = density * Q[Shortcuts::cs] * Q[Shortcuts::cs];
  const auto firstLameParameter          = density * Q[Shortcuts::cp] * Q[Shortcuts::cp] - 2 * shearModulus;
  const auto inverseDensityTimesJacobian = 1 / (density * Q[Shortcuts::jacobian]);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    rhs[Shortcuts::v + axis] = inverseDensityTimesJacobian * rhs[Shortcuts::v + axis];
  }
  const auto lameTimesStressTrace =
    firstLameParameter * (rhs[Shortcuts::sigma + 0] + rhs[Shortcuts::sigma + 1] + rhs[Shortcuts::sigma + 2]);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    rhs[Shortcuts::sigma + axis] = (2 * shearModulus) * rhs[Shortcuts::sigma + axis] + lameTimesStressTrace;
  }
  for (int shearComponent = 3; shearComponent < 6; shearComponent++) {
    rhs[Shortcuts::sigma + shearComponent] = shearModulus * rhs[Shortcuts::sigma + shearComponent];
  }
"""


def refinement_criterion():
    return """
  return ::exahype2::RefinementCommand::Keep;
"""


def riemann_solver():
    return """
  return applications::exahype2::exaseis::solveElasticRiemannProblem<
    Shortcuts,
    {{COMPUTE_PRECISION}},
    DGOrder + 1,
    NumberOfUnknowns,
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    1>(QL, QR, normal, isBoundaryFace, faceIndex, FL, FR);
"""


def userIncludes():
    return [
        "applications/exaseis/riemannSolvers/ElasticRiemannSolver.h",
        "exahype2/Curvi.h",
        "exahype2/dg/Basis.h",
        "peano4/sfc/ActiveCurve.h",
        "tarch/reader/NetCDFFieldSampler.h",
    ]


def abstractUserDefinitions():
    return """
std::unique_ptr<::exahype2::Curvi> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_curvi;

tarch::la::Vector<DIMENSIONS, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::invertProjection(
  const tarch::la::Vector<DIMENSIONS, double> coordinates
) {
  return _curvi->invertProjection(coordinates);
}
"""


def abstractDeclarations():
    return """
  tarch::la::Vector<DIMENSIONS, double> invertProjection(tarch::la::Vector<DIMENSIONS, double> coordinates);

protected:
  static std::unique_ptr<::exahype2::Curvi> _curvi;
"""


def init_grid_step_implementation(scenario_perturbations):
    return """
  // Curvi pins every interface to a face of a cell on the coarsest grid level. Every finer grid
  // level keeps these faces.
  tarch::la::Vector<DIMENSIONS, double> coarsestCellSize = DomainSize;
  while (max(coarsestCellSize) > parameters.maxAdmissiblePatchSize) {
    coarsestCellSize /= static_cast<double>(peano4::sfc::ActiveCurve::SubdivisionFactor);
  }

  // The grid initialisation runs once per refinement round and rebuilds the curvilinear mapping
  // every time.
  _curvi = std::make_unique<::exahype2::Curvi>(DomainOffset, DomainSize, coarsestCellSize);
""" + scenario_perturbations
