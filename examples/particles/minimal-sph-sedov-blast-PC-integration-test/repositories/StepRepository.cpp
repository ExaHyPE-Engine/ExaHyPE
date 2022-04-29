#include "StepRepository.h"

#include <sstream>

#include "peano4/parallel/Node.h"


std::string examples::particles::repositories::StepRepository::toString( Steps value ) {
  switch (value) {
    case Steps::Undef: return "Undef";
    case Steps::CreateGrid: return "CreateGrid";
    case Steps::Init: return "Init";
    case Steps::DensityNonSym: return "DensityNonSym";
    case Steps::NewtonRaphsonSmoothingLength: return "NewtonRaphsonSmoothingLength";
    case Steps::HydroPrepareForce: return "HydroPrepareForce";
    case Steps::ForceNonSym: return "ForceNonSym";
    case Steps::DriftPredictHalfStep: return "DriftPredictHalfStep";
    case Steps::Kick1HalfTimeStep: return "Kick1HalfTimeStep";
    case Steps::Kick2FullTimeStep: return "Kick2FullTimeStep";
    case Steps::HydroKickExtra2HalfTimeStep: return "HydroKickExtra2HalfTimeStep";
    case Steps::DriftFullHalfStep: return "DriftFullHalfStep";
    case Steps::ResetHasConverged: return "ResetHasConverged";
    case Steps::InitTimeStep: return "InitTimeStep";
    case Steps::ImposeBC: return "ImposeBC";
    case Steps::EndTimeStep: return "EndTimeStep";
    case Steps::RearrangeAtStart: return "RearrangeAtStart";
    case Steps::Plot: return "Plot";
  
  }
  return "<undef>";
}


int examples::particles::repositories::StepRepository::toProgramStep( Steps value ) {
  switch (value) {
    case Steps::Undef: return peano4::parallel::Node::UndefProgramStep;
    case Steps::CreateGrid: return 1;
    case Steps::Init: return 2;
    case Steps::DensityNonSym: return 3;
    case Steps::NewtonRaphsonSmoothingLength: return 4;
    case Steps::HydroPrepareForce: return 5;
    case Steps::ForceNonSym: return 6;
    case Steps::DriftPredictHalfStep: return 7;
    case Steps::Kick1HalfTimeStep: return 8;
    case Steps::Kick2FullTimeStep: return 9;
    case Steps::HydroKickExtra2HalfTimeStep: return 10;
    case Steps::DriftFullHalfStep: return 11;
    case Steps::ResetHasConverged: return 12;
    case Steps::InitTimeStep: return 13;
    case Steps::ImposeBC: return 14;
    case Steps::EndTimeStep: return 15;
    case Steps::RearrangeAtStart: return 16;
    case Steps::Plot: return 17;
  
  }
  return peano4::parallel::Node::Terminate;
}


examples::particles::repositories::StepRepository::Steps examples::particles::repositories::StepRepository::toStepEnum( int value ) {
  switch (value) {
    case 1: return Steps::CreateGrid;
    case 2: return Steps::Init;
    case 3: return Steps::DensityNonSym;
    case 4: return Steps::NewtonRaphsonSmoothingLength;
    case 5: return Steps::HydroPrepareForce;
    case 6: return Steps::ForceNonSym;
    case 7: return Steps::DriftPredictHalfStep;
    case 8: return Steps::Kick1HalfTimeStep;
    case 9: return Steps::Kick2FullTimeStep;
    case 10: return Steps::HydroKickExtra2HalfTimeStep;
    case 11: return Steps::DriftFullHalfStep;
    case 12: return Steps::ResetHasConverged;
    case 13: return Steps::InitTimeStep;
    case 14: return Steps::ImposeBC;
    case 15: return Steps::EndTimeStep;
    case 16: return Steps::RearrangeAtStart;
    case 17: return Steps::Plot;
  
  }
  return Steps::Undef;
}


