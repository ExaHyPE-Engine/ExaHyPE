// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"

namespace applications {
  namespace exahype2 {
    namespace GravityWaves {
      namespace Airglow {
        class ProductionRates;
      } // namespace Airglow
    } // namespace GravityWaves
  } // namespace exahype2
} // namespace applications

class applications::exahype2::GravityWaves::Airglow::ProductionRates {

  /********************************************************************************************************************************
   * The collection of constants, functions, and lookup tables contained here is based on * Snively et al. "OH and OI
   *airglow layer modulation by ducted short-period gravity waves: Effects of trapping altitude", 2009.*
   ********************************************************************************************************************************/
public:
  /**************
   * O(S) Model *
   **************/

  static double constexpr DELTA = 0.2;
  static double constexpr ZETA  = 0.03;

  static inline GPU_CALLABLE_METHOD double k1(const double temperature) {
    // three body production of molecular oxygen
    const double factor = 300.0 / temperature;
    return 4.7e-33 * factor * factor;
  }

  static inline GPU_CALLABLE_METHOD double k2() {
    // quenching of O2(c sum(u)) by O2
    return 5.0e-13;
  }

  static inline GPU_CALLABLE_METHOD double k3() {
    // quenching of O2(c sum(u)) by O
    return 6.0e-12;
  }

  static inline GPU_CALLABLE_METHOD double k4(const double temperature) {
    // quenching of O(S)
    return 4.0e-12 * tarch::la::pow(tarch::la::E, -865.0 / temperature);
  }

  static inline GPU_CALLABLE_METHOD double A1() {
    // Radiative loss of O2(c sum(u))
    return 2.0e-2;
  }

  static inline GPU_CALLABLE_METHOD double A2() {
    // Radiative loss of O(S) at all wave lengths
    return 1.105;
  }

  static inline GPU_CALLABLE_METHOD double A5577() {
    // Radiative loss of O(S) at 557.7nm
    return 1.06;
  }

  /***************
   * OH(v) Model *
   ***************/

  static inline GPU_CALLABLE_METHOD double branching(const int vibrationalState) {
// branching rates from Makhlouf et al. 1995, Table 1
#if !defined(WITH_GPU)
    assertion1(vibrationalState < 10, vibrationalState);
    assertion1(vibrationalState > -1, vibrationalState);
#endif

    switch (vibrationalState) {
    case 9:
      return 0.48;
    case 8:
      return 0.27;
    case 7:
      return 0.17;
    case 6:
      return 0.08;
    default:
      return 0.0;
    }
  }

  static inline GPU_CALLABLE_METHOD double k5(const double temperature, const int vibrationalState) {
    // production of OH(vibrationalState)
    return 1.4e-10 * tarch::la::pow(tarch::la::E, -470.0 / temperature) * branching(vibrationalState);
  }

  static inline GPU_CALLABLE_METHOD double k6(const double temperature) {
    // three-body production of O3
    return 6.0e-34 * tarch::la::pow(300.0 / temperature, 2.3);
  }

  static inline GPU_CALLABLE_METHOD double k7(const int vibrationalState) {
// quenching OH(v) and O into H and O2
// rates from Makhlouf et al. "Photochemical-dynamical modeling of the measured response of airglow to gravity waves",
// 1995, Table 1, k_{8v}
#if !defined(WITH_GPU)
    assertion1(vibrationalState < 10, vibrationalState);
    assertion1(vibrationalState > -1, vibrationalState);
#endif
    switch (vibrationalState) {
    case 0:
      return 3.9e-11;
    case 1:
      return 10.5e-11;
    default:
      return 25e-11;
    }
  }

