# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def initial(pointwise_initial_conditions):
    return (
        """
  constexpr int NodesPerAxis = DGOrder + 1;
  for (auto& node : Q) {
    node = {};
  }

  const Coordinates cellCentre = (*_curvi)(x);

  const auto initialiseNode = [&](const Coordinates& x, const Coordinates& cellCentre, auto& Q) -> void {"""
        + pointwise_initial_conditions
        + """
  };

  int node = 0;
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
    const Coordinates curvilinearPosition = (*_curvi)(::exahype2::dg::getQuadraturePoint(
      x,
      h,
      index,
      Quadrature<{{STORAGE_PRECISION}}>::getQuadraturePoints1D().data()
    ));
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      Q[node].curveGrid(axis) = curvilinearPosition[axis];
    }
    initialiseNode(curvilinearPosition, cellCentre, Q[node]);
    node++;
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
          Q[yz * NodesPerAxis + column].metricDerivative(0, axis) += Q[yz * NodesPerAxis + row].curveGrid(axis)
                                                                   * inverseCellSize[0]
                                                                   * derivativeOperator[column * NodesPerAxis + row];
        }
      }
    }
  }
  for (int zNode = 0; zNode < NodesPerAxis; zNode++) {
    for (int xNode = 0; xNode < NodesPerAxis; xNode++) {
      const int lineStart = zNode * NodesPerAxis * NodesPerAxis + xNode;
      UNROLL(NodesPerAxis)
      for (int row = 0; row < NodesPerAxis; row++) {
        UNROLL(NodesPerAxis)
        for (int column = 0; column < NodesPerAxis; column++) {
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            Q[lineStart + column * NodesPerAxis].metricDerivative(1, axis) +=
              Q[lineStart + row * NodesPerAxis].curveGrid(axis) * inverseCellSize[1]
              * derivativeOperator[column * NodesPerAxis + row];
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
          Q[xy + column * NodesPerAxis * NodesPerAxis].metricDerivative(2, axis) +=
            Q[xy + row * NodesPerAxis * NodesPerAxis].curveGrid(axis) * inverseCellSize[2]
            * derivativeOperator[column * NodesPerAxis + row];
        }
      }
    }
  }

  // Replaces the position derivatives of every node by the inverse metric and stores their
  // determinant as the Jacobian.
  for (auto& node : Q) {
    auto&      metric            = node.metricDerivative;
    const auto curveXDerivativeQ = metric(0, 0);
    const auto curveYDerivativeQ = metric(0, 1);
    const auto curveZDerivativeQ = metric(0, 2);
    const auto curveXDerivativeR = metric(1, 0);
    const auto curveYDerivativeR = metric(1, 1);
    const auto curveZDerivativeR = metric(1, 2);
    const auto curveXDerivativeS = metric(2, 0);
    const auto curveYDerivativeS = metric(2, 1);
    const auto curveZDerivativeS = metric(2, 2);

    const auto jacobian = curveXDerivativeQ * (curveYDerivativeR * curveZDerivativeS - curveYDerivativeS * curveZDerivativeR)
                        - curveXDerivativeR * (curveYDerivativeQ * curveZDerivativeS - curveYDerivativeS * curveZDerivativeQ)
                        + curveXDerivativeS * (curveYDerivativeQ * curveZDerivativeR - curveYDerivativeR * curveZDerivativeQ);
    ASSERTION(jacobian != 0, jacobian);

    node.jacobian = jacobian;
    metric(0, 0)  = (1 / jacobian) * (curveYDerivativeR * curveZDerivativeS - curveZDerivativeR * curveYDerivativeS);
    metric(1, 0)  = (1 / jacobian) * (curveZDerivativeQ * curveYDerivativeS - curveYDerivativeQ * curveZDerivativeS);
    metric(2, 0)  = (1 / jacobian) * (curveYDerivativeQ * curveZDerivativeR - curveZDerivativeQ * curveYDerivativeR);
    metric(0, 1)  = (1 / jacobian) * (curveZDerivativeR * curveXDerivativeS - curveXDerivativeR * curveZDerivativeS);
    metric(1, 1)  = (1 / jacobian) * (curveXDerivativeQ * curveZDerivativeS - curveZDerivativeQ * curveXDerivativeS);
    metric(2, 1)  = (1 / jacobian) * (curveZDerivativeQ * curveXDerivativeR - curveXDerivativeQ * curveZDerivativeR);
    metric(0, 2)  = (1 / jacobian) * (curveXDerivativeR * curveYDerivativeS - curveYDerivativeR * curveXDerivativeS);
    metric(1, 2)  = (1 / jacobian) * (curveYDerivativeQ * curveXDerivativeS - curveXDerivativeQ * curveYDerivativeS);
    metric(2, 2)  = (1 / jacobian) * (curveXDerivativeQ * curveYDerivativeR - curveYDerivativeQ * curveXDerivativeR);
  }
"""
    )


