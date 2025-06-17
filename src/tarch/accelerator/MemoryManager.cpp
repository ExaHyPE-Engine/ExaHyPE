// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryManager.h"
#include "accelerator.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"

tarch::logging::Log tarch::accelerator::MemoryManager::_log(
  "tarch::accelerator::MemoryManager"
);

tarch::accelerator::MemoryManager& tarch::accelerator::MemoryManager::getInstance() {
  static tarch::accelerator::MemoryManager singleton;
  return singleton;
}

void* tarch::accelerator::MemoryManager::allocate(
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  tarch::multicore::Lock lock(_semaphore);

  // Is device initialised?
  if (_memoryPages.count(device) == 0) {
    _memoryPages.insert(std::make_pair(device, MemoryPages()));
  }

  // Is a free page left? If not, insert one.
  if (_memoryPages.at(device).freePages[sizeInBytes].empty()) {
    // TODO: We should also check for the memory location here.
    // Not only the size but also the memory location should
    // be used for a lookup.
    _memoryPages.at(device).freePages[sizeInBytes].push_back(std::make_pair(
      accelerator::allocateRawData(sizeInBytes, location, device),
      location
    ));
  }

  // Grab fitting page
  void* resultPtr = _memoryPages.at(device).freePages.at(sizeInBytes).back().first;
  const MemoryLocation resultLocation = _memoryPages.at(device).freePages.at(sizeInBytes).back().second;
  assertion(resultPtr);
  _memoryPages.at(device).freePages.at(sizeInBytes).pop_back();

  // Memorise page meta data, i.e. size and location
  assertion(_memoryPages.at(device).usedPages.count(resultPtr) == 0);
  _memoryPages.at(device).usedPages.insert(
    std::make_pair(resultPtr, std::make_pair(sizeInBytes, resultLocation))
  );

  // Return result
  return resultPtr;
}

void tarch::accelerator::MemoryManager::free(void* pointer, int device) {
  tarch::multicore::Lock lock(_semaphore);
  assertion(pointer);

  assertion(_memoryPages.count(device) == 1);
  assertion(_memoryPages.at(device).usedPages.count(pointer) == 1);

  const std::size_t size = _memoryPages.at(device).usedPages.at(pointer).first;
  const MemoryLocation location = _memoryPages.at(device)
                                    .usedPages.at(pointer)
                                    .second;
  _memoryPages.at(device).usedPages.erase(pointer);

  assertion(_memoryPages.at(device).freePages.count(size) == 1);
  _memoryPages.at(device).freePages.at(size).push_back(
    std::make_pair(pointer, location)
  );
}

tarch::accelerator::MemoryManager::~MemoryManager() {
  clearWithoutLock();
}

void tarch::accelerator::MemoryManager::clearWithoutLock() {
  for (auto& memoryPage : _memoryPages) {
    int device = memoryPage.first;
    logDebug("clear()", "Free data of device " << device);

    if (not memoryPage.second.usedPages.empty()) {
      logWarning(
        "clear()",
        "Used pages on device "
          << device << " are not empty. " << memoryPage.second.usedPages.size()
          << " memory blocks still in use. Likely a memory leak (missing free())."
      );
    }

    for (auto& p : memoryPage.second.freePages)
      for (auto& pp : p.second) {
        accelerator::freeRawData(pp.first, pp.second, device);
      }
  }

  _memoryPages.clear();
}

void tarch::accelerator::MemoryManager::clear() {
  tarch::multicore::Lock lock(_semaphore);
  clearWithoutLock();
}

const std::map<int, tarch::accelerator::MemoryManager::MemoryPages>&
tarch::accelerator::MemoryManager::getMemoryPages() const {
  return _memoryPages;
}

int tarch::accelerator::MemoryManager::getNumberOfFreePages() const {
  int result = 0;
  for (auto& memoryPage : _memoryPages) {
    result += getNumberOfFreePages(memoryPage.first);
  }
  return result;
}

int tarch::accelerator::MemoryManager::getNumberOfUsedPages() const {
  int result = 0;
  for (auto& memoryPage : _memoryPages) {
    result += getNumberOfUsedPages(memoryPage.first);
  }
  return result;
}

