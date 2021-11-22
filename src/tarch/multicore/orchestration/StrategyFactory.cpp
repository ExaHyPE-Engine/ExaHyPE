#include "StrategyFactory.h"

#include "BSP.h"
#include "FuseAll.h"
#include "NativeTasks.h"

#include "tarch/logging/Log.h"


tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::parseRealisation( const std::string& realisationString ) {
  if ( realisationString.compare( "bsp" )==0 ) {
    return new BSP();
  }
  if ( realisationString.compare( "fuse-all" )==0 ) {
    return new FuseAll();
  }
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
  return "bsp,fuse-all,native,default";
}


tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::createDefaultStrategy() {
  return new NativeTasks();
}
