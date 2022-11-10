import os, sys
import argparse
import matplotlib.pyplot as plt

print("""

ExaHyPE 2 - Kernel Benchmarks Bar Plotting

@author 2022 Tobias Weinzierl, Mario Wille

""")

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Kernel Benchmarks Bar Plotting Script')
parser.add_argument("-d",   "--dim",                 dest="dim",                 type=int, default=2,     help="Number of space dimensions")
parser.add_argument("-ps",  "--patch-size",          dest="patch_size",          type=int, default=17,    help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-f",   "--file",                dest="file",                default="",              help="Specify to convert an output file into a plot.")
parser.add_argument("-v",   "--verbose",             dest="verbose",             action="store_true",     help="Verbose mode.")
parser.add_argument("-m",   "--max-fused",           dest="max_fused",           type=int, default=65536, help="Maximum number of fused patches that are studied.")
parser.add_argument("-nv",  "--number-of-variants",  dest="number_of_variants",  type=int, default=-1,    help="Number of kernel variants per measurement (guides the bar width).")
args = parser.parse_args()

algorithms = [
  "patch-wise",
  "batched",
]
#
# Leading space is important to discriminate between SoA and AoSoA
#
data_structures = [
  " AoSoA",
  " AoS",
  " SoA",
  " AoSoA, map",
  " AoS, map",
  " SoA, map",
  " AoSoA, user-managed-memory",
  " AoS, user-managed-memory",
  " SoA, user-managed-memory",
  " in-situ"
]
machine_implementations = [
  "host with heap",
  "device 0 with omp",
]

for machine_implementation in machine_implementations:
  plt.clf()
  number_of_bars_written = 0
  print( "Process machine implementation '{}'".format(machine_implementation) )
  plotted_ticks = False
  for algorithm in algorithms:
    for data_structure in data_structures:
      if args.verbose:
        print( "Open {} for algorithm {}, data structure {} and machine implementation {}".format(args.file,algorithm,data_structure,machine_implementation) )
      input_file = open( args.file, "r" )
      x_labels = []
      y_values = []
      fused_cells_or_patches = 0
      for line in input_file:
        if "Number of patches fused:" in line:
          fused_cells_or_patches = int(line.split(":")[-1])
          if fused_cells_or_patches<=args.max_fused: 
            x_labels.append( fused_cells_or_patches )
        new_measurement = "stateless kernels" in line
        new_measurement = new_measurement and algorithm in line
        if data_structure=="SoA" or data_structure=="AoS":
          new_measurement = new_measurement and data_structure in line
          new_measurement = new_measurement and not "AoSoA" in line
        else:
          new_measurement = new_measurement and data_structure in line
        new_measurement = new_measurement and machine_implementation in line
        if new_measurement and fused_cells_or_patches<=args.max_fused:
          y_values.append( float(line.split("/")[-1]) ) 
      if len(y_values) > 0:
        if args.number_of_variants<=0:
          bar_width = 1.0/(len(algorithms)*len(data_structures)+1)
        else:
          bar_width = 1.0/(args.number_of_variants+1)
        x_values = [i+number_of_bars_written*bar_width for i in range(0,len(y_values))]
        #if len(x_labels)>len(y_values):
        #  print( "Shorten x range for metric {} x {}".format(algorithm,data_structure) )
        #  x_values = x_values[0:len(y_values)-1]
        if len(x_values)!=len(y_values):
          print( "Cannot plot data for {}x{} as x-len is {} vs y-len {}".format(algorithm,data_structure,len(x_values),len(y_values)) )
        else:
          plt.bar( x_values, y_values, width=bar_width, label=algorithm + ", " + data_structure )
          number_of_bars_written += 1 
      if len(y_values)>0 and not plotted_ticks:
        plotted_ticks = True
        plt.xticks(range(0,len(x_labels)), x_labels)
    plt.yscale( "log" )
    plt.legend()
    plt.xlabel( "Assembled patches")
    plt.ylabel( "Time per FV update [s]")
    plt.title( "Size per patch: $ {}^{} $".format( args.patch_size, args.dim) )
    if number_of_bars_written>0:
      plt.savefig( args.file.replace(".","_") + "_" + machine_implementation.replace(" ","_").replace(",","") + ".pdf" )
    else:
      print( "No (valid) data found for machine implementation '{}'".format(machine_implementation) )

print("Done")
