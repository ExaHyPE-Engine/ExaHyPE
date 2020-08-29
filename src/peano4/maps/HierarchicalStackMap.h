// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_MAPS_HIERARCHICAL_STACK_MAP_H_
#define _PEANO4_MAPS_HIERARCHICAL_STACK_MAP_H_


#include "maps.h"

#include "peano4/parallel/Node.h"

#include "tarch/multicore/Lock.h"

#include <map>



namespace peano4 {
  namespace maps {
    template <typename T>
    class HierarchicalStackMap;
  }
}



/**
 *
 *
 * <h2> Software architecture </h2>
 *
 * There's one entry per tree. I use the mapping from global to local trees, so
 * I don't have to maintain too many entries for 'just in case'. Each entry has
 * a semaphore of its own. So if we need a lock, we only lock once per tree.
 *
 *
 */
template <typename T>
class peano4::maps::HierarchicalStackMap {
  private:
    /**
     * Data of one tree.
     */
    struct TreeData {
      std::map< int, T* >                 _stackNumberToData;
    };

    /**
     * A vector of maps. I hold one entry per local spacetree, i.e.
     * map all global ids onto local indices.
     */
    std::vector< TreeData >            _data;

    /**
     * This routine is not thread-safe, i.e. if you need it thread-safe then
     * you have to wrap it into  a semaphore manually.
     */
    void createStack(int localTreeNumber, int stackNumber);
  public:
    HierarchicalStackMap();
    ~HierarchicalStackMap();

    bool empty(int treeId, int stackId) const;

    /**
     * @see STDStackMap::clear()
     */
    void clear();

    /**
     * Get the stack belonging to a tree.
     *
     * There are two versions of the routine: one is thread-safe, the other one
     * does not employ a dedicated semaphore. The one with a semaphore can
     * lazily create stacks; which is the reason why we need the semaphore:
     * everytime we lazily insert new stacks, we run risk that we break the
     * consistency of the underlying container. Originally, I worked with
     * thread-safe accessors only. This resulted in disappointing timings and
     * notably speedup. With a carelfull distinction, I got down the runtime
     * to two third of the original.
     *
     * @return A stack of type T. Actually, it is a pointer and this routine is
     *   a lazy creation, i.e. might create the result upon demand.
     */
    T* getForPush(int treeId, int stackId);
    T* getForPush(const StackKey& key);

    /**
     * @see getForPush(int,int)
     */
    T* getForPop(int treeId, int stackId);
    T* getForPop(const StackKey& key);

    std::string toString() const;

    /**
     * This one should be const, but I might need a semaphore, so I have to
     * omit the const qualifier.
     */
    std::set<StackKey>  getKeys();

    /**
     * @see STDStackMap::garbageCollection(int)
     */
    void garbageCollection(int spacetree);

    /**
     * For debugging/assertions.
     */
    bool holdsStack(int treeId, int stackId) const;
    bool holdsStack(const StackKey& key) const;
};


template <typename T>
void peano4::maps::HierarchicalStackMap<T>::createStack(int localTreeNumber, int stackNumber) {
  assertion(localTreeNumber>=0);
  assertion(localTreeNumber<_data.size());
  if ( _data[localTreeNumber]._stackNumberToData.count(stackNumber)==0 ) {
    _data[localTreeNumber]._stackNumberToData.insert(
      std::pair< int, T* >(
        stackNumber,
		new T()
	  )
    );
  }
}


template <typename T>
bool peano4::maps::HierarchicalStackMap<T>::empty(int treeId, int stackId) const {
  const int localTreeId = peano4::parallel::Node::getInstance().getLocalTreeId(treeId);
  assertion3(localTreeId>=0,localTreeId,treeId,stackId);
  assertion4(localTreeId<_data.size(),localTreeId,_data.size(),treeId,stackId);
  return _data[localTreeId]._stackNumberToData.count(stackId)==0
	  or _data[localTreeId]._stackNumberToData.at(stackId)->empty();
}


template <typename T>
T* peano4::maps::HierarchicalStackMap<T>::getForPush(int treeId, int stackId) {
  return getForPush( StackKey(treeId,stackId) );
}


