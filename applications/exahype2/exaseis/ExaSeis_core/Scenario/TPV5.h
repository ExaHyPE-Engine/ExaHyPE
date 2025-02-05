#pragma once

#include <cassert>

namespace Scenario{

  template<class Shortcuts>
  void initUnknownsPointwise(
    const double* const x,
    const tarch::la::Vector<Dimensions,double>& center,
    const double t,
    const double dt, 
    double* Q
  ){
    
    Q[s.rho] = 2.67;
    Q[s.cp ] = 6.0;
    Q[s.cs ] = 3.464;

  }

  template<class Shortcuts>
  void initPointSourceLocation(double pointSourceLocation[][3]){
    assert(false);
  }

  template<class Shortcuts>
  void setPointSourceVector(
    const double* const Q,
    const double* const x,const double t,const double dt,
    double* forceVector, int n
  ){

    assert(false);

  }

}
