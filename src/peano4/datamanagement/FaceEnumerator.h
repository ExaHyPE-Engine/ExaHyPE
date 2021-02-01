// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_DATAMANAGEMENT_FACE_ENUMERATOR_H_
#define _PEANO4_DATAMANAGEMENT_FACE_ENUMERATOR_H_


#include "peano4/utils/Globals.h"
#include "tarch/Assertions.h"


namespace peano4 {
  namespace datamanagement {
    template <class Face>
    class FaceEnumerator;
  }
}


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

	Face& operator()(int i) const {
	  assertion(i>=0);
	  assertion(i<TwoTimesD);
	  return *(_faces[i]);
	}
};

#endif