int tarch::accelerator::MemoryManager::getNumberOfFreePages(int device) const {
  int result = 0;
  for (auto& freePage : _memoryPages.at(device).freePages) {
    result += freePage.second.size();
  }
  return result;
}

int tarch::accelerator::MemoryManager::getNumberOfUsedPages(int device) const {
  return _memoryPages.at(device).usedPages.size();
}

int tarch::accelerator::MemoryManager::getFreeAllocatedMemory(MemoryUsageFormat format) const {
  return convertMemorySize(getFreeAllocatedMemoryInBytes(), format);
}

int tarch::accelerator::MemoryManager::getUsedAllocatedMemory(MemoryUsageFormat format) const {
  return convertMemorySize(getUsedAllocatedMemoryInBytes(), format);
}

int tarch::accelerator::MemoryManager::getFreeAllocatedMemory(
  MemoryUsageFormat format,
  int               device
) const {
  return convertMemorySize(getFreeAllocatedMemoryInBytes(device), format);
}

int tarch::accelerator::MemoryManager::getUsedAllocatedMemory(
  MemoryUsageFormat format,
  int               device
) const {
  return convertMemorySize(getUsedAllocatedMemoryInBytes(device), format);
}

std::size_t tarch::accelerator::MemoryManager::getFreeAllocatedMemoryInBytes() const {
  std::size_t result = 0;
  for (auto& memoryPage : _memoryPages) {
    result += getFreeAllocatedMemoryInBytes(memoryPage.first);
  }
  return result;
}

std::size_t tarch::accelerator::MemoryManager::getUsedAllocatedMemoryInBytes() const {
  std::size_t result = 0;
  for (auto& memoryPage : _memoryPages) {
    result += getUsedAllocatedMemoryInBytes(memoryPage.first);
  }
  return result;
}

std::size_t tarch::accelerator::MemoryManager::getFreeAllocatedMemoryInBytes(int device) const {
  std::size_t result = 0;
  for (auto& freePage : _memoryPages.at(device).freePages) {
    result += freePage.first;
  }
  return result;
}

std::size_t tarch::accelerator::MemoryManager::getUsedAllocatedMemoryInBytes(int device) const {
  std::size_t result = 0;
  for (auto& usedPage : _memoryPages.at(device).usedPages) {
    result += usedPage.second.first;
  }
  return result;
}

std::string tarch::accelerator::MemoryManager::getStatistics() {
  std::ostringstream msg;

  if (not MemoryManager::getInstance().getMemoryPages().empty()) {
    msg << MemoryManager::getInstance().getNumberOfUsedPages() +
           MemoryManager::getInstance().getNumberOfFreePages()
        << " Page(s): ";

    for (const auto& memoryPage : MemoryManager::getInstance().getMemoryPages()) {
      msg << "(#" << memoryPage.first << ":";
      msg << MemoryManager::getInstance().getNumberOfUsedPages(memoryPage.first) << "/"
          << MemoryManager::getInstance().getNumberOfFreePages(memoryPage.first)
          << ")";
    }
    msg << " (used/free)";

    msg << ", Managed: "
      << MemoryManager::getInstance().getUsedAllocatedMemory(MemoryUsageFormat::MByte) << "/"
      << MemoryManager::getInstance().getFreeAllocatedMemory(MemoryUsageFormat::MByte) << "/"
      << MemoryManager::getInstance().getFreeAllocatedMemory(MemoryUsageFormat::MByte) +
         MemoryManager::getInstance().getUsedAllocatedMemory(MemoryUsageFormat::MByte);

    msg << " MB (used/free/total), ";

    // TODO: Use memory locations to report device and host memory.
  }

  msg << "System: "
      << getMemoryUsage(MemoryUsageFormat::MByte) << "/"
      << getFreeMemory(MemoryUsageFormat::MByte) << "/"
      << getTotalMemory(MemoryUsageFormat::MByte);
  msg << " MB (used/free/total)";

  return msg.str();
}
