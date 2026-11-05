// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  class Curvi;
} // namespace exahype2

/**
 * Maps a cartesian domain onto a curvilinear domain with the same cell connections and back.
 * applyPerturbation() moves a domain face along its normal and split() moves an interior cell
 * boundary. Both stretch the cells along that axis linearly. Each axis takes at most one of these
 * transformations.
 */
class exahype2::Curvi final {
public:
  using Point        = tarch::la::Vector<DIMENSIONS, double>;
  using Mapping      = std::function<Point(Point)>;
  using Perturbation = std::function<double(Point)>;

  INLINE_METHOD Curvi(const Point& domainOffset, const Point& domainSize, const Point& cellSize);

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
  const Point _cellSize;

  bool _axisIsTransformed[DIMENSIONS]{};

  void reserveAxis(int normal);
};

#include "exahype2/Curvi.cpph"
