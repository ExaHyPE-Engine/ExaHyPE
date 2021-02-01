#ifndef _CONVERT_DATA_PATCH_DATA_H_
#define _CONVERT_DATA_PATCH_DATA_H_

#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "Variable.h"


namespace convert {
  namespace data {
    class PatchData;
  }
}


#define MaxDimensions 3


/**
 * A patch is a square or cube in the domain. It can hold variables which have
 * been identified before via a Variable entry.
 */
class convert::data::PatchData {
  public:
    PatchData(int dimensions, double* offset_, double* size_, int dofsPerAxis, int unknownsPerDoF, int originTree);

    bool samePatch( const PatchData& otherPatch );
    bool overlaps( const PatchData& otherPatch );

    /**
     * Free internal dynamic data types if there's still data left.
     */
    void free();

    const int dimensions;

    /*
     * offset of the patch in each dimension
     */
    double offset[MaxDimensions];

    /**
     * size of the patch in each dimension
     */
    double size[MaxDimensions];

    /**
     * Mapping from variables onto the actual data. We store data as AoS. It
     * is furthermore ordered lexicographically, i.e. the x index is the
     * fastest running index.
     */
    double* data;

    /**
     * Tree that has originally written this piece of data. Or -1 if data does not result
     * directly from any tree.
     */
    int originTree;

    void copyData( const PatchData& otherData, int dofsPerAxis, int unknownsPerDoF );
};

#endif
