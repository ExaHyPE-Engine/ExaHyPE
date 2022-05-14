#include "amr-parallel-main.h"
#include "Constants.h"

#include "tarch/logging/Log.h"

#include "tarch/logging/LogFilter.h"
#include "tarch/logging/CommandLineLogger.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "observers/DataRepository.h"

#include "observers/DataRepository.h"
#include "observers/PlotSolution.h"
#include "observers/PlotMaterialParameter.h"
#include "observers/CreateGrid.h"
#include "observers/SetupScenario.h"
#include "observers/ComputeResidualWithGeometricOperators.h"
#include "observers/ComputeGlobalResidualAndError.h"
#include "observers/JacobiUpdate.h"
#include "observers/FusedSolverSteps.h"


tarch::logging::Log _log("");


class ProgramRun {
  private:
  public:
    /**
     * Init spacetree set
     */
    ProgramRun() {
      peano4::parallel::SpacetreeSet::getInstance().init(
        examples::jacobi::actions::SetupScenario::getDomainOffset(),
        examples::jacobi::actions::SetupScenario::getDomainSize()
      );
    }


    void step() {
      #if PeanoDebug>0
      tarch::logging::CommandLineLogger::getInstance().closeOutputStreamAndReopenNewOne();
      #endif

      int stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
      logInfo( "ProgramRun::step()", "run step " << stepIdentifier );

      // @todo Hier waere ein Case schoen
      switch (stepIdentifier) {
        case 0:
          {
            examples::jacobi::observers::CreateGrid  observer;
            peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
          }
          break;
        case 1:
          {
            examples::jacobi::observers::SetupScenario  observer;
            peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
          }
          break;
        case 2:
          {
            examples::jacobi::observers::PlotMaterialParameter  observer;
            peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
          }
          break;
        case 10:
          {
            if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
              if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
                int cellsPerRank = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() / tarch::mpi::Rank::getInstance().getNumberOfRanks();
                logInfo( "ProgramRun::step()", "should host " << cellsPerRank << " cells per rank (total number of cells: " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() << ")" );
                for (int rank=1; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
                  if ( not peano4::parallel::SpacetreeSet::getInstance().split(0,cellsPerRank,rank)) {
                    logWarning( "runParallel(...)", "failed to assign rank " << rank << " " << cellsPerRank << " cell(s)" );
                  }
                }
              }

              examples::jacobi::observers::SetupScenario  observer;
              peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
            }
          }
          break;
        case 11:
          {
            if (tarch::multicore::Core::getInstance().getNumberOfThreads()>1) {
              int cellsPerCore = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() / tarch::multicore::Core::getInstance().getNumberOfThreads();

              logInfo( "ProgramRun::step()", "should host " << cellsPerCore << " cells per core (total count " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() << " cells) - split up in one rush" );

              if ( not peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().empty() ) {
                int localTree = *( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin() );
                logInfo( "ProgramRun::step()", "fork local tree " << localTree << " " << (tarch::multicore::Core::getInstance().getNumberOfThreads()-1) << " times" );
                for (int thread=1; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
                  if ( not peano4::parallel::SpacetreeSet::getInstance().split(localTree,cellsPerCore,tarch::mpi::Rank::getInstance().getRank())) {
                    logWarning( "runParallel(...)", "failed to assign thread " << thread << " " << cellsPerCore << " cell(s)" );
                  }
                }
              }
            }
            examples::jacobi::observers::CreateGrid  observer;
            peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
          }
          break;
        case 12:
          {
            if (tarch::multicore::Core::getInstance().getNumberOfThreads()>1) {
              static int cellsPerCore = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() / tarch::multicore::Core::getInstance().getNumberOfThreads();

              logInfo( "ProgramRun::step()", "should host " << cellsPerCore << " cells per core (total count " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() << " cells) - split up incrementally" );
 
              if ( not peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().empty() ) {
                int localTree = *( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin() );
                logInfo( "ProgramRun::step()", "fork local tree " << localTree );
                if ( not peano4::parallel::SpacetreeSet::getInstance().split(localTree,cellsPerCore,tarch::mpi::Rank::getInstance().getRank())) {
                  logWarning( "runParallel(...)", "failed to assign new thread " << cellsPerCore << " cell(s)" );
                }
                examples::jacobi::observers::SetupScenario  observer;
                peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
                //peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
              }
            }
          }
          break;
        case 50:
          {
            examples::jacobi::observers::FusedSolverSteps  observer;
            peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
          }
          break;
        case 99:
          {
            examples::jacobi::observers::PlotSolution  observer;
            peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
          }
          break;
        default:
          assertionMsg(false, "step unknown");
      }
    }


    enum class GridDecompositionStrategy {
      BuildUpGridCompletelyBeforeWeDecompose,
      SplitUpMPIASAPAndThenSplitUpIntoAllThreadsInOneRush,
      SplitUpMPIASAPAndThenSplitUpIntoThreadsOneByOne
    };


    #if defined(Parallel)
    void runGlobalMaster(int iterations, GridDecompositionStrategy gridDecompositionStrategy = GridDecompositionStrategy::SplitUpMPIASAPAndThenSplitUpIntoThreadsOneByOne ) {
    #else
    void runGlobalMaster(int iterations, GridDecompositionStrategy gridDecompositionStrategy = GridDecompositionStrategy::SplitUpMPIASAPAndThenSplitUpIntoAllThreadsInOneRush ) {
    #endif
      // Construct grid until we are told that it hasn't changed for
      // more than two iterations.
      // ===========================================================
      #if defined(Parallel)
      bool hasSplitRanks        = false;
      #else
      bool hasSplitRanks        = true;
      #endif
      
      #if defined(SharedMemoryParallelisation)
      bool hasSplitSharedMemory = false;
      #else
      bool hasSplitSharedMemory = false;
      #endif

//      const int numberOfLocalUnrefinedCellsBeforeFirstMPISplit = tarch::mpi::Rank::getInstance().getNumberOfRanks();
      const int numberOfLocalUnrefinedCellsBeforeFirstMPISplit = ThreePowerD * ThreePowerD * ThreePowerD;

      do {
        if (
          gridDecompositionStrategy != GridDecompositionStrategy::BuildUpGridCompletelyBeforeWeDecompose
          and
          not hasSplitRanks
          and
          peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() > numberOfLocalUnrefinedCellsBeforeFirstMPISplit
        ) {
          peano4::parallel::Node::getInstance().setNextProgramStep(10); // split mpi
          step();
          peano4::parallel::Node::getInstance().setNextProgramStep(0); // construct mesh
          step();
          hasSplitRanks = true;
        }
        else if (
          peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() > numberOfLocalUnrefinedCellsBeforeFirstMPISplit
          and
          not hasSplitRanks
        ) {
          hasSplitRanks = true;
        }        
        else if (
          gridDecompositionStrategy != GridDecompositionStrategy::BuildUpGridCompletelyBeforeWeDecompose
          and
          hasSplitRanks
          and
          not hasSplitSharedMemory
          and
          peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() > tarch::multicore::Core::getInstance().getNumberOfThreads()
        ) {
          if (gridDecompositionStrategy == GridDecompositionStrategy::SplitUpMPIASAPAndThenSplitUpIntoAllThreadsInOneRush ) {
            peano4::parallel::Node::getInstance().setNextProgramStep(11); // split tbb
            step();
          }
          else {
            for (int thread=1; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
              peano4::parallel::Node::getInstance().setNextProgramStep(12); // split tbb
              step();
            }
          }

          hasSplitSharedMemory = true;
        }
        else if (
          not hasSplitSharedMemory 
          and 
          hasSplitRanks
          and
          peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() > tarch::multicore::Core::getInstance().getNumberOfThreads()
        ) {
          hasSplitSharedMemory = true;
        }

        peano4::parallel::Node::getInstance().setNextProgramStep(0); // construct mesh
        step();

        if (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()>5) {
          logWarning( "runGlobalMaster()", "grid likely too small to keep all cores/ranks busy" );
          hasSplitRanks = true;
          hasSplitSharedMemory = true;
        }
      }
      while (
        peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()<2
        or
        not hasSplitRanks
        or
        not hasSplitSharedMemory
      );

      peano4::parallel::Node::getInstance().setNextProgramStep(1); // setup scenario
      step();

      if ( gridDecompositionStrategy == GridDecompositionStrategy::BuildUpGridCompletelyBeforeWeDecompose ) {
        peano4::parallel::Node::getInstance().setNextProgramStep(11); // split tbb
        step();
      }

      #if !defined(Parallel)
      peano4::parallel::Node::getInstance().setNextProgramStep(2); // dump parameters
      step();
      #endif

      for (int i=0; i<iterations; i++) {
        peano4::parallel::Node::getInstance().setNextProgramStep(50); // solve problem
        step();
      }

      #if !defined(Parallel)
      peano4::parallel::Node::getInstance().setNextProgramStep(99); // plot solution
      step();
      #endif
      
      logInfo( "runGlobalMaster(int)", "terminated successfully" );
    }
};



