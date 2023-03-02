#ifndef EXASEIS_SCENARIOLOHONE_HEADER
#define EXASEIS_SCENARIOLOHONE_HEADER
#include "Scenario.h"

template <class Shortcuts, int basisSize>
class Loh1 : public Scenario<Shortcuts, basisSize>{
 private:
    double m_pointSourceLocation[3];

 public:
    
  Loh1(DomainInformation* info):Scenario<Shortcuts,basisSize>(info){
    //place point source in center of topography 2km below surface
      m_pointSourceLocation[0] = 0.0;
      m_pointSourceLocation[1] = 2.0;
      m_pointSourceLocation[2] = 0.0;
  };


  void initUnknownsPointwise(const double* const x,
                             const tarch::la::Vector<Dimensions,double>& center,
			     const double t,
			     const double dt, 
			     double* Q
                             ){
    Shortcuts s;
    
#ifdef ISOTROPIC
    if( x[1] <= (1.0 + 1.0e-6) && center[1] < 1.0) {
      Q[s.rho] = 2.6;
      Q[s.cp ] = 4.0;
      Q[s.cs ] = 2.0;
    }else if(x[1] >= (1.0 - 1.0e-6) && center[1] > 1.0){
      Q[s.rho] = 2.7;
      Q[s.cp ] = 6.0;
      //      Q[s.cs ] = 3.343;
      Q[s.cs ] = 3.464;
    }
#elif defined ANISOTROPIC
    assertion1(false,"Loh1 Not implemented in the Anisotropic case" );
#else
#error Whole space problem only defined for Isotropic and Anisotropic Scenario
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

    assertion2(n == 0, "Only a single pointSource for Loh1",n);


    constexpr double t0 = 0.1;
    constexpr double M0 = 1000.0;
    
    double f = M0*t/(t0*t0)*std::exp(-t/t0);
    
    Shortcuts s;
    forceVector[s.sigma + 4] = f;
  }

  void refinementCriteria(exahype2::solvers::aderdg::Solver* solver,
                          std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria) override{
    criteria.push_back(new Refinement::StaticAMR<Shortcuts>);
    /*    criteria.push_back(new Refinement::CoarseBoundaryLayer<Shortcuts>(
                                                           solver->getMaximumAdaptiveMeshLevel(),
                                                           solver->getCoarsestMeshLevel(),
                                                           solver->getCoarsestMeshSize(),
                                                           1,
                                                           &(solver->getDomainSize()[0]),
                                                           &(solver->getDomainOffset()[0])));*/
    double lower[3];
    double upper[3];

    criteria.push_back(new Refinement::RefineDownToPosition<Shortcuts>(
                                                           basisSize,
                                                           solver->getMaximumAdaptiveMeshLevel(),
                                                           m_pointSourceLocation,
                                                           1));

  }
};
#endif