  static inline GPU_CALLABLE_METHOD double k8(const int vibrationalState, const int quenchedState) {
// quenching OH(v) and O2 into OH(v') and O2
// rates from Adler-Golden "Kinetic parameters for OH nightglow modeling consisent with recent laboratory measurements",
// 1997, Table 3
#if !defined(WITH_GPU)
    assertion1(vibrationalState < 10, vibrationalState);
    assertion1(vibrationalState > 0, vibrationalState);
    assertion1(quenchedState < 9, quenchedState);
    assertion1(quenchedState > -1, quenchedState);
    assertion2(quenchedState < vibrationalState, vibrationalState, quenchedState);
#endif
    if (vibrationalState == 1) {
      return 2e-13;
    }

    if (vibrationalState == 2) {
      if (quenchedState == 0)
        return 0.0;
      return 4e-13;
    }

    if (vibrationalState == 3) {
      if (quenchedState == 0)
        return 0.0;
      if (quenchedState == 1)
        return 1e-13;
      return 7e-13;
    }

    if (vibrationalState == 4) {
      if (quenchedState == 0)
        return 0.0;
      if (quenchedState == 1)
        return 1e-13;
      if (quenchedState == 2)
        return 2e-13;
      return 10e-13;
    }

    if (vibrationalState == 5) {
      if (quenchedState == 0)
        return 0.0;
      if (quenchedState == 1)
        return 1e-13;
      if (quenchedState == 2)
        return 2e-13;
      if (quenchedState == 3)
        return 6e-13;
      return 16e-13;
    }

    if (vibrationalState == 6) {
      switch (quenchedState) {
      case 0:
        return 1e-13;
      case 1:
        return 1e-13;
      case 2:
        return 3e-13;
      case 3:
        return 6e-13;
      case 4:
        return 11e-13;
      case 5:
        return 22e-13;
      default:
        assert(false);
      }
    }


    if (vibrationalState == 7) {
      switch (quenchedState) {
      case 0:
        return 4e-13;
      case 1:
        return 6e-13;
      case 2:
        return 9e-13;
      case 3:
        return 12e-13;
      case 4:
        return 16e-13;
      case 5:
        return 23e-13;
      case 6:
        return 32e-13;
      default:
        assert(false);
      }
    }

    if (vibrationalState == 8) {
      switch (quenchedState) {
      case 0:
        return 4e-13;
      case 1:
        return 6e-13;
      case 2:
        return 8e-13;
      case 3:
        return 10e-13;
      case 4:
        return 14e-13;
      case 5:
        return 19e-13;
      case 6:
        return 25e-13;
      case 7:
        return 33e-13;
      default:
        assert(false);
      }
    }

    if (vibrationalState == 9) {
      switch (quenchedState) {
      case 0:
        return 28e-13;
      case 1:
        return 29e-13;
      case 2:
        return 31e-13;
      case 3:
        return 32e-13;
      case 4:
        return 34e-13;
      case 5:
        return 36e-13;
      case 6:
        return 38e-13;
      case 7:
        return 40e-13;
      case 8:
        return 42e-13;
      default:
        assert(false);
      }
    }

    return 0.0;
  }

  static inline GPU_CALLABLE_METHOD double k9(const int vibrationalState) {
// quenching OH(v) and N2 into OH(v - 1) and N2
// rates from Adler-Golden "Kinetic parameters for OH nightglow modeling consisent with recent laboratory measurements",
// 1997, Table 1, k_4(v)
#if !defined(WITH_GPU)
    assertion1(vibrationalState < 10, vibrationalState);
    assertion1(vibrationalState > 0, vibrationalState);
#endif

    switch (vibrationalState) {
    case 1:
      return 0.06e-13;
    case 2:
      return 0.1e-13;
    case 3:
      return 0.17e-13;
    case 4:
      return 0.3e-13;
    case 5:
      return 0.52e-13;
    case 6:
      return 0.91e-13;
    case 7:
      return 1.6e-13;
    case 8:
      return 7e-13;
    case 9:
      return 4.8e-13;
    default:
      assert(false);
    }
  }

  /***********************************************************************************************************************************
   * Einstein Coefficients from Khomich et al. "Airglow as an Indicator of Upper Atmospheric Structure and Dynamics",
   *2008, Page 146 *
   ***********************************************************************************************************************************/

  static int constexpr LANGHOFFBANDWIDTH = 5;

