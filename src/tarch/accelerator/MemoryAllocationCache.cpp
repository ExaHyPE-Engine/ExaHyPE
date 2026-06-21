// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryAllocationCache.h"

#include "accelerator.h"
#include "tarch/Assertions.h"

#include <algorithm>
#include <mutex>
#include <new>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

struct DeviceMemoryAllocationCacheKey {
  std::size_t sizeInBytes;
  int         device;

  bool operator==(const DeviceMemoryAllocationCacheKey& other) const {
    return sizeInBytes == other.sizeInBytes and device == other.device;
  }
};

struct DeviceMemoryAllocationCacheKeyHash {
  std::size_t operator()(const DeviceMemoryAllocationCacheKey& key) const {
    std::size_t result = std::hash<std::size_t>{}(key.sizeInBytes);
    result ^= std::hash<int>{}(key.device) + 0x9e3779b9 + (result << 6) + (result >> 2);
    return result;
  }
};

struct CachedAllocationHeader {
  std::size_t             sizeInBytes;
  CachedAllocationHeader* next;
};

struct MemoryAllocationCacheBucket {
  CachedAllocationHeader* first = nullptr;
  std::size_t             count = 0;
};

struct DeviceMemoryAllocationCacheBucket {
  std::vector<void*> pointers;
};

struct DeviceAllocationMetadata {
  std::size_t sizeInBytes;
  int         device;
};

struct MemoryAllocationCacheState {
  std::unordered_map<std::size_t, MemoryAllocationCacheBucket> heapFreeLists;
  std::
    unordered_map<DeviceMemoryAllocationCacheKey, DeviceMemoryAllocationCacheBucket, DeviceMemoryAllocationCacheKeyHash>
      deviceFreeLists;
  std::unordered_map<DeviceMemoryAllocationCacheKey, MemoryAllocationCacheBucket, DeviceMemoryAllocationCacheKeyHash>
              pinnedFreeLists;
  std::size_t cachedBytes = 0;

  MemoryAllocationCacheState();
  ~MemoryAllocationCacheState();
};

static constexpr std::size_t CachedAllocationHeaderSize = ((sizeof(CachedAllocationHeader) + PEANO_WITH_ALIGNMENT - 1) / PEANO_WITH_ALIGNMENT)
                                                          * PEANO_WITH_ALIGNMENT;
static constexpr std::size_t MaxCachedBlockSize       = 8 * 1024 * 1024;
static constexpr std::size_t MaxCachedBytesPerThread  = 64 * 1024 * 1024;
static constexpr std::size_t MaxCachedBlocksPerBucket = 8;

static_assert(
  CachedAllocationHeaderSize % PEANO_WITH_ALIGNMENT == 0,
  "Memory allocation cache metadata has to preserve the public pointer alignment"
);

static std::mutex                                          MemoryAllocationCacheRegistrySemaphore;
static std::vector<MemoryAllocationCacheState*>            MemoryAllocationCacheRegistry;
static thread_local MemoryAllocationCacheState             ThreadLocalMemoryAllocationCache;
static std::shared_mutex                                   DeviceAllocationMetadataSemaphore;
static std::unordered_map<void*, DeviceAllocationMetadata> DeviceAllocationMetadataTable;

static CachedAllocationHeader* getAllocationHeader(void* payloadPointer) {
  return reinterpret_cast<CachedAllocationHeader*>(static_cast<char*>(payloadPointer) - CachedAllocationHeaderSize);
}

static void* getPayloadPointer(CachedAllocationHeader* allocationHeader) {
  return reinterpret_cast<char*>(allocationHeader) + CachedAllocationHeaderSize;
}

static CachedAllocationHeader* allocateUnderlyingMemory(
  std::size_t           totalSizeInBytes,
  tarch::MemoryLocation location,
  int                   device
) {
  if (location == tarch::MemoryLocation::Heap) {
    return static_cast<CachedAllocationHeader*>(tarch::accelerator::allocateHeapMemory(totalSizeInBytes));
  }

  return static_cast<CachedAllocationHeader*>(tarch::accelerator::allocatePinnedMemory(totalSizeInBytes, device));
}

