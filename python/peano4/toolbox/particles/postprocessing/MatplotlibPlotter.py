# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import matplotlib.pyplot as plt


Symbols = [ "o", "s", "<", ">", "^", "v" ]


def scatter_plot_of_data( dataset, filename, column_number ):
  """
  
  Plot data for the particles over time. As we have a scatter plot,
  the y-axis data is determined by column_number, i.e. that is the 
  data column used.
   
  """
  plt.clf()
  keys = dataset.extract_particle_keys()
  print( "dataset hosts {} particles".format(len(keys)) )
  
  symbol_counter = 0
  for particle in keys:
    print( "plot particle {}".format(particle) )
    x_data, y_data = dataset.get_data(particle,column_number)
    if len(x_data)!=len(y_data):
      raise Exception( "x and y data size do not match")
    plt.plot( x_data, y_data, marker=Symbols[ symbol_counter%len(Symbols) ], label="tracer ({},{})".format(particle[0],particle[1]), markevery=1.0/(symbol_counter+3.0) )
    symbol_counter += 1
    
  plt.legend()
  plt.savefig( filename + ".pdf" )
  plt.savefig( filename + ".png" )
  pass