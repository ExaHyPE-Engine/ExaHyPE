#include "Rusanov.h"

#include "peano4/Loop.h"
#include "toolbox/blockstructured/Enumeration.h"

void exahype2::dg::rusanov::solveRiemannProblem_pointwise_in_situ(
  [[maybe_unused]] ::exahype2::dg::Flux                         flux,
  [[maybe_unused]] ::exahype2::dg::NonConservativeProduct       nonConservativeProduct,
  [[maybe_unused]] ::exahype2::dg::MaxEigenvalue                maxEigenvalue,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          order,
  [[maybe_unused]] int                                          unknowns,
  [[maybe_unused]] int                                          auxiliaryVariables,
  [[maybe_unused]] int                                          faceNumber,
  [[maybe_unused]] const double* NOALIAS                        quadraturePoints,
  [[maybe_unused]] bool                                         evaluateFlux,
  [[maybe_unused]] bool                                         evaluateNonconservativeProduct,
  [[maybe_unused]] const double* NOALIAS                        projectedValues,
  [[maybe_unused]] double* NOALIAS                              solution
) {
  const int                             normal       = faceNumber % DIMENSIONS;
  const int                             inputStride  = unknowns + auxiliaryVariables;
  const int                             outputStride = unknowns;
  tarch::la::Vector<DIMENSIONS, double> faceOffset   = faceCentre - 0.5 * cellSize;
  faceOffset(normal) += 0.5 * cellSize(normal);

  double* deltaQ          = new double[unknowns + auxiliaryVariables];
  double* averageQ        = new double[unknowns + auxiliaryVariables];
  double* fluxValuesLeft  = new double[unknowns];
  double* fluxValuesRight = new double[unknowns];
  double* ncpValues       = new double[unknowns];

  peano4::dForEachExclusive(order + 1, faceNumber % DIMENSIONS, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int d = 0; d < DIMENSIONS; d++) {
      x(d) = faceOffset(d);
      x(d) += (d == faceNumber % DIMENSIONS) ? 0.0 : quadraturePoints[dof(d)] * cellSize(d);
    }

    tarch::la::Vector<DIMENSIONS, int> leftDoF  = dof;
    tarch::la::Vector<DIMENSIONS, int> rightDoF = dof;

    leftDoF(normal)  = 0;
    rightDoF(normal) = 1;
    const int leftInputOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(leftDoF, order + 1, 1, normal) * inputStride;
    const int rightInputOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(rightDoF, order + 1, 1, normal) * inputStride;
    const int leftOutputOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(leftDoF, order + 1, 1, normal) * outputStride;
    const int rightOutputOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(rightDoF, order + 1, 1, normal) * outputStride;

    // initialize solution to 0
    for (int var = 0; var < unknowns; var++) {
      solution[leftOutputOffset + var]  = 0.0;
      solution[rightOutputOffset + var] = 0.0;
    }

    for (int var = 0; var < unknowns + auxiliaryVariables; var++) {
      deltaQ[var]   = projectedValues[rightInputOffset + var] - projectedValues[leftInputOffset + var];
      averageQ[var] = 0.5 * projectedValues[rightInputOffset + var] + 0.5 * projectedValues[leftInputOffset + var];
    }

    double leftEigenvalue = maxEigenvalue(projectedValues + leftInputOffset, x, t, dt, normal);

    double rightEigenvalue = maxEigenvalue(projectedValues + rightInputOffset, x, t, dt, normal);

    double maxEigenvalue = std::max(leftEigenvalue, rightEigenvalue);

    for (int var = 0; var < unknowns; var++) {
      solution[leftOutputOffset + var] -= 0.5 * maxEigenvalue * deltaQ[var];
      solution[rightOutputOffset + var] -= 0.5 * maxEigenvalue * deltaQ[var];
    }

    if (evaluateFlux) {
      flux(projectedValues + leftInputOffset, x, t, dt, normal, fluxValuesLeft);
      flux(projectedValues + rightInputOffset, x, t, dt, normal, fluxValuesRight);

      for (int var = 0; var < unknowns; var++) {
        solution[leftOutputOffset + var] += 0.5 * (fluxValuesLeft[var] + fluxValuesRight[var]);
        solution[rightOutputOffset + var] += 0.5 * (fluxValuesLeft[var] + fluxValuesRight[var]);
      }
    }

    if (evaluateNonconservativeProduct) {
      nonConservativeProduct(averageQ, deltaQ, x, t, dt, normal, ncpValues);

      for (int var = 0; var < unknowns; var++) {
        solution[leftOutputOffset + var] += 0.5 * ncpValues[var];
        solution[rightOutputOffset + var] -= 0.5 * ncpValues[var];
      }
    }
  });

  delete[] deltaQ;
  delete[] averageQ;
  delete[] fluxValuesLeft;
  delete[] fluxValuesRight;
  delete[] ncpValues;
}

void exahype2::dg::rusanov::solveRiemannProblem_pointwise_in_situ_with_gradient_projection(
  [[maybe_unused]] ::exahype2::dg::Flux                         flux,
  [[maybe_unused]] ::exahype2::dg::NonConservativeProduct       nonConservativeProduct,
  [[maybe_unused]] ::exahype2::dg::MaxEigenvalue                maxEigenvalue,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          order,
  [[maybe_unused]] int                                          unknowns,
  [[maybe_unused]] int                                          auxiliaryVariables,
  [[maybe_unused]] int                                          faceNumber,
  [[maybe_unused]] const double* NOALIAS                        quadraturePoints,
  [[maybe_unused]] bool                                         evaluateFlux,
  [[maybe_unused]] bool                                         evaluateNonconservativeProduct,
  [[maybe_unused]] const double* NOALIAS                        projectedValues,
  [[maybe_unused]] double* NOALIAS                              solution
) {
  assertion(false, "not implemented yet");
}
