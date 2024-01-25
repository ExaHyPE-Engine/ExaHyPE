# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

def create_halo_layer_construction_with_interpolation_for_reconstructed_patch(solver):
  """
  This is a straightforward modification of ReconstructPatchAndApplyFunctor._Template_TouchCellFirstTime_Fill_Halos
  in the blockstructured toolkit.
  """
  return """
    //
    // Bring in the auxiliary patches, i.e., befill halo.
    //
    for(int d=0; d<Dimensions; d++) {{
      logTraceInWith1Argument( "touchCellFirstTime(...)::loopOverFace", d );
      //
      // d-loop over all dimensions except d. The vector k's entry d is set
      // to 0. We start with the left/bottom face, i.e. the one closer to the
      // coordinate system's origin.
      //
      dfore(k,{DOFS_PER_AXIS},d,0) {{
        std::pair<double,double> oldNewWeightsLeft  = ::exahype2::getInterpolationWeights(
          fineGridFaces""" + solver + """FaceLabel(d).getOldTimeStamp(0),
          fineGridFaces""" + solver + """FaceLabel(d).getNewTimeStamp(0),
          fineGridCell""" + solver + """CellLabel.getTimeStamp()
        );
        std::pair<double,double> oldNewWeightsRight = ::exahype2::getInterpolationWeights(
          fineGridFaces""" + solver + """FaceLabel(d+Dimensions).getOldTimeStamp(1),
          fineGridFaces""" + solver + """FaceLabel(d+Dimensions).getNewTimeStamp(1),
          fineGridCell""" + solver + """CellLabel.getTimeStamp()
        );

        for (int i=0; i<{OVERLAP}; i++) {{
          tarch::la::Vector<Dimensions,int> destinationCell = k + tarch::la::Vector<Dimensions,int>({OVERLAP});
          tarch::la::Vector<Dimensions,int> sourceCell      = k;
          destinationCell(d) = i;
          sourceCell(d)      = i;

          int destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          int sourceCellSerialised        = serialisePatchIndex(sourceCell,d);

          for (int unknown=0; unknown<{NUMBER_OF_UNKNOWNS}; unknown++) {{
            oldQWithHalo[destinationCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown]
              = oldNewWeightsLeft.first  * fineGridFaces""" + solver + """QOld(d).value[ sourceCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown ]
              + oldNewWeightsLeft.second * fineGridFaces""" + solver + """QNew(d).value[ sourceCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown ];
            {ASSERTION_WITH_7_ARGUMENTS}(
              {ASSERTION_PREFIX_FOR_HALO} or
              oldQWithHalo[ destinationCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown ]==oldQWithHalo[ destinationCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown ], 
              sourceCell, destinationCell, unknown, d, marker.toString(), _treeNumber, marker.toString()
            );
          }}

          destinationCell(d) = i+{DOFS_PER_AXIS}+{OVERLAP};
          sourceCell(d)      = i+{OVERLAP};

          destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          sourceCellSerialised        = serialisePatchIndex(sourceCell,d);
          for (int unknown=0; unknown<{NUMBER_OF_UNKNOWNS}; unknown++) {{
            oldQWithHalo[destinationCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown]
              = oldNewWeightsLeft.first  * fineGridFaces""" + solver + """QOld(d+Dimensions).value[ sourceCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown ]
              + oldNewWeightsLeft.second * fineGridFaces""" + solver + """QNew(d+Dimensions).value[ sourceCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown ];
            {ASSERTION_WITH_7_ARGUMENTS}(
              {ASSERTION_PREFIX_FOR_HALO} or
              oldQWithHalo[ destinationCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown ]==oldQWithHalo[ destinationCellSerialised*{NUMBER_OF_UNKNOWNS}+unknown ], 
              sourceCell, destinationCell, unknown, d, marker.toString(), _treeNumber, marker.toString()
            );
          }}
        }}
      }}
      logTraceOut("touchCellFirstTime(...)::loopOverFace");
    }}
"""
