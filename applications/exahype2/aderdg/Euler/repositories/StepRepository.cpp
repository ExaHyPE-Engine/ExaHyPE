#include "StepRepository.h"

#include <sstream>

#include "peano4/parallel/Node.h"


std::string examples::exahype2::euler2D::repositories::StepRepository::toString( Steps value ) {
  switch (value) {
    case Steps::Undef: return "Undef";
    case Steps::CreateGrid: return "CreateGrid";
    case Steps::InitGrid: return "InitGrid";
    case Steps::CreateGridButPostponeRefinement: return "CreateGridButPostponeRefinement";
    case Steps::CreateGridAndConvergeLoadBalancing: return "CreateGridAndConvergeLoadBalancing";
    case Steps::PlotSolution: return "PlotSolution";
    case Steps::TimeStep: return "TimeStep";
  
  }
  return "<undef>";
}


int examples::exahype2::euler2D::repositories::StepRepository::toProgramStep( Steps value ) {
  switch (value) {
    case Steps::Undef: return peano4::parallel::Node::UndefProgramStep;
    case Steps::CreateGrid: return 1;
    case Steps::InitGrid: return 2;
    case Steps::CreateGridButPostponeRefinement: return 3;
    case Steps::CreateGridAndConvergeLoadBalancing: return 4;
    case Steps::PlotSolution: return 5;
    case Steps::TimeStep: return 6;
  
  }
  return peano4::parallel::Node::Terminate;
}


examples::exahype2::euler2D::repositories::StepRepository::Steps examples::exahype2::euler2D::repositories::StepRepository::toStepEnum( int value ) {
  switch (value) {
    case 1: return Steps::CreateGrid;
    case 2: return Steps::InitGrid;
    case 3: return Steps::CreateGridButPostponeRefinement;
    case 4: return Steps::CreateGridAndConvergeLoadBalancing;
    case 5: return Steps::PlotSolution;
    case 6: return Steps::TimeStep;
  
  }
  return Steps::Undef;
}


