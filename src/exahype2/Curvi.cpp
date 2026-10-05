// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Curvi.h"

#include "tarch/Assertions.h"
#include "tarch/la/ScalarOperations.h"

void
exahype2::Curvi::reserveAxis(int normal) {
  assertion(
    normal >= 0 and normal < DIMENSIONS,
    "A curvilinear transformation acts on the axes of the domain only.",
    normal,
    DIMENSIONS
  );
  assertion(not _axisIsTransformed[normal], "Each axis takes one curvilinear transformation only.", normal);
  _axisIsTransformed[normal] = true;
}

/**
 * Moves the domain face faceIndex along its normal by perturbation and stretches the cells between
 * it and the opposite face linearly. Face i and face i+DIMENSIONS are the lower and the upper face
 * along axis i.
 */
void
exahype2::Curvi::applyPerturbation(int faceIndex, Perturbation perturbation) {
  assertion(
    faceIndex >= 0 and faceIndex < 2 * DIMENSIONS,
    "A perturbation acts on the faces of the domain only.",
    faceIndex,
    DIMENSIONS
  );

  const int  normal      = faceIndex % DIMENSIONS;
  const bool isUpperFace = faceIndex >= DIMENSIONS;

  reserveAxis(normal);

  // No earlier transformation moves points along normal, so the cartesian bounds stay valid.
  const double lowerBound = _domainOffset[normal];
  const double upperBound = _domainOffset[normal] + _domainSize[normal];

  // Both maps evaluate perturbation on the face, so they read the same offset.
  const double measurementPosition = isUpperFace ? upperBound : lowerBound;

  const Mapping oldPositionFunction = _positionFunction;

  _positionFunction =
    [normal, isUpperFace, perturbation, oldPositionFunction, lowerBound, upperBound, measurementPosition](
      const Point& x
    ) {
      Point solution = oldPositionFunction(x);

      Point measurementPoint   = solution;
      measurementPoint[normal] = measurementPosition;
      const double perturb     = perturbation(measurementPoint);

      const double a = isUpperFace ? (upperBound + perturb - lowerBound) / (upperBound - lowerBound)
                                   : (upperBound - lowerBound - perturb) / (upperBound - lowerBound);
      assertion(tarch::la::greater(a, 0.0), "A perturbation must keep the domain width positive.", a, perturb, normal);

      if (isUpperFace) {
        solution[normal] = lowerBound + (solution[normal] - lowerBound) * a;
      } else {
        solution[normal] = lowerBound + perturb + (solution[normal] - lowerBound) * a;
      }

      return solution;
    };

  const Mapping oldInversePositionFunction = _inversePositionFunction;

  _inversePositionFunction =
    [normal, isUpperFace, perturbation, oldInversePositionFunction, lowerBound, upperBound, measurementPosition](
      const Point& x
    ) {
      Point solution = x;

      Point measurementPoint   = solution;
      measurementPoint[normal] = measurementPosition;
      const double perturb     = perturbation(measurementPoint);

      const double a = isUpperFace ? (upperBound + perturb - lowerBound) / (upperBound - lowerBound)
                                   : (upperBound - lowerBound - perturb) / (upperBound - lowerBound);
      assertion(tarch::la::greater(a, 0.0), "A perturbation must keep the domain width positive.", a, perturb, normal);

      if (isUpperFace) {
        solution[normal] = (solution[normal] - lowerBound) / a + lowerBound;
      } else {
        solution[normal] = (solution[normal] - lowerBound - perturb) / a + lowerBound;
      }

      return oldInversePositionFunction(solution);
    };
}

/**
 * Moves the interior cell boundary index along axis normal to position plus perturbation and
 * stretches the cells on both sides linearly. Returns the cartesian position of that cell boundary.
 */
double
exahype2::Curvi::split(int normal, int index, double position, Perturbation perturbation) {
  reserveAxis(normal);

  // No earlier transformation moves points along normal, so the cartesian bounds stay valid.
  const double lowerBound    = _domainOffset[normal];
  const double upperBound    = _domainOffset[normal] + _domainSize[normal];
  const double splitPosition = lowerBound + _minCellSize[normal] * index;

  assertion(index >= 1, "An interface must lie strictly inside the domain.", index, normal);
  assertion(
    tarch::la::smaller(splitPosition, upperBound),
    "An interface must lie strictly inside the domain.",
    splitPosition,
    upperBound,
    normal
  );

  const Mapping oldPositionFunction = _positionFunction;

  _positionFunction =
    [normal, position, splitPosition, lowerBound, upperBound, oldPositionFunction, perturbation](const Point& x) {
      Point solution = oldPositionFunction(x);

      // Both maps evaluate perturbation on the interface, so they read the same offset.
      Point measurementPoint   = solution;
      measurementPoint[normal] = splitPosition;
      const double target      = position + perturbation(measurementPoint);

      if (solution[normal] <= splitPosition) {
        const double a   = (target - lowerBound) / (splitPosition - lowerBound);
        const double b   = lowerBound * (1 - a);
        solution[normal] = a * solution[normal] + b;
      } else {
        const double a   = (upperBound - target) / (upperBound - splitPosition);
        const double b   = target - a * splitPosition;
        solution[normal] = a * solution[normal] + b;
      }

      return solution;
    };

  const Mapping oldInversePositionFunction = _inversePositionFunction;

  _inversePositionFunction =
    [normal, position, splitPosition, lowerBound, upperBound, oldInversePositionFunction, perturbation](
      const Point& x
    ) {
      Point solution = x;

      Point measurementPoint   = solution;
      measurementPoint[normal] = splitPosition;
      const double target      = position + perturbation(measurementPoint);

      if (solution[normal] <= target) {
        const double a   = (target - lowerBound) / (splitPosition - lowerBound);
        const double b   = lowerBound * (1 - a);
        solution[normal] = (solution[normal] - b) / a;
      } else {
        const double a   = (upperBound - target) / (upperBound - splitPosition);
        const double b   = target - a * splitPosition;
        solution[normal] = (solution[normal] - b) / a;
      }

      return oldInversePositionFunction(solution);
    };

  return splitPosition;
}
