def get_body_of_enforceCCZ4constraint():
	return """
  {
    #if Dimensions==2
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    #endif

    #if Dimensions==3
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    #endif

    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""

def get_body_of_adm_constraints(patch_size, number_of_output_variable):
	return """ 
        const int patchSize = """ + str( patch_size ) + """;
        double volumeH = ::exahype2::fv::getVolumeLength(marker.h(),patchSize);

    const int n_a_v="""+str(number_of_output_variable)+""";
    const int overlap=3; //make sure you are using fd4 solver!

    if (not marker.willBeRefined() and repositories::instanceOfCCZ4.getSolverState()!=CCZ4::SolverState::GridConstruction and repositories::instanceOfCCZ4.getSolverState()==CCZ4::SolverState::RungeKuttaSubStep0) {
    //if (not marker.willBeRefined() and repositories::instanceOfCCZ4.getSolverState()!=CCZ4::SolverState::GridConstruction) {
      dfor(cell,patchSize) {
        tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(overlap);

        double gradQ[3*59]={ 0 };

        /*for (int d=0; d<3; d++) {
          tarch::la::Vector<Dimensions,int> leftCell  = currentCell;
          tarch::la::Vector<Dimensions,int> rightCell = currentCell;
          leftCell(d)  -= 1;
          rightCell(d) += 1;
          const int leftCellSerialised  = peano4::utils::dLinearised(leftCell, patchSize + 2*overlap);
          const int rightCellSerialised = peano4::utils::dLinearised(rightCell,patchSize + 2*overlap);
          for(int i=0; i<59; i++) {
            gradQ[d*59+i] = ( oldQWithHalo[rightCellSerialised*(59+n_a_v)+i] - oldQWithHalo[leftCellSerialised*(59+n_a_v)+i] ) / 2.0 / volumeH;
          }
        }*/

        for (int d=0; d<3; d++) {
          tarch::la::Vector<Dimensions,int> leftCell  = currentCell;
          tarch::la::Vector<Dimensions,int> rightCell = currentCell;
          tarch::la::Vector<Dimensions,int> DouleftCell  = currentCell;
          tarch::la::Vector<Dimensions,int> DourightCell = currentCell;
          leftCell(d)  -= 1; DouleftCell(d)    -= 2;
          rightCell(d) += 1; DourightCell(d)  += 2; 
          const int leftCellSerialised  = peano4::utils::dLinearised(leftCell, patchSize + 2*overlap);
          const int rightCellSerialised = peano4::utils::dLinearised(rightCell,patchSize + 2*overlap);
          const int DouleftCellSerialised  = peano4::utils::dLinearised(DouleftCell, patchSize + 2*overlap);
          const int DourightCellSerialised = peano4::utils::dLinearised(DourightCell,patchSize + 2*overlap);
          for(int i=0; i<59; i++) {
            gradQ[d*59+i] = ( -1*oldQWithHalo[DourightCellSerialised*(59+n_a_v)+i] + 8*oldQWithHalo[rightCellSerialised*(59+n_a_v)+i] - 8*oldQWithHalo[leftCellSerialised*(59+n_a_v)+i] + 1*oldQWithHalo[DouleftCellSerialised*(59+n_a_v)+i]) / 12.0 / volumeH;
          }
        }

        const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*overlap);
        
        double constraints[n_a_v]={0};
        admconstraints(constraints, oldQWithHalo+cellSerialised*(59+n_a_v), gradQ);
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables ( 1, patchSize, 0, 59, n_a_v);
        for (int i=0;i<n_a_v;i++){
          fineGridCellCCZ4Q.value[ enumeratorWithAuxiliaryVariables(0,cell,59+i) ] = constraints[i]; 
        }
      }  
    } 
