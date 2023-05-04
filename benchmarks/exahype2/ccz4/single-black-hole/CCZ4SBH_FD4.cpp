#include "CCZ4SBH_FD4.h"

#include "exahype2/RefinementControl.h"


tarch::logging::Log   benchmarks::exahype2::ccz4::CCZ4SBH_FD4::_log( "benchmarks::exahype2::ccz4::CCZ4SBH_FD4" );

#ifdef IncludeTwoPunctures
TP::TwoPunctures* _tp = new TP::TwoPunctures();
#endif

#ifdef IncludeTwoPunctures
//pre-process, solve the puncture equations
void benchmarks::exahype2::ccz4::CCZ4SBH_FD4::prepare(TP::TwoPunctures* tp){
    //first we set the parameter. TODO:find a way to read parameter from python script
    //int swi=0;//0--single black hole, 1--BBH hoc, 2--BBH rotation, 3--GW150914
    
    tp->par_b=1.0;
    tp->center_offset[0]=-1.0; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
    tp->target_M_plus=1.0;//adm mass
    tp->par_P_plus[0]=0.0; tp->par_P_plus[1]=0.0; tp->par_P_plus[2]=0.0;//linear momentum
    tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
    tp->target_M_minus=0.0;//adm mass
    tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=0.0; tp->par_P_minus[2]=0.0;//linear momentum
    tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=0.0; //spin    
    tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
    tp->TP_epsilon=1e-6;
    
    tp->PrintParameters();

    //then solve the equation
  tp->Run();
}
#endif

::exahype2::RefinementCommand benchmarks::exahype2::ccz4::CCZ4SBH_FD4::refinementCriterion(
  const double * __restrict__ Q, // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  meshCellH,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", volumeX, meshCellH, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}

benchmarks::exahype2::ccz4::CCZ4SBH_FD4::CCZ4SBH_FD4(){
  #ifdef IncludeTwoPunctures
    prepare(_tp);//we solve the puncture equation here.
    //exit(0);
  #endif
}

void benchmarks::exahype2::ccz4::CCZ4SBH_FD4::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  meshCellCentre,
  const tarch::la::Vector<Dimensions,double>&  meshCellH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", meshCellCentre, meshCellH, gridIsConstructred );

  applications::exahype2::ccz4::ApplyTwoPunctures(Q, meshCellCentre, 0, _tp, not gridIsConstructred); 

  logTraceOut( "initialCondition(...)" );
}

