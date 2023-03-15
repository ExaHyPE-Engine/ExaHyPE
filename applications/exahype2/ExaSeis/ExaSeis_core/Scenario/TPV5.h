#ifndef EXASEIS_SCENARIOTPVFIVE_HEADER
#define EXASEIS_SCENARIOTPVFIVE_HEADER
#include "Scenario.h"

template <class Shortcuts, int basisSize>
class TPV5 : public Scenario<Shortcuts, basisSize>{

 public:
  TPV5(DomainInformation* info):Scenario<Shortcuts,basisSize>(info){
  };


  void initUnknownsPointwise(const double* const x,
			     const tarch::la::Vector<Dimensions,double>& center,
			     const double t,
			     const double dt, 
			     double* Q){
    Shortcuts s;
    
#ifdef ISOTROPIC
    Q[s.rho] = 2.67;
    Q[s.cp ] = 6.0;
    Q[s.cs ] = 3.464;
#elif defined ANISOTROPIC
    assertion1(false,"TPV5 Not implemented in the Anisotropic case" );
#else
#error Whole space problem only defined for Isotropic and Anisotropic Scenario
#endif

  }

  void refinementCriteria(exahype2::solvers::aderdg::Solver* solver,
                          std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria) override{
    //    criteria.push_back(new Refinement::StaticAMR<Shortcuts>);
    double left_edge[3];  left_edge[0]  = 19.9; left_edge[1]  = 7.4; left_edge[2]  = 19.9;
    double right_edge[3]; right_edge[0] = 20.1; right_edge[1] = 7.6; right_edge[2] = 20.1;


    double left_edge_box[3];  left_edge_box[0]  = 19.9; left_edge_box[1]  = -1.0; left_edge_box[2]  = -1.0;
    double right_edge_box[3]; right_edge_box[0] = 20.1; right_edge_box[1] = 41.0; right_edge_box[2] = 41.0;    


    criteria.push_back(new Refinement::RefineBetweenPositions<Shortcuts>(solver->getMaximumAdaptiveMeshLevel(),left_edge,right_edge));
    //    criteria.push_back(new Refinement::trackVelocity<Shortcuts>(basisSize,0.25,0.01));
    criteria.push_back(new Refinement::trackVelocity<Shortcuts>(basisSize,0.05,0.0));
    criteria.push_back(new Refinement::RefineFilterCube<Shortcuts>(left_edge_box, right_edge_box, solver->getMaximumAdaptiveMeshLevel()+1, solver->getMaximumAdaptiveMeshLevel()+1));

  }
};
#endif
