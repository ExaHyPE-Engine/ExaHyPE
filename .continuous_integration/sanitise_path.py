# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Replace checkout-dependent paths in installed configuration files."""

import os
import re
import sys

ci_path = os.path.dirname(os.path.abspath(__file__))
peano_path = os.path.dirname(ci_path)


def iterate_lines(filename):
    with open(filename, encoding="utf-8") as input_file:
        for line_number, line in enumerate(input_file, start=1):
            yield line, line_number


def found_path(line, start_string, end_string, replacement_path):
    # The lookahead pins the end string to a path boundary, so a directory whose
    # name merely starts with the end string cannot truncate the match early.
    pattern = re.compile(rf"{re.escape(start_string)}/.*?{re.escape(end_string)}(?=[/;\"\s]|$)")
    match = pattern.search(line)
    if match:
        # Searching only the suffix prevents replacement text from matching again.
        sanitised_suffix = found_path(
            line[match.end() :], start_string, end_string, replacement_path
        )
        return line[: match.start()] + replacement_path + sanitised_suffix
    return line


def sanitise_file(original_filename, start_string, end_string, replacement_path):
    temporary_filename = f"{peano_path}/temp.txt"
    succeeded = False

    try:
        with open(temporary_filename, "w", encoding="utf-8") as output_file:
            for original_line, _ in iterate_lines(original_filename):
                sanitised_line = found_path(
                    original_line,
                    start_string,
                    end_string,
                    replacement_path,
                )
                output_file.write(sanitised_line)
        os.replace(temporary_filename, original_filename)
        succeeded = True
    except (OSError, UnicodeError):
        pass
    finally:
        try:
            os.remove(temporary_filename)
        except FileNotFoundError:
            pass
        except OSError:
            succeeded = False

    if succeeded:
        print(f"Sanitised {original_filename}")
    return succeeded


if __name__ == "__main__":
    start_string = "/dine/data/do009"
    end_string = "/Peano"
    replacement_path = peano_path

    failed = False
    for filename in sys.argv[1:]:
        if not sanitise_file(filename, start_string, end_string, replacement_path):
            print(f"Error sanitising {filename}")
            failed = True
    if failed:
        sys.exit(1)
