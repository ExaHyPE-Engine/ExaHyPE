#ifndef EXASEIS_GAUSSIANWAVE_HEADER
#define EXASEIS_GAUSSIANWAVE_HEADER

#include "Scenario.h"

template <class Variables,int basisSize>
  class GaussianWave : public Scenario<Variables, basisSize>{
 public:

  GaussianWave(DomainInformation* info):Scenario<Variables,basisSize>(info){};

  virtual void initUnknownsPointwise(const double* const x,
                                     const tarch::la::Vector<Dimensions,double>& center,
				     const double t,
				     const double dt, 
				     double* Q
                                     ){
    Variables s;
    double center_curve[3];

    center_curve[0] = 15.0;
    center_curve[1] = 15.0;
    center_curve[2] = 15.0;    

    double rho = 2.67;
    double cp  = 6.0;
    double cs  = 3.343;
    
    Q[s.rho] = rho;
    Q[s.cp ] = cp;
    Q[s.cs ] = cs;

    double radius = 3.0 ;
    double height = 3.0;
    
    Q[ s.v + 0 ] = std::exp(-((x[0]-center_curve[0])*(x[0]-center_curve[0])+
                              (x[1]-center_curve[1])*(x[1]-center_curve[1])+
                              (x[2]-center_curve[2])*(x[2]-center_curve[2]))/radius) * radius;
    Q[ s.v + 1 ] = Q[ s.v + 0 ];
    Q[ s.v + 2 ] = Q[ s.v + 1 ];
                              

    for(int i= 0 ; i < 6 ; i++){
      Q[ s.sigma + i ] = 0.0;
    }
    
  }

  virtual void initPointSourceLocation(double pointSourceLocation[][3]){
    assertion1(false,"No pointsource for a Gaussian Wave");
  };

  virtual void setPointSourceVector(const double* const Q,
				    const double* const x,const double t,const double dt,
				    double* forceVector, int n){
    assertion1(false,"No pointsource for a Gaussian Wave");
  };
};

#endif
