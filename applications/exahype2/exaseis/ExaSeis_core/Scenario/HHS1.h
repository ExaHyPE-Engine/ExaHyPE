#pragma once

namespace Scenario{

  template<class Shortcuts>
  void initUnknownsPointwise(
    const double* const x,
    const tarch::la::Vector<Dimensions,double>& center,
    const double t,
    const double dt, 
    double* Q
  ){
    
    Q[Shortcuts::rho] = 2.7;
    Q[Shortcuts::cp ] = 6.0;
    Q[Shortcuts::cs ] = 3.464;

  }

  template<class Shortcuts>
  void initPointSourceLocation(double pointSourceLocation[][3]){
    pointSourceLocation[0][0] = 0.0;
    pointSourceLocation[0][1] = 0.693;
    pointSourceLocation[0][2] = 0.0;
  }

  template<class Shortcuts>
  void setPointSourceVector(
    const double* const Q,
    const double* const x,const double t,const double dt,
    double* forceVector, int n
  ){

    assertion2(n == 0, "Only a single pointSource for Hhs1",n);

    constexpr double t0 = 0.1;
    constexpr double M0 = 1000.0;
    
    double f = M0*t/(t0*t0)*std::exp(-t/t0);
    
    forceVector[s::sigma + 4] = f;
  }

}
