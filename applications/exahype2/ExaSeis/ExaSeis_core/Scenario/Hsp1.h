#ifndef EXASEIS_SCENARIOHSPONE_HEADER
#define EXASEIS_SCENARIOHSPONE_HEADER
#include "Scenario.h"

template <class Shortcuts, int basisSize>
class Hsp1 : public Scenario<Shortcuts, basisSize>{
 private:
    double m_pointSourceLocation[3];

 public:
    
  Hsp1(DomainInformation* info):Scenario<Shortcuts,basisSize>(info){
    //place point source in center of topography 2km below surface
      m_pointSourceLocation[0] =   0.0;
      m_pointSourceLocation[1] =  20.0;
      m_pointSourceLocation[2] =   0.0;
  };


  void initUnknownsPointwise(const double* const x,
                             const tarch::la::Vector<Dimensions,double>& center,
			     const double t,
			     const double dt, 
			     double* Q
                             ){
    Shortcuts s;
#if defined ISOTROPIC
    Q[s.rho] = 2.7;
    Q[s.cp ] = 6.0;
    Q[s.cs ] = 3.343;
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

    assertion2(n == 0, "Only a single pointSource for Hsp1",n);


    constexpr double t0 = 0.1;
    constexpr double M0 = 1000.0;
    
    double f = M0*t/(t0*t0)*std::exp(-t/t0);
    
    Shortcuts s;
    forceVector[s.sigma + 4] = f;
  }

  void refinementCriteria(exahype2::solvers::aderdg::Solver* solver,
                          std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria) override{
    criteria.push_back(new Refinement::StaticAMR<Shortcuts>);
    criteria.push_back(new Refinement::RefineCubeAroundPosition<Shortcuts>(m_pointSourceLocation,
									   9.0));

  }
};
#endif
