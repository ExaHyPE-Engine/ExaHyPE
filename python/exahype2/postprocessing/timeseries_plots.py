# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .PerformanceData import PerformanceData
import matplotlib.pyplot as plt


def plot_time_per_time_step(performance_data):
  plt.plot( performance_data.get_time_step_time_stamps(), performance_data.get_time_per_time_step(), label="time per time step"  )
  
  if len(performance_data.plotting_time_stamp)>0:
    max_time_per_time_step = max(performance_data.get_time_per_time_step())
    for i in performance_data.plotting_time_stamp:
      plt.plot( [i,i], [0,max_time_per_time_step], "--", color="#000000"  )
    plt.plot( [performance_data.plotting_time_stamp[0],performance_data.plotting_time_stamp[0]], [0,max_time_per_time_step], "--", color="#000000", label="plot"  )
    
  plt.xlabel( "Simulation time [t]=s" )
  plt.ylabel( "Time per step [t]=s" )
  plt.legend()
