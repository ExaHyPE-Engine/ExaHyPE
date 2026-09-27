// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TaskGraph.h"

#include "tarch/Assertions.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/tasking/Task.h"

namespace {
#if not defined(PEANO_OPENMP_DEPEND_ITERATOR_SUPPORTED)
  std::atomic<bool> loggedConservativeSpawnTaskFallback{false};
#endif

#if not defined(PEANO_OPENMP_TASKWAIT_DEPEND_SUPPORTED)
  std::atomic<bool> loggedConservativeDependencyWaitFallback{false};
#endif

  // Longer dependency lists use vector storage, so this buffer does not limit graph width.
  constexpr int StaticDependencyBufferSize = 16;

  bool containsRaw(char* const* const values, const int numberOfValues, const char* const candidate) {
    for (int index = 0; index < numberOfValues; ++index) {
      if (values[index] == candidate) {
        return true;
      }
    }
    return false;
  }

  class DependencyList final {
  public:
    explicit DependencyList(const std::size_t capacity) :
      _useDynamicStorage(capacity > StaticDependencyBufferSize) {
      if (_useDynamicStorage) {
        _dynamicStorage.reserve(capacity);
      }
    }

    bool contains(const char* const candidate) const { return containsRaw(getData(), getSize(), candidate); }

    void pushBack(char* const token) {
      if (_useDynamicStorage) {
        _dynamicStorage.push_back(token);
      } else {
        assertion(_size < static_cast<int>(_staticStorage.size()));
        _staticStorage[_size++] = token;
      }
    }

    char* const* getData() const { return _useDynamicStorage ? _dynamicStorage.data() : _staticStorage.data(); }

    int getSize() const { return _useDynamicStorage ? static_cast<int>(_dynamicStorage.size()) : _size; }

  private:
    const bool                                    _useDynamicStorage;
    std::array<char*, StaticDependencyBufferSize> _staticStorage{};
    std::vector<char*>                            _dynamicStorage{};
    int                                           _size{0};
  };

  void runAndDelete(tarch::tasking::Task* const task) {
    task->run();
    delete task;
  }
} // namespace

/**
 * Submits a task and transfers ownership to the tasking backend.
 *
 * Null and duplicate tokens are ignored. A token listed as both input and
 * output remains an output dependency because the task produces its value.
 */
void
tarch::tasking::spawnTask(
  tarch::tasking::Task* const     task,
  const std::span<void* const>    inDependencies,
  const std::span<void* const>    outDependencies,
  const tarch::tasking::RunSerial runSerial
) {
  assertion(task != nullptr);

  if (runSerial == tarch::tasking::RunSerial::Yes) {
    runAndDelete(task);
    return;
  }

  DependencyList filteredOutDependencies(outDependencies.size());
  for (void* const dependency : outDependencies) {
    if (dependency == nullptr) {
      continue;
    }

    auto* const token = static_cast<char*>(dependency);
    if (filteredOutDependencies.contains(token)) {
      continue;
    }

    filteredOutDependencies.pushBack(token);
  }

  DependencyList filteredInDependencies(inDependencies.size());
  for (void* const dependency : inDependencies) {
    if (dependency == nullptr) {
      continue;
    }

    auto* const token = static_cast<char*>(dependency);
    if (filteredOutDependencies.contains(token)) {
      continue;
    }
    if (filteredInDependencies.contains(token)) {
      continue;
    }

    filteredInDependencies.pushBack(token);
  }

#if defined(PEANO_OPENMP_DEPEND_ITERATOR_SUPPORTED)
  const int                           numberOfInputDependencies  = filteredInDependencies.getSize();
  const int                           numberOfOutputDependencies = filteredOutDependencies.getSize();
  [[maybe_unused]] char* const* const inTokenData                = filteredInDependencies.getData();
  [[maybe_unused]] char* const* const outTokenData               = filteredOutDependencies.getData();

  // Each task clause names the task pointer in firstprivate. A compiler
  // following OpenMP 4.5 defaults a const variable to the enclosing frame's
  // copy, which the deferred task would read after this call has returned.
  if (numberOfInputDependencies == 0 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task)
    runAndDelete(task);
  } else if (numberOfInputDependencies == 0 and numberOfOutputDependencies == 1) {
#pragma omp task firstprivate(task) depend(out : outTokenData[0][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 0 and numberOfOutputDependencies == 2) {
#pragma omp task firstprivate(task) depend(out : outTokenData[0][0], outTokenData[1][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 1 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) depend(in : inTokenData[0][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 2 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) depend(in : inTokenData[0][0], inTokenData[1][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 3 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) depend(in : inTokenData[0][0], inTokenData[1][0], inTokenData[2][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 4 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) \
  depend(in : inTokenData[0][0], inTokenData[1][0], inTokenData[2][0], inTokenData[3][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 5 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) \
  depend(in : inTokenData[0][0], inTokenData[1][0], inTokenData[2][0], inTokenData[3][0], inTokenData[4][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 6 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) depend( \
    in : inTokenData[0][0], \
      inTokenData[1][0], \
      inTokenData[2][0], \
      inTokenData[3][0], \
      inTokenData[4][0], \
      inTokenData[5][0] \
)
    runAndDelete(task);
  } else if (numberOfInputDependencies == 7 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) depend( \
    in : inTokenData[0][0], \
      inTokenData[1][0], \
      inTokenData[2][0], \
      inTokenData[3][0], \
      inTokenData[4][0], \
      inTokenData[5][0], \
      inTokenData[6][0] \
)
    runAndDelete(task);
  } else if (numberOfInputDependencies == 8 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) depend( \
    in : inTokenData[0][0], \
      inTokenData[1][0], \
      inTokenData[2][0], \
      inTokenData[3][0], \
      inTokenData[4][0], \
      inTokenData[5][0], \
      inTokenData[6][0], \
      inTokenData[7][0] \
)
    runAndDelete(task);
  } else if (numberOfInputDependencies == 9 and numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) depend( \
    in : inTokenData[0][0], \
      inTokenData[1][0], \
      inTokenData[2][0], \
      inTokenData[3][0], \
      inTokenData[4][0], \
      inTokenData[5][0], \
      inTokenData[6][0], \
      inTokenData[7][0], \
      inTokenData[8][0] \
)
    runAndDelete(task);
  } else if (numberOfInputDependencies == 1 and numberOfOutputDependencies == 1) {
#pragma omp task firstprivate(task) depend(in : inTokenData[0][0]) depend(out : outTokenData[0][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 1 and numberOfOutputDependencies == 2) {
#pragma omp task firstprivate(task) depend(in : inTokenData[0][0]) depend(out : outTokenData[0][0], outTokenData[1][0])
    runAndDelete(task);
  } else if (numberOfInputDependencies == 0) {
#pragma omp task firstprivate(task) depend( \
    iterator(outputDependencyIndex = 0 : numberOfOutputDependencies), \
      out : outTokenData[outputDependencyIndex][0] \
)
    runAndDelete(task);
  } else if (numberOfOutputDependencies == 0) {
#pragma omp task firstprivate(task) \
  depend(iterator(inputDependencyIndex = 0 : numberOfInputDependencies), in : inTokenData[inputDependencyIndex][0])
    runAndDelete(task);
  } else {
#pragma omp task firstprivate(task) \
  depend(iterator(inputDependencyIndex = 0 : numberOfInputDependencies), in : inTokenData[inputDependencyIndex][0]) \
  depend( \
      iterator(outputDependencyIndex = 0 : numberOfOutputDependencies), \
        out : outTokenData[outputDependencyIndex][0] \
  )
    runAndDelete(task);
  }
