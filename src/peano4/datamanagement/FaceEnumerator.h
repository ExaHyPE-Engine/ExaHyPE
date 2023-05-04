// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/utils/Globals.h"
#include "tarch/Assertions.h"


namespace peano4 {
  namespace datamanagement {
    template <class Face>
    class FaceEnumerator;
  }
}


/**
 * Enumerator over an array of faces
 *
 * Whenever you hit a cell, you get an array over faces, and you have to access them and know
 * about their arrangement. Faces are enumerated along their normal with the one face closer
 * to the left/bottom/front first. So let there be three coordinate axes x0, x1, x2. The face
 * with the normal parallel to x0 on the left side of the cell (if you assume the cell to be
 * the unit cube, it is the face that runs through the origin) is face no 0. The one with a
 * normal parallel to x1 is face no 1. After all the faces which run through the origin in
 * our example have been enumerated, we enumerate those guys parallel to them in the same order.
 *
 * @image html FaceEnumerator.png
 *
 * This enumeration scheme is a natural extension of the lexicographic enumeration of the
 * VertexEnumerator, where the axes enumeration induces a vertex enumeration. It also is an
 * easy enumeration to extend to arbitrary dimensions.
 *
 * @see peano4::datamanagement::VertexEnumerator for the same discussion for vertices.
 */
template <class Face>
class peano4::datamanagement::FaceEnumerator {
  private:
    Face* _faces[ TwoTimesD ];

  public:
    /**
     * Usually is only used by the observers, i.e. users should not interact
     * with this routine.
     */
    FaceEnumerator() {
      #if PeanoDebug>0
      for (int i=0; i<TwoTimesD; i++) {
    	_faces[i] = nullptr;
      }
      #endif
    }


    /**
     * Face enumerator with standard ordering of faces within a consecutive
     * array.
     */
    FaceEnumerator(Face* firstFace) {
      assertion(firstFace!=nullptr);
      for (int i=0; i<TwoTimesD; i++) {
        _faces[i] = firstFace+i;
      }
    }

	/**
	 * Usually is only used by the observers, i.e. users should not interact
	 * with this routine.
	 */
	void setPointer(int i, Face* value) {
	  assertion(i>=0);
	  assertion(i<TwoTimesD);
	  assertion(value!=nullptr);
	  _faces[i] = value;
	}

	/**
	 * Get a certain face.
	 */
	Face& operator()(int i) const {
	  assertion1(i>=0,i);
	  assertion1(i<TwoTimesD,i);
	  assertion1( _faces[i]!=nullptr,i );
	  return *(_faces[i]);
	}
};