def boundary():
    return """
  Qoutside = Qinside;
"""


def eigenvalue():
    return """
  ASSERTION(not (std::isnan(Q.v(0)) or std::isnan(Q.v(1)) or std::isnan(Q.v(2))), Q.v(0), Q.v(1), Q.v(2));
  const auto& metric = Q.metricDerivative;
  double      result = 1.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const auto metricNorm =
      std::sqrt(metric(axis, 0) * metric(axis, 0) + metric(axis, 1) * metric(axis, 1) + metric(axis, 2) * metric(axis, 2));
    result = std::max(result, static_cast<double>(metricNorm * Q.cp));
    result = std::max(result, static_cast<double>(metricNorm * Q.cs));
  }
  return result;
"""


def flux():
    return """
  const auto& metric = Q.metricDerivative;
  F                  = {};
  for (int row = 0; row < 3; row++) {
    F.v(row) = -Q.jacobian
             * (metric(normal, 0) * Q.sigma(row, 0) + metric(normal, 1) * Q.sigma(row, 1)
                + metric(normal, 2) * Q.sigma(row, 2));
  }
"""


def ncp():
    return """
  const auto& metric       = Q.metricDerivative;
  BTimesDeltaQ             = {};
  BTimesDeltaQ.sigma(0, 0) = -metric(normal, 0) * deltaQ.v(0);
  BTimesDeltaQ.sigma(1, 1) = -metric(normal, 1) * deltaQ.v(1);
  BTimesDeltaQ.sigma(2, 2) = -metric(normal, 2) * deltaQ.v(2);
  BTimesDeltaQ.sigma(0, 1) = -(metric(normal, 1) * deltaQ.v(0) + metric(normal, 0) * deltaQ.v(1));
  BTimesDeltaQ.sigma(0, 2) = -(metric(normal, 2) * deltaQ.v(0) + metric(normal, 0) * deltaQ.v(2));
  BTimesDeltaQ.sigma(1, 2) = -(metric(normal, 2) * deltaQ.v(1) + metric(normal, 1) * deltaQ.v(2));
"""


def multiplyMaterialParameterMatrix():
    return """
  const auto density                     = Q.rho;
  const auto shearModulus                = density * Q.cs * Q.cs;
  const auto firstLameParameter          = density * Q.cp * Q.cp - 2 * shearModulus;
  const auto inverseDensityTimesJacobian = 1 / (density * Q.jacobian);
  rhs.v                                  = inverseDensityTimesJacobian * rhs.v;
  const auto lameTimesStressTrace = firstLameParameter * (rhs.sigma(0, 0) + rhs.sigma(1, 1) + rhs.sigma(2, 2));
  for (int axis = 0; axis < 3; axis++) {
    rhs.sigma(axis, axis) = (2 * shearModulus) * rhs.sigma(axis, axis) + lameTimesStressTrace;
  }
  rhs.sigma(0, 1) = shearModulus * rhs.sigma(0, 1);
  rhs.sigma(0, 2) = shearModulus * rhs.sigma(0, 2);
  rhs.sigma(1, 2) = shearModulus * rhs.sigma(1, 2);
"""


def refinement_criterion():
    return """
  return ::exahype2::RefinementCommand::Keep;
"""


def riemann_solver():
    return """
  return applications::exahype2::exaseis::solveElasticRiemannProblem<{{COMPUTE_PRECISION}}, 1>(
    QL,
    QR,
    normal,
    isBoundaryFace,
    faceIndex,
    FL,
    FR
  );
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
