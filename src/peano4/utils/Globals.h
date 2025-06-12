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

#define DimensionsTimesTwo (Dimensions * 2)
#define TwoTimesD (2 * Dimensions)
#define TwoTimesDTimesTwo (2 * Dimensions * 2)

#define TwoPowerD (BasePowerExponent<2, Dimensions>::value)
#define TwoPowerDPlusTwo (BasePowerExponent<2, Dimensions>::value + 2)
#define TwoPowerDTimesTwoPowerD (BasePowerExponent<2, Dimensions>::value * BasePowerExponent<2, Dimensions>::value)
#define TwoPowerDTimesTwoPowerDMinusOne \
  (BasePowerExponent<2, Dimensions>::value * (BasePowerExponent<2, Dimensions>::value - 1))

#define ThreePowerD (BasePowerExponent<3, Dimensions>::value)
#define FourPowerD (BasePowerExponent<4, Dimensions>::value)
#define FivePowerD (BasePowerExponent<5, Dimensions>::value)
#define SevenPowerD (BasePowerExponent<7, Dimensions>::value)
#define NinePowerD (BasePowerExponent<9, Dimensions>::value)
