#ifndef EXASEIS_SCENARIOWHOLESPACEPROBLEM_HEADER
#define EXASEIS_SCENARIOWHOLESPACEPROBLEM_HEADER

#include "Scenario.h"

template <class VariableShortcuts,int basisSize>
class WholeSpaceProblem : public Scenario<VariableShortcuts,basisSize>{
 private:
  double m_pointSourceLocation[3];
  
 public:
 WholeSpaceProblem(DomainInformation* info):Scenario<VariableShortcuts,basisSize>(info){
    //place point source in center of domain
    m_pointSourceLocation[0] = 15.0;
    m_pointSourceLocation[1] = 15.0;
    m_pointSourceLocation[2] = 15.0;
  };
  
  void initUnknownsPointwise(const double* const x,
                             const tarch::la::Vector<Dimensions,double>& center,
			     const double t,
			     const double dt,
			     double* Q
                             ) override{
    VariableShortcuts s;
    double rho = 2.67;
    double cp  = 6.0;
    double cs  = 3.464;
    

#ifdef ISOTROPIC
    Q[s.rho] = rho;
    Q[s.cp ] = cp;
    Q[s.cs ] = cs;

#elif defined ANISOTROPIC
    double mue = Q[s.rho] *cs*cs;
    double lambda = Q[s.rho]*(cp*cp -2*cs*cs);
    
    Q[s.c + 0] = 2*mue+lambda; //c11
    Q[s.c + 1] = 2*mue+lambda; //c22
    Q[s.c + 2] = 2*mue+lambda; //c33
    
    Q[s.c + 3] = lambda;  //c44
    Q[s.c + 4] = lambda;  //c55
    Q[s.c + 5] = lambda;  //c66

    Q[s.c + 6] = mue;  //c12
    Q[s.c + 7] = mue;  //c13
    Q[s.c + 8] = mue;  //c23
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

    assertion2(n == 0, "Only a single pointSource for WholeSpaceProblem",n);

    constexpr  double pi = 3.14159265359;
    constexpr  double sigma = 0.1149;
    constexpr  double t0 = 0.7;
    constexpr  double M0 = 1000.0;
    
    double f = M0*(1.0/(sigma*std::sqrt(2.0*pi)))*(std::exp(-((t-t0)*(t-t0))/(2.0*sigma*sigma)));

    VariableShortcuts s;
    forceVector[s.sigma + 0] = f;
    forceVector[s.sigma + 1] = f;
    forceVector[s.sigma + 2] = f;
  }
};
#endif
 
