#include "SeparateResolutions.h"


#include "visualisation/data/Variable.h"
#include "visualisation/data/DataSet.h"


#include "tarch/la/VectorCompare.h"


#include <map>


tarch::logging::Log visualisation::filter::SeparateResolutions::_log( "visualisation::filter::SeparateResolutions" );


void visualisation::filter::SeparateResolutions::apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) {
  std::vector<visualisation::data::PatchData> inputData = dataSet.getData( inputVariable );

  std::map<tarch::la::Vector<3,double>, visualisation::data::Variable, tarch::la::VectorCompare<3>  >  targetVariables;
  std::map<visualisation::data::Variable, std::vector<visualisation::data::PatchData> >                targetPatchData;

  logInfo( "apply(...)", "decompose input data set " << inputVariable.name << " into its resolutions" );
  for (auto& p: inputData) {
    tarch::la::Vector<3,double>  key(0.0);
    for (int d=0; d<inputVariable.dimensions; d++) {
      key(d) = p.size[d];
    }

    if ( targetVariables.count(key)==0 ) {
      visualisation::data::Variable  targetVariable(
        targetSelectorName + "-" + std::to_string(key(0)),
		inputVariable.dofsPerAxis, inputVariable.unknowns, inputVariable.type, inputVariable.dimensions
      );
      logInfo( "apply(...)", "create new data set for resolution " << key );
      targetVariables.insert( std::pair< tarch::la::Vector<3,double>, visualisation::data::Variable >(key,targetVariable) );
      targetPatchData.insert( std::pair< visualisation::data::Variable, std::vector<visualisation::data::PatchData> >(targetVariable,std::vector<visualisation::data::PatchData>()) );
    }

    const visualisation::data::Variable targetVariable = targetVariables.at(key);
    visualisation::data::PatchData newPatch(
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
