// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FVRusanovSolver.h"

#include "Constants.h"
#include "math/CentralDifferencePartial2D.h"
#include "projections/EllipsoidProjection.h"

tarch::logging::Log applications::exahype2::swe::FVRusanovSolver::_log("applications::exahype2::swe::FVRusanovSolver");


void applications::exahype2::swe::FVRusanovSolver::initialCondition(
  [[maybe_unused]] double* const NOALIAS                        Q, // Q[3+1]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const bool                                   gridIsConstructed
) {
  Q[s::h]  = 100;
  Q[s::hu] = 0;
  Q[s::hv] = 0;
  Q[s::b]  = -100;
}


void applications::exahype2::swe::FVRusanovSolver::boundaryConditions(
  [[maybe_unused]] const double* const NOALIAS                  Qinside,  // Qinside[3+1]
  [[maybe_unused]] double* const NOALIAS                        Qoutside, // Qoutside[3+1]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  Qoutside[s::h]  = Qinside[s::h];
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
  Qoutside[s::b]  = Qinside[s::b];
}


::exahype2::RefinementCommand applications::exahype2::swe::FVRusanovSolver::refinementCriterion(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[3+1]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t
) {
  auto result = ::exahype2::RefinementCommand::Keep;
  // @todo Implement
  return result;
}


double applications::exahype2::swe::FVRusanovSolver::maxEigenvalue(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[3+1]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal
) {
  double L[3];
  L[s::h]  = 0.0;
  L[s::hu] = 0.0;
  L[s::hv] = 0.0;

  constexpr double DRY_TOL = std::numeric_limits<double>::epsilon();
  if (Q[s::h] <= DRY_TOL) {
    return 0.0;
  }

  const double u = Q[s::hu + normal] / Q[s::h];
  const double c = std::sqrt(GRAV * Q[s::h]);
  L[s::h]        = u;
  L[s::hu]       = u + c;
  L[s::hv]       = u - c;

  return std::fmax(std::fabs(L[0]), std::fmax(std::fabs(L[1]), std::fabs(L[2])));
}


void applications::exahype2::swe::FVRusanovSolver::flux(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[3+1]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const NOALIAS                        F // F[3]
) {
  F[s::h]  = 0.0;
  F[s::hu] = 0.0;
  F[s::hv] = 0.0;
  F[s::b]  = 0.0;

  constexpr double DRY_TOL = std::numeric_limits<double>::epsilon();
  if (Q[s::h] <= DRY_TOL) {
    return;
  }

  const double ih = 1.0 / Q[s::h];

  F[s::h]  = Q[s::hu + normal];
  F[s::hu] = Q[s::hu + normal] * Q[s::hu] * ih;
  F[s::hv] = Q[s::hu + normal] * Q[s::hv] * ih;
  F[s::hu + normal] += 0.5 * GRAV * Q[s::h] * Q[s::h];
}


void applications::exahype2::swe::FVRusanovSolver::nonconservativeProduct(
  [[maybe_unused]] const double* const NOALIAS                  Q,      // Q[3+1]
  [[maybe_unused]] const double* const NOALIAS                  deltaQ, // deltaQ[3+1]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const NOALIAS                        BTimesDeltaQ // BTimesDeltaQ[3]
) {
  BTimesDeltaQ[s::h]  = 0.0;
  BTimesDeltaQ[s::hu] = 0.0;
  BTimesDeltaQ[s::hv] = 0.0;
  BTimesDeltaQ[s::b]  = 0.0;

  constexpr double DRY_TOL = std::numeric_limits<double>::epsilon();
  if (Q[s::h] <= DRY_TOL) {
    return;
  }

  BTimesDeltaQ[s::hu + normal] = GRAV * Q[s::h] * (deltaQ[s::h] + deltaQ[s::b]);
}

namespace corriolis_force {
  namespace {
    /**
     * @brief Returns the Coriolis parameter f for a given position.
     *
     * This function provides the Coriolis parameter using either the f-plane
     * approximation (constant value) or the β-plane approximation (linearly varying with y).
     * Uncomment the relevant block depending on the desired model. The Coriolis force is
     * essential for geophysical flows and depends on Earth's rotation and latitude.
     *
     * @param x Cartesian x-coordinate (optional, not used unless advanced models are introduced)
     * @param y Cartesian y-coordinate representing the north-south direction
     * @return The Coriolis parameter f at the given location
     */
    auto constant([[maybe_unused]] const auto x, [[maybe_unused]] const auto y) {
      /**
       * --- Option 1: f-plane approximation ---
       *  Uncomment this block if you're using the f-plane model,
       *  where the Coriolis parameter is assumed constant over the domain.
       *  ⇒ Use this for small-scale simulations where latitude variation is negligible.
       */

      return 1.04e-4; // Example value for mid-latitudes
    };

