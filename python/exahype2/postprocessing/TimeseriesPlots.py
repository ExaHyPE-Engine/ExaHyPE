# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .PerformanceData import PerformanceData
from .utils           import next_symbol
from .utils           import next_markevery

import matplotlib.pyplot as plt


from enum import Enum
class XAxis(Enum):
  Iterations    = 1
  RealTime      = 2
  SimulatedTime = 3



def plot_runtime_per_time_step(performance_data,label,sum=1):
  x_data = []
  y_data = []
  if sum>1:
    i=0
    while i<len(performance_data.get_time_step_real_time_stamps()) and i<len(performance_data.get_time_per_time_step()):
      if i%sum==0:
        x_data.append(performance_data.get_time_step_real_time_stamps()[i])
        y_data.append(performance_data.get_time_per_time_step()[i])
      else:
        #x_data[-1]  = performance_data.get_time_step_time_stamps()[i]
        y_data[-1] += performance_data.get_time_per_time_step()[i]
      i+=1
      
    x_data = x_data[0:-1]
    y_data = y_data[0:-1]
  else:
    x_data = performance_data.get_time_step_real_time_stamps()
    y_data = performance_data.get_time_per_time_step()
  plt.plot( x_data, y_data, next_symbol(), markevery=next_markevery(performance_data.timesteps()), label=label  )
  
  if len(performance_data.plotting_time_stamp)>0:
    max_time_per_time_step = max(performance_data.get_time_per_time_step())
    min_time_per_time_step = min(performance_data.get_time_per_time_step())
    for i in performance_data.plotting_time_stamp:
      plt.plot( [i,i], [min_time_per_time_step,max_time_per_time_step], "--", color="#000000"  )
    plt.plot( [performance_data.plotting_time_stamp[0],performance_data.plotting_time_stamp[0]], [min_time_per_time_step,max_time_per_time_step], "--", color="#000000", label="plot"  )
    
  plt.xlabel( "Runtime [t]=s" )
  plt.ylabel( "Time per step [t]=s" )



def plot_runtime_against_simulated_time(performance_data,label):
  """
     
  """
  plt.plot( performance_data.get_time_step_real_time_stamps(), performance_data.get_time_step_simulated_time_stamps(), next_symbol(), markevery=next_markevery(performance_data.timesteps()), label=label  )
  plt.xlabel( "Real time [t]=s" )
  plt.ylabel( "Simulated time" )


def plot_time_step_size_per_step(performance_data,label,xaxis,verbose):
  """
  
  use_real_time: Boolean
    If this flag is wrong, the x-axis is "time steps". Otherwise, I use the
    real simulation time. 
     
  """
  if xaxis==XAxis.RealTime:
    if len(performance_data.get_time_step_real_time_stamps()) != len(performance_data.get_time_step_time_step_size()):
      raise Exception( "Size of fields do not match: " + str(len(performance_data.get_time_step_real_time_stamps())) + " vs. " + str(len(performance_data.get_time_step_time_step_size())))
    plt.plot( performance_data.get_time_step_real_time_stamps(), performance_data.get_time_step_time_step_size(), next_symbol(), markevery=next_markevery(performance_data.timesteps()), label=label  )
    plt.xlabel( "Real time [t]=s" )
  elif xaxis==XAxis.Iterations:
    #  performance_data.get_time_step_time_step_size()[-1]/11
    plt.plot( range(0,len(performance_data.get_time_step_time_step_size())), performance_data.get_time_step_time_step_size(), next_symbol(), markevery=next_markevery(performance_data.timesteps()), label=label  )
    plt.xlabel( "Simulation step" )
  elif xaxis==XAxis.SimulatedTime:
    if len(performance_data.get_time_step_simulated_time_stamps()) != len(performance_data.get_time_step_time_step_size()):
      raise Exception( "Size of fields do not match: " + str(len(performance_data.get_time_step_simulated_time_stamps())) + " vs. " + str(len(performance_data.get_time_step_time_step_size())))
    plt.plot( performance_data.get_time_step_simulated_time_stamps(), performance_data.get_time_step_time_step_size(), next_symbol(), markevery=next_markevery(performance_data.timesteps()), label=label  )
    plt.xlabel( "Simulated time" )
  else:
    raise Exception( "enum value not supported" )
  plt.ylabel( "Time step size" )

