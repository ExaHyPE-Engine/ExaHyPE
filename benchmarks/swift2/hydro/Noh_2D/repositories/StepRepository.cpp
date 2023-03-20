#include "StepRepository.h"

#include <sstream>

#include "peano4/parallel/Node.h"


std::string benchmarks::swift2::noh::repositories::StepRepository::toString( Steps value ) {
  switch (value) {
    case Steps::Undef: return "Undef";
    case Steps::CreateGrid: return "CreateGrid";
    case Steps::Init: return "Init";
    case Steps::Sort: return "Sort";
    case Steps::Plot: return "Plot";
    case Steps::StephydroPartFlagBoundaryParticles_sweep0: return "StephydroPartFlagBoundaryParticles_sweep0";
    case Steps::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1: return "StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1";
    case Steps::StephydroPartForceLoopInitial_sweep2: return "StephydroPartForceLoopInitial_sweep2";
    case Steps::StephydroPartKick1_sweep3: return "StephydroPartKick1_sweep3";
    case Steps::StephydroPartKickExtra1_sweep4: return "StephydroPartKickExtra1_sweep4";
    case Steps::StephydroPartDrift_sweep5: return "StephydroPartDrift_sweep5";
    case Steps::StephydroPartPredictHydro_sweep6: return "StephydroPartPredictHydro_sweep6";
    case Steps::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7: return "StephydroPartDensityLoopWithConstantSmoothingLength_sweep7";
    case Steps::StephydroPartForceLoop_sweep8: return "StephydroPartForceLoop_sweep8";
    case Steps::StephydroPartKick2_sweep9: return "StephydroPartKick2_sweep9";
    case Steps::StephydroPartKickExtra2_sweep10: return "StephydroPartKickExtra2_sweep10";
    case Steps::StephydroPartReduceGlobalQuantities_sweep11: return "StephydroPartReduceGlobalQuantities_sweep11";
  
  }
  return "<undef>";
}


int benchmarks::swift2::noh::repositories::StepRepository::toProgramStep( Steps value ) {
  switch (value) {
    case Steps::Undef: return peano4::parallel::Node::UndefProgramStep;
    case Steps::CreateGrid: return 1;
    case Steps::Init: return 2;
    case Steps::Sort: return 3;
    case Steps::Plot: return 4;
    case Steps::StephydroPartFlagBoundaryParticles_sweep0: return 5;
    case Steps::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1: return 6;
    case Steps::StephydroPartForceLoopInitial_sweep2: return 7;
    case Steps::StephydroPartKick1_sweep3: return 8;
    case Steps::StephydroPartKickExtra1_sweep4: return 9;
    case Steps::StephydroPartDrift_sweep5: return 10;
    case Steps::StephydroPartPredictHydro_sweep6: return 11;
    case Steps::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7: return 12;
    case Steps::StephydroPartForceLoop_sweep8: return 13;
    case Steps::StephydroPartKick2_sweep9: return 14;
    case Steps::StephydroPartKickExtra2_sweep10: return 15;
    case Steps::StephydroPartReduceGlobalQuantities_sweep11: return 16;
  
  }
  return peano4::parallel::Node::Terminate;
}


benchmarks::swift2::noh::repositories::StepRepository::Steps benchmarks::swift2::noh::repositories::StepRepository::toStepEnum( int value ) {
  switch (value) {
    case 1: return Steps::CreateGrid;
    case 2: return Steps::Init;
    case 3: return Steps::Sort;
    case 4: return Steps::Plot;
    case 5: return Steps::StephydroPartFlagBoundaryParticles_sweep0;
    case 6: return Steps::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1;
    case 7: return Steps::StephydroPartForceLoopInitial_sweep2;
    case 8: return Steps::StephydroPartKick1_sweep3;
    case 9: return Steps::StephydroPartKickExtra1_sweep4;
    case 10: return Steps::StephydroPartDrift_sweep5;
    case 11: return Steps::StephydroPartPredictHydro_sweep6;
    case 12: return Steps::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7;
    case 13: return Steps::StephydroPartForceLoop_sweep8;
    case 14: return Steps::StephydroPartKick2_sweep9;
    case 15: return Steps::StephydroPartKickExtra2_sweep10;
    case 16: return Steps::StephydroPartReduceGlobalQuantities_sweep11;
  
  }
  return Steps::Undef;
}


