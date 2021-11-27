#include "StrategyFactory.h"
#include "Hardcoded.h"

#include "tarch/multicore/Tasks.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"


tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::parseRealisation( const std::string& realisationString ) {
  if ( realisationString==std::string( "bsp" ) ) {
    return Hardcoded::createBSP();
  }
  else if ( realisationString==std::string( "backfill" ) ) {
    return Hardcoded::createBackfill();
  }
  else if ( realisationString.find( "fuse-immediately-" )==0 ) {
    std::string argument = realisationString.substr( std::string("fuse-immediately-").size() );
    int maxFusion = atoi( argument.c_str() );
    assertion(maxFusion>=1);
    return Hardcoded::createFuseAll(maxFusion, true, tarch::multicore::Task::Host);
  }
  else if ( realisationString.find( "fuse-late-" )==0 ) {
    std::string argument = realisationString.substr( std::string("fuse-late-").size() );
    int maxFusion = atoi( argument.c_str() );
    assertion(maxFusion>=1);
    return Hardcoded::createFuseAll(maxFusion, false, tarch::multicore::Task::Host);
  }
  else if ( realisationString==std::string( "native" ) ) {
    return Hardcoded::createNative();
  }
  else if ( realisationString==std::string( "default" ) ) {
    return createDefaultStrategy();
  }
  else {
    return nullptr;
  }
}


std::string tarch::multicore::orchestration::getListOfRealisations() {
  return "bsp,backfill,fuse-immediately-[1,2,3...],fuse-late-[1,2,3...],native,default";
}


tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::createDefaultStrategy() {
  return Hardcoded::createNative();
}