"""

def get_body_of_Psi_Calc(patch_size, number_of_output_variable):
	return """ 
        const int patchSize = """ + str( patch_size ) + """;
        double volumeH = ::exahype2::fv::getVolumeLength(marker.h(),patchSize);

    const int n_a_v="""+str(number_of_output_variable)+""";
    const int overlap=3; //make sure you are using fd4 solver!

    if (not marker.willBeRefined() and repositories::instanceOfCCZ4.getSolverState()!=CCZ4::SolverState::GridConstruction and repositories::instanceOfCCZ4.getSolverState()==CCZ4::SolverState::RungeKuttaSubStep0) {
    //if (not marker.willBeRefined() and repositories::instanceOfCCZ4.getSolverState()!=CCZ4::SolverState::GridConstruction) {
      dfor(cell,patchSize) {
        tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(overlap);

        double gradQ[3*59]={ 0 };

        for (int d=0; d<3; d++) {
          tarch::la::Vector<Dimensions,int> leftCell  = currentCell;
          tarch::la::Vector<Dimensions,int> rightCell = currentCell;
          leftCell(d)  -= 1;
          rightCell(d) += 1;
          const int leftCellSerialised  = peano4::utils::dLinearised(leftCell, patchSize + 2*overlap);
          const int rightCellSerialised = peano4::utils::dLinearised(rightCell,patchSize + 2*overlap);
          for(int i=0; i<59; i++) {
            gradQ[d*59+i] = ( oldQWithHalo[rightCellSerialised*(59+n_a_v)+i] - oldQWithHalo[leftCellSerialised*(59+n_a_v)+i] ) / 2.0 / volumeH;
          }
        }

        const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*overlap);
        
        double Psi4[n_a_v]={0};
        double currentPosition[3]; 
        for (int d=0; d<3; d++) currentPosition[d]=marker.getOffset()(d)+(cell(d)+0.5)*volumeH;
        Psi4Calc(Psi4, oldQWithHalo+cellSerialised*(59+n_a_v), gradQ, currentPosition);
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables   ( 1, patchSize, 0, 59, n_a_v);
        fineGridCellCCZ4Q.value[ enumeratorWithAuxiliaryVariables(0,cell,59+0) ] = Psi4[0]; 
        fineGridCellCCZ4Q.value[ enumeratorWithAuxiliaryVariables(0,cell,59+1) ] = Psi4[1];
      }  
    } 
"""

def get_body_of_SommerfeldCondition(scenario, unknowns, auxiliary_variables):
	if scenario=="single-puncture":
		return """
      ::exahype2::fd::applySommerfeldConditions(
        [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, gridCellH, t, dt, normal );
        },
        [&](
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH
        ) -> void {
          for (int i=0; i<"""+str(unknowns + auxiliary_variables)+"""; i++) {
            Q[i] = 0.0;
          }
          Q[0] = 1.0; Q[3] = 1.0; Q[5] = 1.0;
          //const double r=tarch::la::norm2(faceCentre);
          //Q[16] = 0.5*(1+(1-1.0/2/r)/(1+1.0/2/r)); 
          //Q[54] = 1/(1+1.0/2/r)/(1+1.0/2/r);
          Q[16] = 1.0; Q[54] = 1.0;
        },
        marker.x(),
        marker.h(),
        {{FACE_METADATA_ACCESSOR}}.getOldTimeStamp(marker.getSelectedFaceNumber()<Dimensions ? 1 : 0),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        {{OVERLAP}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        {0.0, 0.0, 0.0},
        fineGridFace{{UNKNOWN_IDENTIFIER}}Old.value,
        fineGridFace{{UNKNOWN_IDENTIFIER}}New.value
      );
"""
	else:
		return """
      ::exahype2::fd::applySommerfeldConditions(
        [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, gridCellH, t, dt, normal );
        },
        [&](
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH
        ) -> void {
          for (int i=0; i<"""+str(unknowns + auxiliary_variables)+"""; i++) {
            Q[i] = 0.0;
          }
          Q[0] = 1.0; Q[3] = 1.0; Q[5] = 1.0;
          Q[16] = 1.0; Q[54] = 1.0;
        },
        marker.x(),
        marker.h(),
        {{FACE_METADATA_ACCESSOR}}.getOldTimeStamp(marker.getSelectedFaceNumber()<Dimensions ? 1 : 0),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        {{OVERLAP}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        {0.0, 0.0, 0.0},
        fineGridFace{{UNKNOWN_IDENTIFIER}}Old.value,
        fineGridFace{{UNKNOWN_IDENTIFIER}}New.value
      );
"""