#!/usr/bin/env python3

import os
import sys
import argparse
import subprocess


verbose = True # will be overwritten by arg parser
peano_repo_rootdir_name = "Peano"
default_clang_format_file_name = ".clang-format"
target_branch = "p4"




def find_peano_root_dir():
    """
    Starting from current working directory, go up the directory
    tree until you find the Peano root directory and return that
    path
    """

    current_dir = os.getcwd()
    peano_root = None

    while current_dir != os.sep: # until we reach root
        if os.path.basename(current_dir) == peano_repo_rootdir_name:
            peano_root = current_dir
            break

        current_dir = os.path.dirname(current_dir)

    if peano_root is None:
        raise FileNotFoundError("Couldn't find Peano root directory?")

    if verbose:
        print("Found Peano root", peano_root)

    return peano_root


def get_default_clang_format(peano_root):
    """
    Try and find the default clang format style file
    """

    clang_format_style_file = os.path.join(peano_root, default_clang_format_file_name)

    if not os.path.exists(clang_format_style_file):
        raise FileNotFoundError("Couldn't find default clang format style file "+clang_format_style_file)

    return clang_format_style_file


def get_black_command(peano_root, dry_run):
    """
    Get the full command used to invoke the python formatter 'black'.
    First get the path to the 'black' executable. It should be in the
    $PEANO_REPOSITORY_ROOT/.black_formatting_env/MLADEN_ADD_FULL_PATH
    directory.

    If that directory doesn't exist, we first need to set up a virtualenv
    there and install a specific version of 'black'.
    """

    bfe_path = os.path.join(peano_root, ".black_formatting_env")

    if not os.path.exists(bfe_path):
        print("Python formatting env not found, installing it")
        os.mkdir(bfe_path)
        run = subprocess.run("python3 -m venv {bfe_path}", shell=True, check=True)
        # basically Peano/.black_formatting_env/bin/python3
        bfepython = os.path.join(bfe_path, "bin", "python3")
        run = subprocess.run("{bfe_python} -m pip install click==8.1.3 black==23.3.0", shell=True, check=True)

    black_path = bfe_path = os.path.join(peano_root, ".black_formatting_env", "bin", "black")

    # check that the executable runs
    run = subprocess.run(black_path+" --version", shell=True, stdout=subprocess.DEVNULL, check=True)

    black_cmd = black_path
    # TODO: do we want more specifics? Options I have in mind:
    # --target-version py38? py39? py310?
    # --line-length
    black_cmd += " --quiet "
    if dry_run:
        black_cmd += " --check "

    return black_cmd


def get_clang_command(clang_format_file, dry_run):
    """
    Get the full command used to invoke the clang-format tool.
    """

    clang_cmd = "clang-format-14"

    # check that the executable runs
    run = subprocess.run(clang_cmd+" --version", shell=True, stdout=subprocess.DEVNULL, check=True)

    if dry_run:
        clang_cmd += " --dry-run "
    else:
        # format files in-place
        clang_cmd += " -i"
    # change warnings to errors so I can work with the error code as
    # return values
    clang_cmd += " --Werror"
    # no maximum error limit
    clang_cmd += " --ferror-limit=0"
    # skip formatting if formatting file isn't found. Better safe than sorry.
    clang_cmd += " --fallback-style=none"

    return clang_cmd




def setup_parser(peano_root, clangfile):
    """
    Set up the argument parser.
    """

    parser = argparse.ArgumentParser(
      prog='format.py',
      description='run clang-format on cpp files, run black on py files',
      epilog="""
    By default, this formatting script will compare your current git branch with the
    p4 branch, and format all the files that you have modified on your local branch.
    Alternatively, you can specify to compare against a different branch or to provide
    a list of files you want formatted by specifying the corresponding flags,
    respectively.
    """,
      allow_abbrev=True,
    )

    parser.add_argument("-d", "--dry-run", action="store_true", required=False, default=False,
                        help="Dry run where it only reports the files that have inappropriate formatting")
    parser.add_argument("-s", "--style", required=False,
                        help="Path to a non-default clang-format style file (default is "+clangfile+")")
    parser.add_argument("-b", "--branch", required=False,
                        help="Which git branch to compare current branch against to detect modifications (default is "+target_branch+")")
    parser.add_argument("-f", "--files", required=False, nargs="+", help="Path to specific file(s) to run the formatter on")
    parser.add_argument("-v", "--verbose", action="store_true", required=False, default=False,
                        help="Be verbose about what's going on")

    return parser



def is_pythonfile(filename):
    """
    Does this look like a python file?
    """

    return filename.endswith(".py")


def is_cppfile(filename):
    """
    Does this look like a cpp file?
    """

    if filename.endswith(".h") or \
        filename.endswith(".hpp") or \
        filename.endswith(".cpph") or \
        filename.endswith(".c") or \
        filename.endswith(".cpp") or \
        filename.endswith(".cu") or \
        filename.endswith(".cuh") :
        return True

    return False



def run_python_formatter(filename, dry_run, peano_root):
    """
    Run the black formatter on the provided file.
    If dry_run is True, don't modify the file, just report
    whether it would modify the file.

    Returns 1 if file was modified or if it could be modified
    and the dry_run flag was provided, or 0 otherwise.
    """

    black_cmd = get_black_command(peano_root, dry_run)

    if verbose:
        print(f"running {black_cmd} {filename}")
    run = subprocess.run(black_cmd + " "+ filename, stdout=subprocess.DEVNULL, shell=True)

    if run.returncode != 0:
        return 1
    return 0


