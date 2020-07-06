# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet



def get_face_overlap_merge_implementation(patch_overlap):
  d = {
    "OVERLAP": patch_overlap.dim[0]/2,
    "DOFS_PER_AXIS": patch_overlap.dim[1],
    "UNKNOWNS": patch_overlap.no_of_unknowns
  }
  
  
  template = """
  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {{
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {{
      result += overlapCell(d) * base;
      if (d==normal) {{
        base *= {OVERLAP}*2;
      }}
      else {{
        base *= {DOFS_PER_AXIS};
      }}
    }}
    return result;
  }};
  
  //
  // The face merges are always called from a cell. So it is one particular
  // cell that loads a particular face for the first time and thus 
  //
  //
  #if PeanoDebug>0
  tarch::logging::Log _log( "Q" );
  logDebug( "merge()", "merge at x=" << marker.toString() << " with an outer normal of " << marker.outerNormal() ); 
  #endif
  
  const int faceNormal = marker.getSelectedFaceNumber() % Dimensions;
  dfore(i,{DOFS_PER_AXIS},faceNormal,0) {{
    for (int j=0; j<{OVERLAP}; j++) {{
      tarch::la::Vector<Dimensions,int> volume = i;
      volume(faceNormal) += marker.outerNormal()(faceNormal)>0 ? j + {OVERLAP} : j;
      
      int volumeSerialised = serialisePatchIndex(volume, faceNormal);
      for (int k=0; k<{UNKNOWNS}; k++) {{
        value[volumeSerialised*{UNKNOWNS}+k] = neighbour.value[volumeSerialised*{UNKNOWNS}+k];
      }}
      
      // This should be non-critical assertion, but this assertion is only
      // generically available in ExaHyPE 2, so I comment the assertion out
      // here.
      // assertion(value[volumeSerialised]==value[volumeSerialised]);
    }}
  }}
"""

  return template.format(**d)
