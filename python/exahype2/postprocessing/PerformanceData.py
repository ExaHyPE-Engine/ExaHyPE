# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from fileinput import filename


import re




class PerformanceData(object):
  def __init__(self,file_name,verbose=False):
    """
        
    self._start_time_stepping    Is a time stamp
    self._end_time_stepping      Is a time stamp
    self._start_last_time_step   Is a time stamp
    
    """
    self._file_name            = file_name

    self._threads              = 1
    self._ranks                = 1
    self._cores_per_node       = 1
    
    self.total_construction_time  = 0
    self.total_time_stepping_time = 0
    self.total_plotting_time      = 0

    self.total_construction_steps  = 0
    self.total_time_stepping_steps = 0
    self.total_plotting_steps      = 0

    self._start_time_step_time_stamp = []
    self.plotting_time_stamp  = []

    self._number_of_time_steps = 0
    
    """
     The code tries to parse the machine format first. This format however is only
     used by the command line logger. If you use the other loggers, they tend to 
     dump their data in human readable format; which is less accurate. I thought
     about giving the user the option which format to use or to require her to use
     the command line logger. But that's all too complicated: Why not try to parse
     the most accurate format first. If that fails, we can still use the human 
     readable one. See parse_machine_time_format.
    """
    self._parse_machine_time_format = True

    self.valid                 = False
    self.parse(verbose)
    pass

  def __convert_machine_readable_timestamp_to_seconds(self,data):
    """
   
     Hand in a string in nanoseconds and return double as s
    
    """
    result = float(data)
    return result / 1000 / 1000 / 1000


  def __convert_human_readable_timestamp_to_seconds(self,data):
    """
   
     Hand in a string in the format hh:mm:ss
    
    """
    match = re.findall( r"\d\d\:\d\d\:\d\d", data) # found, match.group() == "123"
    
    hours   = match[0].split(":")[0]
    minutes = match[0].split(":")[1]
    seconds = match[0].split(":")[2]
    return float(hours)*60*60 + float(minutes)*60 + float(seconds)


  def __str__(self, *args, **kwargs):
      return "(ranks=" + str(self._ranks) + ",threads=" + str(self._threads) + \
             ",#time-steps=" + str(self._number_of_time_steps) + \
             ",#cells=" + str(self._total_cell_count) + \
             ",start-time=" + str(self._start_time_stepping) + \
             ",end-time=" + str(self._end_time_stepping) + \
             ",start-last-time-step=" + str(self._start_last_time_step) + \
             ",valid=" + str(self.valid) + \
             ")"

  def __extract_time_stamp_from_run_call(self,line):             
    result = 0
    if self._parse_machine_time_format:
      try:
        result = self.__convert_machine_readable_timestamp_to_seconds(line.strip().split( " " )[0])
      except:
        print( "Warning: Have not found machine-readable log format. Use command line logger for this one. Will continue with human readable format.")
        self._parse_machine_time_format = False
              
    if not self._parse_machine_time_format:
      result = self.__convert_human_readable_timestamp_to_seconds(line.strip().split( " " )[0])
      
    return result


  def parse(self,verbose):
    
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


        if "run" in line and "TimeStep" in line and "rank:0" in line:
          time_stamp = self.__extract_time_stamp_from_run_call(line)
          if verbose:
            print( "started new time step at " + str(time_stamp) + "s" )
          self._start_time_step_time_stamp.append( time_stamp )

        if "run" in line and "PlotSolution" in line and "rank:0" in line:
          time_stamp = self.__extract_time_stamp_from_run_call(line)
          if verbose:
            print( "triggered plot at " + str(time_stamp) + "s" )
          self.plotting_time_stamp.append( time_stamp )
          
        if "terminated successfully" in line and "rank:0" in line:
          time_stamp = self.__extract_time_stamp_from_run_call(line)
          if verbose:
            print( "terminated simulation at " + str(time_stamp) + "s" )
          self._start_time_step_time_stamp.append( time_stamp )
        
        if "run" in line and "TimeStep" in line and "rank:0" in line:
          self._number_of_time_steps += 1

        
        if "initial grid construction:" in line:
          match = re.findall( r"\d+\.\d+s", line)
          self.total_construction_time  = float( match[0].split( "s" )[0] )
          match = re.findall( r"measurements=\d+", line)
          self.total_construction_steps  = int( match[0].split( "=" )[1] )
          print( "grid construction lasts " + str(self.total_construction_time) + " over " + str(self.total_construction_steps) + " steps")
            
        
        if "time stepping:" in line:
          match = re.findall( r"\d+\.\d+s", line)
          self.total_time_stepping_time  = float( match[0].split( "s" )[0] )
          match = re.findall( r"measurements=\d+", line)
          self.total_time_stepping_steps  = int( match[0].split( "=" )[1] )
          print( "time stepping lasts " + str(self.total_time_stepping_time) + " over " + str(self.total_time_stepping_steps) + " steps" )
        
        
        if "plotting:" in line:
          match = re.findall( r"\d+\.\d+s", line)
          self.total_plotting_time  = float( match[0].split( "s" )[0] )
          match = re.findall( r"measurements=\d+", line)
          self.total_plotting_steps  = int( match[0].split( "=" )[1] )
          print( "plotting lasts " + str(self.total_plotting_time) + " over " + str(self.total_plotting_steps) + " steps" )
        
          
    except Exc as ex:
      print( "parsing failed: " + str(ex))
      self.valid = False
    
    if self._number_of_time_steps<=0:
      print( "file " + self._file_name + " is invalid as number of time steps equals zero" )
      self.valid = False
    
    
  def get_time_per_time_step(self):
    result = []
    for i in range(1,len(self._start_time_step_time_stamp)):
      result.append( self._start_time_step_time_stamp[i]-self._start_time_step_time_stamp[i-1] )
    return result


  def get_time_step_time_stamps(self):
    """
     This is not a mere copy, as the last entry in the local set is the end
     of the simulation
    """
    result = [x for x in self._start_time_step_time_stamp]
    result = result[0:-1]
    return result


      
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


