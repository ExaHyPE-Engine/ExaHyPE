// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/utils/Globals.h"
#include "tarch/Assertions.h"


namespace peano4 {
  namespace datamanagement {
    template <class Vertex>
    struct VertexEnumerator;
  }
}


/**
 * Vertex enumerator within array
 *
 * When Peano invokes an event (function) for a cell, it internally operates
 * over an array of the @f$ 2^d @f$ vertices that are adjacent to this cell.
 * That is, every cell has access to its adjacent vertices. However, we never
 * provide direct access to an array of these vertices, since you could
 * neither assume that this array has
 * exactly @f$ 2^d @f$ entries, not should any code every make an assumption
 * about the physical ordering of these vertices in memory. The data might be
 * scattered, and it might be permuted. Peano internally decides how to order
 * the data.
 *
 * Yet, Peano logically orders the vertices. They are enumerated from
 * @f$ [0,2^d-1] @f$ lexicographically with the bottom left (2d) bottom left
 * front vertex being vertex number 0.
 *
 * @image html VertexEnumerator.png
 *
 * Every cell event now is given an instance of VertexEnumerator, and you can
 * use the functor of this object to get the vertex with a logical number.
 * That is
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * vertexEnumerator(0).foo();
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * will invoke foo() on vertex 0. The enumerator encapsulates how the
 * vertices are ordered in memory. User codes do not see this explicitly,
 *
 * Internally, the vertex enumerator is only a collection of @f$ 2^d @f$
 * pointers to vertices. The grid traversal automaton builds up this
 * enumerator, i.e. it sets these pointers via setPointer(). When user
 * codes use the functor, they are given back the respective object to
 * which the ith pointer points to.
 *
 * @see peano4::datamanagement::FaceEnumerator for the same discussion for vertices.
 */
template <class Vertex>
struct peano4::datamanagement::VertexEnumerator {
  private:
    Vertex* _vertices[ TwoPowerD ];
  public:
    VertexEnumerator() {
      #if PeanoDebug>0
      for (int i=0; i<TwoPowerD; i++) {
        _vertices[i] = nullptr;
      }
      #endif
    }

    /**
     * Constructs vertex enumerator with default layout for consecutively stored vertices
     *
     */
    VertexEnumerator(Vertex* firstVertex) {
      assertion( firstVertex!=nullptr );
      for (int i=0; i<TwoPowerD; i++) {
        _vertices[i] = firstVertex+i;
      }
    }


    /**
     * Usually is only used by the observers, i.e. users should not interact
     * with this routine.
     */
    void setPointer(int i, Vertex* value) {
      assertion(i>=0);
      assertion(i<TwoPowerD);
      assertion(value!=nullptr);
      _vertices[i] = value;
    }

    /**
     * Access the ith vertex
     *
     * Consult the class enumeration for detailed information. The
     * vertices within a cell are enumerated lexicographically as
     * illustrated below.
     *
     * @image html VertexEnumerator.png
     */
    Vertex& operator()(int i) const {
      assertion1(i>=0,i);
      assertion1(i<TwoPowerD,i);
      assertion1( _vertices[i]!=nullptr,i );
      return *(_vertices[i]);
    }
};


