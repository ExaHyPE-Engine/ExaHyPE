#include "StrategyFactory.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/Tasks.h"

tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::parseRealisation(
  const std::string& realisationString
) {
  if (realisationString == std::string("bsp")) {
    return Hardcoded::createBSP();
  } else if (realisationString == std::string("backfill")) {
    return Hardcoded::createBackfill();
  } else if (realisationString.find("fuse-immediately-") == 0) {
    std::string argument  = realisationString.substr(std::string("fuse-immediately-").size());
    int         maxFusion = atoi(argument.c_str());
    assertion(maxFusion >= 1);
    return Hardcoded::createFuseAll(maxFusion, true, false, tarch::multicore::Task::Host);
  } else if (realisationString.find("fuse-or-process-immediately-") == 0) {
    std::string argument  = realisationString.substr(std::string("fuse-immediately-").size());
    int         maxFusion = atoi(argument.c_str());
    assertion(maxFusion >= 1);
    return Hardcoded::createFuseAll(maxFusion, true, true, tarch::multicore::Task::Host);
  } else if (realisationString.find("fuse-late-") == 0) {
    std::string argument  = realisationString.substr(std::string("fuse-late-").size());
    int         maxFusion = atoi(argument.c_str());
    assertion(maxFusion >= 1);
    return Hardcoded::createFuseAll(maxFusion, false, false, tarch::multicore::Task::Host);
  } else if (realisationString.find("all-on-gpu-") == 0) {
    std::string argument = realisationString.substr(std::string("all-on-gpu-").size());
    int         device   = atoi(argument.c_str());
    assertion(device >= 0);
    return new AllOnGPU(device);
  } else if (realisationString == std::string("deploy-round-robin-")) {
    std::string argument  = realisationString.substr(std::string("deploy-round-robin-").size());
    int         minFusion = atoi(argument.c_str());
    assertion(minFusion >= 0);
    return new BackfillAndDeployRoundRobin(minFusion);
  } else if (realisationString == std::string("native")) {
    return Hardcoded::createNative();
  } else if (realisationString == std::string("default")) {
    return createDefaultStrategy();
  } else {
    return nullptr;
  }
}

std::string tarch::multicore::orchestration::getListOfRealisations() {
  return "bsp,backfill,fuse-immediately-[1,2,3...],fuse-or-process-immediately-[1,2,3...],fuse-late-[1,2,3...],native,"
         "all-on-gpu-[0,1,...],genetic-optimisation,deploy-round-robin-[1,2,3,...],default  (default is "
         "deploy-round-robin with min fused task size 16)";
}

tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::createDefaultStrategy() {
  return Hardcoded::createNative();
}
