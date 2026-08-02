#pragma once

namespace applications{
  namespace exahype2 {
    namespace exaseis {

      // Elements of the initial stress tensor
      template<typename T>
      static inline
      void initialStressTensor(
        const double* const x,
        T& sxx,
        T& syy,
        T& szz,
        T& sxy,
        T& sxz,
        T& syz
      ){
        // Must assign values to the various stresses
        {{initialStresses}}
      }

      // additional stress on the fault in yz direction normal to the fault
      template<typename T>
      static inline
      T n_yz(
        const double* const x
      ){
        {{n_yz}}
      }

      template<typename T>
      static inline
      T fault_cohesion(
        const double* const x
      ){
        {{cohesion}}
      }

      template<typename T>
      static inline
      T forced_rupture_time(
        const double* const x
      ){
        {{forced_rupture_time}}
      }

      // Slip-weakening critical distance (=d_c)
      template<typename T>
      static inline
      T slipWeakeningCriticalDistance(
        const double* const x
      ){
        {{d_c}}
      }

      // Constant Rupture parameters

      //Dynamic coefficient of friction (dimensionless)
      constexpr double mu_d = {{mu_d}};
      //Static coefficient of friction (dimensionless)
      constexpr double mu_s = {{mu_s}};

      //duration of forced rupture (seconds)
      // the forces holding the fault together will decrease linearly for this duration starting at the forced rupture time.
      constexpr double t0_rupture = {{t0_rupture}}; // (=t_0)

      /**
       * These define the position of the area with frictional cohesion.
       * Specifically, these define a rectangle of size 2*f_wy x 2*f_wz
       * in the y-z axis centered around the point f_cy x f_cz.
       * If within this rectangle, frictional cohesion applies, otherwise
       * it does not.
       * i.e. if -f_wy < y - f_cy < f_wy and -f_wz < z - f_cz < f_wz,
       * frictional cohesion applies.
       */
      constexpr double f_cy = {{f_cy}};
      constexpr double f_wy = {{f_wy}};
      constexpr double f_cz = {{f_cz}};
      constexpr double f_wz = {{f_wz}};

    } // namespace exaseis
  } // namespace exahype2
} // namespace applications