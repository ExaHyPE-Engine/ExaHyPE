// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "peano4/utils/Globals.h"
#include "peano4/grid/TraversalObserver.h"

#ifdef Parallel
#include <mpi.h>
#include <functional>
#endif

namespace peano4{
namespace grid{
  struct GridTraversalEvent;
}
}

struct peano4::grid::GridTraversalEvent {
  public:
    GridTraversalEvent() {}
    GridTraversalEvent(tarch::la::Vector<Dimensions,double>  __x, tarch::la::Vector<Dimensions,double>  __h, std::bitset<TwoPowerD>  __hasBeenRefined, std::bitset<TwoPowerD>  __willBeRefined, std::bitset<TwoPowerD>  __isVertexLocal, std::bitset<TwoTimesD>  __isFaceLocal, bool  __isCellLocal, bool  __isParentCellLocal, std::bitset<TwoPowerD>  __isVertexAdjacentToParallelDomainBoundary, std::bitset<TwoTimesD>  __isFaceAdjacentToParallelDomainBoundary, tarch::la::Vector<TwoPowerD,int>  __vertexDataFrom, tarch::la::Vector<TwoPowerD,int>  __vertexDataTo, tarch::la::Vector<TwoTimesD,int>  __faceDataFrom, tarch::la::Vector<TwoTimesD,int>  __faceDataTo, int  __cellData, tarch::la::Vector<Dimensions,int>  __relativePositionToFather, int  __invokingSpacetree, bool  __invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing);

    tarch::la::Vector<Dimensions,double>   getX() const;
    void   setX(const tarch::la::Vector<Dimensions,double>& value);
    double   getX(int index) const;
    void   setX(int index, double value);
    tarch::la::Vector<Dimensions,double>   getH() const;
    void   setH(const tarch::la::Vector<Dimensions,double>& value);
    double   getH(int index) const;
    void   setH(int index, double value);
    std::bitset<TwoPowerD>   getHasBeenRefined() const;
    void   setHasBeenRefined(const std::bitset<TwoPowerD>&  value);
    bool   getHasBeenRefined(int index) const;
    void   setHasBeenRefined(int index, bool value);
    void   flipHasBeenRefined(int index);
    std::bitset<TwoPowerD>   getWillBeRefined() const;
    void   setWillBeRefined(const std::bitset<TwoPowerD>&  value);
    bool   getWillBeRefined(int index) const;
    void   setWillBeRefined(int index, bool value);
    void   flipWillBeRefined(int index);
    std::bitset<TwoPowerD>   getIsVertexLocal() const;
    void   setIsVertexLocal(const std::bitset<TwoPowerD>&  value);
    bool   getIsVertexLocal(int index) const;
    void   setIsVertexLocal(int index, bool value);
    void   flipIsVertexLocal(int index);
    std::bitset<TwoTimesD>   getIsFaceLocal() const;
    void   setIsFaceLocal(const std::bitset<TwoTimesD>&  value);
    bool   getIsFaceLocal(int index) const;
    void   setIsFaceLocal(int index, bool value);
    void   flipIsFaceLocal(int index);
    bool   getIsCellLocal() const;
    void   setIsCellLocal(bool value);
    bool   getIsParentCellLocal() const;
    void   setIsParentCellLocal(bool value);
    std::bitset<TwoPowerD>   getIsVertexAdjacentToParallelDomainBoundary() const;
    void   setIsVertexAdjacentToParallelDomainBoundary(const std::bitset<TwoPowerD>&  value);
    bool   getIsVertexAdjacentToParallelDomainBoundary(int index) const;
    void   setIsVertexAdjacentToParallelDomainBoundary(int index, bool value);
    void   flipIsVertexAdjacentToParallelDomainBoundary(int index);
    std::bitset<TwoTimesD>   getIsFaceAdjacentToParallelDomainBoundary() const;
    void   setIsFaceAdjacentToParallelDomainBoundary(const std::bitset<TwoTimesD>&  value);
    bool   getIsFaceAdjacentToParallelDomainBoundary(int index) const;
    void   setIsFaceAdjacentToParallelDomainBoundary(int index, bool value);
    void   flipIsFaceAdjacentToParallelDomainBoundary(int index);
    tarch::la::Vector<TwoPowerD,int>   getVertexDataFrom() const;
    void   setVertexDataFrom(const tarch::la::Vector<TwoPowerD,int>& value);
    int   getVertexDataFrom(int index) const;
    void   setVertexDataFrom(int index, int value);
    tarch::la::Vector<TwoPowerD,int>   getVertexDataTo() const;
    void   setVertexDataTo(const tarch::la::Vector<TwoPowerD,int>& value);
    int   getVertexDataTo(int index) const;
    void   setVertexDataTo(int index, int value);
    tarch::la::Vector<TwoTimesD,int>   getFaceDataFrom() const;
    void   setFaceDataFrom(const tarch::la::Vector<TwoTimesD,int>& value);
    int   getFaceDataFrom(int index) const;
    void   setFaceDataFrom(int index, int value);
    tarch::la::Vector<TwoTimesD,int>   getFaceDataTo() const;
    void   setFaceDataTo(const tarch::la::Vector<TwoTimesD,int>& value);
    int   getFaceDataTo(int index) const;
    void   setFaceDataTo(int index, int value);
    int   getCellData() const;
    void   setCellData(int value);
    tarch::la::Vector<Dimensions,int>   getRelativePositionToFather() const;
    void   setRelativePositionToFather(const tarch::la::Vector<Dimensions,int>& value);
    int   getRelativePositionToFather(int index) const;
    void   setRelativePositionToFather(int index, int value);
    int   getInvokingSpacetree() const;
    void   setInvokingSpacetree(int value);
    bool   getInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing() const;
    void   setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(bool value);
    GridTraversalEvent(const GridTraversalEvent& copy);

#ifdef Parallel
    static void sendAndPollDanglingMessages(const peano4::grid::GridTraversalEvent& message, int destination, int tag, MPI_Comm communicator=tarch::mpi::Rank::getInstance().getCommunicator());
    static void receiveAndPollDanglingMessages(peano4::grid::GridTraversalEvent& message, int source, int tag, MPI_Comm communicator=tarch::mpi::Rank::getInstance().getCommunicator() );
#endif