static void freeUnderlyingMemory(CachedAllocationHeader* allocationHeader, tarch::MemoryLocation location, int device) {
  if (location == tarch::MemoryLocation::Heap) {
    tarch::accelerator::freeHeapMemory(allocationHeader);
    return;
  }

  tarch::accelerator::freePinnedMemory(allocationHeader, device);
}

static void bookmarkDeviceAllocation(void* pointer, std::size_t sizeInBytes, int device) {
  std::unique_lock<std::shared_mutex> guard(DeviceAllocationMetadataSemaphore);
  [[maybe_unused]] const bool         inserted = DeviceAllocationMetadataTable
                                           .emplace(pointer, DeviceAllocationMetadata{sizeInBytes, device})
                                           .second;
  assertion(inserted, pointer, sizeInBytes);
}

static DeviceAllocationMetadata getDeviceAllocationMetadata(void* pointer) {
  std::shared_lock<std::shared_mutex> guard(DeviceAllocationMetadataSemaphore);
  auto                                entry = DeviceAllocationMetadataTable.find(pointer);
  assertion(entry != DeviceAllocationMetadataTable.end(), pointer);
  return entry->second;
}

static void removeDeviceAllocationMetadata(void* pointer) {
  std::unique_lock<std::shared_mutex> guard(DeviceAllocationMetadataSemaphore);
  [[maybe_unused]] const std::size_t  erased = DeviceAllocationMetadataTable.erase(pointer);
  assertion(erased == 1, pointer);
}

static void clearMemoryAllocationCacheBucket(
  MemoryAllocationCacheBucket& bucket,
  tarch::MemoryLocation        location,
  int                          device
) {
  while (bucket.first != nullptr) {
    CachedAllocationHeader* next = bucket.first->next;
    freeUnderlyingMemory(bucket.first, location, device);
    bucket.first = next;
  }

  bucket.count = 0;
}

static void clearDeviceMemoryAllocationCacheBucket(DeviceMemoryAllocationCacheBucket& bucket, int device) {
  for (void* pointer : bucket.pointers) {
    tarch::accelerator::freeDeviceMemory(pointer, device);
    removeDeviceAllocationMetadata(pointer);
  }

  bucket.pointers.clear();
}

static void clearMemoryAllocationCacheState(MemoryAllocationCacheState& state) {
  for (auto& [sizeInBytes, bucket] : state.heapFreeLists) {
    clearMemoryAllocationCacheBucket(bucket, tarch::MemoryLocation::Heap, 0);
  }

  for (auto& [key, bucket] : state.deviceFreeLists) {
    clearDeviceMemoryAllocationCacheBucket(bucket, key.device);
  }

  for (auto& [key, bucket] : state.pinnedFreeLists) {
    clearMemoryAllocationCacheBucket(bucket, tarch::MemoryLocation::Pinned, key.device);
  }

  state.heapFreeLists.clear();
  state.deviceFreeLists.clear();
  state.pinnedFreeLists.clear();
  state.cachedBytes = 0;
}

static MemoryAllocationCacheBucket* getMemoryAllocationCacheBucket(
  MemoryAllocationCacheState& state,
  std::size_t                 sizeInBytes,
  tarch::MemoryLocation       location,
  int                         device
) {
  if (location == tarch::MemoryLocation::Heap) {
    auto bucket = state.heapFreeLists.find(sizeInBytes);
    return bucket == state.heapFreeLists.end() ? nullptr : &bucket->second;
  }

  const DeviceMemoryAllocationCacheKey key{sizeInBytes, device};
  auto                                 bucket = state.pinnedFreeLists.find(key);
  return bucket == state.pinnedFreeLists.end() ? nullptr : &bucket->second;
}

