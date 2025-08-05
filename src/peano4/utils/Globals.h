// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

template <int Base, int Exponent>
class BasePowerExponent {
public:
  enum { value = Base * BasePowerExponent<Base, Exponent - 1>::value };
};

template <int Base>
class BasePowerExponent<Base, 0> {
public:
  enum { value = 1 };
};

#define TWO_TIMES_D (2 * DIMENSIONS)
#define TWO_POWER_D (BasePowerExponent<2, DIMENSIONS>::value)
#define THREE_POWER_D (BasePowerExponent<3, DIMENSIONS>::value)
