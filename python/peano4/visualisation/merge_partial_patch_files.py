# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import sys


def merge_partial_files(partial_files, merged_file_path):
  with open(merged_file_path, 'w') as merged_patch_file:
    # First we need to write the header for our new patch
    # file. We can open any of the partial patch files
    # and pass this across
    with open(partial_files[0], 'r') as patch_file:
      for line in patch_file.readlines():
        if 'begin patch' in line:
          break # We don't want patches yet
        else:
          merged_patch_file.write(line)

    for infile in partial_files:
      reading_patches = False # allows us to skip headers
      with open(infile, 'r') as patch_file:
        for line in patch_file:
          if 'begin patch' in line and not reading_patches:
            reading_patches = True
            merged_patch_file.write('\n')
          if reading_patches:
            merged_patch_file.write(line)

def read_meta_file(path_to_metafile):
  merged_files = [] # stores names of merged files
  partial_files = [] # stores partial files that need to be merged
                     # into the same file
  merged_file_number = 0
  with open(path_to_metafile, 'r') as metafile:
    for line in metafile.readlines():
      if 'end dataset' in line:
        # merged files will be put in the same dir as the original meta-file
        merged_file_name = 'merged_patch_file_' + str(merged_file_number) + '.peano-patch-file'
        
        if os.path.isabs(path_to_metafile):
            merged_file_name = os.path.join(os.path.dirname(path_to_metafile), merged_file_name)

        merge_partial_files(partial_files, merged_file_name)
        merged_files.append(merged_file_name)

        partial_files.clear()
        merged_file_number += 1

      words = line.split()
      
      if 'include' in words:
        file_path = words[1].strip('"') # this is input path
          
        # Where the filepath listed in the meta-file is not
        # an absolute path with assume the file lives in the
        # same directory as the meta-file itself:
        if not os.path.isabs(file_path):
          file_path = os.path.join(os.path.dirname(path_to_metafile), file_path)
        
        partial_files.append(file_path)

  # write a meta file out of the merged_files
  # to the directory of the original meta-file
  with open(os.path.join(
      os.path.dirname(path_to_metafile),
      'merged_meta_file.peano-patch-file'), 'w') as new_meta_file:
    new_meta_file.write("# \n"
                        "# Peano patch file \n"
                        "# Version 0.2 \n"
                        "# \n"
                        "format ASCII \n"
                        "\n")
    for merged_file in merged_files:
      new_meta_file.write("begin dataset\n"
                          "\n"
                          f'  include "{merged_file}"\n'
                          "end dataset\n"
                          "\n")


if __name__ == "__main__":
  parser = argparse.ArgumentParser(
    description='Peano 4 - condense a partial patchfiles '
    'that are linked to by a meta file')
  parser.add_argument(
    '--meta-file',
    dest='metafile',
    help='The main file that links to a set of Peano patch files',
    required=True)
  args = parser.parse_args()

  if not os.path.exists(args.metafile):
    print("Error, specified input file '{}' does not exist, exiting...".format(
            args.metafile))
    sys.exit(1)
  
  read_meta_file(args.metafile)

