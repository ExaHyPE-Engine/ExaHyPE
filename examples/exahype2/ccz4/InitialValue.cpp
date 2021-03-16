#include "InitialValue.h"
#include "Properties.h"

/**
 *  *  * This file is automatically created by Peano. I need it to interact with
 *   *   * the Python API, i.e. to read out data set there.
 *    *    */
#include "Constants.h"

#include "tarch/la/Vector.h"

#include "peano4/utils/Globals.h"

#include <stdio.h>
#include <string.h>



void examples::exahype2::ccz4::gaugeWave(
  double * __restrict__ Q, // Q[64+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double t
) {
  constexpr int nVars = 59;
  constexpr double pi = M_PI;
  constexpr double peak_number = 2.0;
  constexpr double ICA = 0.1; ///< Amplitude of the wave 
  double HH     = 1.0 - ICA*sin( peak_number*pi*( x[0] - t));
  double dxH    = -peak_number*pi*ICA*cos( peak_number * pi*(x[0] - t));
  double dxphi  = - pow(HH,(-7.0/6.0))*dxH/6.0;
  double phi    = pow(( 1.0 / HH),(1.0/6.0));
  double Kxx    = - 0.5*peak_number*pi*ICA*cos( peak_number * pi*(x[0] - t))/sqrt( 1.0 - ICA*sin( peak_number*pi*( x[0] - t))  );
  double traceK = Kxx/HH;
  memset(Q, .0, nVars*sizeof(double));
  Q[0]  = phi*phi*HH ;
  Q[3]  = phi*phi  ;                         
  Q[5]  = phi*phi                            ;
  Q[6]  = phi*phi*(Kxx - 1.0/3.0*traceK*HH ) ;
  Q[9] =  phi*phi*(0.0 - 1.0/3.0*traceK*1.0) ;
  Q[11] = phi*phi*(0.0 - 1.0/3.0*traceK*1.0)  ;
  Q[16] = log(sqrt(HH));
  Q[13] = 2.0/(3.0*pow(HH,(5.0/3.0)))*dxH        ;
  Q[23] = 1.0/(2.0*HH)*dxH               ;   
  Q[35] = pow(HH,(-1.0/3.0))*dxH/3.0         ;
  Q[38] = phi*dxphi                     ;
  Q[40] = phi*dxphi                    ;
  Q[53] = traceK;
  Q[54] = log(phi);
  Q[55] = dxphi/phi;
}



