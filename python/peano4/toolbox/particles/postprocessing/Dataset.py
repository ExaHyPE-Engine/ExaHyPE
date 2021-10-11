# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


import pandas
import glob


def load_file_sequence(file_prefix,dimensions):
  """
  
  Load all files that start with file_prefix and merge them into one 
  instance of Dataset.
  
  """
  result = None
  filtered_files = glob.glob( file_prefix + "*.csv" )
  for filtered_file in filtered_files:
    new_dataset = Dataset()
    new_dataset.parse(filtered_file,dimensions)
    if new_dataset.valid and result==None:
      result = new_dataset
    elif new_dataset.valid:
      result.append( new_dataset )
    else:
      print( "ERROR: file " + filtered_file + " was not a valid Peano 4 particle database file" )
  return result
    
    
def assert_ascending(data):
  for i in range(0,len(data)-1):
    if data[i]>data[i+1]:
      raise Exception( "not in ascending order at position {}: {} vs {}".format(i,data[i],data[i+1]) )
  

class Dataset(object):
  def __init__(self):
    self.valid                   = False
    self.number_of_data_columns_ = -1
    self.dimensions_             = -1
    self.data_                   = None

  __Tree_Number_Identifier                = "tree_number"
  __Tree_Local_Particle_Number_Identifier = "particle_number"
    
    
  def _make_column_names(self):
    colnames = [ self.__Tree_Number_Identifier, self.__Tree_Local_Particle_Number_Identifier, "t"]
    if self.dimensions_==2:
      colnames += [ "x", "y" ]
    else:
      colnames += [ "x", "y", "z" ]
    for i in range(self.number_of_data_columns_): colnames.append("Q{}".format(i))
    return colnames


  def _parse_number_of_entries(self,filename):
    read_header  = open(filename, "r" )
    current_line = read_header.readline()
    while "#" in current_line:
      current_line = read_header.readline()
    # Assume first row without # is header, so read first data row
    current_line = read_header.readline()
    self.number_of_data_columns_ = current_line.count( "," ) + 1 - self.dimensions_ - 2 - 1

    
  def parse(self,filename,dimensions):
    print( "" )
    print( "-------------------------------------------------------")
    print( "parse file " + filename )
    print( "-------------------------------------------------------")

    self.dimensions_   = dimensions
    self.valid         = True
    
    self._parse_number_of_entries(filename)
    print( "file hosts {} data entries per particle snapshot".format(self.number_of_data_columns_) ) 
    self.data_ = pandas.read_csv(filename,names=self._make_column_names(), skiprows=1)

 
  def append(self,other_dataset):
    if self.dimensions_!=other_dataset.dimensions_:
      raise Exception( "dimensions of datasets do not match")
    if self.number_of_data_columns_!=other_dataset.number_of_data_columns_:
      raise Exception( "number of data columns per particle do not match")
        
    self.data_ = pandas.concat([self.data_, other_dataset.data_])
    

  def extract_particle_keys(self):
    """
    
    Particles are identified by integer tuples and not by a single 
    number. This operation returns a set of thes tuples.
    
    """  
    return set(zip(self.data_[self.__Tree_Number_Identifier],self.data_[self.__Tree_Local_Particle_Number_Identifier])) 


  def get_data(self,particle,column_number,clean_up=1e-3):
    """
    
    particle: (Int,Int)
      Tuple identifying a particular particle
      
    column_number: Int
      Integer with the data column requested
      
    Returns two lists: A lits of time stamps and a list of data entries
    
    """
    data_frame = pandas.DataFrame(self.data_)
    
    filtered_data = data_frame.loc[    data_frame[self.__Tree_Number_Identifier]==particle[0] ]
    filtered_data = filtered_data.loc[ filtered_data[self.__Tree_Local_Particle_Number_Identifier]==particle[1] ]
    
    filtered_data = filtered_data.sort_values( by="t" )
    
    result = list(filtered_data.t), list(filtered_data["Q{}".format(column_number)])
    assert_ascending(result[0])
    if clean_up!=0.0 and result[0][1]-result[0][0]<=clean_up:
      result = result[0][1:],result[1][1:]
    return result


  def get_position(self,particle):
    """
    
    Pass in a particle identified by a two-tuple and return d+1 arrays.
    The first result array is the timestamps, then we have d arrays with 
    the particle locations.
    
    """
    data_frame = pandas.DataFrame(self.data_)
    
    filtered_data = data_frame.loc[    data_frame[self.__Tree_Number_Identifier]==particle[0] ]
    filtered_data = filtered_data.loc[ filtered_data[self.__Tree_Local_Particle_Number_Identifier]==particle[1] ]
    
    filtered_data = filtered_data.sort_values( by="t" )
    
    if self.dimensions_==2:
      return list(filtered_data.t), list(filtered_data["x"]), list(filtered_data["y"])
    else:
      return list(filtered_data.t), list(filtered_data["x"]), list(filtered_data["y"]), list(filtered_data["z"])
