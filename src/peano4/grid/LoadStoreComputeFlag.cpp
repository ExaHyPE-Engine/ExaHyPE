#include "LoadStoreComputeFlag.h"

#include "tarch/Assertions.h"


std::string peano4::grid::toString(peano4::grid::LoadStoreComputeFlag flag) {
  switch (flag) {
    case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream:
      return "load-provide-store";
      break;
    case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard:
      return "load-provide-discard";
      break;
    case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream:
      return "create-dummy-provide-store";
      break;
    case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard:
      return "create-dummy-provide-discard";
      break;
    case LoadStoreComputeFlag::NoData:
      return "no-data";
      break;
  }
  return "<undef>";
}

bool peano4::grid::loadPersistently(LoadStoreComputeFlag flag) {
  switch (flag) {
    case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream:
      return true;
      break;
    case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard:
      return true;
      break;
    case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream:
      return false;
      break;
    case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard:
      return false;
      break;
    case LoadStoreComputeFlag::NoData:
      return false;
      break;
  }
  assertion(false);
  return false;
}

bool peano4::grid::storePersistently(LoadStoreComputeFlag flag) {
  switch (flag) {
    case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream:
      return true;
      break;
    case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard:
      return false;
      break;
    case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream:
      return true;
      break;
    case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard:
      return false;
      break;
    case LoadStoreComputeFlag::NoData:
      return false;
      break;
  }
  assertion(false);
  return false;
}


bool peano4::grid::computeOnData(LoadStoreComputeFlag flag) {
  switch (flag) {
    case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream:
      return true;
      break;
    case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard:
      return true;
      break;
    case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream:
      return true;
      break;
    case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard:
      return true;
      break;
    case LoadStoreComputeFlag::NoData:
      return false;
      break;
  }
  assertion(false);
  return false;
}


peano4::grid::LoadStoreComputeFlag peano4::grid::constructLoadStoreComputeFlag(bool predicateForLoad, bool predicateForStore) {
  return constructLoadStoreComputeFlag(true, predicateForLoad, predicateForStore);
}

peano4::grid::LoadStoreComputeFlag peano4::grid::constructLoadStoreComputeFlag(
  bool predicateToUseData, bool predicateForLoad, bool predicateForStore
) {
  if (not predicateToUseData and not predicateForLoad and not predicateForStore)
    return peano4::grid::LoadStoreComputeFlag::NoData;
  if (not predicateToUseData and predicateForLoad and not predicateForStore)
    return peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard;
  if (not predicateToUseData and not predicateForLoad and predicateForStore)
    return peano4::grid::LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream;
  if (not predicateToUseData and predicateForLoad and predicateForStore)
    assertion(false);
  if (predicateToUseData and not predicateForLoad and not predicateForStore)
    return peano4::grid::LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard;
  if (predicateToUseData and predicateForLoad and not predicateForStore)
    return peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard;
  if (predicateToUseData and not predicateForLoad and predicateForStore)
    return peano4::grid::LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream;
  if (predicateToUseData and predicateForLoad and predicateForStore)
    return peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream;

  assertion(false);
  return peano4::grid::LoadStoreComputeFlag::NoData;
}
