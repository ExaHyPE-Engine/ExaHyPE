#include "Intersection.h"


#include "convert/data/Variable.h"
#include "convert/data/DataSet.h"


#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"


tarch::logging::Log  convert::filter::Intersection::_log( "convert::filter::Intersection" );


convert::filter::Intersection::Intersection( Strategy strategy ):
  _strategy(strategy) {
}


void convert::filter::Intersection::apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) {
  convert::data::Variable  targetVariable( targetSelectorName, inputVariable.dofsPerAxis, inputVariable.unknowns, inputVariable.type, inputVariable.dimensions );

  std::vector<convert::data::PatchData> inputData = dataSet.getData( inputVariable );
  std::vector<convert::data::PatchData> targetData;

  for (auto p: inputData) {
    bool memberOfFineGrid = true;
    for (auto pp: inputData) {
      if (
        not p.samePatch(pp)
        and
        p.overlaps(pp)
      ) {
        bool allSizesSmaller = true;
        for (int d=0; d<inputVariable.dimensions; d++)
          allSizesSmaller &= tarch::la::smaller( pp.size[d], p.size[d] );

        if (allSizesSmaller)
    	  memberOfFineGrid = false;
      }
    }

    if (memberOfFineGrid) {
      convert::data::PatchData newPatch(
        inputVariable.dimensions,
        p.offset,
        p.size,
        inputVariable.dofsPerAxis,
        inputVariable.unknowns,
        p.originTree
      );

      newPatch.copyData(p,inputVariable.dofsPerAxis,inputVariable.unknowns);

      targetData.push_back( newPatch );
    }
  }

  dataSet.add( targetVariable, targetData );

  logInfo( "apply(...)", "extracted " << targetData.size() << " patches from " << inputData.size() << " that belong to the fine grid" );
}
