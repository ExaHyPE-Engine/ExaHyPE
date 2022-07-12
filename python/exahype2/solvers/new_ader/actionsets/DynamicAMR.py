# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2
import jinja2


from .AbstractAderDGActionSet import AbstractAderDGActionSet


class DynamicAMR( AbstractAderDGActionSet ):
  """
  
   The ExaHyPE 2 Finite Volume handling of (dynamically) adaptive meshes
   
   This class is basically a plain copy of the DynamicAMR action set
   from the toolbox. However, we have to ensure that we also set the
   update markers appropriately such that restricted data are taken 
   into account. This action set thus has to be studied in combination 
   with the ProjectPatchOntoFaces action set which is very similar and
   also enriches the toolbox version by this marker aspect.
  
  """
    
    
  def __init__(self,solver, guard):
    """
    
     # Interpolation
     
     Don't interpolate in initialisation. If you have a parallel run with AMR, then some 
     boundary data has not been received yet and your face data thus is not initialised 
     at all.

     # Restriction
     
     In contrast to the interpolation, it is absolutely key that we restrict already in
     the initialisation. If we have a cell that is adjacent to a refined cell, then this
     cell requires proper face data for the subsequent first time step. So we have to 
     restrict in the initialisation even if there's no update yet - just to get the 
     coarse representation of fine hanging faces initialised.
          
     # Restriction of destructed faces
     
     A destructed face has to restrict its data: We've already restricted the cells,
     but now we also have to restrict the faces, as faces span time spans. The cell
     is only a snapshot, so we have to manually restrict.
     
     It is important to also disable the projection in return. The projection onto the
     faces happens in leaveCell. However, if you have a face in-between two 3x3 patches,
     then you will have a destruction of the left patch (and a corresponding cell restriction),
     the coarse cell will then project its data onto the face, you go down on the other
     side and now the face will be destroyed. If you restrict now, you overwrite the 
     projected data with some (old) data, and you will get an inconsistent face data.
     However, you always have to restrict both sides of persistent face when it is 
     destroyed. If you only restricted the inner part, you'd run into situations where
     a 3x3 patch is coarsened, but its neighbour remains refined. That is, logically the
     face transitions from a persistent one to a hanging face. When you go through the 
     mesh next time, you'll need, on the coarse level, some valid data. So we have to 
     restrict both sides.
     
     There's still a inconsistency here. To have really consistent data, we'd have to 
     average 3^{d-1} cells when we project them onto the face, such that the cell-restricted
     data followed by a (single cell) projection gives the same result as a projection 
     first and then a face restriction. I neglect this fact here.
          
    """
    super(DynamicAMR,self).__init__(solver)      

    
  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super(DynamicAMR,self).get_includes() + """
#include <cstring>
"""    