def run_clang_formatter(filename, dry_run, clang_format_file):
    """
    Run the clang formatter on the provided file.
    If dry_run is True, don't modify the file, just report
    whether it would modify the file.

    Returns 1 if file was modified or if it could be modified
    and the dry_run flag was provided, or 0 otherwise
    """

    clang_cmd = get_clang_command(clang_format_file, dry_run)

    if verbose:
        print(f"running {clang_cmd} {filename}")
    # pipe sterr to /dev/null too here. In dry run mode, every line that needs
    # to be changed will otherwise be printed to stderr.
    run = subprocess.run(clang_cmd + " "+ filename, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)

    if dry_run:
        # returns an error code if a file can be formatted.
        if run.returncode != 0:
            return 1
        return 0

    # assume file has been formatted.
    # TODO: potentially the wrong choice to return 1 here if we're using
    # CI to actually format files, not just the --dry-run mode to check
    # for formatting
    return 1




if __name__ == "__main__":

    peano_root = find_peano_root_dir()
    clang_format_file = get_default_clang_format(peano_root)

    parser = setup_parser(peano_root, clang_format_file)
    args = parser.parse_args()


    # are we using default clang file?
    if args.style is not None:
        clang_format_file = args.style
        print(clang_format_file)
        if not os.path.exists(clang_format_file):
            raise FileNotFoundError("Provided clang format file '"+clang_format_file+"' doesn't exist")

    verbose = args.verbose

    if verbose:
        print("using clang format file", clang_format_file)

    # is this dry run?
    dry_run = args.dry_run
    if dry_run and verbose:
        print("starting a dry run")

    # update target branch?
    if args.branch is not None:
        target_branch = args.branch
    if verbose:
        print("Comparing against", target_branch, "branch")

    # make sure we're either comparing a branch or a list of files
    if args.branch is not None and args.files is not None:
        print("Error: Select either the -f/--files flag, or the -b/--branch flag, not both.")
        quit(1)

    # make sure that the branch exists
    try:
        git_check_branch_exists_cmd = f"git rev-parse --verify {target_branch}"
        subprocess.run(git_check_branch_exists_cmd, shell=True, check=True, stdout=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        print("Something went wrong when trying to check whether branch exists.")
        print("Command that failed was:", git_check_branch_exists_cmd)
        sys.exit(1)


    # which files are we formatting?
    filelist = []

    # Check first if file list was given by user
    if args.files is not None:
        if verbose:
            print("File list provided. Formatting only provided list.")

        for f in args.files:
            if not os.path.exists(f):
                if verbose:
                    print(f"Didn't find file {f}, ignoring it")
                continue
            if not (is_pythonfile(f) or is_cppfile(f)):
                if verbose:
                    print(f"File {f} doesn't look like a python nor cpp file to me, skipping it")
                continue
            filelist.append(f)

    else:
        # No file list means we're checking for git diffs
        # So get file list from a git diff now
        gitdiff = subprocess.run(f"git diff {target_branch} --name-only", shell=True, check=True, capture_output=True)
        gitstdout = gitdiff.stdout
        if isinstance(gitstdout, bytes):
            gitstdout = gitstdout.decode("utf8")
        diffstr = gitstdout.strip()
        difffiles = diffstr.split("\n")

        for f in difffiles:
            # by default git returns paths relative to root of the repository.
            # We want this script to run from any subdirectory, so we need a
            # full path here.
            fullpathf = os.path.join(peano_root, f)
            if not os.path.exists(fullpathf):
                if verbose:
                    print(f"File {fullpathf} doesn't exist. Did you remove it in this branch? Skipping it")
                continue
            if not (is_pythonfile(fullpathf) or is_cppfile(fullpathf)):
                if verbose:
                    print(f"File {fullpathf} doesn't look like a python nor cpp file to me, skipping it")
                continue
            filelist.append(fullpathf)


    if len(filelist) == 0:
        print("Found no files to be formatted. Exiting")
        sys.exit(0)


    # keep track whether any file was or could be modified.
    nmodified = 0
    formatted_files = []
    for f in filelist:

        if is_pythonfile(f):
            return_code = run_python_formatter(f, dry_run, peano_root)
            nmodified += return_code
            if return_code != 0:
                formatted_files.append(f)


        elif is_cppfile(f):
            return_code = run_clang_formatter(f, dry_run, clang_format_file)
            nmodified += return_code
            if return_code != 0:
                formatted_files.append(f)


        else:
            raise ValueError("How did we get here???")


    # Report what you did
    if dry_run:
        print("Dry run:", nmodified, "can be formatted (run with --verbose to see file list)")
        prefix = "Can be formatted:"
    else:
        print("Formatted", nmodified, "files. (run with --verbose to see file list)")
        prefix = "Has been formatted:"

    if verbose:
        formatted_files.sort()
        for f in formatted_files:
            print(prefix, f)

    # "fail" if we can/did format any files, so CI can pick this
    # up as an "error"
    sys.exit(nmodified)


