#ifndef EXASEIS_CONTEXT_HEADER
#define EXASEIS_CONTEXT_HEADER

#include "../Scenario/Scenario.h"
#include "../Scenario/ScenarioFactory.h"
#include "../Refinement/refinement.h"

template<class Shortcuts, int basisSize>
class Context{
 protected:
  Scenario<Shortcuts,basisSize>* scenario = nullptr;

 public:

  Context(std::string& scenario_string, DomainInformation* info){
    scenario = ScenarioFactory::createScenario<Shortcuts,basisSize>(scenario_string,info);    
  }

  ~Context(){
    delete scenario;
  }

  virtual void initUnknownsPatch(double *luh, 
				 const tarch::la::Vector<Dimensions,double>& center,
				 const tarch::la::Vector<Dimensions,double>& dx,
				 double t,double dt)=0;

  virtual void initUnknownsPointwise(const double *const x,
				     const double t,
				     const double dt, 
				     double* Q){

    Shortcuts s;
    if (tarch::la::equals(t,0.0)) {
      tarch::la::Vector<Dimensions,double> center;
      center[0] = x[0];
      center[1] = x[1];
      center[2] = x[2];
      this->scenario->initUnknownsPointwise(x,center,t,dt,Q);
    }
  }


  virtual void initPointSourceLocation(double pointSourceLocation[][3]){
    this->scenario->initPointSourceLocation(pointSourceLocation);
  }
  
  virtual void setPointSourceVector(const double* const Q,
				    const double* const x,const double t,const double dt,
				    double* forceVector, int n) final {
    this->scenario->setPointSourceVector(Q,x,t,dt,forceVector,n);
  }

  void setRefinementCriteria(exahype2::solvers::aderdg::Solver* solver,
                             std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria){
    this->scenario->refinementCriteria(solver,criteria);
  }

};


#endif
