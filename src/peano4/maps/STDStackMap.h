// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_MAPS_STD_MAP_H_
#define _PEANO4_MAPS_STD_MAP_H_


#include "maps.h"
#include "tarch/multicore/Lock.h"

#include <map>


namespace peano4 {
  namespace maps {
    template <typename T>
    class STDStackMap;
  }
}


template <typename T>
class peano4::maps::STDStackMap {
  private:
    static tarch::logging::Log  _log;

    std::map< StackKey, T* >  _data;

    /**
     * Semaphore to protect _data.
     */
    tarch::multicore::BooleanSemaphore                      _semaphore;

    /**
     * This routine is not thread-safe, i.e. if you need it thread-safe then
     * you have to wrap it into  a semaphore manually.
     */
    void createStack(const StackKey& key);
  public:
    ~STDStackMap();

    bool empty(int treeId, int stackId) const;

    /**
     * Get the stack belonging to a tree.
     *
     * There are two versions of the routine: one for pushes, one for pops. I
     * distinguish the two, as the push variant might create new stacks lazily
     * whereas the pop knowns a priori that the requested stack does exist.
     *
     * <h2> Implementation details </h2>
     *
     * We nevertheless have to be careful with pops. In the vanilla version,
     * pops cannot create new stacks. But we use one global map for all stacks
     * of all spacetrees hosted by this rank. So concurrent pushes can create
     * new stacks on-the-fly. So we need some thread-saving mechanisms.
     *
     * @return A stack of type T. It is a pointer and this routine realises
     *   a lazy creation, i.e. might create the result upon demand.
     */
    T* getForPush(int treeId, int stackId);
    T* getForPush(const StackKey& key);

    /**
     * @see getForPush(int,int)
     *
     * @return A stack of type T.
     */
    T* getForPop(int treeId, int stackId);
    T* getForPop(const StackKey& key);

    /**
     * Clear the whole stack map. A normal C++ code might deploy this into the
     * destructor, but I had weird seg faults when I did so.
     *
     * My explanation is that this map is also used in some library functions
     * and that they are typically global variables. The order in which stuff
     * is destroyed is thus non-deterministic (in the way that it is left to
     * the linker). So I rather shutdown stuff explicitly.
     */
    void clear();

    /**
     * With this clear, I have to manually delete all stacks. It is not enough
     * to call clear(). Clear() usually only resets the size to zero, and we
     * then wait for the garbage collection to free the actual memory. This
     * garbage collection however usually leaves all in/out (lets call them
     * storage) stacks intact, as it knows that we'll gonna use them in the
     * next sweep - an assumption that's invalid if we delete a tree and
     * consequently invoke clear(int).
     */
    void clear(int spacetree);

    std::string toString() const;

    /**
     * This one should be const, but I might need a semaphore, so I have to
     * omit the const qualifier.
     */
    std::set<StackKey>  getKeys();

    /**
     * Invoke garbage collection
     *
     * The standard stack in C++ has a clear operation. However, it does not
     * actively free memory. Indeed, the C++ standard leaves it up to the STL
     * implementation to decide whether to free or not. Most don't. As a
     * consequence, the memory hunger of Peano applications could rise
     * dramatically - in particular throughout forks when whole spacetrees are
     * (temporarily) replicated.
     *
     * The only way around the lack of a proper free is the total elimination
     * of a vector (via a delete) and the recreation. We do thus rely on this
     * mechanism though we only apply it to communication stacks. All the
     * temporary stacks and in/out stacks are never erased.
     */
    void garbageCollection(int spacetree);

    /**
     * For debugging/assertions.
     */
    bool holdsStack(int treeId, int stackId) const;
    bool holdsStack(const StackKey& key) const;
};


#include "tarch/multicore/Lock.h"


template <typename T>
tarch::logging::Log  peano4::maps::STDStackMap<T>::_log( "peano4::maps::STDStackMap<T>" );


template <typename T>
void peano4::maps::STDStackMap<T>::createStack(const StackKey& key) {
  if ( _data.count( key )==0 ) {
    _data.insert(
      std::pair< StackKey, T* >(
        key,
        new T()
      )
    );
  }
}


template <typename T>
bool peano4::maps::STDStackMap<T>::empty(int treeId, int stackId) const {
  return _data.count( StackKey(treeId,stackId) )==0
      or _data.at( StackKey(treeId,stackId) )->empty();
}


template <typename T>
T* peano4::maps::STDStackMap<T>::getForPush(int treeId, int stackId) {
  return getForPush( StackKey(treeId,stackId) );
}


template <typename T>
T* peano4::maps::STDStackMap<T>::getForPush(const StackKey& key) {
  tarch::multicore::Lock lock(_semaphore);
  createStack(key);
  return _data[key];
}


template <typename T>
T* peano4::maps::STDStackMap<T>::getForPop(int treeId, int stackId) {
  return getForPop( StackKey(treeId,stackId) );
}


template <typename T>
T* peano4::maps::STDStackMap<T>::getForPop(const StackKey& key) {
  tarch::multicore::Lock lock(_semaphore);
  assertion( _data.count(key)==1 );
  return _data.at( key );
}


template <typename T>
std::string peano4::maps::STDStackMap<T>::toString() const {
  std::ostringstream msg;
  msg << "(" << _data.size();
  for (auto& p: _data) {
    msg << ","
        << p.first.first << "x" << p.first.second << ":" << p.second->size();
  }
  msg << ")";
  return msg.str();
}


template <typename T>
std::set<peano4::maps::StackKey>  peano4::maps::STDStackMap<T>::getKeys() {
  tarch::multicore::Lock lock(_semaphore);
  std::set<peano4::maps::StackKey> result;
  for (auto& p: _data) {
    result.insert(p.first);
  }
  return result;
}


template <typename T>
bool peano4::maps::STDStackMap<T>::holdsStack(int treeId, int stackId) const {
  return holdsStack( StackKey(treeId,stackId) );
}


template <typename T>
bool peano4::maps::STDStackMap<T>::holdsStack(const StackKey& key) const {
  return _data.count(key)==1;
}


template <typename T>
void peano4::maps::STDStackMap<T>::clear() {
  for (auto& p: _data) {
    delete p.second;
  }

  _data.clear();
}


template <typename T>
peano4::maps::STDStackMap<T>::~STDStackMap() {
  assertionMsg( _data.empty(), "forgot to call clear()" );
}


template <typename T>
void peano4::maps::STDStackMap<T>::garbageCollection(int spacetree) {
  const int localTreeId = peano4::parallel::Node::getInstance().getLocalTreeId(spacetree);
  for (auto& p: _data) {
    bool isCorrectOwner        = p.first.first==localTreeId;
    bool isEmptyTemporaryStack = isCorrectOwner
        and
        p.second->empty()
        and
        not peano4::parallel::Node::isStorageStackNumber(p.first.second);


    if ( isEmptyTemporaryStack ) {
      logDebug( "garbageCollection(...)", "remove stack " << p.first.first << "x" << p.first.second << ": " << p.second->toString() );
      delete p.second;
      p.second = new T();
	  }
  }
}


template <typename T>
void peano4::maps::STDStackMap<T>::clear(int spacetree) {
  const int localTreeId = peano4::parallel::Node::getInstance().getLocalTreeId(spacetree);
  for (auto& p: _data) {
    if (
      p.first.first==localTreeId
    ) {
      logDebug( "garbageCollection(...)", "remove stack " << p.first.first << "x" << p.first.second << ": " << p.second->toString() );
      delete p.second;
      p.second = new T();
    }
  }
}

#endif

