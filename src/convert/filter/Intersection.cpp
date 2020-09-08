#include "Intersection.h"


#include "visualisation/data/Variable.h"
#include "visualisation/data/DataSet.h"


#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"


tarch::logging::Log  visualisation::filter::Intersection::_log( "visualisation::filter::Intersection" );


visualisation::filter::Intersection::Intersection( Strategy strategy ):
  _strategy(strategy) {
}


void visualisation::filter::Intersection::apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) {
  visualisation::data::Variable  targetVariable( targetSelectorName, inputVariable.dofsPerAxis, inputVariable.unknowns, inputVariable.type, inputVariable.dimensions );

  std::vector<visualisation::data::PatchData> inputData = dataSet.getData( inputVariable );
  std::vector<visualisation::data::PatchData> targetData;

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
      visualisation::data::PatchData newPatch(
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
