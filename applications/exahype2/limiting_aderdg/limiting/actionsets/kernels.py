# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

import jinja2

def reset_troubled_markers():
    return """
    // reset cell and face labels for limiting
    fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::ADER);

    for(int d=0; d<Dimensions; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::ADER);
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::ADER);
    }
"""

def compute_local_min_and_max_and_project():
    return """
    constexpr int numberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
    double localMinPerVariables[numberOfObservables], localMaxPerVariables[numberOfObservables];
    generated::kernels::limiter::findCellLocalMinAndMax(
      fineGridCell{{SOLVER_NAME}}Q_old.value,
      repositories::{{SOLVER_INSTANCE}},
      localMinPerVariables, localMaxPerVariables
    );

    for(int d=0; d<Dimensions; d++){
        //Projection of cell local min and max
        std::copy_n(localMinPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+numberOfObservables);
        std::copy_n(localMinPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value);
        std::copy_n(localMaxPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+3*numberOfObservables);
        std::copy_n(localMaxPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+2*numberOfObservables);
    }
"""

def check_troubledness(usePAC, useDMP, useTroubledMarkers):
    templatePAC = """
    isUnTroubled &= generated::kernels::limiter::isPhysicallyAdmissible(
      luh,
      repositories::{{SOLVER_INSTANCE}},
      marker.x(),
      marker.h(),
      timeStamp+timeStepSize
    );
    """
    template_DMP = """
    constexpr int numberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
    double boundaryMinPerVariables[2*Dimensions*numberOfObservables];
    double boundaryMaxPerVariables[2*Dimensions*numberOfObservables];

    for(int d=0; d<Dimensions; d++){
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value,                                 numberOfObservables, &boundaryMinPerVariables[d*numberOfObservables]);
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+numberOfObservables,  numberOfObservables, &boundaryMinPerVariables[(d+Dimensions)*numberOfObservables]);

        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+2*numberOfObservables,            numberOfObservables, &boundaryMaxPerVariables[d*numberOfObservables]);
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+3*numberOfObservables, numberOfObservables, &boundaryMaxPerVariables[(d+Dimensions)*numberOfObservables]);
    }

    isUnTroubled &= generated::kernels::limiter::discreteMaximumPrincipleAndMinAndMaxSearch(
      luh,
      repositories::{{SOLVER_INSTANCE}},
      repositories::{{SOLVER_INSTANCE}}.RelaxationParameter, //const double relaxationParameter
      repositories::{{SOLVER_INSTANCE}}.DifferencesScaling, //const double differenceScaling
      boundaryMinPerVariables, 
      boundaryMaxPerVariables
    );
"""
    template_set_troubled_markers = """
    if(!isUnTroubled){
      repositories::{{SOLVER_INSTANCE}}.addTroubledCell();
      fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED);
      for(int d=0; d<2*Dimensions; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED);
      }
    }
    else{
      fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::REGULAR);
      for(int d=0; d<2*Dimensions; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::REGULAR);
      }
    }

"""
    return ("""
    bool isUnTroubled = true;
""" + (templatePAC if usePAC else "")
    + (template_DMP if useDMP else "")
    + (template_set_troubled_markers if useTroubledMarkers else "")
    )