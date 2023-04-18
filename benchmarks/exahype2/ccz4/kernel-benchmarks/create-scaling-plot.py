import os
import sys
import argparse

from enum import Enum

import matplotlib.pyplot as plt
import numpy as np

import tikzplotlib

kernel_variants = {
  "patch-wise":"patch-wise",
  "batched":"batched",
}

data_structures = {
  "AoSoA":"AoSoA",
  "AoS":"AoS",
  "SoA":"SoA",
}

machine_implementations = {
  "host":"host",
  "device":"device",
}

class PlotType(Enum):
  Bar = 1
  Line = 2
plot_types = {
  "bar":PlotType.Bar,
  "line":PlotType.Line,
}

class ScalingType(Enum):
  Weak = 1
  Strong = 2
scaling_types = {
  "weak":ScalingType.Weak,
  "strong":ScalingType.Strong,
}

class MeasurementType(Enum):
  TimeToSolution = 1
  Throughput = 2
measurement_types = {
  "time-to-solution":MeasurementType.TimeToSolution,
  "throughput":MeasurementType.Throughput
}

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Kernel Benchmarks Scaling - Plotting Script')
parser.add_argument("-ps",    "--patch-size",             dest="patch_size",              type=int,                               default=9,                help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-max",   "--max-fused",              dest="max_fused",               type=int,                               default=16384,            help="Maximum number of fused patches that are studied.")
parser.add_argument("-f",     "--file",                   dest="file",                                                            default="",               help="Specify to convert an output file into a plot.")
parser.add_argument("-fc",    "--files-to-compare",       dest="files_to_compare",                                                default="",               help="Specify (multiple) output file(s) to compare to.", nargs="*")
parser.add_argument("-pt",    "--plot-type",              dest="plot_type",               choices=plot_types.keys(),              default="bar",           help="Type of plot to create")
parser.add_argument("-st",    "--scaling-type",           dest="scaling_type",            choices=scaling_types.keys(),           default="strong",         help="Type of scaling to create plots for.")
parser.add_argument("-mt",    "--measurement-type",       dest="measurement_type",        choices=measurement_types.keys(),       default="time-to-solution",   help="Type of scaling to create plots for.")
parser.add_argument("-mi",    "--machine-implementation", dest="machine_implementation",  choices=machine_implementations.keys(), default="device",         help="The machine implementation to create plots for.")
parser.add_argument("-ds",    "--data-structure",         dest="data_structure",          choices=data_structures.keys(),         default="AoSoA",          help="The data structure to create plots for.")
parser.add_argument("-kv",    "--kernel-variant",         dest="kernel_variant",          choices=kernel_variants.keys(),         default="batched",        help="The kernel variant to create plots for.")
args = parser.parse_args()

markers = [
  'D',
  '*',
  'o',
  's',
  'p',
  'x',
  'H',
  'h'
]

if args.file:
  plt.clf()
  fig, ax = plt.subplots()

  print( "Open {} for kernel variant {}, data structure {} and machine implementation {}".format(args.file,args.kernel_variant,args.data_structure,args.machine_implementation) )
  input_file = open( args.file, "r" )

  y_values = []
  thread_count = []
  total_patches = []
  fused_cells_or_patches = 0
  number_of_entries_written = 0

  for line in input_file:
    if scaling_types[args.scaling_type] == ScalingType.Weak:
      if "Number of patches fused:" in line:
        fused_cells_or_patches = int(line.split(":")[-1])
    else:
      if "Number of total patches:" in line:
        fused_cells_or_patches = int(line.split(":")[-1])
    new_measurement = "stateless kernels" in line
    new_measurement = new_measurement and kernel_variants[args.kernel_variant] in line
    if data_structures[args.data_structure]=="SoA" or data_structures[args.data_structure]=="AoS":
      new_measurement = new_measurement and data_structures[args.data_structure] in line
      new_measurement = new_measurement and not "AoSoA" in line
    else:
      new_measurement = new_measurement and data_structures[args.data_structure] in line
    new_measurement = new_measurement and machine_implementations[args.machine_implementation] in line
    if new_measurement and fused_cells_or_patches==args.max_fused:
      threads=int(line.split("threads")[0].split(",")[-1])
      thread_count.append(threads)
      total_patches.append( r"$2^{%s}$" % int(np.log2(fused_cells_or_patches * threads)))
      if measurement_types[args.measurement_type] == MeasurementType.TimeToSolution:
        y_values.append( float(line.split("/")[0].split(":")[-1]) )
        #print(float(line.split("/")[0].split(":")[-1]))
      else:
        y_values.append( float(line.split("/")[-1]) )
        #print(float(line.split("/")[-1]))

  bar_width = 1.0/(len(args.files_to_compare)+2)

  if len(y_values) > 0:
    x_values = np.arange(len(y_values))
    if len(x_values)!=len(y_values):
      print( "Cannot plot data for {}x{} as x-len is {} vs y-len {}".format(args.kernel_variant,args.data_structure,len(x_values),len(y_values)) )
    else:
      def ref(x, y):
        return (1 / np.power(2, x)) * y[0]
      if plot_types[args.plot_type] == PlotType.Bar:
        ax.bar(x_values, y_values, width=bar_width, label=str(args.file).split('.')[0])
        ax.plot( x_values, ref(x_values, y_values), linewidth=1.0, linestyle='dashed', label="Ideal", color='black')
      else:
        ax.plot( x_values, y_values, linewidth=1.5, marker=str(markers[number_of_entries_written]), markersize=4.0, label=str(args.file).split('.')[0] )
        ax.plot( x_values, ref(x_values, y_values), linewidth=1.0, linestyle='dashed', label="Ideal", color='black')
      number_of_entries_written += 1

  if args.files_to_compare:
    for file_to_compare in args.files_to_compare:
      print( "Open {} for kernel variant {}, data structure {} and machine implementation {}".format(file_to_compare,args.kernel_variant,args.data_structure,args.machine_implementation) )
      input_file_to_compare = open( file_to_compare, "r" )

      y_values_to_compare = []
      fused_cells_or_patches_to_compare = 0

      for line in input_file_to_compare:
        if scaling_types[args.scaling_type] == ScalingType.Weak:
          if "Number of patches fused:" in line:
            fused_cells_or_patches_to_compare = int(line.split(":")[-1])
        else:
          if "Number of total patches:" in line:
            fused_cells_or_patches_to_compare = int(line.split(":")[-1])
        new_measurement_to_compare = "stateless kernels" in line
        new_measurement_to_compare = new_measurement_to_compare and kernel_variants[args.kernel_variant] in line
        if data_structures[args.data_structure]=="SoA" or data_structures[args.data_structure]=="AoS":
          new_measurement_to_compare = new_measurement_to_compare and data_structures[args.data_structure] in line
          new_measurement_to_compare = new_measurement_to_compare and not "AoSoA" in line
        else:
          new_measurement_to_compare = new_measurement_to_compare and data_structures[args.data_structure] in line
        new_measurement_to_compare = new_measurement_to_compare and machine_implementations[args.machine_implementation] in line
        if new_measurement_to_compare and fused_cells_or_patches_to_compare==args.max_fused:
          if measurement_types[args.measurement_type] == MeasurementType.TimeToSolution:
            y_values_to_compare.append( float(line.split("/")[0].split(":")[-1]) )
            print(float(line.split("/")[0].split(":")[-1]))
          else:
            y_values_to_compare.append( float(line.split("/")[-1]) )
            print(float(line.split("/")[-1]))

      if len(y_values_to_compare) > 0:
        x_values_to_compare = np.arange(len(y_values_to_compare))
        if len(x_values_to_compare)!=len(y_values_to_compare):
          print( "Cannot plot data for {}x{} as x-len is {} vs y-len {}".format(args.kernel_variant,args.data_structure,len(x_values),len(y_values)) )
        else:
          if plot_types[args.plot_type] == PlotType.Bar:
            ax.bar(x_values_to_compare + bar_width, y_values_to_compare, width=bar_width, label=str(file_to_compare).split('.')[0])
          else:
            ax.plot( x_values_to_compare, y_values_to_compare, linewidth=1.5, marker=str(markers[number_of_entries_written]), markersize=4.0, label=str(file_to_compare).split('.')[0] )
          number_of_entries_written += 1

  plt.style.use("ggplot")

  if plot_types[args.plot_type] == PlotType.Bar and args.files_to_compare:
    plt.xticks(np.arange(len(thread_count)) + bar_width / (len(args.files_to_compare)+1), thread_count)
  else:
    plt.xticks(np.arange(len(thread_count)), thread_count)
  #plt.yscale( "log", base=10 )

  plt.legend(loc='upper right', fancybox=True, shadow=True)
  plt.xlabel("Number of threads")

  if measurement_types[args.measurement_type] == MeasurementType.TimeToSolution:
    plt.ylabel("Time to solution (s)")
  else:
    plt.ylabel("Time per FV update (s)")

  plt.title("CCZ4, " + r"$ P = {}, p = {}^{} $".format( args.max_fused, args.patch_size, 3))
  plt.grid(color='grey', alpha=0.5, linestyle='dashed', linewidth=0.5)

  if number_of_entries_written>0:
    plt.savefig( str(args.file).split('.')[0] + "-" + args.machine_implementation + "-" + args.kernel_variant + "-" + args.data_structure + "-" + args.scaling_type + "-scaling" + ".pdf", bbox_inches='tight' )
    plt.savefig( str(args.file).split('.')[0] + "-" + args.machine_implementation + "-" + args.kernel_variant + "-" + args.data_structure + "-" + args.scaling_type + "-scaling" + ".png", bbox_inches='tight' )
  else:
    print( "No (valid) data found for machine implementation '{}'".format(args.machine_implementation) )
