#include "DeviceInfo.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <iostream>

#if defined(WITH_CUDA_API) or defined(GPUOffloadingCUDA)
#include "cuda/DeviceInfo.h"
#elif defined(GPUOffloadingSYCL)
#include "sycl/DeviceInfo.h"
#elif defined(GPUOffloadingOMP)
#include "omp/DeviceInfo.h"
#elif defined(GPUOffloadingCPP)
#include "cpp/DeviceInfo.h"
#else
#include <iostream>
#endif

// #else if defined(WITH_SYCL_API) // Possible TODO to differentiate SYCL_API and Sycl

namespace tarch::accelerator::info
{
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
#if defined(WITH_CUDA_API) or defined(GPUOffloadingCUDA)
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
#if defined(WITH_CUDA_API) or defined(GPUOffloadingCUDA)
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


  size_t getPropertyOffset(const char name[])
  {
    auto it = std::find(properties.begin(), properties.end(), std::string(name));
    if (it == properties.end())
    {
      throw std::runtime_error("Property input name not a part of the properties list!");
    }
    return std::distance(properties.begin(), it);
  }

  std::string parseDeviceInformation(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices)
  {
    unsigned int propertyCount = properties.size();

    std::stringstream ss;

    // This size is the padding we need to add the porpety-name column
    unsigned int maxPropertyLengthWithPadding
      = std::max_element(
          properties.begin(),
          properties.end(),
          [](const std::string& lhs, const std::string& rhs) { return lhs.size() < rhs.size(); }
        )->size()
        + 1;

    // Get the string entry of a columng with the highest length
    std::vector<unsigned int> columnMaxStringSizes(properties.size(), 0);
    for (unsigned int row = 0; row < properties.size(); row++)
    {
      std::vector<unsigned int> col_sizes;
      for (unsigned int col = 0; col < numberOfDevices; col++)
      {
        col_sizes.push_back(propertyMatrix[row][col].size());
      }

      // Device property is also a part of the column
      col_sizes.push_back(properties[row].size());

      unsigned int maxLength    = *std::max_element(col_sizes.begin(), col_sizes.end());
      columnMaxStringSizes[row] = maxLength;
    }

    unsigned int maxIdLength = std::max(std::to_string(numberOfDevices).size(), std::string("DeviceId").size());

    // Iterate the matrix and write per row, also while accessing the padding we need to add
    std::string propIdPad(maxIdLength + 1 - std::string("DeviceId").size(), ' ');
    unsigned int lineLength = 0;
    
    ss << propIdPad << "DeviceId |";
    lineLength += propIdPad.size() + 10;
    for (unsigned int x = 0; x < properties.size(); x++){
      std::string propPadding(columnMaxStringSizes[x] + 1 - properties[x].size(), ' ');
      ss << propPadding << properties[x] << " |";
      lineLength += propPadding.size() + properties[x].size() + 2;
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
      for  (unsigned int y = 0; y < properties.size(); y ++) //(const auto& row : propertyMatrix)
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


  std::string parseDeviceInformationDeviceAsCol(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices)
  {
    unsigned int propertyCount = properties.size();

    std::stringstream ss;

    // This size is the padding we need to add the porpety-name column
    unsigned int maxPropertyLengthWithPadding
      = std::max_element(
          properties.begin(),
          properties.end(),
          [](const std::string& lhs, const std::string& rhs) { return lhs.size() < rhs.size(); }
        )->size()
        + 1;

    // Get the string entry of a columng with the highest length
    std::vector<unsigned int> columnMaxStringSizes(numberOfDevices, 0);
    for (unsigned int col = 0; col < numberOfDevices; col++)
    {
      std::vector<unsigned int> col_sizes;
      for (unsigned int row = 0; row < properties.size(); row++)
      {
        col_sizes.push_back(propertyMatrix[row][col].size());
      }

      // Device Id is also a part of the column
      col_sizes.push_back(std::to_string(col).size());

      unsigned int maxLength    = *std::max_element(col_sizes.begin(), col_sizes.end());
      columnMaxStringSizes[col] = maxLength;
    }

    // Generate the first 2 lines with the device ids and dash seperator
    unsigned int lineLength = 0;
    std::string  propPadding(maxPropertyLengthWithPadding, ' ');
    ss << propPadding << "|";
    lineLength += maxPropertyLengthWithPadding + 1;
    for (unsigned int i = 0; i < numberOfDevices; i++)
    {
      std::string paddingNeeded = std::string((columnMaxStringSizes[i] + 1) - std::to_string(i).size(), ' ');
      ss << paddingNeeded << i << "  ";
      lineLength += paddingNeeded.size() + std::to_string(i).size() + 2;
    }
    lineLength -= 2;
    ss.seekp(-2, ss.cur);
    ss << "\n";
    std::string dashLine(lineLength, '-');
    ss << dashLine << "\n";
    // Generation of first 2 lines ends here

    // Iterate the matrix and write per row, also while accessing the padding we need to add
    unsigned int x = 0;
    unsigned int y = 0;
    for (const auto& row : propertyMatrix)
    {
      std::string propPadding(maxPropertyLengthWithPadding - properties[x].size(), ' ');
      ss << properties[x] << propPadding << "|";
      y = 0;
      for (const auto& el : row)
      {
        unsigned int paddingLength = columnMaxStringSizes[y] + 1 - el.size();
        std::string  s(paddingLength, ' ');
        ss << s << el << "  ";
        y += 1;
      }
      ss.seekp(-1, ss.cur);
      ss << "\n";
      x += 1;
    }
    return ss.str();
  }

  void offloadCapabilityTest(){
#if  defined(GPUOffloadingCUDA)
    std::vector<bool> testResults = tarch::accelerator::info::cuda::testKernelLaunch();
#elif defined(GPUOffloadingSYCL)
    std::vector<bool> testResults = tarch::accelerator::info::sycl::testKernelLaunch();
#elif defined(GPUOffloadingOMP)
    // An empty table, with OpenMP we will only know how many GPUs we have
    std::vector<bool> testResults = tarch::accelerator::info::omp::testKernelLaunch();
#elif defined(GPUOffloadingCPP)
    // Can't get any information with cpp GPU offloading rn
    std::vector<bool> testResults = tarch::accelerator::info::cpp::testKernelLaunch();
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
} // namespace tarch::accelerator::info
