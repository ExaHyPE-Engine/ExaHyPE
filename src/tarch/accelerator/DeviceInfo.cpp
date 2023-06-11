#include "DeviceInfo.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <iostream>

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

// #else if defined(WITH_SYCL_API) // Possible TODO to differentiate SYCL_API and Sycl

namespace tarch::accelerator::info
{
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
#if defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)
    return tarch::accelerator::info::cuda::queryDeviceInformation();
#elif defined(GPUOffloadingSYCL)
    return tarch::accelerator::info::sycl::queryDeviceInformation();
#elif defined(GPUOffloadingOMP)
    return tarch::accelerator::info::omp::queryDeviceInformation();
#elif defined(GPUOffloadingCPP)
    return tarch::accelerator::info::cpp::queryDeviceInformation();
#else
    std::vector<std::vector<std::string>> propertyMatrix;
    return propertyMatrix;
#endif
  }

  void printDeviceInformation()
  {
#if defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)
    return tarch::accelerator::info::cuda::printDeviceInformation();
#elif defined(GPUOffloadingSYCL)
    return tarch::accelerator::info::sycl::printDeviceInformation();
#elif defined(GPUOffloadingOMP)
    // An empty table, with OpenMP we will only know how many GPUs we have
    return tarch::accelerator::info::omp::printDeviceInformation();
#elif defined(GPUOffloadingCPP)
    // Can't get any information with cpp GPU offloading rn
    return tarch::accelerator::info::cpp::printDeviceInformation();
#else
    // Print noinformation since we do not have an accelerator
    std::cout << "No GPU Accelerator Backend is enabled. The code will run on CPUs." << std::endl;
#endif
  }


  size_t getPropertyOffset(const char name[], const std::vector<std::string>& identifiers)
  {
    auto it = std::find(identifiers.begin(), identifiers.end(), std::string(name));
    if (it == identifiers.end())
    {
      throw std::runtime_error("Property input name not a part of the identifiers list!");
    }
    return std::distance(identifiers.begin(), it);
  }

  std::string parseMatrix(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices, const std::vector<std::string>& identifiers)
  {
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
    for (unsigned int row = 0; row < identifiers.size(); row++)
    {
      std::vector<unsigned int> col_sizes;
      for (unsigned int col = 0; col < numberOfDevices; col++)
      {
        col_sizes.push_back(propertyMatrix[row][col].size());
      }

      // Device property is also a part of the column
      col_sizes.push_back(identifiers[row].size());

      unsigned int maxLength    = *std::max_element(col_sizes.begin(), col_sizes.end());
      columnMaxStringSizes[row] = maxLength;
    }

    unsigned int maxIdLength = std::max(std::to_string(numberOfDevices).size(), std::string("DeviceId").size());

    // Iterate the matrix and write per row, also while accessing the padding we need to add
    std::string propIdPad(maxIdLength + 1 - std::string("DeviceId").size(), ' ');
    unsigned int lineLength = 0;
    
    ss << propIdPad << "DeviceId |";
    lineLength += propIdPad.size() + 10;
    for (unsigned int x = 0; x < identifiers.size(); x++){
      std::string propPadding(columnMaxStringSizes[x] + 1 - identifiers[x].size(), ' ');
      ss << propPadding << identifiers[x] << " |";
      lineLength += propPadding.size() + identifiers[x].size() + 2;
    }
    ss.seekp(-1, ss.cur);
    lineLength -= 1;
    ss << "\n";

    std::string dash(lineLength, '-');
    ss << dash << "\n";

    for (unsigned int x = 0; x < numberOfDevices; x++)
    {
      std::string idPad(maxIdLength + 1 - std::to_string(x).size(), ' ');
      ss << idPad << x << " |";
      for  (unsigned int y = 0; y < identifiers.size(); y ++) //(const auto& row : propertyMatrix)
      {
        unsigned int paddingLength = columnMaxStringSizes[y] + 1 - propertyMatrix[y][x].size();
        std::string  s(paddingLength, ' ');
        //ss << s << el << "  ";
        ss << s << propertyMatrix[y][x] << " |";
      }
      ss.seekp(-1, ss.cur);
      ss << "\n";
    }
    return ss.str();
  }

  void offloadCapabilityTest(){
#if defined(GPUOffloadingCUDA)
    std::vector<bool> testResults = tarch::accelerator::info::cuda::testKernelLaunch();
#elif defined(GPUOffloadingSYCL)
    std::vector<bool> testResults = tarch::accelerator::info::sycl::testKernelLaunch();
#elif defined(GPUOffloadingOMP)
    std::vector<bool> testResults = tarch::accelerator::info::omp::testKernelLaunch();
#elif defined(GPUOffloadingCPP)
    std::vector<bool> testResults = tarch::accelerator::info::cpp::testKernelLaunch();
    if (testResults.size() == 0){
      std::cout << "Iso C++ GPU-Parallelism with NVHPC compilers can't check kernel launch tests currently, skipping" << std::endl;
      return;
    } else if (testResults.size() == 1 && !testResults[0]){
      throw std::runtime_error("None of the found GPUs could successfully launch GPU Kernels (Check for wrong compile configuration)");
    }
#else
    // Print noinformation since we do not have an accelerator
    std::cout << "No GPU Accelerator Backend is enabled. The code will run on CPUs." << std::endl;
    return;
#endif

    unsigned int offloadCapableGPUCount = 0;
    std::string failMessage;
    for (unsigned int i = 0; i < testResults.size(); i++){
      if (!testResults[i]){
        // Using std::format would be nice but 10.06.2023 CUDA does support gcc upto 12, no std::format supprot
        failMessage += "GPU Device " + std::to_string(i) + " could not successfully launch a GPU kernel\n";
      } else {
        offloadCapableGPUCount += 1;
      }
    }

    if (testResults.size() != 0){
      if (offloadCapableGPUCount < testResults.size()){
        if (offloadCapableGPUCount == 0){
          throw std::runtime_error("None of the found GPUs could successfully launch GPU Kernels");
        }else{
          throw std::runtime_error(failMessage);
        }
      } else {
        std::cout << "Kernel launch test successful" << std::endl;
      }
    } else {
      throw std::runtime_error("Peano 4 was configured to run on accelerators, but no offloading capable GPU was found.");
    }
  }


  void runBenchmarks(){
    std::cout << "Started Streaming Benchmarks" << std::endl;
    
    std::vector<BenchmarkResultInNanoSeconds> results;
#if defined(GPUOffloadingCUDA)
    for (uint i : {32, 64, 128, 256, 512, 1024}){
      std::cout << "Benchmark with Threads/Block = " << i << std::endl;
      results = tarch::accelerator::info::cuda::runBenchmarks(i);
      parseBenchmarkResults(results);
    }
#elif defined(GPUOffloadingSYCL)
    results = tarch::accelerator::info::sycl::runBenchmarks();
    parseBenchmarkResults(results);
#elif defined(GPUOffloadingOMP)
    results = tarch::accelerator::info::omp::runBenchmarks();
    parseBenchmarkResults(results);
#elif defined(GPUOffloadingCPP)
    results = tarch::accelerator::info::cpp::runBenchmarks();
    parseBenchmarkResults(results);
#else
    return;
#endif

    std::cout << "Completed Streaming Benchmarks" << std::endl;
  }

  void parseBenchmarkResults(std::vector<BenchmarkResultInNanoSeconds>& benchmarkResults){
    std::vector<std::vector<std::string>> benchmarkResultsStringRepresentations(4, std::vector<std::string>(benchmarkResults.size(), ""));

    // A * 1e-9 / (B * 1e-9) = A / B
    // Therefore Byte / NanoSecond = GB / Second
    for (uint i = 0; i < benchmarkResults.size(); i++)
    {
      std::stringstream ss;
      auto& benchmarkResult = benchmarkResults[i];
      ss << benchmarkResult.allocatedDataSize * benchmarkResult.repeats * (1.0/static_cast<double>(benchmarkResult.CPUtoGPUCopy)) << " Gb/s";
      benchmarkResultsStringRepresentations[tarch::accelerator::info::getPropertyOffset("CPU-to-GPU Copy", benchmarkIdentifiers)][i] = ss.str();
      ss.str(std::string());
      ss << benchmarkResult.allocatedDataSize * benchmarkResult.repeats * (1.0/static_cast<double>(benchmarkResult.GPUtoCPUCopy)) << " Gb/s";
      benchmarkResultsStringRepresentations[tarch::accelerator::info::getPropertyOffset("GPU-to-CPU Copy", benchmarkIdentifiers)][i] = ss.str(); 
      ss.str(std::string());
      ss << benchmarkResult.allocatedDataSize * benchmarkResult.repeats * (1.0/static_cast<double>(benchmarkResult.GPUMalloc))    << " Gb/s";
      benchmarkResultsStringRepresentations[tarch::accelerator::info::getPropertyOffset("GPU-Malloc",      benchmarkIdentifiers)][i]      = ss.str(); 
      ss.str(std::string());
      ss << benchmarkResult.copiedDataSize    * benchmarkResult.repeats * (1.0/static_cast<double>(benchmarkResult.GPUtoGPUCopy)) << " Gb/s";
      benchmarkResultsStringRepresentations[tarch::accelerator::info::getPropertyOffset("GPU-to-GPU Copy", benchmarkIdentifiers)][i] = ss.str(); 
      ss.str(std::string());
    }

    std::string benchmarkTableOutput = parseMatrix(benchmarkResultsStringRepresentations, benchmarkResults.size(),  benchmarkIdentifiers);
    std::cout << benchmarkTableOutput << std::endl;
  }
} // namespace tarch::accelerator::info