  static inline GPU_CALLABLE_METHOD double langhoffEinsteinCoefficients(
    const int vibrationalState,
    const int quenchedState
  ) {
#if !defined(WITH_GPU)
    assertion1(vibrationalState < 10, vibrationalState);
    assertion1(vibrationalState > 0, vibrationalState);
    assertion1(quenchedState < 9, quenchedState);
    assertion1(quenchedState > -1, quenchedState);
    assertion2(quenchedState < vibrationalState, vibrationalState, quenchedState);
    assertion3(
      vibrationalState - LANGHOFFBANDWIDTH <= quenchedState,
      vibrationalState,
      quenchedState,
      LANGHOFFBANDWIDTH
    );
#endif

    if (vibrationalState == 1) {
      return 15.0;
    }

    if (vibrationalState == 2) {
      if (quenchedState == 0)
        return 8.5;
      return 21.0;
    }

    if (vibrationalState == 3) {
      if (quenchedState == 0)
        return 0.69;
      if (quenchedState == 1)
        return 23.5;
      return 19.0;
    }

    if (vibrationalState == 4) {
      if (quenchedState == 0)
        return 0.065;
      if (quenchedState == 1)
        return 2.7;
      if (quenchedState == 2)
        return 43.0;
      return 14.0;
    }

    if (vibrationalState == 5) {
      if (quenchedState == 0)
        return 0.011;
      if (quenchedState == 1)
        return 0.34;
      if (quenchedState == 2)
        return 6.4;
      if (quenchedState == 3)
        return 65.0;
      return 7.0;
    }

    if (vibrationalState == 6) {
      if (quenchedState == 1)
        return 0.055;
      if (quenchedState == 2)
        return 0.91;
      if (quenchedState == 3)
        return 12.6;
      if (quenchedState == 4)
        return 85.0;
      return 3.0;
    }

    if (vibrationalState == 7) {
      if (quenchedState == 2)
        return 0.19;
      if (quenchedState == 3)
        return 2.1;
      if (quenchedState == 4)
        return 21.0;
      if (quenchedState == 5)
        return 103.0;
      return 3.0;
    }

    if (vibrationalState == 8) {
      if (quenchedState == 3)
        return 0.42;
      if (quenchedState == 4)
        return 4.0;
      if (quenchedState == 5)
        return 33.0;
      if (quenchedState == 6)
        return 111.0;
      return 8.0;
    }

    if (vibrationalState == 9) {
      if (quenchedState == 4)
        return 0.96;
      if (quenchedState == 5)
        return 6.9;
      if (quenchedState == 6)
        return 48.0;
      if (quenchedState == 7)
        return 108.0;
      return 21.0;
    }

    return 0.0;
  }

  static int constexpr TURNBULLLOWEBANDWIDTH = 6;

  static inline GPU_CALLABLE_METHOD double turnbullLoweEinsteinCoefficients(
    const int vibrationalState,
    const int quenchedState
  ) {
#if !defined(WITH_GPU)
    assertion1(vibrationalState < 10, vibrationalState);
    assertion1(vibrationalState > 0, vibrationalState);
    assertion1(quenchedState < 9, quenchedState);
    assertion1(quenchedState > -1, quenchedState);
    assertion2(quenchedState < vibrationalState, vibrationalState, quenchedState);
    assertion3(
      vibrationalState - TURNBULLLOWEBANDWIDTH <= quenchedState,
      vibrationalState,
      quenchedState,
      TURNBULLLOWEBANDWIDTH
    );
#endif

    if (vibrationalState == 1) {
      return 22.7;
    }

    if (vibrationalState == 2) {
      if (quenchedState == 0)
        return 15.4;
      return 30.4;
    }

    if (vibrationalState == 3) {
      if (quenchedState == 0)
        return 2.03;
      if (quenchedState == 1)
        return 40.3;
      return 28.1;
    }

    if (vibrationalState == 4) {
      if (quenchedState == 0)
        return 0.3;
      if (quenchedState == 1)
        return 7.19;
      if (quenchedState == 2)
        return 69.8;
      return 20.3;
    }

    if (vibrationalState == 5) {
      if (quenchedState == 0)
        return 0.051;
      if (quenchedState == 1)
        return 1.32;
      if (quenchedState == 2)
        return 15.9;
      if (quenchedState == 3)
        return 99.4;
      return 11.0;
    }

    if (vibrationalState == 6) {
      if (quenchedState == 0)
        return 0.01;
      if (quenchedState == 1)
        return 0.27;
      if (quenchedState == 2)
        return 34.8;
      if (quenchedState == 3)
        return 27.9;
      if (quenchedState == 4)
        return 126.0;
      return 4.0;
    }

    if (vibrationalState == 7) {
      if (quenchedState == 1)
        return 0.063;
      if (quenchedState == 2)
        return 0.85;
      if (quenchedState == 3)
        return 7.16;
      if (quenchedState == 4)
        return 42.9;
      if (quenchedState == 5)
        return 145.0;
      return 2.3;
    }

    if (vibrationalState == 8) {
      if (quenchedState == 2)
        return 0.23;
      if (quenchedState == 3)
        return 2.01;
      if (quenchedState == 4)
        return 12.7;
      if (quenchedState == 5)
        return 60.0;
      if (quenchedState == 6)
        return 154.0;
      return 8.6;
    }

    if (vibrationalState == 9) {
      if (quenchedState == 3)
        return 0.62;
      if (quenchedState == 4)
        return 4.05;
      if (quenchedState == 5)
        return 19.9;
      if (quenchedState == 6)
        return 78.6;
      if (quenchedState == 7)
        return 149;
      return 23.7;
    }

    return 0.0;
  }