template <typename T>
bool peano4::maps::HierarchicalStackMap<T>::holdsStack(int treeId, int stackId) const {
  return holdsStack( StackKey(treeId,stackId) );
}


template <typename T>
bool peano4::maps::HierarchicalStackMap<T>::holdsStack(const StackKey& key) const {
  const int localTreeId = peano4::parallel::Node::getInstance().getLocalTreeId(key.first);
  assertion3(localTreeId>=0,localTreeId,key.first,key.second);
  assertion4(localTreeId<_data.size(),localTreeId,_data.size(),key.first,key.second);
  return _data[localTreeId]._stackNumberToData.count(key.second)==1;
}


template <typename T>
T* peano4::maps::HierarchicalStackMap<T>::getForPush(const StackKey& key) {
  const int localTreeId = peano4::parallel::Node::getInstance().getLocalTreeId(key.first);
  assertion3(localTreeId>=0,localTreeId,key.first,key.second);
  assertion4(localTreeId<_data.size(),localTreeId,_data.size(),key.first,key.second);
  createStack(localTreeId,key.second);
  assertion4( _data[localTreeId]._stackNumberToData[key.second] != nullptr, localTreeId, key.first, key.second, tarch::mpi::Rank::getInstance().getRank() );
  return _data[localTreeId]._stackNumberToData[key.second];
}


template <typename T>
T* peano4::maps::HierarchicalStackMap<T>::getForPop(int treeId, int stackId) {
  return getForPop( StackKey(treeId,stackId) );
}


template <typename T>
T* peano4::maps::HierarchicalStackMap<T>::getForPop(const StackKey& key) {
  const int localTreeId = peano4::parallel::Node::getInstance().getLocalTreeId(key.first);
  assertion3(localTreeId>=0,localTreeId,key.first,key.second);
  assertion4(localTreeId<_data.size(),localTreeId,_data.size(),key.first,key.second);
  assertion4(_data[localTreeId]._stackNumberToData.count(key.second)==1,localTreeId,_data.size(),key.first,key.second);
  return _data[localTreeId]._stackNumberToData.at(key.second);
}


template <typename T>
std::string peano4::maps::HierarchicalStackMap<T>::toString() const {
  std::string result = "(" + std::to_string( _data.size() );
/*
  for (auto& p: _data) {
	result += ",";
	result += std::to_string(p.first.first) + "x" + std::to_string(p.first.second) + ":" + std::to_string(p.second->size());
  }
*/
  result += ")";
  return result;
}


template <typename T>
std::set<peano4::maps::StackKey>  peano4::maps::HierarchicalStackMap<T>::getKeys() {
  std::set<peano4::maps::StackKey> result;
  for (int i=0; i<_data.size(); i++) {
    for (auto& pp: _data[i]._stackNumberToData) {
      result.insert( peano4::maps::StackKey( peano4::parallel::Node::getInstance().getGlobalTreeId(i),pp.first) );
    }
  }
  return result;
}


template <typename T>
peano4::maps::HierarchicalStackMap<T>::~HierarchicalStackMap() {
  for (auto& p: _data) {
	for (auto& pp: p._stackNumberToData) {
	  delete pp.second;
	}
  }
}


template <typename T>
peano4::maps::HierarchicalStackMap<T>::HierarchicalStackMap():
  _data(peano4::parallel::Node::MaxSpacetreesPerRank) {
}


template <typename T>
void peano4::maps::HierarchicalStackMap<T>::garbageCollection(int spacetree) {
  if (_data.size()>spacetree) {
    for (auto& p: _data[spacetree]._stackNumberToData) {
      if (
        p.second->empty()
        and
        not peano4::parallel::Node::isStorageStackNumber(p.first)
      ) {
        delete p.second;
        p.second = new T();
      }
    }
  }
}


template <typename T>
void peano4::maps::HierarchicalStackMap<T>::clear() {
  for (auto& p: _data) {
    for (auto& pp: p._stackNumberToData) {
      if (pp.second->empty()) {
        delete pp.second;
        pp.second = new T();
      }
    }
  }
}

#endif
