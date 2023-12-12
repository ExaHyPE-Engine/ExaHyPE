#pragma once

#include <iomanip>

#include "AbstractSSInfall.h"
#include "tarch/logging/Log.h"

namespace applications::exahype2::euler::sphericalaccretion {
  class SSInfall;
} // namespace applications::exahype2::euler::sphericalaccretion


class applications::exahype2::euler::sphericalaccretion::SSInfall: public AbstractSSInfall {
private:
  static tarch::logging::Log _log;

  std::string plotGlobalMTot(bool plotCopy) const;
  std::string plotRs() const;

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  /**
   * This is a straight 1:1 copy of r_s[sample_number-1]. We need a
   * scalar here, as this one goes to to the GPU.
   */
  static double _rMax;

  /**
   * This is a straight 1:1 copy of rho_x[sample_number-1]. We need
   * a scalar here, as this one goes to the GPU.
   */
  static double _rhoMax;

  /**
   * From m_tot_copy[sample_number-1].
   */
  static double _mTotMax;

  // @todo I've just removed this field. Couldn't find out where it is used.
  //    double t_record=0.0;

#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

  /**
   * Returns m_in for time t=0.
   */
  double getInitialMIn(double r_coord) const;

  /**
   * @return force density norm
   * @param m_in Typically the result of getInitialMIn()
   * @param t    Timestamp
   * @param density Typically Q[0]
   */
  static double getForceDensityNorm(double r_coord, double m_in, double t, double density, double a_input);

public:
  static const tarch::la::Vector<Dimensions, double> OverDensityCentre;

  void startTimeStep(
    double globalMinTimeStamp, double globalMaxTimeStamp, double globalMinTimeStepSize, double globalMaxTimeStepSize
  ) override;

  void finishTimeStep() override;

  ::exahype2::RefinementCommand refinementCriterion(
    const double* __restrict__ Q, // Q[5+0],
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t
  ) override;


  void initialCondition(
    double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    bool                                         gridIsConstructed
  ) override;


  /**
   * There are two variants of the source term. This one is the modified
   * gravity variant with an inhomogeneous source term.
   */
  void sourceTerm(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    double* __restrict__ S
  ) override;


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  /**
   * This is the variation that's used on the GPU. It works with
   * a homogeneous source term, i.e. zeroes on the right-hand side.
   */
  static void sourceTerm(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    double* __restrict__ S,
    Offloadable
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


  virtual void boundaryConditions(
    const double* __restrict__ Qinside, // Qinside[5+0]
    double* __restrict__ Qoutside,      // Qoutside[5+0]
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    int                                          normal
  ) override;


  /**
   * See flux() for a description why there are two maxEigenvalue() versions.
   */
  virtual double maxEigenvalue(
    const double* __restrict__ Q, // Q[5+0],
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) override;


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  static double maxEigenvalue(
    const double* __restrict__ Q, // Q[5+0],
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    Offloadable
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

  /**
   * The flux in the SSInfall does not depend on any variable of the
   * solver. Therefore, we wrote a second variant of the flux() function
   * which is static. This version basically invokes its static counterpart
   * subject to some additional assertions and logging.
   *
   * The static version fits to the GPU (see guidebook).
   */
  virtual void flux(
    const double* __restrict__ Q, // Q[5+0],
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F // F[5]
  ) override;


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  static void flux(
    const double* __restrict__ Q, // Q[5+0],
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F, // F[5]
    Offloadable
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


  void add_mass(const double r_coor, const double rho, const double size);

  double mass_interpolate(const double r_coor, const int MassCal);

  /**
   * Within the mass accumulation radius, I return false. Outside, I
   * return true. In the initial step, we do not allow anything to be
   * offloaded to an accelerator.
   */
  virtual bool patchCanUseStatelessPDETerms(
    const tarch::la::Vector<Dimensions, double>& patchCentre,
    const tarch::la::Vector<Dimensions, double>& patchH,
    double                                       t,
    double                                       dt
  ) const;


  /**
   * This is a very conservative routine. It takes the difference
   * to centre, computes the L2 norm, adds a safety distance of
   * @f$ \sqrt{3} h/2 @f$ and return true if an only if this holds.
   *
   * If a patch is definitely outside of the largest radius, there's
   * a couple of very natural consequences:
   *
   * - The interpolation is way easier than int eh general case: It
   *   will always be the fallback/generic version only using the
   *   static fields to interpolate.
   * - A patch outside of the largest radius will definitely not
   *   contribute at all to the global mass, density, ...
   *
   */
  bool isOutsideOfLargestRadius(
    const tarch::la::Vector<Dimensions, double>& patchCentre, const tarch::la::Vector<Dimensions, double>& patchH
  ) const;
};
