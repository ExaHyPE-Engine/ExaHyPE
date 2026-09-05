// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Curvi.h"

#include "tarch/la/ScalarOperations.h"

/**
 * Asserts that normal is a valid direction which does not carry a transformation yet, and records
 * that it does from now on.
 */
void exahype2::Curvi::reserveAxis(int normal) {
  assertion(normal >= 0 and normal < DIMENSIONS, "Coordinate direction out of range", normal, DIMENSIONS);
  assertion(
    not _axisIsTransformed[normal],
    "Each coordinate direction may carry at most one transformation, see Curvi's preconditions",
    normal
  );
  _axisIsTransformed[normal] = true;
}

/**
 * Moves one face of the domain by perturbation and stretches the interior along the face normal so
 * that it follows.
 *
 * @param faceIndex    Peano face index in [0, 2*DIMENSIONS). An index below DIMENSIONS denotes the
 *                     lower face along axis faceIndex, the remaining ones the upper face along
 *                     axis faceIndex-DIMENSIONS.
 * @param perturbation Offset of the face, as a function of the coordinates transverse to the face
 *                     normal.
 */
void exahype2::Curvi::applyPerturbation(int faceIndex, Perturbation perturbation) {
  assertion(faceIndex >= 0 and faceIndex < 2 * DIMENSIONS, "Face index out of range", faceIndex, DIMENSIONS);

  const int  normal      = faceIndex % DIMENSIONS;
  const bool isUpperFace = faceIndex >= DIMENSIONS;

  reserveAxis(normal);

  // No earlier transformation touches this axis, so the domain still ends where the cartesian
  // domain ends and both bounds are constants. That keeps the chain linear in the number of
  // transformations and lets the forward and the inverse map measure against the same thing.
  const double lowerBound = _domainOffset[normal];
  const double upperBound = _domainOffset[normal] + _domainSize[normal];

  // A point keeps its transverse coordinates across this transformation, so pinning the coordinate
  // along normal onto the face makes the forward and the inverse map sample the same location.
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

      // A perturbation which reaches the opposite bound leaves nothing to stretch, and one which
      // reaches past it turns the domain inside out.
      const double a = isUpperFace ? (upperBound + perturb - lowerBound) / (upperBound - lowerBound)
                                   : (upperBound - lowerBound - perturb) / (upperBound - lowerBound);
      assertion(tarch::la::greater(a, 0.0), "A perturbation may not collapse or invert the domain", a, perturb, normal);

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
      assertion(tarch::la::greater(a, 0.0), "A perturbation may not collapse or invert the domain", a, perturb, normal);

      if (isUpperFace) {
        solution[normal] = (solution[normal] - lowerBound) / a + lowerBound;
      } else {
        solution[normal] = (solution[normal] - lowerBound - perturb) / a + lowerBound;
      }

      return oldInversePositionFunction(solution);
    };
}

/**
 * Pins an interior interface to a cell boundary and stretches both sides so that they follow it.
 *
 * @param normal       Coordinate direction the interface is normal to, in [0, DIMENSIONS).
 * @param index        Index of the cell boundary the interface is pinned to. Has to lie strictly
 *                     inside the domain, i.e. be neither the lower nor the upper boundary.
 * @param position     Position the interface is moved to.
 * @param perturbation Offset added to position, as a function of the coordinates transverse to
 *                     normal.
 * @return The cartesian position of the cell boundary the interface was pinned to.
 */
double exahype2::Curvi::split(int normal, int index, double position, Perturbation perturbation) {
  reserveAxis(normal);

  // The axis carries no earlier transformation, so both bounds are still the cartesian ones.
  const double lowerBound    = _domainOffset[normal];
  const double upperBound    = _domainOffset[normal] + _domainSize[normal];
  const double splitPosition = lowerBound + _minCellSize[normal] * index;

  // A split pinned to either domain boundary leaves nothing to stretch on that side and divides
  // by zero below.
  assertion(index >= 1, "An interface has to be pinned strictly inside the domain", index, normal);
  assertion(
    tarch::la::smaller(splitPosition, upperBound),
    "An interface has to be pinned strictly inside the domain",
    splitPosition,
    upperBound,
    normal
  );

  const Mapping oldPositionFunction = _positionFunction;

  _positionFunction =
    [normal, position, splitPosition, lowerBound, upperBound, oldPositionFunction, perturbation](const Point& x) {
      Point solution = oldPositionFunction(x);

      // Pinning the coordinate along normal onto the interface makes the sample point identical in
      // the forward and the inverse direction, which the round trip depends on.
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
