#include "Copy.h"


#include "convert/data/Variable.h"
#include "convert/data/DataSet.h"


tarch::logging::Log convert::filter::Copy::_log( "convert::filter::Copy" );


void convert::filter::Copy::apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) {
  convert::data::Variable  targetVariable( targetSelectorName, inputVariable.dofsPerAxis, inputVariable.unknowns, inputVariable.type, inputVariable.dimensions );

  std::vector<convert::data::PatchData> inputData = dataSet.getData( inputVariable );
  std::vector<convert::data::PatchData> targetData;

  for (auto& p: inputData) {
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

  dataSet.add( targetVariable, targetData );

  logInfo( "apply(...)", "copied " << targetData.size() << " data entries" );
}
