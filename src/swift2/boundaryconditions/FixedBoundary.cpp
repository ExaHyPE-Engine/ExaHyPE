#include "FixedBoundary.h"


std::pair< tarch::la::Vector<Dimensions, double>, tarch::la::Vector<Dimensions, double> >  swift2::boundaryconditions::getUpdateDueToFixedBoundaryCondition(
  const tarch::la::Vector<Dimensions, double>&  particleX,
  const tarch::la::Vector<Dimensions, double>&  particleV,
  double                                        maxV,
  double                                        maxDt,
  double                                        h,
  const tarch::la::Vector<Dimensions, double>&  domainOffset,
  const tarch::la::Vector<Dimensions, double>&  domainSize,
  double                                        relativeDomainHalo
) {
  static tarch::logging::Log _log( "swift2::boundaryconditions" );
  tarch::la::Vector<Dimensions, double> newX = particleX;
  tarch::la::Vector<Dimensions, double> newV = particleV;

  assertion1( relativeDomainHalo>=0.0, relativeDomainHalo );
  assertion1( relativeDomainHalo<=1.0, relativeDomainHalo );

  const double domainHalo = relativeDomainHalo * h;

  const double relativeBufferDueToTimeStepping = 0.5  * h + maxV * maxDt;
  for (int d=0; d<Dimensions; d++) {
    // reposition particle left
    if ( tarch::la::smallerEquals( particleX(d), domainOffset(d) ) ) {
      logWarning( "getUpdateDueToFixedBoundaryCondition()", "reset particle at " << particleX << " as it penetrates left boundary along axis " << d );
      newX(d) = domainOffset(d);
    }

    // reposition particle right
    if ( tarch::la::greaterEquals( particleX(d), domainOffset(d) + domainSize(d)) ) {
      logWarning( "getUpdateDueToFixedBoundaryCondition()", "reset particle at " << particleX << " as it penetrates right boundary along axis " << d );
      newX(d) = domainOffset(d) + domainSize(d);
    }

    // damp velocity left
    if (
      tarch::la::smallerEquals(particleX(d), domainOffset(d) + relativeBufferDueToTimeStepping)
      and
      particleV(d)<0.0
    ) {
      double scaleOriginalVelocity = (particleX(d) - domainOffset(d)) / relativeBufferDueToTimeStepping;
      assertion5(scaleOriginalVelocity>=0.0, scaleOriginalVelocity, particleX, domainOffset, relativeBufferDueToTimeStepping, h );
      assertion5(scaleOriginalVelocity<=1.0, scaleOriginalVelocity, particleX, domainOffset, relativeBufferDueToTimeStepping, h );
      newV(d) = scaleOriginalVelocity * particleV(d);
    }


    // damp velocity right
    if (
      tarch::la::greaterEquals(particleX(d), domainOffset(d) + domainSize(d) - relativeBufferDueToTimeStepping)
      and
      particleV(d)>0.0
    ) {
      double scaleOriginalVelocity = (domainOffset(d) + domainSize(d) - particleX(d)) / relativeBufferDueToTimeStepping;
      assertion5(scaleOriginalVelocity>=0.0, scaleOriginalVelocity, particleX, domainOffset, relativeBufferDueToTimeStepping, h );
      assertion5(scaleOriginalVelocity<=1.0, scaleOriginalVelocity, particleX, domainOffset, relativeBufferDueToTimeStepping, h );
      newV(d) = scaleOriginalVelocity * particleV(d);
    }
  }

  return {newX, newV};
}

