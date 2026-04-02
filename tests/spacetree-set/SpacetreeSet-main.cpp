// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeSet-main.h"

#include "Constants.h"
#include "CreateGridObserver.h"
#include "EmptyTraversalObserver.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/peano4.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/mpi/Rank.h"
#include "tarch/timing/Watch.h"
#include "toolbox/loadbalancing/LoadBalancer.h"

#include <iomanip>
#include <limits>

using namespace tests::peano4::parallel;

namespace {
  tarch::logging::Log _log("");

  constexpr int CreateGridWithRefinement    = 0;
  constexpr int CreateGridWithoutRefinement = 1;
  constexpr int EmptyTraversal              = 2;

  static toolbox::loadbalancing::LoadBalancer loadBalancer;

  static tarch::timing::Watch watch("::", "", false);

  void setupEnvironment(int argc, char** argv) {
#if DIMENSIONS == 2
    const auto DomainOffset = {0.0, 0.0};
    const auto DomainSize   = {1.0, 1.0};
#else
    const auto DomainOffset = {0.0, 0.0, 0.0};
    const auto DomainSize   = {1.0, 1.0, 1.0};
#endif

    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetDebug,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "tarch",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetTrace,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "tarch",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetInfo,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "tarch",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetInfo,
        0,
        "tarch",
        tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );

    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetDebug,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "peano4",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetTrace,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "peano4",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetInfo,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "peano4",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetInfo,
        0,
        "peano4",
        tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );

    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetDebug,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "toolbox",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetTrace,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "toolbox",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetInfo,
        tarch::logging::LogFilter::FilterListEntry::AnyRank,
        "toolbox",
        tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );
    tarch::logging::LogFilter::getInstance().addFilterListEntry(
      tarch::logging::LogFilter::FilterListEntry(
        tarch::logging::LogFilter::FilterListEntry::TargetInfo,
        0,
        "toolbox",
        tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
        tarch::logging::LogFilter::FilterListEntry::AlwaysOn
      )
    );

    peano4::init(&argc, &argv, DomainOffset, DomainSize, 0);
    peano4::writeCopyrightMessage();
  }

  void shutdownEnvironment() { peano4::shutdown(); }

  void logGridTraversalStatistics(const char* step, double dt) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      logInfo(
        "logGridTraversalStatistics()",
        step
          << " traversal took " << std::fixed << std::setprecision(4) << dt << "s"
          << ": Leaves=" << loadBalancer.getStatistics().getGlobalNumberOfInnerUnrefinedCells()
          << ", Trees=" << loadBalancer.getStatistics().getGlobalNumberOfTrees()
      );
    }
  }

  void runNextAlgorithmicStep() {
    const int currentStep = peano4::parallel::Node::getInstance().getCurrentProgramStep();

    switch (currentStep) {
    case CreateGridWithRefinement: {
      tarch::logging::LogFilter::getInstance().switchProgramPhase("CreateGridWithRefinement");
      CreateGridObserver createGridObserverWithRefinement(true);
      watch.start();
      peano4::parallel::SpacetreeSet::getInstance().traverse(createGridObserverWithRefinement);
      watch.stop();
      loadBalancer.update();
      logGridTraversalStatistics("CreateGrid", watch.getCalendarTime());
      break;
    }
    case CreateGridWithoutRefinement: {
      tarch::logging::LogFilter::getInstance().switchProgramPhase("CreateGridWithoutRefinement");
      CreateGridObserver createGridObserverWithoutRefinement(false);
      watch.start();
      peano4::parallel::SpacetreeSet::getInstance().traverse(createGridObserverWithoutRefinement);
      watch.stop();
      loadBalancer.update();
      logGridTraversalStatistics("CreateGrid", watch.getCalendarTime());
      break;
    }
    case EmptyTraversal: {
      tarch::logging::LogFilter::getInstance().switchProgramPhase("EmptyTraversal");
      EmptyTraversalObserver emptyTraversalObserver;
      watch.start();
      peano4::parallel::SpacetreeSet::getInstance().traverse(emptyTraversalObserver);
      watch.stop();
      loadBalancer.update();
      logGridTraversalStatistics("Empty", watch.getCalendarTime());
      break;
    }
    default:
      assertion(false);
      break;
    }
  }

  bool selectNextAlgorithmicStep() {
    static bool   initialised                  = false;
    static bool   gridConstructed              = false;
    static double currentMinH                  = std::numeric_limits<double>::max();
    static int    currentGlobalNumberOfTrees   = 0;
    static bool   finishConditionSeenLastSweep = false;

    if (not initialised) {
      peano4::parallel::Node::getInstance().setNextProgramStep(CreateGridWithRefinement);
      initialised = true;
      return true;
    }

    // If we have already executed the empty traversal, we are done.
    if (gridConstructed and peano4::parallel::Node::getInstance().getCurrentProgramStep() == EmptyTraversal) {
      return false;
    }

    // Once the grid is constructed, run exactly one final empty traversal and then terminate.
    if (gridConstructed) {
      peano4::parallel::Node::getInstance().setNextProgramStep(EmptyTraversal);
      return true;
    }

    // Grid construction phase.
    assertion(loadBalancer.getStatistics().getGlobalNumberOfTrees() > 0);

    const int currentProgramStep = peano4::parallel::Node::getInstance().getCurrentProgramStep();

    if (tarch::la::smaller(loadBalancer.getStatistics().getGlobalMinH(), currentMinH)) {
      currentMinH                  = loadBalancer.getStatistics().getGlobalMinH();
      finishConditionSeenLastSweep = false;
      peano4::parallel::Node::getInstance().setNextProgramStep(CreateGridWithoutRefinement);
      return true;
    }

    if (loadBalancer.getStatistics().getGlobalNumberOfTrees() > currentGlobalNumberOfTrees) {
      currentGlobalNumberOfTrees   = loadBalancer.getStatistics().getGlobalNumberOfTrees();
      finishConditionSeenLastSweep = false;
      peano4::parallel::Node::getInstance().setNextProgramStep(CreateGridWithoutRefinement);
      return true;
    }

    const bool hasReachedGridConstructionConvergence = tarch::la::smallerEquals(currentMinH, MinH)
                                                       and loadBalancer.isStagnating()
                                                       and currentProgramStep == CreateGridWithRefinement;

    if (hasReachedGridConstructionConvergence) {
      const bool hasConfirmedStationarySweep = loadBalancer.getStatistics().getGlobalNumberOfStationarySweeps() > 0;
      if (finishConditionSeenLastSweep and hasConfirmedStationarySweep) {
        gridConstructed = true;
        peano4::parallel::Node::getInstance().setNextProgramStep(EmptyTraversal);
        return true;
      }

      finishConditionSeenLastSweep = true;
      peano4::parallel::Node::getInstance().setNextProgramStep(CreateGridWithRefinement);
      return true;
    }

    finishConditionSeenLastSweep = false;

    peano4::parallel::Node::getInstance().setNextProgramStep(CreateGridWithRefinement);
    return true;
  }

  int runGlobalMaster() {
    while (selectNextAlgorithmicStep()) {
      peano4::parallel::Node::getInstance().continueToRun();
      runNextAlgorithmicStep();
    }

    // Broadcast termination to all other ranks.
    peano4::parallel::Node::getInstance().setNextProgramStep(peano4::parallel::Node::Terminate);
    peano4::parallel::Node::getInstance().continueToRun();

    return EXIT_SUCCESS;
  }

  void runWorker() {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      runNextAlgorithmicStep();
    }
  }
} // namespace

int main(int argc, char** argv) {
  setupEnvironment(argc, argv);

  int result = EXIT_SUCCESS;
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    result = runGlobalMaster();
  } else {
    runWorker();
  }

  shutdownEnvironment();
  return result;
}
