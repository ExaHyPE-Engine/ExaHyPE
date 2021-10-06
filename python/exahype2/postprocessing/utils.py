# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org


Symbols = [ "o", "^", "s", "d", "<", ">", "v", "D"]
internal_symbol_counter = 0


def next_markevery(size_of_data):
  global internal_symbol_counter

  if size_of_data<1:
    raise Exception( "Too few entries to pick a correct markevery value for matplotlib")

  default_number_of_markers = 5
  if size_of_data<default_number_of_markers+1:
    return 1
  else:
    ten_per_plot = int( size_of_data / default_number_of_markers )
    shift        = internal_symbol_counter % ten_per_plot - 1

    return ten_per_plot+shift
  

def next_symbol():
  global internal_symbol_counter
  
  result = "-"
  if internal_symbol_counter >= len(Symbols):
    result = "--"
  if internal_symbol_counter >= 2*len(Symbols):
    result = ":"
  if internal_symbol_counter >= 3*len(Symbols):
    result = ""
  result = result + Symbols[internal_symbol_counter % len(Symbols) ]
  internal_symbol_counter+=1
  internal_symbol_counter = internal_symbol_counter % len(4*Symbols)
  return result


def linear_runtime_trend_line(x_data, y_data):
  """
    We assume this is performance data, i.e. x_data denotes the 
    number of cores or ranks, and y_data are runtimes. We give
    only calibrated y data back. x_data has to be ordered.
  """
  result = []
  for i in range(0,len(x_data)):
    result.append( y_data[0]/x_data[i] )
  return result
