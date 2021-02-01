// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_UTILS_GLOBALS_H_
#define _PEANO4_UTILS_GLOBALS_H_

  template <int base, int exponent>
  class BasePowerExponent {
    public:
     enum {value = base*BasePowerExponent<base,exponent-1>::value};
  };

  template <int base>
  class BasePowerExponent<base,0> {
    public:
    enum {value = 1};
  };

  #define DimensionsTimesTwo (Dimensions*2)

  #define TwoPowerD                (BasePowerExponent<2,Dimensions>::value)
  #define TwoPowerDPlusTwo         (BasePowerExponent<2,Dimensions>::value+2)
  #define TwoPowerDTimesTwoPowerD  (BasePowerExponent<2,Dimensions>::value*BasePowerExponent<2,Dimensions>::value)
  #define TwoPowerDTimesTwoPowerDMinusOne  (BasePowerExponent<2,Dimensions>::value*(BasePowerExponent<2,Dimensions>::value-1))

  #define ThreePowerD        (BasePowerExponent<3,Dimensions>::value)
  #define FivePowerD         (BasePowerExponent<5,Dimensions>::value)
  #define SevenPowerD        (BasePowerExponent<7,Dimensions>::value)
  #define NinePowerD         (BasePowerExponent<9,Dimensions>::value)

  #define TwoTimesD          (2*Dimensions)
  #define TwoTimesDTimesTwo  (2*Dimensions*2)

/*
  #define TwoPowerDTimesD   (BasePowerExponent<2,Dimensions>::value*Dimensions)
  #define TwoPowerD_MINUS_ONE (BasePowerExponent<2,Dimensions>::value-1)
  #define TwoPowerD_TIMES_TwoPowerD (BasePowerExponent<2,Dimensions>::value*BasePowerExponent<2,Dimensions>::value)
  #define TwoPowerD_TIMES_TwoPowerD_DIVIDED_BY_TWO (BasePowerExponent<2,Dimensions>::value*BasePowerExponent<2,Dimensions-1>::value)
  #define TwoPowerD_TIMES_THREE_POWER_D (BasePowerExponent<2,Dimensions>::value*BasePowerExponent<3,Dimensions>::value)
  #define TwoPowerD_TIMES_FIVE_POWER_D (BasePowerExponent<2,Dimensions>::value*BasePowerExponent<5,Dimensions>::value)
  #define TwoPowerD_TIMES_THREE_POWER_D_DIVIDED_BY_THREE (BasePowerExponent<2,Dimensions>::value*BasePowerExponent<3,Dimensions-1>::value)
  #define TwoPowerD_DIVIDED_BY_TWO (BasePowerExponent<2,Dimensions-1>::value)
  #define TwoPowerDTimesD (BasePowerExponent<2,Dimensions>::value*Dimensions)
  #define TWO_POWER_TwoPowerD (BasePowerExponent<2,TwoPowerD>::value)
  #define THREE_POWER_D (BasePowerExponent<3,Dimensions>::value)
  #define THREE_POWER_D_MINUS_ONE (BasePowerExponent<3,Dimensions>::value-1)
  #define THREE_POWER_D_DIVIDED_BY_THREE (BasePowerExponent<3,Dimensions-1>::value)
  #define THREE_POWER_D_TIMES_FOUR_POWER_D (BasePowerExponent<3,Dimensions>::value*BasePowerExponent<4,Dimensions>::value)
  #define FOUR_POWER_D  (BasePowerExponent<4,Dimensions>::value)
  #define FOUR_POWER_D_DIVIDED_BY_FOUR  (BasePowerExponent<4,Dimensions-1>::value)
  #define FIVE_POWER_D  (BasePowerExponent<5,Dimensions>::value)
  #define FIVE_POWER_D_DIVIDED_BY_FIVE  (BasePowerExponent<5,Dimensions-1>::value)
  #define SIX_POWER_D  (BasePowerExponent<6,Dimensions>::value)
  #define SEVEN_POWER_D  (BasePowerExponent<7,Dimensions>::value)
  #define NINE_POWER_D  (BasePowerExponent<9,Dimensions>::value)
  #define ELEVEN_POWER_D  (BasePowerExponent<11,Dimensions>::value)
  #define THIRTEEN_POWER_D  (BasePowerExponent<13,Dimensions>::value)
  #define FIFTEEN_POWER_D  (BasePowerExponent<15,Dimensions>::value)
  #define SEVENTEEN_POWER_D  (BasePowerExponent<17,Dimensions>::value)
  #define NUMBER_OF_VERTICES_PER_ELEMENT TwoPowerD
  #define Dimensions_TIMES_TWO (2*Dimensions)
  #define Dimensions_TIMES_TwoPowerD (Dimensions*TwoPowerD)
  #define Dimensions_TIMES_FOUR (4*Dimensions)
  #define Dimensions_TIMES_Dimensions (Dimensions*Dimensions)
*/

#endif
