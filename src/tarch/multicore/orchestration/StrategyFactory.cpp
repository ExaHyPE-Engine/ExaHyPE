#include "StrategyFactory.h"

#include "NativeTasks.h"

#include "tarch/logging/Log.h"


tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::parseRealisation( const std::string& realisationString ) {
  if ( realisationString.compare( "native" )==0 ) {
    return new NativeTasks();
  }
  if ( realisationString.compare( "default" )==0 ) {
    return createDefaultStrategy();
  }
  else {
    return nullptr;
  }
}


std::string tarch::multicore::orchestration::getListOfRealisations() {
  return "native,default";
}


tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::createDefaultStrategy() {
  return new NativeTasks();
}

/*

    enum class Realisation {
      MapOntoNativeTasks,
      HoldTasksBackInLocalQueue,
      HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask,
      HoldTasksBackInLocalQueueAndBackfill,
      HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask,
      HoldTasksBackInLocalQueueMergeAndBackfill,
      HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask
    };

    std::string toString( Realisation realisation );


    void configureTaskFusion( int maxNumberOfFusedAssemblies, int maxSizeOfFusedTaskSet );


std::string tarch::multicore::getListOfRealisations() {
  return toString(Realisation::MapOntoNativeTasks)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueue)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueAndBackfill)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueMergeAndBackfill)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask);
}


std::string tarch::multicore::toString( Realisation realisation ) {
  switch (realisation) {
    case Realisation::MapOntoNativeTasks:
      return "map-onto-native-tasks";
    case Realisation::HoldTasksBackInLocalQueue:
      return "hold-tasks-back-in-local-queue";
    case Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask:
      return "hold-tasks-back-in-local-queue-and-eventually-map-onto-native-task";
    case Realisation::HoldTasksBackInLocalQueueAndBackfill:
      return "hold-tasks-back-in-local-queue-and-backfill";
    case Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask:
      return "hold-tasks-back-in-local-queue-and-backfill-and-eventually-map-onto-native-task";
    case Realisation::HoldTasksBackInLocalQueueMergeAndBackfill:
      return "hold-tasks-back-in-local-queue-merge-and-backfill";
    case Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask:
      return "hold-tasks-back-in-local-queue-merge-and-backfill-and-eventually-map-onto-native-task";
  }
  return "<undef>";
}


bool tarch::multicore::parseRealisation( const std::string& realisationString ) {
  if ( realisationString.compare( toString(Realisation::MapOntoNativeTasks) )==0 ) {
    realisation = Realisation::MapOntoNativeTasks;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueue) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueue;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueAndBackfill) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueAndBackfill;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueMergeAndBackfill) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueMergeAndBackfill;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask;
  }
  else {
    tarch::logging::Log _log( "tarch::multicore" );
    logError( "parseRealisation(std::string)", "realisation variant " << realisationString << " not known" );
    return false;
  }
  return true;
}


void tarch::multicore::configureTaskFusion( int maxNumberOfFusedAssemblies, int maxSizeOfFusedTaskSet ) {
  numberOfTasksThatShouldBeFused  = maxSizeOfFusedTaskSet;
  maxNumberOfFusedTasksAssemblies = maxNumberOfFusedAssemblies;
}


*/
