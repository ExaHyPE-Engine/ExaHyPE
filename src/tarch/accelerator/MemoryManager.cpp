// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryManager.h"

#include "accelerator.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"

tarch::logging::Log tarch::accelerator::MemoryManager::_log("tarch::accelerator::MemoryManager");

tarch::accelerator::MemoryManager& tarch::accelerator::MemoryManager::getInstance() {
  static tarch::accelerator::MemoryManager singleton;
  return singleton;
}

void* tarch::accelerator::MemoryManager::allocate(std::size_t sizeInBytes, MemoryLocation location, int device) {
  {
    tarch::multicore::Lock lock(_semaphore);
    if (_memoryPages.find(device) == _memoryPages.end()) {
      _memoryPages.emplace(device, MemoryPages{});
    }
  }

  MemoryPages&           pages = _memoryPages.at(device);
  tarch::multicore::Lock lock(_semaphore);

  auto& freeList = pages.freePages[sizeInBytes];

  // Is a free page left? If not, insert one.
  if (freeList.empty()) {
    // TODO: We should also check for the memory location here.
    // Not only the size but also the memory location should
    // be used for a lookup.
    freeList.emplace_back(accelerator::allocateRawData(sizeInBytes, location, device), location);
  }

  // Take last free block
  auto [ptr, loc] = freeList.back();
  freeList.pop_back();

  assertion(ptr);
  assertion(pages.usedPages.count(ptr) == 0);

  // Memorise page meta data, i.e. size and location
  pages.usedPages[ptr] = {sizeInBytes, loc};

  return ptr;
}

void tarch::accelerator::MemoryManager::free(void* pointer, int device) {
  assertion(pointer);

  MemoryPages&           pages = _memoryPages.at(device);
  tarch::multicore::Lock lock(_semaphore);

  assertion(pages.usedPages.count(pointer) == 1);
  auto [size, loc] = pages.usedPages[pointer];
  pages.usedPages.erase(pointer);

  assertion(pages.freePages.count(size) == 1);
  pages.freePages[size].emplace_back(pointer, loc);
}

tarch::accelerator::MemoryManager::~MemoryManager() { clearWithoutLock(); }

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

    for (auto& p : memoryPage.second.freePages) {
      for (auto& pp : p.second) {
        accelerator::freeRawData(pp.first, pp.second, device);
      }
    }

    memoryPage.second.freePages.clear();
    memoryPage.second.usedPages.clear();
  }

  _memoryPages.clear();
}

void tarch::accelerator::MemoryManager::clear() {
  tarch::multicore::Lock lock(_semaphore);
  clearWithoutLock();
}

const std::map<int, tarch::accelerator::MemoryManager::MemoryPages>& tarch::accelerator::MemoryManager::getMemoryPages(
) const {
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

int tarch::accelerator::MemoryManager::getFreeAllocatedMemory(MemoryUsageFormat format, int device) const {
  return convertMemorySize(getFreeAllocatedMemoryInBytes(device), format);
}

int tarch::accelerator::MemoryManager::getUsedAllocatedMemory(MemoryUsageFormat format, int device) const {
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
    msg << MemoryManager::getInstance().getNumberOfUsedPages() + MemoryManager::getInstance().getNumberOfFreePages()
        << " Page(s): ";

    for (const auto& memoryPage : MemoryManager::getInstance().getMemoryPages()) {
      msg << "(#" << memoryPage.first << ":";
      msg << MemoryManager::getInstance().getNumberOfUsedPages(memoryPage.first) << "/"
          << MemoryManager::getInstance().getNumberOfFreePages(memoryPage.first) << ")";
    }
    msg << " (used/free)";

    msg
      << ", Managed: " << MemoryManager::getInstance().getUsedAllocatedMemory(MemoryUsageFormat::MByte) << "/"
      << MemoryManager::getInstance().getFreeAllocatedMemory(MemoryUsageFormat::MByte) << "/"
      << MemoryManager::getInstance().getFreeAllocatedMemory(MemoryUsageFormat::MByte)
           + MemoryManager::getInstance().getUsedAllocatedMemory(MemoryUsageFormat::MByte);

    msg << " MB (used/free/total), ";

    // TODO: Use memory locations to report device and host memory.
  }

  msg << "System: " << getMemoryUsage(MemoryUsageFormat::MByte) << "/" << getFreeMemory(MemoryUsageFormat::MByte) << "/"
      << getTotalMemory(MemoryUsageFormat::MByte);
  msg << " MB (used/free/total)";

  return msg.str();
}
