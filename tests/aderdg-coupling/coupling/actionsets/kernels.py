# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

import jinja2

def reset_troubled_markers():
    return """
    // reset cell and face labels for limiting
    fineGridCell{{SOLVER_NAME}}CellLabel.setTroubledMarker(celldata::{{SOLVER_NAME}}CellLabel::TroubledMarker::ADER);

    for(int d=0; d<DIMENSIONS; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubledMarker(facedata::{{SOLVER_NAME}}FaceLabel::TroubledMarker::ADER);
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d+DIMENSIONS).setTroubledMarker(facedata::{{SOLVER_NAME}}FaceLabel::TroubledMarker::ADER);
    }
"""

def compute_local_min_and_max_and_project():
    return """
    constexpr int numberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
    double localMinPerVariables[numberOfObservables], localMaxPerVariables[numberOfObservables];
    generated::kernels::limiter::findCellLocalMinAndMax(
      fineGridCell{{SOLVER_NAME}}Q_old.data(),
      repositories::{{SOLVER_INSTANCE}},
      localMinPerVariables, localMaxPerVariables
    );

    for(int d=0; d<DIMENSIONS; d++){
        //Projection of cell local min and max
        std::copy_n(localMinPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).data()+numberOfObservables);
        std::copy_n(localMinPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+DIMENSIONS).data());
        std::copy_n(localMaxPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).data()+3*numberOfObservables);
        std::copy_n(localMaxPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+DIMENSIONS).data()+2*numberOfObservables);
    }
"""

def check_troubledness(usePAC, useDMP, useTroubledMarkers):
    templatePAC = """
    isUnTroubled &= generated::kernels::limiter::isPhysicallyAdmissible(
      luh,
      repositories::{{SOLVER_INSTANCE}},
      marker.getX(),
      marker.getH(),
      timeStamp+timeStepSize
    );
    """
    template_DMP = """
    constexpr int numberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
    double boundaryMinPerVariables[2*DIMENSIONS*numberOfObservables];
    double boundaryMaxPerVariables[2*DIMENSIONS*numberOfObservables];

    for(int d=0; d<DIMENSIONS; d++){
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).data(),                                 numberOfObservables, &boundaryMinPerVariables[d*numberOfObservables]);
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+DIMENSIONS).data()+numberOfObservables,  numberOfObservables, &boundaryMinPerVariables[(d+DIMENSIONS)*numberOfObservables]);

        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).data()+2*numberOfObservables,            numberOfObservables, &boundaryMaxPerVariables[d*numberOfObservables]);
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+DIMENSIONS).data()+3*numberOfObservables, numberOfObservables, &boundaryMaxPerVariables[(d+DIMENSIONS)*numberOfObservables]);
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
      fineGridCell{{SOLVER_NAME}}CellLabel.setTroubledMarker(celldata::{{SOLVER_NAME}}CellLabel::TroubledMarker::TROUBLED);
      for(int d=0; d<2*DIMENSIONS; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubledMarker(facedata::{{SOLVER_NAME}}FaceLabel::TroubledMarker::TROUBLED);
      }
    }
    else{
      fineGridCell{{SOLVER_NAME}}CellLabel.setTroubledMarker(celldata::{{SOLVER_NAME}}CellLabel::TroubledMarker::REGULAR);
      for(int d=0; d<2*DIMENSIONS; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubledMarker(facedata::{{SOLVER_NAME}}FaceLabel::TroubledMarker::REGULAR);
      }
    }

"""
    return ("""
    bool isUnTroubled = true;
""" + (templatePAC if usePAC else "")
    + (template_DMP if useDMP else "")
    + (template_set_troubled_markers if useTroubledMarkers else "")
    )