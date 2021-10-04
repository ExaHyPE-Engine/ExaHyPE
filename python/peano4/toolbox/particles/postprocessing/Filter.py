# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org



def select_tracer_from_subdomain(dataset,dimensions,x_min=None,x_max=None,y_min=None,y_max=None,z_min=None, z_max=None):
  """
  
  dataset: Dataset
    Dataset that has to be studied. We take the available tracer at time
    t and pick out those keys that are within a certain area
    
  Return array of (Int,Int) tuples which you can, for example, add back
  to the dataset via add_filter.
  
  """
  result = []
  
  keys = dataset.extract_particle_keys()
  for key in keys:
    if dimensions==2:
      time, pos_x, pos_y = dataset.get_position(key)
      pos_z = [ 0.0 ]
    else:
      time, pos_x, pos_y, pos_z = flattened_dataset.get_position(key)
    add_to_result = True
    if x_min!=None and x_min>pos_x[0]: add_to_result = False
    if x_max!=None and x_max<pos_x[0]: add_to_result = False
    if y_min!=None and y_min>pos_y[0]: add_to_result = False
    if y_max!=None and y_max<pos_y[0]: add_to_result = False
    if z_min!=None and z_min>pos_z[0]: add_to_result = False
    if z_max!=None and z_max<pos_z[0]: add_to_result = False
    if add_to_result:
      result.append(key)
  print( "dataset hosts {} particles of which we selected {}: {}".format(len(keys),len(result),str(result)) )
  return result
      
  
def get_initial_position_labels(dataset,dimensions,keys=None):
  """
  
  Constructs a list of the initial positions of the particles which 
  you can use hand over the matplotlib, e.g.
  
  """  
  result = []
  if keys==None:
    keys = dataset.extract_particle_keys()
  for key in keys:
    if dimensions==2:
      time, pos_x, pos_y = dataset.get_position(key)
      result.append( "$({},{})^T$".format(pos_x[0],pos_y[0]))
    else:
      time, pos_x, pos_y, pos_z = dataset.get_position(key)
      result.append( "$({},{},{})^T$".format(pos_x[0],pos_y[0],pos_z[0]))
  return result


