#include "Copy.h"


#include "visualisation/data/Variable.h"
#include "visualisation/data/DataSet.h"


tarch::logging::Log visualisation::filter::Copy::_log( "visualisation::filter::Copy" );


void visualisation::filter::Copy::apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) {
  visualisation::data::Variable  targetVariable( targetSelectorName, inputVariable.dofsPerAxis, inputVariable.unknowns, inputVariable.type, inputVariable.dimensions );

  std::vector<visualisation::data::PatchData> inputData = dataSet.getData( inputVariable );
  std::vector<visualisation::data::PatchData> targetData;

  for (auto& p: inputData) {
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

  dataSet.add( targetVariable, targetData );

  logInfo( "apply(...)", "copied " << targetData.size() << " data entries" );
}
