#include "Tracer.h"


double exahype2::dg::internal::projectValueOntoParticle(
  const peano4::datamanagement::CellMarker&    marker,
  int                                          order,
  const double* __restrict__                   QuadratureNodes1d,
  int                                          unknownsPerDoF,
  const double* __restrict__                   Q,
  const tarch::la::Vector<Dimensions,double>&  particleX,
  int                                          unknown
) {
  return 0.0;
}
