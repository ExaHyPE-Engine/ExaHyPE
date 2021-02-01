#include "ConvertTreeIdIntoDataField.h"


#include "convert/data/Variable.h"
#include "convert/data/DataSet.h"


#include "tarch/la/VectorCompare.h"


#include <map>


tarch::logging::Log convert::filter::ConvertTreeIdIntoDataField::_log( "convert::filter::ConvertTreeIdIntoDataField" );


convert::filter::ConvertTreeIdIntoDataField::ConvertTreeIdIntoDataField() {
}


void convert::filter::ConvertTreeIdIntoDataField::apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) {
  convert::data::Variable  targetVariable( targetSelectorName, inputVariable.dofsPerAxis, 1, inputVariable.type, inputVariable.dimensions );

  std::vector<convert::data::PatchData> inputData = dataSet.getData( inputVariable );
  std::vector<convert::data::PatchData> targetData;

  for (auto& p: inputData) {
    convert::data::PatchData newPatch(
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
