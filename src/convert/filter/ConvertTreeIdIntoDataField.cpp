#include "ConvertTreeIdIntoDataField.h"


#include "visualisation/data/Variable.h"
#include "visualisation/data/DataSet.h"


#include "tarch/la/VectorCompare.h"


#include <map>


tarch::logging::Log visualisation::filter::ConvertTreeIdIntoDataField::_log( "visualisation::filter::ConvertTreeIdIntoDataField" );


visualisation::filter::ConvertTreeIdIntoDataField::ConvertTreeIdIntoDataField() {
}


void visualisation::filter::ConvertTreeIdIntoDataField::apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) {
  visualisation::data::Variable  targetVariable( targetSelectorName, inputVariable.dofsPerAxis, 1, inputVariable.type, inputVariable.dimensions );

  std::vector<visualisation::data::PatchData> inputData = dataSet.getData( inputVariable );
  std::vector<visualisation::data::PatchData> targetData;

  for (auto& p: inputData) {
    visualisation::data::PatchData newPatch(
      inputVariable.dimensions,
      p.offset,
      p.size,
      inputVariable.dofsPerAxis,
      1,
      p.originTree
    );

    int points = tarch::la::aPowI(inputVariable.dimensions,inputVariable.dofsPerAxis);
    for (int i=0; i<points; i++) {
      newPatch.data[i] = p.originTree;
    }

    targetData.push_back( newPatch );
  }

  dataSet.add( targetVariable, targetData );

  logInfo( "apply(...)", "created " << targetData.size() << " data entries" );
}
