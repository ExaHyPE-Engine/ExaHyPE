# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from fileinput import filename




class PerformanceDataPoint(object):
  def __init__(self,file_name):
    """
        
    self._start_time_stepping    Is a time stamp
    self._end_time_stepping      Is a time stamp
    self._start_last_time_step   Is a time stamp
    
    """
    self._file_name            = file_name
    self._start_time_stepping  = -1
    self._end_time_stepping    = -1
    self._start_last_time_step = -1
    self._threads              = 1
    self._ranks                = 1
    self._cores_per_node       = 1
    self._number_of_time_steps = 0
    self._total_cell_count     = 1
    self.valid                 = False
    self.parse()
    pass

  def convert_to_seconds(self,data):
    """
   
     Hand in a string in nanoseconds and return double as s
    
    """
    result = float(data)
    return result / 1000 / 1000 / 1000


  def __str__(self, *args, **kwargs):
      return "(ranks=" + str(self._ranks) + ",threads=" + str(self._threads) + \
             ",#time-steps=" + str(self._number_of_time_steps) + \
             ",#cells=" + str(self._total_cell_count) + \
             ",start-time=" + str(self._start_time_stepping) + \
             ",end-time=" + str(self._end_time_stepping) + \
             ",start-last-time-step=" + str(self._start_last_time_step) + \
             ",valid=" + str(self.valid) + \
             ")"
             

  def parse(self):
    file       = open(self._file_name, "r")
    self.valid = True
    try:
      for line in file:
        if "tarch::multicore::Core::configure(...)" in line:
          self._threads = int( line.split( "threads used to" )[1] )
        if "build:" in line:
          if "2d" in line:
            self._d = 2
          if "3d" in line:
            self._d = 3
          if "tbb" in line or "omp" in line:
            self._threads = int( line.split( "threads" )[0].split( "(")[-1] )
            self._cores_per_node = self._threads
          if "build:" in line or "mpi" in line:
            self._ranks = int( line.split( "ranks)" )[0].split( "(")[-1] )
              
        if "run TimeStep" in line and self._start_time_stepping<0:
          self._start_time_stepping = self.convert_to_seconds(line.strip().split( " " )[0])
        elif "run" in line and "rank:0 " in line:
          self._start_last_time_step = self._end_time_stepping
          self._end_time_stepping    = self.convert_to_seconds(line.strip().split( " " )[0])
        
        if "run TimeStep" in line and "rank:0 " in line:
          self._number_of_time_steps += 1
          
        if "global cell count" in line:
          self._total_cell_count = float( line.split( "global cell count=" )[1].split(",")[0] )
    except:
      print( "parsing failed")
      self.valid = False
    
    if self._number_of_time_steps<=0:
      print( "file " + self._file_name + " is invalid as number of time steps equals zero" )
      self.valid = False
      
      
def extract_grid_construction_times(performance_data_points):
  """
     
   Returns a tuple of arrays to be plotted
    
  """
  x_data = []
  y_data = []
    
  for point in performance_data_points:
    x_value = point._threads + (point._ranks-1) * point._cores_per_node
    insert_at_position = 0
    while insert_at_position<len(x_data) and x_data[insert_at_position]<x_value:
      insert_at_position += 1
    x_data.insert( insert_at_position, x_value )
    y_data.insert( insert_at_position, point._start_time_stepping )
    
  return (x_data,y_data)


def extract_times_per_step(performance_data_points,show_data_for_last_time_step,max_cores_per_rank=0):
  """
     
   Returns a tuple of arrays to be plotted
    
  """
  x_data = []
  y_data = []
    
  for point in performance_data_points:
    x_value = point._ranks
    if max_cores_per_rank>0:
      x_value += 0.5*point._threads/max_cores_per_rank
    insert_at_position = 0
    while insert_at_position<len(x_data) and x_data[insert_at_position]<x_value:
      insert_at_position += 1
    x_data.insert( insert_at_position, x_value )
    raw_data = 0
    if show_data_for_last_time_step:
      raw_data = point._end_time_stepping - point._start_last_time_step
    else:
      raw_data = (point._end_time_stepping - point._start_time_stepping)/point._number_of_time_steps
    y_data.insert( insert_at_position, raw_data/point._total_cell_count )
    
  return (x_data,y_data)


