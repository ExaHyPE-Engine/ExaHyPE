// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <map>
#include <tuple>
#include <queue>

#include "tarch/la/VectorCompare.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <cctype>

#include "tarch/logging/Log.h"

namespace tarch {
  namespace reader {
    /**
     * Read Peano Patch file back into the memory for certain
     * features, e.g. checkpoint.
     */

    /** accept the list of checkpoint files and then read in 
     *  the coordinates and patch data from them. Quite memory
     *  intensive and need optimization.
     */
    void readInCheckpointFiles(
      std::vector<std::string> CheckpointFilesOfSolver,
      std::vector<double>& domainDataFromFiles,
      std::vector<double>& offsetIndex,
      int numberOfUnknowns,
      int numberOfAuxiliaries,
      int dimension,
      int patchSize);

    struct offsetHash {
      std::size_t operator()(const std::tuple<double, double, double>& offset) const {
        double tolerance = 1e-4;
        auto roundToTolerance = [tolerance](double value) {
          return std::round(value / tolerance) * tolerance;
        };

        double x = roundToTolerance(std::get<0>(offset));
        double y = roundToTolerance(std::get<1>(offset));
        double z = roundToTolerance(std::get<2>(offset));

        std::size_t hash1 = std::hash<double>()(x);
        std::size_t hash2 = std::hash<double>()(y);
        std::size_t hash3 = std::hash<double>()(z);

        return hash1 ^ (hash2 << 1) ^ (hash3 << 2);
      }
    };

    /*struct offsetEqual {
      bool operator()(const std::tuple<double, double, double>& offset1,
                      const std::tuple<double, double, double>& offset2) const {
        double tolerance = 1e-3;
        return std::fabs(std::get<0>(offset1) - std::get<0>(offset2)) < tolerance &&
               std::fabs(std::get<1>(offset1) - std::get<1>(offset2)) < tolerance &&
               std::fabs(std::get<2>(offset1) - std::get<2>(offset2)) < tolerance;
      }
    };*/

    struct offsetEqual {
            bool operator()(const std::tuple<double, double, double>& coord1,
                            const std::tuple<double, double, double>& coord2) const {
                double tolerance = 1e-6; // Define your tolerance here

                // Compare each element of the tuple within the tolerance
                bool areEqual = std::fabs(std::get<0>(coord1) - std::get<0>(coord2)) < tolerance &&
                                std::fabs(std::get<1>(coord1) - std::get<1>(coord2)) < tolerance &&
                                std::fabs(std::get<2>(coord1) - std::get<2>(coord2)) < tolerance;

                // Debugging: print the differences to check the comparison
                if (!areEqual) {
                    std::cout << "Comparing: (" 
                              << std::get<0>(coord1) << ", " 
                              << std::get<1>(coord1) << ", "
                              << std::get<2>(coord1) << ") vs ("
                              << std::get<0>(coord2) << ", "
                              << std::get<1>(coord2) << ", "
                              << std::get<2>(coord2) << ")\n";
                    std::cout << "Differences: "
                              << "X: " << std::fabs(std::get<0>(coord1) - std::get<0>(coord2)) << ", "
                              << "Y: " << std::fabs(std::get<1>(coord1) - std::get<1>(coord2)) << ", "
                              << "Z: " << std::fabs(std::get<2>(coord1) - std::get<2>(coord2)) << "\n";
                }
                return areEqual;
            }
    };

    /**
     * read through all checkpoint files and create a map from patch offset to 
     * the subfile it belongs to, which can be used to locate the subfile it needs to read data
     * from in the real domain data assignment.
     */
    void readInCheckpointOffsetIndex(
      std::map< tarch::la::Vector<3,double> , std::string, tarch::la::VectorCompare<3> >& offsetIndexMap,
      std::vector<std::string> CheckpointFilesOfSolver);

    /**
     * A reader class allowing code to load checkpoint subfile if it hasn't
     * be loaded in memory and search in loaded subfiles to find the correct
     * patch data for restarting. 
     * 
     * Designed for memory-wise option of restarting, ideally this option will
     * be faster than pure loaded approach and also avold momoery fault in for huge domain.
     *
     */
    class CheckpointFilesManager {
      private:
        size_t memoryLimit; // Memory limit in bytes
        size_t currentMemoryUsage = 0;
        std::unordered_map<std::string, std::vector<std::string>> CheckpointFileDataset; // Map of filename to file content
        std::queue<std::string> loadedCheckpointFileNames; // Order of loaded checkpointfiles

        size_t calculateMemoryUsage(const std::vector<std::string>& data) {
          size_t size = 0;
          for (const auto& line : data) {
            size += line.size() + sizeof(line);
          }
          return size;
        }

        void freeOldestFile() {
          if (!loadedCheckpointFileNames.empty()) {
            const std::string& oldestFile = loadedCheckpointFileNames.front();
            currentMemoryUsage -= calculateMemoryUsage(CheckpointFileDataset[oldestFile]);
            CheckpointFileDataset.erase(oldestFile);
            loadedCheckpointFileNames.pop();
          }
        }

        void loadNewFile(const std::string& filename) {
          std::ifstream file(filename);

          std::vector<std::string> lines;
          std::string line;
          while (std::getline(file, line)) {
              lines.push_back(line);
          }
          file.close();

          size_t fileMemoryUsage = calculateMemoryUsage(lines);

          while (currentMemoryUsage + fileMemoryUsage > memoryLimit && !loadedCheckpointFileNames.empty()) {
            std::cout<<"memory limit is "<<memoryLimit<<" but have used "<<currentMemoryUsage + fileMemoryUsage<<" so remove oldest file."<<std::endl;
            freeOldestFile();
          }

          CheckpointFileDataset[filename] = std::move(lines);
          loadedCheckpointFileNames.push(filename);
          currentMemoryUsage += fileMemoryUsage;
          //std::cout<<filename<<" loaded"<<std::endl;
        }

      public:
          CheckpointFilesManager(size_t limit) : memoryLimit(limit) {
             //std::cout << "Memory limit initialized to: " << memoryLimit << std::endl;
          }

          const std::vector<std::string>& getCheckpointFileData(const std::string& filename) {
              if (CheckpointFileDataset.find(filename) == CheckpointFileDataset.end()) {
                //std::cout<<filename<<" will be loaded"<<std::endl;
                loadNewFile(filename);
              } //else { std::cout<<filename<<" has be loaded already."<<std::endl; }
              return CheckpointFileDataset[filename];
          }

          /*bool searchInFile(const std::string& filename, const std::string& keyword) {
              const auto& lines = getFileData(filename);
              for (const auto& line : lines) {
                  if (line.find(keyword) != std::string::npos) {
                      return true;
                  }
              }
              return false;
          }*/
    };

  }
}