int main(int argc, char** argv) {
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;

  peano4::initParallelEnvironment(&argc,&argv);
  peano4::fillLookupTables();

  examples::jacobi::observers::DataRepository::initDatatypes();

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, 
    tarch::logging::LogFilter::FilterListEntry::AnyRank, 
    "peano4", 
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, 
    tarch::logging::LogFilter::FilterListEntry::AnyRank, 
    "peano4", 
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, 
    tarch::logging::LogFilter::FilterListEntry::AnyRank, 
    "peano4", 
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, 
    tarch::logging::LogFilter::FilterListEntry::AnyRank, 
    "tarch", 
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, 
    tarch::logging::LogFilter::FilterListEntry::AnyRank, 
    "tarch", 
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, 
    tarch::logging::LogFilter::FilterListEntry::AnyRank, 
    "tarch", 
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
 

  //tarch::logging::CommandLineLogger::getInstance().setOutputFile( "trace.log" );
  tarch::logging::CommandLineLogger::getInstance().setLogFormat(
    " ", true, false, true, false, false, true, "trace.log"
  );


  #if PeanoDebug>=1
  tarch::mpi::Rank::getInstance().setDeadlockTimeOut(0);
  #endif


  int coreCount = 1;
  if (argc==2) {
    #ifdef SharedMemoryParallelisation
    coreCount = std::atoi( argv[1] );
    logInfo( "main()", "use " << coreCount << " core(s)" );
    #else
    logWarning( "main()", "you should not set the thread count if you translate without multicore support" );
    #endif
    tarch::multicore::Core::getInstance().configure(coreCount);
  }
  else {
    #ifdef SharedMemoryParallelisation
    logError( "main()", "expect core count as additional argument" );
    exit(-1);
    #endif
  }


  ProgramRun programRun;
  
  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    #if PeanoDebug>=1
    const int MaxIterations = 2;
    #else  
    const int MaxIterations = 20;
    #endif
    programRun.runGlobalMaster(MaxIterations);
  }
  else {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      programRun.step();
    }
  }

  examples::jacobi::observers::DataRepository::shutdownDatatypes();

  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
