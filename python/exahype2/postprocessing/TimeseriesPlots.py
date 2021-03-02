# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .PerformanceData import PerformanceData
import matplotlib.pyplot as plt


def plot_time_per_time_step(performance_data,label,sum=1):
  x_data = []
  y_data = []
  if sum>1:
    i=0
    while i<len(performance_data.get_time_step_time_stamps()):
      if i%sum==0:
        x_data.append(performance_data.get_time_step_time_stamps()[i])
        y_data.append(performance_data.get_time_per_time_step()[i])
      else:
        x_data[-1]  = performance_data.get_time_step_time_stamps()[i]
        y_data[-1] += performance_data.get_time_per_time_step()[i]
      i+=1
  else:
    x_data = performance_data.get_time_step_time_stamps()
    y_data = performance_data.get_time_per_time_step()
  plt.plot( x_data, y_data, label=label  )
  
  if len(performance_data.plotting_time_stamp)>0:
    max_time_per_time_step = max(performance_data.get_time_per_time_step())
    min_time_per_time_step = min(performance_data.get_time_per_time_step())
    for i in performance_data.plotting_time_stamp:
      plt.plot( [i,i], [min_time_per_time_step,max_time_per_time_step], "--", color="#000000"  )
    plt.plot( [performance_data.plotting_time_stamp[0],performance_data.plotting_time_stamp[0]], [min_time_per_time_step,max_time_per_time_step], "--", color="#000000", label="plot"  )
    
  plt.xlabel( "Simulation time [t]=s" )
  plt.ylabel( "Time per step [t]=s" )
  plt.legend()



def plot_time_step_size_per_time_step(performance_data,label,use_real_time=False):
  """
  
  use_real_time: Boolean
    If this flag is wrong, the x-axis is "time steps". Otherwise, I use the
    real simulation time. 
     
  """
  if use_real_time:
    #if len(performance_data.start_time_step_time_stamp)!=len(performance_data.time_step_size):
    #  raise Exception( "Size of fields do not match: " + str(performance_data) )
    print( "last one " + str( performance_data.get_time_step_time_stamps()[-1] ))
    plt.plot( performance_data.get_time_step_time_stamps(), performance_data.time_step_size, label=label  )
    plt.xlabel( "Simulation time [t]=s" )
  else:
    plt.plot( range(0,len(performance_data.time_step_size)), performance_data.time_step_size, label=label  )
    plt.xlabel( "Time step" )
  plt.ylabel( "Time step size" )
  plt.legend()
