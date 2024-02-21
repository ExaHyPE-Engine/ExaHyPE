# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import sys


def merge_partial_files(partial_files, merged_file_path):
    print("start to merge file:", merged_file_path, end="...")
    with open(merged_file_path, "w") as merged_patch_file:
        # First we need to write the header for our new patch
        # file. We can open any of the partial patch files
        # and pass this across
        with open(partial_files[0], "r") as patch_file:
            for line in patch_file.readlines():
                if "begin patch" in line:
                    break  # We don't want patches yet
                else:
                    merged_patch_file.write(line)

        count = 0
        for infile in partial_files:
            reading_patches = False  # allows us to skip headers
            count += 1
            # print(str(count)+"/"+str(len(partial_files)))
            with open(infile, "r") as patch_file:
                for line in patch_file:
                    if "begin patch" in line and not reading_patches:
                        reading_patches = True
                        merged_patch_file.write("\n")
                    if reading_patches:
                        merged_patch_file.write(line)
    print("Merged.")


def read_meta_file(path_to_metafile, start, end):
    #merged_files = []  # stores names of merged files
    partial_files = []  # stores partial files that need to be merged
    # into the same file
    #timestamps = []
    merged_file_number = 0

    f=open(
        os.path.join(
            os.path.dirname(path_to_metafile), "merged_meta_file.peano-patch-file"
        ),
        "w",
    )
    f.write(
            "# \n"
            "# Peano patch file \n"
            "# Version 0.2 \n"
            "# \n"
            "format ASCII \n"
            "\n"
        )

    with open(path_to_metafile, "r") as metafile:
        for line in metafile.readlines():
            if "end dataset" in line and (merged_file_number < start or merged_file_number > end):
                partial_files.clear()
                print("jump snapshot", merged_file_number)
                print()
                merged_file_number += 1
            elif "end dataset" in line:
                # merged files will be put in the same dir as the original meta-file
                merged_file_name = (
                    "merged_patch_file_" + str(merged_file_number) + ".peano-patch-file"
                )

                if os.path.isabs(path_to_metafile):
                    merged_file_name = os.path.join(
                        os.path.dirname(path_to_metafile), merged_file_name
                    )

                merge_partial_files(partial_files, merged_file_name)

                partial_files.clear()
                merged_file_number += 1
                print()

                f.write(
                "begin dataset\n" + timestamp_line + "\n"
                f'  include "{merged_file_name}"\n'
                "end dataset\n"
                "\n"
                )

            if "timestamp" in line and merged_file_number >= start:
                timestamp_line=line
                print(line[0:-2])

            words = line.split()

            if "include" in words:
                file_path = words[1].strip('"')  # this is input path

                # Where the filepath listed in the meta-file is not
                # an absolute path with assume the file lives in the
                # same directory as the meta-file itself:
                if not os.path.isabs(file_path):
                    file_path = os.path.join(
                        os.path.dirname(path_to_metafile), file_path
                    )

                partial_files.append(file_path)

    f.close()

def count_total_snapshot_number(path_to_metafile):
    count=0
    with open(path_to_metafile, "r") as metafile:
        for line in metafile.readlines():
            if "timestamp" in line:
                count+=1
    return count

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Peano 4 - condense a partial patchfiles "
        "that are linked to by a meta file"
    )
    parser.add_argument(
        "--meta-file",
        dest="metafile",
        help="The main file that links to a set of Peano patch files",
        required=True,
    )
    parser.add_argument(
        "-start", "--start-snapshot-index",
        dest="start", type=int,
        help="set the snapshot number where the merge starts(all snapshot before it will be ignore), default is 0 (start from beginning)",
        default=0,)
    parser.add_argument(
        "-end", "--end-snapshot-index",
        dest="end", type=int,
        help="set the snapshot number where the merge ends(all snapshot after it will be ignore), default is -1 (end at the last)",
        default=-1,)
    
    args = parser.parse_args()

    if not os.path.exists(args.metafile):
        print(
            "Error, specified input file '{}' does not exist, exiting...".format(
                args.metafile
            )
        )
        sys.exit(1)

    total_snapshot_number=count_total_snapshot_number(args.metafile)
    if ( (args.start>args.end and args.end!=-1) or args.start<0 or args.end<-1):
        print("Error, specified snapshot index is wrong, please check.")
        sys.exit(1)
    elif args.end==-1:
        start_snapshot=args.start
        end_snapshot=total_snapshot_number-1 #processing all snapshot if no end snapshot is specified.
    elif args.end>(total_snapshot_number-1):
        start_snapshot=args.start
        end_snapshot=total_snapshot_number-1
        print("Warning, specified end snapshot index exceed the totoal number, reset to the last snapshot.")
    else:
        start_snapshot=args.start
        end_snapshot=args.end

    print("The meta file include", total_snapshot_number, "snapshots, processing snapshot", start_snapshot, "to", end_snapshot)
    print("==============================================================")

    read_meta_file(args.metafile, start_snapshot, end_snapshot)