  static int constexpr GOLDMANBANDWIDTH = 6;

  static inline GPU_CALLABLE_METHOD double goldmanEinsteinCoefficients(
    const int vibrationalState,
    const int quenchedState
  ) {
#if !defined(WITH_GPU)
    assertion1(vibrationalState < 10, vibrationalState);
    assertion1(vibrationalState > 0, vibrationalState);
    assertion1(quenchedState < 9, quenchedState);
    assertion1(quenchedState > -1, quenchedState);
    assertion2(quenchedState < vibrationalState, vibrationalState, quenchedState);
    assertion3(vibrationalState - GOLDMANBANDWIDTH <= quenchedState, vibrationalState, quenchedState, GOLDMANBANDWIDTH);
#endif

    if (vibrationalState == 1) {
      return 17.33;
    }

    if (vibrationalState == 2) {
      if (quenchedState == 0)
        return 10.31;
      return 23.58;
    }

    if (vibrationalState == 3) {
      if (quenchedState == 0)
        return 1.12;
      if (quenchedState == 1)
        return 27.55;
      return 22.2;
    }

    if (vibrationalState == 4) {
      if (quenchedState == 0)
        return 0.13;
      if (quenchedState == 1)
        return 4.12;
      if (quenchedState == 2)
        return 48.68;
      return 16.45;
    }

    if (vibrationalState == 5) {
      if (quenchedState == 0)
        return 0.019;
      if (quenchedState == 1)
        return 0.63;
      if (quenchedState == 2)
        return 9.43;
      if (quenchedState == 3)
        return 70.70;
      return 9.31;
    }

    if (vibrationalState == 6) {
      if (quenchedState == 0)
        return 0.003;
      if (quenchedState == 1)
        return 0.108;
      if (quenchedState == 2)
        return 1.74;
      if (quenchedState == 3)
        return 17.14;
      if (quenchedState == 4)
        return 91.44;
      return 3.78;
    }

    if (vibrationalState == 7) {
      if (quenchedState == 1)
        return 0.023;
      if (quenchedState == 2)
        return 0.37;
      if (quenchedState == 3)
        return 3.78;
      if (quenchedState == 4)
        return 27.19;
      if (quenchedState == 5)
        return 107.25;
      return 2.27;
    }

    if (vibrationalState == 8) {
      if (quenchedState == 2)
        return 0.088;
      if (quenchedState == 3)
        return 0.92;
      if (quenchedState == 4)
        return 7.04;
      if (quenchedState == 5)
        return 39.09;
      if (quenchedState == 6)
        return 116.58;
      return 7.25;
    }

    if (vibrationalState == 9) {
      if (quenchedState == 3)
        return 0.25;
      if (quenchedState == 4)
        return 1.96;
      if (quenchedState == 5)
        return 11.72;
      if (quenchedState == 6)
        return 51.92;
      if (quenchedState == 7)
        return 117.6;
      return 20.41;
    }

    return 0.0;
  }

}; // namespace applications::exahype2::GravityWaves::ChemicalAdvection::ProductionRates
