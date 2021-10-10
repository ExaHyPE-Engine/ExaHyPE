# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import matplotlib.pyplot as plt


Symbols = [ "o", "s", "<", ">", "^", "v" ]


def seismogram( dataset, column_number, labels, keys=None, marker_offset=0, total_marker_count=7, marker_size=6):
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
    #
    # First entry is offset, the second one the actual frequency
    #
    #keys.index(particle)
    spacing     = max(1,int(len(x_data)/total_marker_count))
    offset      = int(spacing/len(keys))*keys.index(particle)
    mark_every  = (offset,spacing)

    if len(x_data)!=len(y_data):
      raise Exception( "x and y data size do not match")
    if labels==None:
      plt.plot( x_data, y_data, marker=Symbols[ symbol_counter%len(Symbols) ], markevery=mark_every, markersize=marker_size )
    else:
      plt.plot( x_data, y_data, marker=Symbols[ symbol_counter%len(Symbols) ], label=labels[symbol_counter], markevery=mark_every, markersize=marker_size )
    symbol_counter += 1
    
  pass