import argparse
import csv
import os
import sys

from peano4.visualisation.OutputFileParser import OutputFileParser

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Peano 4 - probe a specific '
                                               'point in a set of Peano patch files')
  parser.add_argument('--input-file',     dest='filepath',       help='File path to a Peano patch file')
  parser.add_argument('--meta-file',      dest='metafile',       help='The main file that links to a set of Peano patch files - these must be in the same directory')
  parser.add_argument('-x',               dest='x_axis',         help='coordinate: x-axis', required=True, type=float)
  parser.add_argument('-y',               dest='y_axis',         help='coordinate: y-axis', required=True, type=float)
  parser.add_argument('-z',               dest='z_axis',         help='coordinate: z-axis', type=float)
  parser.add_argument('--set-identifier', dest='set_identifier', help='Name of the set of unknowns we want to extract', default='')
  parser.add_argument('--ofname',         dest='ofname',         help='Name of the output csv to which the probe writes its results', default='probe_output.csv')
  args = parser.parse_args()

  if args.filepath:
    if not os.path.exists(args.filepath):
      print("Error, specified input file '{}' does not exist, exiting...".format(args.filepath))
      sys.exit(1)

    ofparser = OutputFileParser(args.filepath, args.set_identifier, 0)

    with open('probe_output.csv', 'w') as csv_file:
      writer = csv.writer(csv_file, delimiter=',')
      probe_result = ofparser.probe(args.x_axis, args.y_axis, args.z_axis)
      if probe_result:  
        writer.writerow(probe_result)

  if args.metafile:
    if not os.path.exists(args.metafile):
      print("Error, specified input file '{}' does not exist, exiting...".format(args.directory_path))
      sys.exit(1)
    
    patch_file_names = []
    with open(args.metafile, 'r') as metafile:
      for line in metafile.readlines():
        words = line.split()
        if 'include' in words:
          patch_file_names.append(words[1].strip('"'))
    
    with open(args.ofname, 'w') as csv_file:
      writer = csv.writer(csv_file, delimiter=',')
      for file in patch_file_names:
        ofparser = OutputFileParser(os.path.join(os.path.dirname(args.metafile), file), args.set_identifier, 0)
        probe_result = ofparser.probe(args.x_axis, args.y_axis, args.z_axis)
        if probe_result:
          writer.writerow(probe_result)

