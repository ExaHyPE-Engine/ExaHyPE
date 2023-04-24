import os
import sys
import argparse
import subprocess

# Example usage:
# python3 run-clang-format.py -d -r /Peano/src -s /Peano/.clang-format
parser = argparse.ArgumentParser(
  prog='Run-Clang-Format Script',
  description='Recursively run clang-format on files'
)

parser.add_argument("-r", "--root-path", default=".", required=True,
                    type=str, help="Start path to process files")
parser.add_argument("-d", "--dry-run", action="store_true", required=False, default=False,
                    help="Dry run where it only reports the files that have inappropriate formatting")
parser.add_argument("-s", "--style", required=True,
                    help="Path to the clang-format style file")
args = parser.parse_args()

return_code = 0
dry_run = args.dry_run
rootdir = args.root_path
style_file = args.style

for root, subdirs, files in os.walk(rootdir):
  for file in files:
    if not file.endswith(".h") \
    and not file.endswith(".hpp") \
    and not file.endswith(".cpph") \
    and not file.endswith(".c") \
    and not file.endswith(".cpp") \
    and not file.endswith(".cu") \
    and not file.endswith(".cuh"):
      continue

    path = os.path.join(root, file)
    output = subprocess.check_output(f"clang-format {path} --style=file:{style_file}", shell=True)
    filecontent = open(path, "rb").read()

    if output != filecontent:
      if dry_run:
        print(f"The file {path} would be changed after formatting")
        return_code = 1
    if not dry_run:
      os.system(f"clang-format --style=file:{style_file} -i {path} -verbose")

sys.exit(return_code)
