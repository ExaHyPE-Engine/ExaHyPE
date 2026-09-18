// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  class Curvi;
} // namespace exahype2

/**
 * Transformation back and forth between a cartesian domain such as the ones which Peano can
 * handle, and an equivalent curvilinear domain, i.e. one where the elements are connected in the
 * same way, but are not necessarily equidistant.
 *
 * The class is centred around two mappings: _positionFunction and _inversePositionFunction, which
 * respectively hold the transformation from cartesian space to the curvilinear space, and from the
 * curvilinear space to the cartesian space.
 *
 * Two operations define the curvilinear space:
 *
 * applyPerturbation() applies a perturbation to a given surface of the cartesian space. This
 * fits, for example, the boundary of the domain to a given edge, say the surface of a 3D
 * earthquake simulation to a mountain range's topography, or the side of a 2D simulation of the
 * pressure in a cylinder to the curve of said cylinder.
 *
 * split() defines interfaces within the domain that the mesh should conform to, for example a
 * fault in an earthquake simulation, or an airfoil in the centre of the domain. Such an interface
 * is placed on the boundary between two cells in the cartesian space, so that it does not end up
 * in the middle of a cell.
 *
 * Once the domain has been fully defined, operator() computes the position in curvilinear space
 * for a given point in cartesian space, and invertProjection() recovers the point in cartesian
 * space which maps onto a given curvilinear point.
 *
 * The core concept is that both split() and applyPerturbation() change the mesh in one direction
 * at a time, hence their change to the position of any given point is a 1D linear interpolation
 * between the domain that said point was in, and the position it is in now. In the figure below,
 * the left face has been shifted linearly, and all points in the domain are shifted by a linear
 * interpolation between the left face and the right face.
 *
 *      O - - - O            O - - - O           O - - - O
 *      | . . . |             \. . . |            \ . . . \             O - - - O
 *      | . . . |    - - - >   \.. . |   - - - >   \ . . . \   - - - >   \ : : : \
 *      | . . . |               \... |              \ . . . \             \ . . . \
 *      O - - - O                O---O               O - - - O             O - - - O
 *
 * A linear interpolation moves points only along its own direction and leaves every other
 * direction unchanged, so a transformation in another direction composes with the ones already
 * applied without breaking them. Modifying a second surface leaves the first surface where it is,
 * except for shifting the edge the two surfaces meet along in the second surface's normal
 * direction.
 *
 * ## Preconditions
 *
 * Each coordinate direction may carry at most one transformation, i.e. applyPerturbation() and
 * split() together have to be called at most once per axis. The class asserts this.
 *
 * The restriction is what makes the composition exact. Because every earlier transformation leaves
 * the current axis untouched, the domain bounds along that axis are still the cartesian ones, and
 * both split() and applyPerturbation() measure against them directly, in the forward as well as in
 * the inverse direction. Lifting the restriction means measuring bounds that have themselves
 * moved, and doing so consistently in both directions, which the linear ansatz used here cannot
 * express. A second split along an axis that has already been stretched would in particular no
 * longer land on a cell boundary, which is the entire purpose of split().
 *
 * A perturbation is always evaluated on the surface it moves, with the coordinate along the
 * transformed axis pinned to the face for applyPerturbation() and to the interface's cell boundary
 * for split(). Only the transverse coordinates therefore vary, so the forward and the inverse map
 * read the same value: The coordinate along the axis is the one the transformation changes, and
 * sampling at the incoming point gives the two directions different answers.
 *
 * ## Cost
 *
 * Every transformation wraps the previous one in a new std::function, so one point evaluation
 * walks one indirect call per transformation plus one evaluation of that transformation's
 * perturbation. The chain is linear in the number of transformations, but it is a chain of
 * indirect calls, and the perturbations themselves may be expensive - a NetCDF sampler, say.
 * operator() is evaluated per degree of freedom per cell while the solution is initialised, so
 * keep the perturbations cheap.
 *
 * ## Limitations
 *
 * Perturbations can only be given to slices of the domain, be it surfaces via applyPerturbation()
 * or interior slices via split(), and those perturbations can only be orthogonal to the given
 * slice. One can shift the interior slice at (x=x0, y=:) such that it is at (x=x0+f(x,y), y=:) but
 * one cannot shift its y-position in the same perturbation; the interpolation is linear, and
 * such a shift would require a transfinite interpolation.
 *
 * Since the resulting mesh must be equivalent to that in cartesian space, perturbations that break
 * this equivalence are not admissible: Two splits cannot cross, two splits very near one another
 * can end up superposed and make the middle section disappear, and stretching the space too far in
 * any given dimension degrades the quality of the resulting transformation.
 */
class exahype2::Curvi final {
public:
  using Point        = tarch::la::Vector<DIMENSIONS, double>;
  using Mapping      = std::function<Point(Point)>;
  using Perturbation = std::function<double(Point)>;

  INLINE_METHOD Curvi(const Point& domainOffset, const Point& domainSize, const Point& minCellSize);

  /**
   * The stored mappings close over everything they need by value, so that they never outlive the
   * state they read. Transforming either half of a copy further would leave the two silently out of
   * step.
   */
  Curvi(const Curvi&)            = delete;
  Curvi(Curvi&&)                 = delete;
  Curvi& operator=(const Curvi&) = delete;
  Curvi& operator=(Curvi&&)      = delete;

  ~Curvi() = default;

  void applyPerturbation(int faceIndex, Perturbation perturbation);

  double split(int normal, int index, double position, Perturbation perturbation);

  INLINE_METHOD double split(int normal, double position, Perturbation perturbation);

  INLINE_METHOD Point operator()(const Point& x) const;

  INLINE_METHOD Point invertProjection(const Point& x) const;

private:
  Mapping _positionFunction{[](Point x) { return x; }};
  Mapping _inversePositionFunction{[](Point x) { return x; }};

  const Point _domainOffset;
  const Point _domainSize;
  const Point _minCellSize;

  /**
   * One flag per coordinate direction, raised once that direction carries a transformation.
   */
  bool _axisIsTransformed[DIMENSIONS]{};

  void reserveAxis(int normal);
};

#include "exahype2/Curvi.cpph"
