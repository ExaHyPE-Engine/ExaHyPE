#ifndef EXASEIS_SCENARIOZUGSPITZE_HEADER
#define EXASEIS_SCENARIOZUGSPITZE_HEADER

#include "Scenario.h"

template <class Shortcuts,int basisSize>
class Zugspitze : public Scenario<Shortcuts,basisSize>{
 private:
  double m_pointSourceLocation[3];
  
 public:
 Zugspitze(DomainInformation* info):Scenario<Shortcuts,basisSize>(info){
    //place point source in center of domain
    double center[3];
    info->getCenter(center);
    m_pointSourceLocation[0] = 4424.756;
    //    m_pointSourceLocation[1] =   11.320;
    m_pointSourceLocation[1] =   10.0;
    m_pointSourceLocation[2] = 2675.783;
  };
  
  void initUnknownsPointwise(const double* const x,
                             const tarch::la::Vector<Dimensions,double>& center,
			     const double t,
			     const double dt,
			     double* Q
                             ) override{
    Shortcuts s;
    double rho = 2.7;
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

    assertion2(n == 0, "Only a single pointSource for Zugspitze",n);

    constexpr  double pi = 3.14159265359;
    constexpr  double sigma = 0.1149;
    constexpr  double t0 = 0.1;
    constexpr  double M0 = 1000.0;
    
    double f =  M0*t/(t0*t0)*std::exp(-t/t0);

    Shortcuts s;
    forceVector[s.sigma + 4] = f;
  }

  void refinementCriteria(exahype2::solvers::aderdg::Solver* solver,
                          std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria) override{
    criteria.push_back(new Refinement::StaticAMR<Shortcuts>);
    double position_lower[3] = {4305.0 ,-5.0 , 2650.0};
    double position_upper[3] = {4445.0 , 0.0 , 2750.0};
    criteria.push_back(new Refinement::RefineBetweenPositions<Shortcuts>(
                                                              solver->getMaximumAdaptiveMeshLevel(),
                                                              position_lower,
                                                              position_upper));

  }
};
#endif
 
