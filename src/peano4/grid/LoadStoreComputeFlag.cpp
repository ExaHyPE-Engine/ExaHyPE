// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadStoreComputeFlag.h"

#include <stdexcept>

std::string peano4::grid::toString(peano4::grid::LoadStoreComputeFlag flag) {
  switch (flag) {
  case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream:
    return "LoadProvideStore";
  case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard:
    return "LoadProvideDiscard";
  case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream:
    return "CreateDummyProvideStore";
  case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard:
    return "CreateDummyProvideDiscard";
  case LoadStoreComputeFlag::NoData:
    return "NoData";
  }
  return "<undef>";
}

bool peano4::grid::loadPersistently(LoadStoreComputeFlag flag) {
  switch (flag) {
  case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream:
    return true;
  case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard:
    return true;
  case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream:
    return false;
  case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard:
    return false;
  case LoadStoreComputeFlag::NoData:
    return false;
  }
  return false;
}

bool peano4::grid::storePersistently(LoadStoreComputeFlag flag) {
  switch (flag) {
  case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream:
    return true;
  case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard:
    return false;
  case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream:
    return true;
  case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard:
    return false;
  case LoadStoreComputeFlag::NoData:
    return false;
  }
  return false;
}

bool peano4::grid::computeOnData(LoadStoreComputeFlag flag) {
  switch (flag) {
  case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream:
    return true;
  case LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard:
    return true;
  case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream:
    return true;
  case LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard:
    return true;
  case LoadStoreComputeFlag::NoData:
    return false;
  }
  return false;
}

peano4::grid::LoadStoreComputeFlag peano4::grid::constructLoadStoreComputeFlag(
  bool predicateForLoad,
  bool predicateForStore
) {
  return constructLoadStoreComputeFlag(true, predicateForLoad, predicateForStore);
}

peano4::grid::LoadStoreComputeFlag peano4::grid::
  constructLoadStoreComputeFlag(bool predicateToUseData, bool predicateForLoad, bool predicateForStore) {
  if (not predicateToUseData and not predicateForLoad and not predicateForStore) {
    return peano4::grid::LoadStoreComputeFlag::NoData;
  }
  if (not predicateToUseData and predicateForLoad and not predicateForStore) {
    return peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard;
  }
  if (not predicateToUseData and not predicateForLoad and predicateForStore) {
    return peano4::grid::LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream;
  }
  if (not predicateToUseData and predicateForLoad and predicateForStore) {
    throw std::logic_error("Invalid combination: load && store but !use (copy-through not supported)!");
  }
  if (predicateToUseData and not predicateForLoad and not predicateForStore) {
    return peano4::grid::LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_Discard;
  }
  if (predicateToUseData and predicateForLoad and not predicateForStore) {
    return peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_Discard;
  }
  if (predicateToUseData and not predicateForLoad and predicateForStore) {
    return peano4::grid::LoadStoreComputeFlag::CreateDummy_ProvideToCalculations_StoreToOutputStream;
  }
  if (predicateToUseData and predicateForLoad and predicateForStore) {
    return peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream;
  }

  return peano4::grid::LoadStoreComputeFlag::NoData;
}
