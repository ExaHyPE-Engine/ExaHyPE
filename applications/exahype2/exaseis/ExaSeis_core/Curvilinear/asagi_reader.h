#ifndef ASAGIREADER_CLASS_HEADER
#define ASAGIREADER_CLASS_HEADER

#include <asagi.h>
#include <easi/util/AsagiReader.h>

#ifdef WITH_ASAGI

enum NUMACache_Mode
{
	NUMA_OFF, NUMA_ON, NUMA_CACHE
};

class AsagiReader : public easi::AsagiReader{
 private:
  const std::string m_envPrefix;
 public:
  virtual ~AsagiReader(){};
  
   AsagiReader(const char* envPrefix) : m_envPrefix(envPrefix){}
  
  virtual ::asagi::Grid* open(char const* file, char const* varname) override{
  //asagi::Grid* grid = asagi::Grid::create();
  asagi::Grid* grid = asagi::Grid::createArray();
  //  grid->setParam(numberOfThreads());
  grid->setParam("NUMA_COMMUNICATION", "OFF",0);
  
  grid->setParam("GRID", "CACHE",0);
  
  grid->setParam("VALUE_POSITION","VERTEX_CENTERED");
  grid->setParam("VARIABLE",varname);
  
  grid->setParam("BLOCK_SIZE_0", "64",0);
  grid->setParam("BLOCK_SIZE_1", "64",0);
  grid->setParam("BLOCK_SIZE_2", "64",0);
  grid->setParam("CACHE_SIZE", "128",0);

  asagi::Grid::Error err = grid->open(file);
  
  if(err != ::asagi::Grid::SUCCESS){
    std::cout << "Could not open "<< file << std::endl;
  }
  
  return grid;
}
  
  virtual unsigned numberOfThreads() const override { return 1; }
  
};


#endif
#endif