    /**
     * @brief Returns the Coriolis parameter f for a given position.
     *
     * This function provides the Coriolis parameter using either the f-plane
     * approximation (constant value) or the β-plane approximation (linearly varying with y).
     * Uncomment the relevant block depending on the desired model. The Coriolis force is
     * essential for geophysical flows and depends on Earth's rotation and latitude.
     *
     * @param x Cartesian x-coordinate (optional, not used unless advanced models are introduced)
     * @param y Cartesian y-coordinate representing the north-south direction
     * @return The Coriolis parameter f at the given location
     */
    auto betaPlane([[maybe_unused]] const auto x, [[maybe_unused]] const auto y) {
      /**
       * --- Option 2: β-plane approximation ---
       * Uncomment this block if you want to model the Coriolis force as a linear
       * function of the north-south coordinate (y-direction): f(y) = f₀ + β·y
       * ⇒ Suitable for mid-scale domains where f changes slightly with latitude.
       */

      constexpr double f0   = 1.0e-4;  // f0 = reference Coriolis value
      constexpr double beta = 2.0e-11; // β: meridional gradient of the Coriolis parameter (df/dy), used in β-plane
                                       // approximation
      return f0 + (beta * y);
    };

    /**
     * @brief Returns the Coriolis parameter f for a given position.
     *
     * This function provides the Coriolis parameter using either the f-plane
     * approximation (constant value) or the β-plane approximation (linearly varying with y).
     * Uncomment the relevant block depending on the desired model. The Coriolis force is
     * essential for geophysical flows and depends on Earth's rotation and latitude.
     *
     * @param x Cartesian x-coordinate (optional, not used unless advanced models are introduced)
     * @param y Cartesian y-coordinate representing the north-south direction
     * @return The Coriolis parameter f at the given location
     */
    auto sphere([[maybe_unused]] const auto x, [[maybe_unused]] const auto y) {
      /**
       * --- Option 3: Breitengrad-abhängige Corioliskraft ---
       * Uncomment this block if you want to use the full spherical model,
       * where the Coriolis parameter is computed as f = 2 * Ω * sin(φ).
       * (φ = geographic latitude, Ω = Earth's rotation rate)
       * ⇒ f varies with latitude.
       */

      constexpr double omega = 7.2921150e-5;
      constexpr double toRad = M_PI / 180.0;

      const auto [lon, lat] = projection::albers_equal_area::ellipsoid::inverse(x, y);
      const auto latRad     = lat * toRad;
      return 2.0 * omega * std::sin(latRad);
    };
  } // namespace
} // namespace corriolis_force

namespace {
  /**
   * @brief Computes the Manning bottom friction source term.
   *
   * This function calculates the frictional using the empirical Manning formula. The result depends on the local water
   * height and momentum components. A small water depth threshold is enforced to avoid numerical instability as h → 0.
   *
   * The friction coefficient is defined by the Manning parameter, which may need to be
   * adjusted depending on the bathymetry, resolution, or physical characteristics of
   * the domain.
   *
   * @param h  Water height
   * @param hu x-direction
   * @param hv y-direction
   * @return Bottom friction force contribution to the momentum equations
   */
  auto manningFriction(const auto h, const auto hu, const auto hv) {
    namespace constants            = ::applications::exahype2::swe;
    constexpr auto manningFriction = 0.025; // sm^(1/3)
    constexpr auto hMin            = 0.2;   // threshold to ensure stable bottom friction computation when h → 0

    const auto hSafe = (h > hMin ? h : hMin);    // minimal water depth to avoid division by zero or instability near
                                                 // dry cells
    const auto h43 = (hSafe * std::cbrt(hSafe)); // h^(4/3) is not stable for small h -> 0, TODO may need some more
                                                 // work

    return -constants::GRAV * manningFriction * manningFriction * std::sqrt((hu * hu) + (hv * hv)) / h43;
  }

