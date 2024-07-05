#ifndef EXASEIS_SCENARIO_APATITE_HEADER
#define EXASEIS_SCENARIO_APATITE_HEADER

#include "Scenario.h"
#include "../Context/DomainInformation.h"

/*From Elastic Surface waves in crystals Komatitsch Part 2*/

template <class VariableShortcuts,int basisSize>
class Apatite : public Scenario<VariableShortcuts,basisSize>{
 private:
  double m_pointSourceLocation[3];
  
 public:
 Apatite(DomainInformation* info):Scenario<VariableShortcuts,basisSize>(info){
    //place point source in center of topography right below the surface
    m_pointSourceLocation[0]=info->domainSize[0] * 0.5 + info->domainOffset[0];
    m_pointSourceLocation[1]=info->domainOffset[1];                        
    m_pointSourceLocation[2]=info->domainSize[2] * 0.5 + info->domainOffset[2];
  };
  
  void initUnknownsPointwise(const double* const x,
                             const tarch::la::Vector<Dimensions,double>& center,
			     const double t,
			     const double dt,
			     double* Q
                             ) override{
    VariableShortcuts s;

#ifdef ISOTROPIC
    assertion1(false,"Apatite not implemented in the Isotropic case" );
#elif defined ANISOTROPIC

    Q[s.rho] = 3.19;

    Q[s.c + 0] = 167.0; //c11
    Q[s.c + 1] = 167.0; //c22
    Q[s.c + 2] = 140.0; //c33
    
    Q[s.c + 3] = 66.0;  //c44
    Q[s.c + 4] = 66.0;  //c55
    Q[s.c + 5] = (167-13.1)/2.0;  //c66

    Q[s.c + 6] = 13.1;  //c12
    Q[s.c + 7] = 66.0;  //c13
    Q[s.c + 8] = 66.0;  //c23

#else
#error Whole space problem only defined for Isotropic and Anisotropic Scenario
#endif
  }
  
  void initPointSourceLocation(double pointSourceLocation[][3]) override{
    pointSourceLocation[0][0]=m_pointSourceLocation[0];
    pointSourceLocation[0][1]=m_pointSourceLocation[1];
    pointSourceLocation[0][2]=m_pointSourceLocation[2];
  }
  
  void setPointSourceVector(const double* const Q,
			    const double* const x,const double t,const double dt,
			    double* forceVector, int n) override{

    assertion2(n == 0, "Only a single pointSource for WholeSpaceProblem_Anisotropic",n);

    constexpr double f0 = 250.0*1000.0;
    constexpr double t0 = 3.0/(2*f0)+5*10-6;
    
    double f = std::cos(2.0*M_PI*(t-t0)*f0)*std::exp(-2.0*(t-t0)*(t-t0)*f0*f0);

    VariableShortcuts s;
    forceVector[s.v + 1] = f;
  }
};
#endif
 