#else
  if (filteredInDependencies.getSize() == 0 and filteredOutDependencies.getSize() == 0) {
#pragma omp task firstprivate(task)
    runAndDelete(task);
  } else {
    if (not loggedConservativeSpawnTaskFallback.exchange(true, std::memory_order_relaxed)) {
      logWarning(
        "OpenMP iterator dependencies are unavailable on this toolchain, so a task with dependencies waits for them "
        "and runs inline. Expect degraded performance."
      );
    }

    for (int dependencyIndex = 0; dependencyIndex < filteredInDependencies.getSize(); dependencyIndex++) {
      tarch::tasking::waitForDependency(filteredInDependencies.getData()[dependencyIndex]);
    }

    for (int dependencyIndex = 0; dependencyIndex < filteredOutDependencies.getSize(); dependencyIndex++) {
      tarch::tasking::waitForDependency(filteredOutDependencies.getData()[dependencyIndex]);
    }

    runAndDelete(task);
  }
#endif
}

/**
 * Waits until the task that produces the dependency token has completed.
 */
void
tarch::tasking::waitForDependency(void* const dependency) {
  assertion(dependency);
  tarch::tasking::waitForDependencies({&dependency, 1});
}

/**
 * Waits until every task producing one of the listed tokens has completed.
 * The wait is a task scheduling point, so the calling thread may execute any
 * spawned task meanwhile, listed or not. Without compiler support for
 * dependent task waits, the function waits for every child task of the caller.
 */
void
tarch::tasking::waitForDependencies(const std::span<void* const> dependencies) {
#if defined(PEANO_OPENMP_TASKWAIT_DEPEND_SUPPORTED)
  DependencyList filteredDependencies(dependencies.size());
  for (void* const dependency : dependencies) {
    if (dependency == nullptr) {
      continue;
    }
    auto* const token = static_cast<char*>(dependency);
    if (not filteredDependencies.contains(token)) {
      filteredDependencies.pushBack(token);
    }
  }

  [[maybe_unused]] const int          numberOfDependencies = filteredDependencies.getSize();
  [[maybe_unused]] char* const* const tokenData            = filteredDependencies.getData();
  if (numberOfDependencies == 0) {
    return;
  }
  // One wait covers the whole list, so the caller registers with the runtime once per call.
#pragma omp taskwait depend(iterator(dependencyIndex = 0 : numberOfDependencies), in : tokenData[dependencyIndex][0])
#else
  if (dependencies.empty()) {
    return;
  }

  if (not loggedConservativeDependencyWaitFallback.exchange(true, std::memory_order_relaxed)) {
    logWarning(
      "OpenMP dependent task waits are unavailable on this toolchain, so a dependency wait waits for every child "
      "task. Expect degraded performance."
    );
  }

#pragma omp taskwait
#endif
}
