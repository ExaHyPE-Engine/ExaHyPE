#include "DeviceInfo.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

#if defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)
#include "cuda/DeviceInfo.h"
#endif

#if defined(GPUOffloadingSYCL)
#include "sycl/DeviceInfo.h"
#endif

#if defined(GPUOffloadingOMP)
#include "omp/DeviceInfo.h"
#endif

#if defined(GPUOffloadingCPP)
#include "cpp/DeviceInfo.h"
#endif

#include <cassert>
// #else if defined(WITH_SYCL_API) // Possible TODO to differentiate SYCL_API and Sycl

namespace tarch::accelerator {
  std::vector<std::vector<std::string>> queryDeviceInformation() {
    std::vector<std::vector<std::string>> devInfo;
#if defined(GPUOffloadingCPP)
    tarch::accelerator::cpp::queryDeviceInformation(devInfo);
#elif defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)
    tarch::accelerator::cuda::queryDeviceInformation(devInfo);
#elif defined(GPUOffloadingSYCL)
    tarch::accelerator::sycl::queryDeviceInformation(devInfo);
#elif defined(GPUOffloadingOMP)
    tarch::accelerator::omp::queryDeviceInformation(devInfo);
#else
    return devInfo;
#endif
    tarch::accelerator::runBenchmarks(devInfo);
    return devInfo;
  }

  void printDeviceInformation() {
    auto propertymatrix = queryDeviceInformation();
#if defined(GPUOffloadingCPP)
    // Can't get any information with cpp GPU offloading rn
    tarch::accelerator::cpp::printDeviceInformation(propertymatrix);
#elif defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)
    tarch::accelerator::cuda::printDeviceInformation(propertymatrix);
#elif defined(GPUOffloadingSYCL)
    tarch::accelerator::sycl::printDeviceInformation(propertymatrix);
#elif defined(GPUOffloadingOMP)
    // An empty table, with OpenMP we will only know how many GPUs we have
    tarch::accelerator::omp::printDeviceInformation(propertymatrix);
#else
    // Print noinformation since we do not have an accelerator
    // The following logInfo not really needed
    // logInfo("printDeviceInformation()", "No GPU Accelerator Backend is enabled. The code will run on CPUs.");
#endif
  }

  size_t getPropertyOffset(const char name[], const std::vector<std::string>& identifiers) {
    auto it = std::find(identifiers.begin(), identifiers.end(), std::string(name));
    if (it == identifiers.end()) {
      throw std::runtime_error("Property input name not a part of the identifiers list!");
    }
    return std::distance(identifiers.begin(), it);
  }

  std::string parseMatrix(
    const std::vector<std::vector<std::string>>& propertyMatrix,
    int                                          numberOfDevices,
    const std::vector<std::string>&              identifiers
  ) {
    unsigned int propertyCount = identifiers.size();

    std::stringstream ss;

    // This size is the padding we need to add the porpety-name column
    unsigned int maxPropertyLengthWithPadding
      = std::max_element(
          identifiers.begin(),
          identifiers.end(),
          [](const std::string& lhs, const std::string& rhs) { return lhs.size() < rhs.size(); }
        )->size()
        + 1;

    // Get the string entry of a columng with the highest length
    std::vector<unsigned int> columnMaxStringSizes(identifiers.size(), 0);
    for (unsigned int row = 0; row < identifiers.size(); row++) {
      std::vector<unsigned int> col_sizes;
      for (unsigned int col = 0; col < numberOfDevices; col++) {
        col_sizes.push_back(propertyMatrix[row][col].size());
      }

      // Device property is also a part of the column
      col_sizes.push_back(identifiers[row].size());

      unsigned int maxLength    = *std::max_element(col_sizes.begin(), col_sizes.end());
      columnMaxStringSizes[row] = maxLength;
    }

    unsigned int maxIdLength = std::max(std::to_string(numberOfDevices).size(), std::string("DeviceId").size());

    // Iterate the matrix and write per row, also while accessing the padding we need to add
    std::string  propIdPad(maxIdLength + 1 - std::string("DeviceId").size(), ' ');
    unsigned int lineLength = 0;

    ss << propIdPad << "DeviceId |";
    lineLength += propIdPad.size() + 10;
    for (unsigned int x = 0; x < identifiers.size(); x++) {
      std::string propPadding(columnMaxStringSizes[x] + 1 - identifiers[x].size(), ' ');
      ss << propPadding << identifiers[x] << " |";
      lineLength += propPadding.size() + identifiers[x].size() + 2;
    }
    ss.seekp(-1, ss.cur);
    lineLength -= 1;
    ss << "\n";

    std::string dash(lineLength, '-');
    ss << dash << "\n";

    for (unsigned int x = 0; x < numberOfDevices; x++) {
#if defined(GPUOffloadingCPP)
      std::string idPad(maxIdLength, ' ');
      ss << idPad << "X"
         << " |";
#else
      std::string idPad(maxIdLength + 1 - std::to_string(x).size(), ' ');
      ss << idPad << x << " |";
#endif
      for (unsigned int y = 0; y < identifiers.size(); y++) //(const auto& row : propertyMatrix)
      {
        unsigned int paddingLength = columnMaxStringSizes[y] + 1 - propertyMatrix[y][x].size();
        std::string  s(paddingLength, ' ');
        // ss << s << el << "  ";
        ss << s << propertyMatrix[y][x] << " |";
      }
      ss.seekp(-1, ss.cur);
      ss << "\n";
    }
    return ss.str();
  }