static DeviceMemoryAllocationCacheBucket* getDeviceMemoryAllocationCacheBucket(
  MemoryAllocationCacheState& state,
  std::size_t                 sizeInBytes,
  int                         device
) {
  const DeviceMemoryAllocationCacheKey key{sizeInBytes, device};
  auto                                 bucket = state.deviceFreeLists.find(key);
  return bucket == state.deviceFreeLists.end() ? nullptr : &bucket->second;
}

static MemoryAllocationCacheBucket& createMemoryAllocationCacheBucket(
  MemoryAllocationCacheState& state,
  std::size_t                 sizeInBytes,
  tarch::MemoryLocation       location,
  int                         device
) {
  if (location == tarch::MemoryLocation::Heap) {
    return state.heapFreeLists[sizeInBytes];
  }

  const DeviceMemoryAllocationCacheKey key{sizeInBytes, device};
  return state.pinnedFreeLists[key];
}

static DeviceMemoryAllocationCacheBucket& createDeviceMemoryAllocationCacheBucket(
  MemoryAllocationCacheState& state,
  std::size_t                 sizeInBytes,
  int                         device
) {
  const DeviceMemoryAllocationCacheKey key{sizeInBytes, device};
  DeviceMemoryAllocationCacheBucket&   bucket = state.deviceFreeLists[key];
  if (bucket.pointers.capacity() == 0) {
    bucket.pointers.reserve(MaxCachedBlocksPerBucket);
  }
  return bucket;
}

MemoryAllocationCacheState::MemoryAllocationCacheState() {
  heapFreeLists.reserve(32);
  deviceFreeLists.reserve(16);
  pinnedFreeLists.reserve(16);

  std::lock_guard<std::mutex> guard(MemoryAllocationCacheRegistrySemaphore);
  MemoryAllocationCacheRegistry.push_back(this);
}

MemoryAllocationCacheState::~MemoryAllocationCacheState() {
  std::lock_guard<std::mutex> guard(MemoryAllocationCacheRegistrySemaphore);
  clearMemoryAllocationCacheState(*this);

  auto it = std::find(MemoryAllocationCacheRegistry.begin(), MemoryAllocationCacheRegistry.end(), this);
  if (it != MemoryAllocationCacheRegistry.end()) {
    MemoryAllocationCacheRegistry.erase(it);
  }
}

tarch::accelerator::MemoryAllocationCache& tarch::accelerator::MemoryAllocationCache::getInstance() {
  static MemoryAllocationCache instance;
  return instance;
}

void* tarch::accelerator::MemoryAllocationCache::
  allocate(std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(
    location == MemoryLocation::Heap or location == MemoryLocation::Device or location == MemoryLocation::Pinned
  );
  assertion(sizeInBytes > 0, sizeInBytes);

  if (location == MemoryLocation::Device) {
    DeviceMemoryAllocationCacheBucket* bucket = getDeviceMemoryAllocationCacheBucket(
      ThreadLocalMemoryAllocationCache,
      sizeInBytes,
      device
    );

    if (bucket != nullptr and not bucket->pointers.empty()) {
      void* result = bucket->pointers.back();
      bucket->pointers.pop_back();
      ThreadLocalMemoryAllocationCache.cachedBytes -= sizeInBytes;

      return result;
    }

    void* result = tarch::accelerator::allocateDeviceMemory(sizeInBytes, device);
    assertion(result);
    bookmarkDeviceAllocation(result, sizeInBytes, device);
    return result;
  }

  MemoryAllocationCacheBucket* bucket = getMemoryAllocationCacheBucket(
    ThreadLocalMemoryAllocationCache,
    sizeInBytes,
    location,
    device
  );

  if (bucket != nullptr and bucket->first != nullptr) {
    CachedAllocationHeader* allocationHeader = bucket->first;
    bucket->first                            = allocationHeader->next;
    bucket->count--;
    ThreadLocalMemoryAllocationCache.cachedBytes -= sizeInBytes;

    assertion(allocationHeader->sizeInBytes == sizeInBytes, allocationHeader->sizeInBytes, sizeInBytes);
    allocationHeader->next = nullptr;
    return getPayloadPointer(allocationHeader);
  }

  const std::size_t       totalSizeInBytes = CachedAllocationHeaderSize + sizeInBytes;
  CachedAllocationHeader* allocationHeader = allocateUnderlyingMemory(totalSizeInBytes, location, device);
  assertion(allocationHeader);
  ::new (allocationHeader) CachedAllocationHeader{sizeInBytes, nullptr};
  return getPayloadPointer(allocationHeader);
}

