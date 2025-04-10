#include "PeanoTextPatchFileReader.h"
#include "tarch/mpi/Rank.h"


void tarch::reader::readInCheckpointFiles(
  const std::vector<std::string>&  checkpointFilesOfSolver,
  std::vector<double>&             domainDataFromFiles,
  std::vector<double>&             offsetIndex,
  int numberOfUnknowns,
  int numberOfAuxiliaries,
  int dimension,
  int patchSize
) {
  domainDataFromFiles.clear();
  offsetIndex.clear();

  for (const auto& filename : checkpointFilesOfSolver) {
    std::ifstream file(filename);
    std::string line;
    bool InAPatch=false;
    while (std::getline(file, line)) {
      if ( (not InAPatch) and line.find("  offset") == 0 ) { 
        std::istringstream iss(line);
        std::string token;

        while (iss >> token) { try {offsetIndex.push_back(std::stod(token)); } catch (...){ } }
        InAPatch=true;
      }

      if ( InAPatch and (line.find("     ") == 0) ){
        std::istringstream iss(line);
        std::string token;

        while (iss >> token) { domainDataFromFiles.push_back(std::stod(token));}
        InAPatch=false;
      }
    }
  }

  //for (int j=0; j<=(59 + 0)*27; j++){
  //  if (domainDataFromFiles[j] > 0) 
  //    std::cout<<domainDataFromFiles[j]<<std::endl;
  //}
  //std::cout<<offsetIndex[-999]<<std::endl;
}


//      calling example
//      std::unordered_map<std::tuple<double, double, double>, std::string, tarch::reader::offsetHash, tarch::reader::offsetEqual> OffsetIndexMap;
//      tarch::reader::readInCheckpointOffsetIndex(OffsetIndexMap, CheckpointFilesOfCCZ4);
void tarch::reader::readInCheckpointOffsetIndex(
      std::map< tarch::la::Vector<3,double> , std::string, tarch::la::VectorCompare<3> >& offsetIndexMap,
      std::vector<std::string> checkpointFilesOfSolver){
  //do nothing yet
  //std::cout<<"print sth as it is called."<<std::endl;
  for (const auto& filename : checkpointFilesOfSolver) {
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
      if (line.find("  offset") == 0 ) { 
        std::istringstream iss(line);
        std::string token; 
        int i=0;

        tarch::la::Vector<3,double> tempOffset;
        while (iss >> token) { try {tempOffset[i]=std::stod(token); i++; } catch (...){ } }
        offsetIndexMap[tempOffset]=filename;

      }
    }
  }

//  std::tuple<double, double, double> test = {-0.5+1e-7, -5.000000e-01, -5.000000e-01};
//  std::cout<<offsetIndexMap[test]<<std::endl;
/*    std::tuple<double, double, double> query_coord = {-0.5+1e-7, -0.5, -0.5};
    if (offsetIndexMap.find(query_coord) != offsetIndexMap.end()) {
        std::cout << "Coordinate maps to: " << offsetIndexMap[query_coord] << std::endl;
    } else {
        std::cout << "Coordinate not found." << std::endl;
    }*/


}
