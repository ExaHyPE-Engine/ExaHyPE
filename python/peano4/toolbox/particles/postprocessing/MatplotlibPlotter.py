# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import matplotlib.pyplot as plt


Symbols = [ "o", "s", "<", ">", "^", "v" ]


def seismogram( dataset, column_number, labels, keys=None, marker_offset=0 ):
  """
  
  Plot data for the particles over time. As we have a scatter plot,
  the y-axis data is determined by column_number, i.e. that is the 
  data column used.
  
  legend: [String]
    Use None if you don't want a legend.
    
  """
  if keys==None:
    keys = dataset.extract_particle_keys()
  print( "dataset hosts {} particles".format(len(keys)) )
  
  symbol_counter = 0
  for particle in keys:
    print( "plot particle {}".format(particle) )
    x_data, y_data = dataset.get_data(particle,column_number)
    if len(x_data)!=len(y_data):
      raise Exception( "x and y data size do not match")
    if labels==None:
      plt.plot( x_data, y_data, marker=Symbols[ symbol_counter%len(Symbols) ], markevery=1.0/(symbol_counter+3.0+marker_offset) )
    else:
      plt.plot( x_data, y_data, marker=Symbols[ symbol_counter%len(Symbols) ], label=labels[symbol_counter], markevery=1.0/(symbol_counter+3.0+marker_offset) )
    symbol_counter += 1
    
  pass