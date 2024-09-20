import os, re

"""!
This file sanitises the library paths for
files that we retrieve from Git, writes
every line out to a temp file and then
copies the temp file in place of file.
"""

ci_path    = os.path.dirname(os.path.abspath(__file__))
peano_path = os.path.dirname(ci_path)

def iterate_lines(filename):
    with open(filename) as f:
        line = f.readline()
        lineNumber = 1
        while line:
            yield line, lineNumber
            line = f.readline()
            lineNumber += 1


def found_path(line, start_string, end_string, replacement_path):
    # Produces a match if we see something that looks like:
    # /dine/do009/ *** /Peano/ ***
    pattern = re.compile(rf"{re.escape(start_string)}/.*?{re.escape(end_string)}")
    regex = pattern.search(line)
    if regex:
        # Here we do a little recursion since the regex only catches
        # the FIRST instance of a match. So, we fix the first instance,
        # then run this function again on the fixed version!

        # Send the latter part of the line into this function again to be further
        # sanitised. Will simply return if a directory path isn't found.
        second_part_of_line = found_path(
            line[regex.end() :], start_string, end_string, replacement_path
        )

        newline = line[: regex.start()] + replacement_path + second_part_of_line
        return newline
    else:
        return line


def sanitise_file(original_filename, start_string, end_string, replacement_path):
    tempfile = f"{peano_path}/temp.txt"

    try:
        with open(tempfile, "w") as tmp:
            for original_line, _ in iterate_lines(original_filename):
                # Only sanitise if there is something to do
                line = found_path(
                    original_line, start_string, end_string, replacement_path
                )
                tmp.write(line)
    except:
        # Error occurred
        os.remove(tempfile)
        return
    os.rename(tempfile, original_filename)

    print(f"Sanitised {original_filename}")


if __name__ == "__main__":
    start_string = "/dine/data/do009"
    end_string = "/Peano"
    replacement_path = peano_path

    import sys
    for filename in sys.argv[1:]:
        try:
            sanitise_file(filename, start_string, end_string, replacement_path)
        except:
            print(f"Error sanitising {filename}")