void tarch::accelerator::MemoryAllocationCache::free(void* pointer, MemoryLocation location, int device) {
  assertion(
    location == MemoryLocation::Heap or location == MemoryLocation::Device or location == MemoryLocation::Pinned
  );

  if (pointer == nullptr) {
    return;
  }

  if (location == MemoryLocation::Device) {
    const DeviceAllocationMetadata metadata = getDeviceAllocationMetadata(pointer);
    assertion(metadata.device == device, metadata.device, device);

    if (metadata.sizeInBytes > MaxCachedBlockSize
        or ThreadLocalMemoryAllocationCache.cachedBytes + metadata.sizeInBytes > MaxCachedBytesPerThread) {
      tarch::accelerator::freeDeviceMemory(pointer, device);
      removeDeviceAllocationMetadata(pointer);
      return;
    }

    DeviceMemoryAllocationCacheBucket& bucket = createDeviceMemoryAllocationCacheBucket(
      ThreadLocalMemoryAllocationCache,
      metadata.sizeInBytes,
      device
    );
    if (bucket.pointers.size() >= MaxCachedBlocksPerBucket) {
      tarch::accelerator::freeDeviceMemory(pointer, device);
      removeDeviceAllocationMetadata(pointer);
      return;
    }

    bucket.pointers.push_back(pointer);
    ThreadLocalMemoryAllocationCache.cachedBytes += metadata.sizeInBytes;
    return;
  }

  CachedAllocationHeader* allocationHeader = getAllocationHeader(pointer);
  assertion(allocationHeader->sizeInBytes > 0, allocationHeader->sizeInBytes);

  if (allocationHeader->sizeInBytes > MaxCachedBlockSize
      or ThreadLocalMemoryAllocationCache.cachedBytes + allocationHeader->sizeInBytes > MaxCachedBytesPerThread) {
    freeUnderlyingMemory(allocationHeader, location, device);
    return;
  }

  MemoryAllocationCacheBucket& bucket = createMemoryAllocationCacheBucket(
    ThreadLocalMemoryAllocationCache,
    allocationHeader->sizeInBytes,
    location,
    device
  );
  if (bucket.count >= MaxCachedBlocksPerBucket) {
    freeUnderlyingMemory(allocationHeader, location, device);
    return;
  }

  allocationHeader->next = bucket.first;
  bucket.first           = allocationHeader;
  bucket.count++;
  ThreadLocalMemoryAllocationCache.cachedBytes += allocationHeader->sizeInBytes;
}

void tarch::accelerator::MemoryAllocationCache::clear() {
  // clear() is only called during shutdown or test setup/teardown, i.e. when
  // allocator traffic is quiescent. Heap/pinned reuse and device allocation
  // reuse avoid global locks; device free still needs the metadata table. Keep
  // the registry lock while walking it, as thread-local cache destructors also
  // unregister through this mutex. Otherwise a worker thread can destroy a TLS
  // cache after we copy the registry but before we clear the copied pointer.
  std::lock_guard<std::mutex> guard(MemoryAllocationCacheRegistrySemaphore);
  for (MemoryAllocationCacheState* state : MemoryAllocationCacheRegistry) {
    if (state != nullptr) {
      clearMemoryAllocationCacheState(*state);
    }
  }
}