  auto atmosphericPressure(const auto x, const auto y, const auto t) {
    namespace constants = ::applications::exahype2::swe;
    constexpr auto aLon = constants::pointLonA;
    constexpr auto aLat = constants::pointLatA;

    constexpr auto bLon = constants::pointLonB;
    constexpr auto bLat = constants::pointLatB;

    // https://edanya.uma.es/hysea/index.php/benchmarks/meteo-tsunami/143-reproducing-real-meteotsunami-in-the-gulf-of-mexico
    auto pressureGaus = [](const auto x, const auto y) {
      constexpr auto C  = 0.5;
      constexpr auto AL = 37.61;
      constexpr auto AR = 4.665;
      constexpr auto BL = 0.31;
      constexpr auto BR = 0.5;
      const auto     y2 = ((y / C) * (y / C));
      if (x < 0) {
        return -AL * x * std::exp(-y2 - ((x / BL) * (x / BL)));
      }
      if (x > 0) {
        return -AR * x * std::exp(-y2 - ((x / BR) * (x / BR)));
      }
      return 0.0;
    };

    auto distanceOverTime = [](const auto t) {
      constexpr auto speed = 20.0; // 20 m/s
      return speed * t;
    };

    // Computes the geographic point at distance l (in meters) along the great-circle vector from a to b,
    // using projected coordinates for distance calculation and linear interpolation in lon/lat.
    auto pointOnVector = [](const auto l) {
      const auto [aX, aY]        = projection::albers_equal_area::ellipsoid::forward(aLon, aLat);
      const auto [bX, bY]        = projection::albers_equal_area::ellipsoid::forward(bLon, bLat);
      static const auto distance = std::sqrt(((bX - aX) * (bX - aX)) + ((bY - aY) * (bY - aY)));
      return std::pair{aLon + (l * (bLon - aLon) / distance), aLat + (l * (bLat - aLat) / distance)};
    };

    // Rotates (lon, lat) so that the path from a to b lies along the x-axis; used to align atmospheric pressure
    // spatially.
    auto rotLonLat = [](const auto lon, const auto lat) {
      constexpr auto dLon = bLon - aLon;
      constexpr auto dLat = bLat - aLat;

      static const auto rad = std::atan2(dLat, dLon);

      static const auto sin = std::sin(rad);
      static const auto cos = std::cos(rad);

      const auto lonNew = (lon * cos) + (lat * sin);
      const auto latNew = (-lon * sin) + (lat * cos);
      return std::pair{lonNew, latNew};
    };

    const auto distance               = distanceOverTime(t);
    const auto [lon, lat]             = projection::albers_equal_area::ellipsoid::inverse(x, y);
    const auto [lonCenter, latCenter] = pointOnVector(distance);
    const auto [lonRot, latRot]       = rotLonLat(lon - lonCenter, lat - latCenter);
    return pressureGaus(lonRot, latRot);
  };
} // namespace

void applications::exahype2::swe::FVRusanovSolver::sourceTerm(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[3+1]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] double* const NOALIAS                        S // S[3]
) {
  namespace constants = ::applications::exahype2::swe;

  /**
   * @brief Field case : A meteotsunami in the Gulf of Mexico (2010)
   * @see
   * https://edanya.uma.es/hysea/index.php/benchmarks/meteo-tsunami/143-reproducing-real-meteotsunami-in-the-gulf-of-mexico
   */

  // default values
  S[s::h]  = 0;
  S[s::hu] = 0;
  S[s::hv] = 0;

  // Skip dry cells
  constexpr double DRY_TOL = std::numeric_limits<double>::epsilon();
  if (Q[s::h] <= DRY_TOL) {
    return;
  }

  /**
   * @brief Water density in the Gulf of Mexico, expressed in sigma-t (σₜ) units.
   *
   * Unit: kg/(m^3)
   *
   * @see https://www.harteresearch.org/sites/default/files/inline-files/3.pdf
   */
  constexpr auto waterDensity = constants::waterDensity;

  const auto xCoord = x(0);
  const auto yCoord = x(1);

  const auto uVel = Q[s::hu] / Q[s::h];
  const auto vVel = Q[s::hv] / Q[s::h];

  // Update Source term
  S[s::hu] = corriolis_force::sphere(xCoord, yCoord) * uVel;
  S[s::hv] = -corriolis_force::sphere(xCoord, yCoord) * vVel;

  S[s::hu] += manningFriction(Q[s::h], Q[s::hu], Q[s::hv]) * Q[s::hu];
  S[s::hv] += manningFriction(Q[s::h], Q[s::hu], Q[s::hv]) * Q[s::hv];

  auto wrapperAtmosphericPressure = [](const auto x, const auto y, const auto t) {
    return atmosphericPressure(x, y, t);
  };

  S[s::hu] += -math::differentiation::numeric_3D::central_difference::
                partialX(wrapperAtmosphericPressure, xCoord, yCoord, t)
              / waterDensity;
  S[s::hv] += -math::differentiation::numeric_3D::central_difference::
                partialY(wrapperAtmosphericPressure, xCoord, yCoord, t)
              / waterDensity;
}
