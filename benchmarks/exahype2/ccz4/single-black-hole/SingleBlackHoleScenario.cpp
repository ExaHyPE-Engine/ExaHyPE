#include "SingleBlackHoleScenario.h"
#include "Constants.h"

namespace {
  tarch::logging::Log _log( "benchmarks::exahype2::ccz4" );
}

TP::TwoPunctures* benchmarks::exahype2::ccz4::twoPunctures = nullptr;

void benchmarks::exahype2::ccz4::prepareTwoPunctures() {
  if (twoPunctures == nullptr) {
    twoPunctures = new TP::TwoPunctures();

    // first we set the parameter. TODO:find a way to read parameter from python script
    // int swi=0;//0--single black hole, 1--BBH hoc, 2--BBH rotation, 3--GW150914

    twoPunctures->par_b             = 1.0;
    twoPunctures->center_offset[0]  = -1.0;
    twoPunctures->center_offset[1]  = 0.0;
    twoPunctures->center_offset[2]  = 0.0;
    twoPunctures->target_M_plus     = 1.0; // adm mass
    twoPunctures->par_P_plus[0]     = 0.0;
    twoPunctures->par_P_plus[1]     = 0.0;
    twoPunctures->par_P_plus[2]     = 0.0; // linear momentum
    twoPunctures->par_S_plus[0]     = 0.0;
    twoPunctures->par_S_plus[1]     = 0.0;
    twoPunctures->par_S_plus[2]     = 0.0; // spin
    twoPunctures->target_M_minus    = 0.0; // adm mass
    twoPunctures->par_P_minus[0]    = 0.0;
    twoPunctures->par_P_minus[1]    = 0.0;
    twoPunctures->par_P_minus[2]    = 0.0; // linear momentum
    twoPunctures->par_S_minus[0]    = 0.0;
    twoPunctures->par_S_minus[1]    = 0.0;
    twoPunctures->par_S_minus[2]    = 0.0;          // spin
    twoPunctures->grid_setup_method = "evaluation"; // evaluation or Taylor expansion
    twoPunctures->TP_epsilon        = 1e-6;

    twoPunctures->PrintParameters();

    // then solve the equation
    twoPunctures->Run();
  }
}


::exahype2::RefinementCommand benchmarks::exahype2::ccz4::artificialRefinement(
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h
) {
  return tarch::la::norm2(x)<tarch::la::max(h)
       ? ::exahype2::RefinementCommand::Refine
       : ::exahype2::RefinementCommand::Keep;
}


::exahype2::RefinementCommand benchmarks::exahype2::ccz4::refinementCriterion(
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       maxH,
  double                                       minH
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  std::cout << "max=" << maxH << ", min=" << minH << ", h=" << h << std::endl;
  assertion2( maxH>=minH, maxH, minH );

  int refinementLevels = 0;
  double currentFinestLevelRelativeToCoarsest = maxH;
  while (currentFinestLevelRelativeToCoarsest > minH) {
    refinementLevels++;
    currentFinestLevelRelativeToCoarsest /= 3.0;
  }

  if ( h(0)>maxH ) {
    result = ::exahype2::RefinementCommand::Refine;
  }
  else if (refinementLevels==0) {
  }
  else if (refinementLevels==1) {
    constexpr int NumberOfRefinementLayers = 2;
    double Radius[NumberOfRefinementLayers] = {3.0, 1.5};
    double MaxH[NumberOfRefinementLayers]   = {0.15,0.04};
    result = ::exahype2::RefinementCommand::Keep;
    for (int i=0; i<NumberOfRefinementLayers; i++) {
      if (tarch::la::norm2(x)<Radius[i] and tarch::la::max(h)>MaxH[i]) {
       result=::exahype2::RefinementCommand::Refine;
      }
    }
  }
  else if (refinementLevels==2) {
    constexpr int NumberOfRefinementLayers = 3;
    double Radius[NumberOfRefinementLayers] = {8.0, 5.0, 2.0};
    double MaxH[NumberOfRefinementLayers]   = {0.25, 0.08, 0.03};
    result = ::exahype2::RefinementCommand::Keep;
    for (int i=0; i<NumberOfRefinementLayers; i++) {
      if (tarch::la::norm2(x)<Radius[i] and tarch::la::max(h)>MaxH[i]) {
       result=::exahype2::RefinementCommand::Refine;
      }
    }
  }
  else if (refinementLevels==3) {
    constexpr int NumberOfRefinementLayers = 5;
    double Radius[NumberOfRefinementLayers] = {60.0, 30.0, 15.0, 8.0, 4.0};
    double MaxH[NumberOfRefinementLayers]   = {2.1, 0.7, 0.25, 0.08, 0.03};
    result = ::exahype2::RefinementCommand::Keep;
    for (int i=0; i<NumberOfRefinementLayers; i++) {
      if (tarch::la::norm2(x)<Radius[i] and tarch::la::max(h)>MaxH[i]) {
       result=::exahype2::RefinementCommand::Refine;
      }
    }
  }
  else {
    logWarning( "refinementCriterion(...)", "no support for " << refinementLevels << " different refinement levels" );
  }

/*
  else {
    result = tarch::la::norm2(x) - std::sqrt(Dimensions)*h(0) < BlackHoleFVRegion
       ? ::exahype2::RefinementCommand::Refine
       : ::exahype2::RefinementCommand::Keep;
  }
*/
  return result;
}
