// Copyright (C) 2009 Technische Universitaet Muenchen
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Scalar.h"
#include "tarch/la/Vector.h"

namespace tarch::la {
  template <int N>
  struct VectorCompare;
} // namespace tarch::la

template <int N>
struct tarch::la::VectorCompare {
public:
  INLINE_METHOD
  VectorCompare(double accuracy = NUMERICAL_ZERO_DIFFERENCE);

  INLINE_METHOD
  bool operator()(const Vector<N, double>& left, const Vector<N, double>& right) const;

private:
  const double _accuracy;
};

#include "tarch/la/VectorCompare.cpph"
