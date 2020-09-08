# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .PerformanceData import PerformanceData
import matplotlib.pyplot as plt


def plot_pie_chart_over_simulation_phases(performance_data):
  """
   Pie chart over three phases. The time stepping is highlighted.
   
   performance_data: PerformanceData
  """
  labels  = [
      "grid construction: " + str(performance_data.total_construction_time) + "s", 
      "plotting: " + str(performance_data.total_plotting_time) + "s", 
      "time stepping: " + str(performance_data.total_time_stepping_time) + "s"
      ]
  sizes   = [performance_data.total_construction_time, performance_data.total_plotting_time, performance_data.total_time_stepping_time]
  explode = (0, 0, 0.1)  

  fig1, ax1 = plt.subplots()
  ax1.pie(sizes, explode=explode, labels=labels, autopct='%1.1f%%', shadow=True, startangle=90)
  ax1.axis('equal')  # Equal aspect ratio ensures that pie is drawn as a circle.
