// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceInfo.h"

#include <algorithm>
#include <cassert>
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


std::vector<std::vector<std::string>> tarch::accelerator::queryDeviceInformation() {
  std::vector<std::vector<std::string>> devInfo;
#if defined(GPUOffloadingCPP)
  tarch::accelerator::cpp::queryDeviceInformation(devInfo);
  tarch::accelerator::runBenchmarks(devInfo);
#endif
#if defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)
  tarch::accelerator::cuda::queryDeviceInformation(devInfo);
  tarch::accelerator::runBenchmarks(devInfo);
#endif
#if defined(GPUOffloadingSYCL)
  tarch::accelerator::sycl::queryDeviceInformation(devInfo);
  tarch::accelerator::runBenchmarks(devInfo);
#endif
#if defined(GPUOffloadingOMP)
  tarch::accelerator::omp::queryDeviceInformation(devInfo);
  tarch::accelerator::runBenchmarks(devInfo);
#endif
  return devInfo;
}


void tarch::accelerator::printDeviceInformation() {
  auto propertyMatrix = queryDeviceInformation();
#if defined(GPUOffloadingCPP)
  tarch::accelerator::cpp::printDeviceInformation(propertyMatrix);
#endif
#if defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)
  tarch::accelerator::cuda::printDeviceInformation(propertyMatrix);
#endif
#if defined(GPUOffloadingSYCL)
  tarch::accelerator::sycl::printDeviceInformation(propertyMatrix);
#endif
#if defined(GPUOffloadingOMP)
  tarch::accelerator::omp::printDeviceInformation(propertyMatrix);
#endif
#endif
}


std::size_t tarch::accelerator::getPropertyOffset(const char name[], const std::vector<std::string>& identifiers) {
  auto it = std::find(identifiers.begin(), identifiers.end(), std::string(name));
  if (it == identifiers.end()) {
    throw std::runtime_error("Property input name is not a part of the identifiers list!");
  }
  return std::distance(identifiers.begin(), it);
}


std::string tarch::accelerator::parseMatrix(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices, const std::vector<std::string>& identifiers) {
  const unsigned int propertyCount = identifiers.size();

  std::stringstream ss;

  // This size is the padding we need to add the porpety-name column
  unsigned int maxPropertyLengthWithPadding
    = std::max_element(identifiers.begin(), identifiers.end(), [](const std::string& lhs, const std::string& rhs) { return lhs.size() < rhs.size(); })->size() + 1;

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
#endif
#if defined(GPUOffloadingSYCL)
  testResults = tarch::accelerator::sycl::testKernelLaunch();
#endif
#if defined(GPUOffloadingOMP)
  testResults = tarch::accelerator::omp::testKernelLaunch();
#endif
#if defined(GPUOffloadingCPP)
  testResults = tarch::accelerator::cpp::testKernelLaunch();
#endif

  if (testResults.size() == 0) {
    return;
  } else if (testResults.size() == 1 && !testResults[0]) {
    throw std::runtime_error("None of the found devices could successfully launch GPU kernels. Check for wrong build configuration.");
  }

#ifndef GPUOffloadingOff
  unsigned int offloadCapableGPUCount = 0;
  std::string  failMessage;

  // If one of the GPUs fail to launch but we have multiple, then we inform which ones
  // failed by adding them to the message in the first loop.
  // If all of the available GPUs fail, we throw a runtime error that everything failed.
  for (unsigned int i = 0; i < testResults.size(); i++) {
    if (!testResults[i]) {
      failMessage += "Device " + std::to_string(i) + " could not successfully launch a GPU kernel\n";
    } else {
      offloadCapableGPUCount += 1;
    }
  }

  if (testResults.size() != 0) {
    if (offloadCapableGPUCount < testResults.size()) {
      if (offloadCapableGPUCount == 0) {
        throw std::runtime_error("None of the found devices could successfully launch GPU kernels");
      } else {
        throw std::runtime_error(failMessage);
      }
    } else {
      logInfo("offloadCapabiltiyTest()", "Kernel launch test successful");
    }
  } else {
    throw std::runtime_error("Peano was configured to run on accelerators, but no offload capable devices were found.");
  }
#else
  logInfo("offloadCapabiltiyTest()", "Peano was not configured to run on accelerators. The code will run on CPUs.");
#endif
}


void runBenchmarks(std::vector<std::vector<std::string>>& propertyMatrix) {
  std::vector<BenchmarkResultInNanoSeconds> results;
#if defined(GPUOffloadingCUDA)
  results = tarch::accelerator::cuda::runBenchmarks();
  parseBenchmarkResults(results, propertyMatrix);
#endif
#if defined(GPUOffloadingSYCL)
  results = tarch::accelerator::sycl::runBenchmarks();
  parseBenchmarkResults(results, propertyMatrix);
#endif
#if defined(GPUOffloadingOMP)
  results = tarch::accelerator::omp::runBenchmarks();
  parseBenchmarkResults(results, propertyMatrix);
#endif
#if defined(GPUOffloadingCPP)
  results = tarch::accelerator::cpp::runBenchmarks();
  parseBenchmarkResults(results, propertyMatrix);
#endif
}


void parseBenchmarkResults(std::vector<BenchmarkResultInNanoSeconds>& benchmarkResults, std::vector<std::vector<std::string>>& benchmarkResultsStringRepresentations) {
  // A * 1e-9 / (B * 1e-9) = A / B but, MB -> B 1024*1024 and for time it is 1e9.
  for (uint i = 0; i < benchmarkResults.size(); i++) {
    // Parse the results, bring nanoseconds to something humans would like more and add the units.
    std::stringstream ss;
    auto&             benchmarkResult = benchmarkResults[i];
    constexpr double  byteToGB        = (1.0 / (1000.0 * 1000.0 * 1000.0));
    ss << (static_cast<double>(benchmarkResult.allocatedDataSize) * static_cast<double>(benchmarkResult.repeats) * byteToGB
          ) * (1.0 / (static_cast<double>(benchmarkResult.GPUtoGPUCopy) * 1e-9))
       << " Gb/s";
    benchmarkResultsStringRepresentations[tarch::accelerator::getPropertyOffset("Bandwidth")][i] = ss.str();
    ss.str(std::string());
  }
}
