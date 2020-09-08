#include "SelectValue.h"


#include "visualisation/data/Variable.h"
#include "visualisation/data/DataSet.h"


tarch::logging::Log visualisation::filter::SelectValue::_log( "visualisation::filter::SelectValue" );


visualisation::filter::SelectValue::SelectValue(double from, double to):
  _from(from),
  _to(to) {
  logInfo( "SelectValue(double,double)", "extract all patches which hold data between " << from << " and " << to );
}


void visualisation::filter::SelectValue::apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) {
  visualisation::data::Variable  targetVariable( targetSelectorName, inputVariable.dofsPerAxis, inputVariable.unknowns, inputVariable.type, inputVariable.dimensions );

  std::vector<visualisation::data::PatchData> inputData = dataSet.getData( inputVariable );
  std::vector<visualisation::data::PatchData> targetData;

  for (auto& p: inputData) {
    bool holdsData = false;
    for (int i=0; i<inputVariable.getTotalNumberOfDofsPerPatch(); i++) {
      holdsData |= (
        p.data[i] >= _from and p.data[i] <= _to
      );
    }

    if (holdsData) {
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

  logInfo( "apply(...)", "selected (and copied) " << targetData.size() << " out of " << inputData.size() << " data entries (patches) matching range constraint" );
}