    #ifdef Parallel
    PragmaPush
    SilenceUnknownAttribute
    /**
     * Hands out MPI datatype if we work without the LLVM MPI extension.
     * If we work with this additional feature, this is the routine where
     * the lazy initialisation is done and the datatype is also cached.
     */
    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getForkDatatype();

    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getJoinDatatype();

    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getBoundaryExchangeDatatype();

    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getMultiscaleDataExchangeDatatype();

    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getGlobalCommunciationDatatype();
    PragmaPop

    /**
     * @return The rank of the sender of an object. It only make ssense to call
     *         this routine after you've invoked receive with MPI_ANY_SOURCE.
     */
    int getSenderRank() const;

    /**
     * Wrapper around getDatatype() to trigger lazy evaluation if we
     * use the lazy initialisation.
     */
    static void initDatatype();

    /**
     * Free the underlying MPI datatype.
     */
    static void shutdownDatatype();

    /**
     * In DaStGen (the first version), I had a non-static version of the send
     * as well as the receive. However, this did not work with newer C++11 
     * versions, as a member function using this as pointer usually doesn't 
     * see the vtable while the init sees the object from outside, i.e. 
     * including a vtable. So this routine now is basically an alias for a
     * blocking MPI_Send. 
     */
    static void send(const peano4::grid::GridTraversalEvent& buffer, int destination, int tag, MPI_Comm communicator );
    static void receive(peano4::grid::GridTraversalEvent& buffer, int source, int tag, MPI_Comm communicator );

    /**
     * Alternative to the other send() where I trigger a non-blocking send an 
     * then invoke the functor until the corresponding MPI_Test tells me that 
     * the message went through. In systems with heavy MPI usage, this can 
     * help to avoid deadlocks.
     */
    static void send(const peano4::grid::GridTraversalEvent& buffer, int destination, int tag, std::function<void()> startCommunicationFunctor, std::function<void()> waitFunctor, MPI_Comm communicator );
    static void receive(   peano4::grid::GridTraversalEvent& buffer, int source,      int tag, std::function<void()> startCommunicationFunctor, std::function<void()> waitFunctor, MPI_Comm communicator );
    #endif

    std::string toString() const;

  private:
    PragmaPush
    SilenceUnknownAttribute
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::truncate_mantissa(23)]]  double   _x[Dimensions];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      tarch::la::Vector<Dimensions,double>   _x;
#endif
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::truncate_mantissa(23)]]  double   _h[Dimensions];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      tarch::la::Vector<Dimensions,double>   _h;
#endif
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::pack]]  bool   _hasBeenRefined[TwoPowerD];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      std::bitset<TwoPowerD>   _hasBeenRefined;
#endif
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::pack]]  bool   _willBeRefined[TwoPowerD];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      std::bitset<TwoPowerD>   _willBeRefined;
#endif
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::pack]]  bool   _isVertexLocal[TwoPowerD];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      std::bitset<TwoPowerD>   _isVertexLocal;
#endif
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::pack]]  bool   _isFaceLocal[TwoTimesD];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      std::bitset<TwoTimesD>   _isFaceLocal;
#endif
    [[clang::pack]]  bool   _isCellLocal;
    [[clang::pack]]  bool   _isParentCellLocal;
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::pack]]  bool   _isVertexAdjacentToParallelDomainBoundary[TwoPowerD];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      std::bitset<TwoPowerD>   _isVertexAdjacentToParallelDomainBoundary;
#endif
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::pack]]  bool   _isFaceAdjacentToParallelDomainBoundary[TwoTimesD];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      std::bitset<TwoTimesD>   _isFaceAdjacentToParallelDomainBoundary;
#endif
    tarch::la::Vector<TwoPowerD,int>   _vertexDataFrom;
    tarch::la::Vector<TwoPowerD,int>   _vertexDataTo;
    tarch::la::Vector<TwoTimesD,int>   _faceDataFrom;
    tarch::la::Vector<TwoTimesD,int>   _faceDataTo;
      int   _cellData;
#if defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
    [[clang::pack_range(0,3)]]  int   _relativePositionToFather[Dimensions];
#endif
#if !defined(__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__)
      tarch::la::Vector<Dimensions,int>   _relativePositionToFather;
#endif
    [[clang::pack_range(-1,std::numeric_limits<int>::max())]]  int   _invokingSpacetree;
    [[clang::pack]]  bool   _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;
    PragmaPop

    #ifdef Parallel
    private:
      int _senderDestinationRank;

      #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
      /**
       * Whenever we use LLVM's MPI extension (DaStGe), we rely on lazy
       * initialisation of the datatype. However, Peano calls init explicitly
       * in most cases. Without the LLVM extension which caches the MPI 
       * datatype once constructed, this field stores the type.
       */
      static MPI_Datatype  Datatype;
      #endif
    #endif
};
