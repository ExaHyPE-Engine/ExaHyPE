#ifndef EXASEIS_SCENARIOSINUSODIAL_HEADER
#define EXASEIS_SCENARIOSINUSODIAL_HEADER
#include "Hsp1.h"
#include <math.h>

template <class Shortcuts, int basisSize>
class Sinusodial : public Scenario<Shortcuts, basisSize>{
 private:
    double m_pointSourceLocation[3];

 public:
    
 Sinusodial(DomainInformation* info): Scenario<Shortcuts,basisSize>(info){
      //place point source in center of topography 2000m below surface
      m_pointSourceLocation[0] = 0.0;
      m_pointSourceLocation[1] = 0.2;
      m_pointSourceLocation[2] = 0.0;
    };

    void initUnknownsPointwise(const double* const x,
                               const tarch::la::Vector<Dimensions,double>& center,
                               const double t,
                               const double dt, 
                               double* Q
                               ){
      Shortcuts s;
#if defined ISOTROPIC
      Q[s.rho] = 2.0;
      Q[s.cs ] = 2.000;
      Q[s.cp ] = 3.464101615;
#else
#error Whole space problem only defined for Isotropic Simulations
#endif
      
    }

  void initPointSourceLocation(double pointSourceLocation[][3]){
    pointSourceLocation[0][0]=m_pointSourceLocation[0];
    pointSourceLocation[0][1]=m_pointSourceLocation[1];
    pointSourceLocation[0][2]=m_pointSourceLocation[2];
  }
    
    
    void setPointSourceVector(const double* const Q,
                              const double* const x,const double t,const double dt,
                              double* forceVector, int n){
      
      assertion2(n == 0, "Only a single pointSource for Sinusodial",n);
      
      constexpr double t0 = 0.2;
      constexpr double f0 = 10.0;
      constexpr double M0 = 1000.0;
      double pi = 2*std::acos(0.0);
      double alpha = -pi*pi*f0*f0;
      
      //     double f = 2.0 * alpha*(1.0 + 2.0 * alpha * (t-t0)*(t-t0)) * std::exp(alpha * (t-t0) * (t-t0));

      // We need em fu*** integral
      double f = std::exp(alpha*(t-t0)*(t-t0)) * 2 * alpha *(t-t0);
      
      Shortcuts s;
      forceVector[s.sigma + 0] = f;
      forceVector[s.sigma + 1] = f;
      forceVector[s.sigma + 2] = f;
    }
    
};
#endif
