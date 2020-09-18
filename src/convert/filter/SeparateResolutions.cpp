#include "SeparateResolutions.h"


#include "convert/data/Variable.h"
#include "convert/data/DataSet.h"


#include "tarch/la/VectorCompare.h"


#include <map>


tarch::logging::Log convert::filter::SeparateResolutions::_log( "convert::filter::SeparateResolutions" );


void convert::filter::SeparateResolutions::apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) {
  std::vector<convert::data::PatchData> inputData = dataSet.getData( inputVariable );

  std::map<tarch::la::Vector<3,double>, convert::data::Variable, tarch::la::VectorCompare<3>  >  targetVariables;
  std::map<convert::data::Variable, std::vector<convert::data::PatchData> >                targetPatchData;

  logInfo( "apply(...)", "decompose input data set " << inputVariable.name << " into its resolutions" );
  for (auto& p: inputData) {
    tarch::la::Vector<3,double>  key(0.0);
    for (int d=0; d<inputVariable.dimensions; d++) {
      key(d) = p.size[d];
    }

    if ( targetVariables.count(key)==0 ) {
      convert::data::Variable  targetVariable(
        targetSelectorName + "-" + std::to_string(key(0)),
		inputVariable.dofsPerAxis, inputVariable.unknowns, inputVariable.type, inputVariable.dimensions
      );
      logInfo( "apply(...)", "create new data set for resolution " << key );
      targetVariables.insert( std::pair< tarch::la::Vector<3,double>, convert::data::Variable >(key,targetVariable) );
      targetPatchData.insert( std::pair< convert::data::Variable, std::vector<convert::data::PatchData> >(targetVariable,std::vector<convert::data::PatchData>()) );
    }

    const convert::data::Variable targetVariable = targetVariables.at(key);
    convert::data::PatchData newPatch(
      inputVariable.dimensions,
      p.offset,
      p.size,
      inputVariable.dofsPerAxis,inputVariable.unknowns,
      p.originTree
    );
    newPatch.copyData(p,inputVariable.dofsPerAxis,inputVariable.unknowns);
    targetPatchData[ targetVariable ].push_back( newPatch );
  }


  for (auto& p: targetPatchData) {
    dataSet.add( p.first, p.second );
    logInfo( "apply(...)", "added " << p.second.size() << " data entries for identifier " << p.first.name << " to data set" );
  }
}