  void offloadCapabilityTest() {
    std::vector<bool> testResults;
#if defined(GPUOffloadingCUDA)
    testResults = tarch::accelerator::cuda::testKernelLaunch();
#elif defined(GPUOffloadingSYCL)
    testResults = tarch::accelerator::sycl::testKernelLaunch();
#elif defined(GPUOffloadingOMP)
    testResults = tarch::accelerator::omp::testKernelLaunch();
#elif defined(GPUOffloadingCPP)
    testResults = tarch::accelerator::cpp::testKernelLaunch();
    if (testResults.size() == 0) {
      logInfo(
        "offloadCapabiltiyTest()",
        "Iso C++ GPU-Parallelism with NVHPC compilers can't check kernel launch tests currently"
      );
      return;
    } else if (testResults.size() == 1 && !testResults[0]) {
      throw std::runtime_error(
        "None of the found GPUs could successfully launch GPU Kernels (Check for wrong compile configuration)"
      );
    }
#else
    // Print noinformation since we do not have an accelerator
    logInfo("offloadCapabiltiyTest()", "No GPU Accelerator Backend is enabled. The code will run on CPUs.");
    return;
#endif

    unsigned int offloadCapableGPUCount = 0;
    std::string  failMessage;
    for (unsigned int i = 0; i < testResults.size(); i++) {
      if (!testResults[i]) {
        // Using std::format would be nice but 10.06.2023 CUDA does support gcc upto 12, no std::format supprot
        failMessage += "GPU Device " + std::to_string(i) + " could not successfully launch a GPU kernel\n";
      } else {
        offloadCapableGPUCount += 1;
      }
    }

    if (testResults.size() != 0) {
      if (offloadCapableGPUCount < testResults.size()) {
        if (offloadCapableGPUCount == 0) {
          throw std::runtime_error("None of the found GPUs could successfully launch GPU Kernels");
        } else {
          throw std::runtime_error(failMessage);
        }
      } else {
        logInfo("offloadCapabiltiyTest()", "Kernel launch test successful");
      }
    } else {
      throw std::runtime_error(
        "Peano 4 was configured to run on accelerators, but no offloading capable GPU were found."
      );
    }
  }

  void runBenchmarks(std::vector<std::vector<std::string>>& propertyMatrix) {
    std::vector<BenchmarkResultInNanoSeconds> results;
#if defined(GPUOffloadingCUDA)
    results = tarch::accelerator::cuda::runBenchmarks();
    parseBenchmarkResults(results, propertyMatrix);
#elif defined(GPUOffloadingSYCL)
    results     = tarch::accelerator::sycl::runBenchmarks();
    parseBenchmarkResults(results, propertyMatrix);
#elif defined(GPUOffloadingOMP)
    results     = tarch::accelerator::omp::runBenchmarks();
    parseBenchmarkResults(results, propertyMatrix);
#elif defined(GPUOffloadingCPP)
    results = tarch::accelerator::cpp::runBenchmarks();
    parseBenchmarkResults(results, propertyMatrix);
#endif
  }

  void parseBenchmarkResults(
    std::vector<BenchmarkResultInNanoSeconds>& benchmarkResults,
    std::vector<std::vector<std::string>>&     benchmarkResultsStringRepresentations
  ) {

    // A * 1e-9 / (B * 1e-9) = A / B
    // Therefore Byte / NanoSecond = GB / Second
    for (uint i = 0; i < benchmarkResults.size(); i++) {
      std::stringstream ss;
      auto&             benchmarkResult = benchmarkResults[i];
      double            cpuToGpu        = benchmarkResult.allocatedDataSize * benchmarkResult.repeats
                        * (1.0 / static_cast<double>(benchmarkResult.CPUtoGPUCopy));
      double gpuToCpu = benchmarkResult.allocatedDataSize * benchmarkResult.repeats
                        * (1.0 / static_cast<double>(benchmarkResult.GPUtoCPUCopy));
      double averageStream = (cpuToGpu + gpuToCpu) / 2.0;
      ss << averageStream << " Gb/s";
      benchmarkResultsStringRepresentations[tarch::accelerator::getPropertyOffset("Streaming Bandwidth")][i] = ss.str();
      ss.str(std::string());
      ss << benchmarkResult.allocatedDataSize * benchmarkResult.repeats
              * (1.0 / static_cast<double>(benchmarkResult.GPUtoGPUCopy))
         << " Gb/s";
      benchmarkResultsStringRepresentations[tarch::accelerator::getPropertyOffset("GPU-GPU Copy")][i] = ss.str();
      ss.str(std::string());
    }
  }
} // namespace tarch::accelerator
