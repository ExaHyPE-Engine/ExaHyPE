// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryManager.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/Assertions.h"
#include "tarch/memory/memory.h"
#include "tarch/multicore/Lock.h"


tarch::logging::Log tarch::memory::MemoryManager::_log("tarch::memory::MemoryManager");


tarch::memory::MemoryManager& tarch::memory::MemoryManager::getInstance() {
  static tarch::memory::MemoryManager singleton;
  return singleton;
}


void* tarch::memory::MemoryManager::allocate(std::size_t size, MemoryLocation location, int device) {
  tarch::multicore::Lock lock(_semaphore);

  // Is device initialised?
  if (_memoryPages.count(device) == 0) {
    _memoryPages.insert(std::make_pair(device, MemoryPages()));
  }

  // Is a free page left? If not, insert one.
  if (_memoryPages.at(device).freePages[size].empty()) {
    // TODO: We should also check for the memory location here.
    // I.e., not only the size but also the memory location should
    // be used for a lookup.
    _memoryPages.at(device).freePages[size].push_back(std::make_pair(accelerator::allocateRawData(size, location, device), location));
  }

  // Grab fitting page
  void*                resultPtr      = _memoryPages.at(device).freePages.at(size).back().first;
  const MemoryLocation resultLocation = _memoryPages.at(device).freePages.at(size).back().second;
  assertion(resultPtr);
  _memoryPages.at(device).freePages.at(size).pop_back();

  // Memorise page meta data, i.e., size and location
  assertion(_memoryPages.at(device).usedPages.count(resultPtr) == 0);
  _memoryPages.at(device).usedPages.insert(std::make_pair(resultPtr, std::make_pair(size, resultLocation)));

  // Return result
  return resultPtr;
}


void tarch::memory::MemoryManager::free(void* pointer, int device) {
  tarch::multicore::Lock lock(_semaphore);
  assertion(pointer);

  assertion(_memoryPages.count(device) == 1);
  assertion(_memoryPages.at(device).usedPages.count(pointer) == 1);

  const std::size_t    size     = _memoryPages.at(device).usedPages.at(pointer).first;
  const MemoryLocation location = _memoryPages.at(device).usedPages.at(pointer).second;
  _memoryPages.at(device).usedPages.erase(pointer);

  assertion(_memoryPages.at(device).freePages.count(size) == 1);
  _memoryPages.at(device).freePages.at(size).push_back(std::make_pair(pointer, location));
}


tarch::memory::MemoryManager::~MemoryManager() { clear(); }


void tarch::memory::MemoryManager::clear() {
  tarch::multicore::Lock lock(_semaphore);

  for (auto& memoryPage : _memoryPages) {
    int device = memoryPage.first;
    logDebug("clear()", "Free data of device " << device);

    if (not memoryPage.second.usedPages.empty()) {
      logError(
        "clear()",
        "Used pages on device " << device << " are not empty. " << memoryPage.second.usedPages.size() << " memory blocks still in use. Likely a memory leak (missing free())."
      );
    }

    for (auto& p : memoryPage.second.freePages)
      for (auto& pp : p.second) {
        accelerator::freeRawData(pp.first, pp.second, device);
      }
  }

  _memoryPages.clear();
}


int tarch::memory::MemoryManager::getNumberOfFreePages() const {
  int result = 0;
  for (auto& memoryPage : _memoryPages) {
    result += getNumberOfFreePages(memoryPage.first);
  }
  return result;
}


int tarch::memory::MemoryManager::getNumberOfUsedPages() const {
  int result = 0;
  for (auto& memoryPage : _memoryPages) {
    result += getNumberOfUsedPages(memoryPage.first);
  }
  return result;
}


int tarch::memory::MemoryManager::getNumberOfFreePages(int device) const {
  int result = 0;
  for (auto& freePage : _memoryPages.at(device).freePages) {
    result += freePage.second.size();
  }
  return result;
}


int tarch::memory::MemoryManager::getNumberOfUsedPages(int device) const { return _memoryPages.at(device).usedPages.size(); }


int tarch::memory::MemoryManager::getFreeAllocatedMemory(MemoryUsageFormat format) const { return convertMemorySize(getFreeAllocatedMemoryInBytes(), format); }


int tarch::memory::MemoryManager::getUsedAllocatedMemory(MemoryUsageFormat format) const { return convertMemorySize(getUsedAllocatedMemoryInBytes(), format); }


int tarch::memory::MemoryManager::getFreeAllocatedMemory(MemoryUsageFormat format, int device) const { return convertMemorySize(getFreeAllocatedMemoryInBytes(device), format); }


int tarch::memory::MemoryManager::getUsedAllocatedMemory(MemoryUsageFormat format, int device) const { return convertMemorySize(getUsedAllocatedMemoryInBytes(device), format); }


std::size_t tarch::memory::MemoryManager::getFreeAllocatedMemoryInBytes() const {
  std::size_t result = 0;
  for (auto& memoryPage : _memoryPages) {
    result += getFreeAllocatedMemoryInBytes(memoryPage.first);
  }
  return result;
}


std::size_t tarch::memory::MemoryManager::getUsedAllocatedMemoryInBytes() const {
  std::size_t result = 0;
  for (auto& memoryPage : _memoryPages) {
    result += getUsedAllocatedMemoryInBytes(memoryPage.first);
  }
  return result;
}


std::size_t tarch::memory::MemoryManager::getFreeAllocatedMemoryInBytes(int device) const {
  std::size_t result = 0;
  for (auto& freePage : _memoryPages.at(device).freePages) {
    result += freePage.first;
  }
  return result;
}


std::size_t tarch::memory::MemoryManager::getUsedAllocatedMemoryInBytes(int device) const {
  std::size_t result = 0;
  for (auto& usedPage : _memoryPages.at(device).usedPages) {
    result += usedPage.second.first;
  }
  return result;
}


void tarch::memory::MemoryManager::dumpStatistics() {
  std::ostringstream msg;

  msg << MemoryManager::getInstance().getNumberOfUsedPages() + MemoryManager::getInstance().getNumberOfFreePages() << " Pages: ";
  for (auto& memoryPage : MemoryManager::getInstance()._memoryPages) {
    msg << "(#" << memoryPage.first << ":";
    msg << MemoryManager::getInstance().getNumberOfUsedPages(memoryPage.first) << "/" << MemoryManager::getInstance().getNumberOfFreePages(memoryPage.first) << ")";
  }

  msg
    << ", Heap: " << MemoryManager::getInstance().getFreeAllocatedMemory(MemoryUsageFormat::GByte) + MemoryManager::getInstance().getUsedAllocatedMemory(MemoryUsageFormat::GByte)
    << "/" << MemoryManager::getInstance().getFreeAllocatedMemory(MemoryUsageFormat::GByte) << "/" << MemoryManager::getInstance().getUsedAllocatedMemory(MemoryUsageFormat::GByte)
    << " GB (Total/Free/Used)"
    << ", System: " << getTotalMemory(MemoryUsageFormat::GByte) << "/" << getFreeMemory(MemoryUsageFormat::GByte) << "/" << getMemoryUsage(MemoryUsageFormat::GByte)
    << " GB (Total/Free/Used)";

  logInfo("dumpStatistics()", msg.str());
}
