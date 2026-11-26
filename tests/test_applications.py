# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import shlex
import subprocess
import sys

from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[1]
LAUNCHER = shlex.split(os.environ.get("PARALLEL_LAUNCH", ""))

SCENARIOS = """
Group                 | Folder                                             | Generator                         | Executable              | Peano arguments              | Hilbert arguments            | Error check
ExaSkred Tsunami      | applications/exaskred/tsunami/artificial           | artificial.py                     | ./ExaSkred-Tsunami      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Tsunami      | applications/exaskred/tsunami/chile                | chile.py                          | ./ExaSkred-Tsunami      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Tsunami      | applications/exaskred/tsunami/oscillating-lake     | oscillating-lake.py               | ./ExaSkred-Tsunami      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Tsunami      | applications/exaskred/tsunami/solitary-wave        | solitary-wave.py                  | ./ExaSkred-Tsunami      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Tsunami      | applications/exaskred/tsunami/three-mounds-channel | three-mounds-channel.py           | ./ExaSkred-Tsunami      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Tsunami      | applications/exaskred/tsunami/tohoku               | tohoku.py                         | ./ExaSkred-Tsunami      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Meteotsunami | applications/exaskred/meteotsunami/gulf-of-mexico  | gulf-of-mexico.py                 | ./ExaSkred-Meteotsunami | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Meteotsunami | applications/exaskred/meteotsunami/r1              | r1.py                             | ./ExaSkred-Meteotsunami | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Landslide    | applications/exaskred/landslide/tafjord            | tafjord.py                        | ./ExaSkred-Landslide    | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Coupled      | applications/exaskred/coupled/impulse-wave         | impulse-wave.py                   | ./ExaSkred-Coupled      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSkred Coupled      | applications/exaskred/coupled/tafjord              | tafjord.py                        | ./ExaSkred-Coupled      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/cartesian                     | cartesian.py                      | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/curvilinear                   | curvilinear.py -s LOH1            | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/curvilinear                   | curvilinear.py -s Zugspitze       | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml                           | pml.py -s LOH1                    | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml                           | pml.py -s Zugspitze               | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/dynamic-rupture               | dynamic-rupture.py -s TPV5        | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/dynamic-rupture               | dynamic-rupture.py -s TPV6        | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/dynamic-rupture               | dynamic-rupture.py -s TPV16       | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/dynamic-rupture               | dynamic-rupture.py -s TPV26       | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/dynamic-rupture               | dynamic-rupture.py -s TPV28       | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/dynamic-rupture               | dynamic-rupture.py -s TPV29       | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/dynamic-rupture               | dynamic-rupture.py -s TPV34       | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/dynamic-rupture               | dynamic-rupture.py -s Husavik     | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml-dynamic-rupture           | pml-dynamic-rupture.py -s TPV5    | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml-dynamic-rupture           | pml-dynamic-rupture.py -s TPV6    | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml-dynamic-rupture           | pml-dynamic-rupture.py -s TPV16   | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml-dynamic-rupture           | pml-dynamic-rupture.py -s TPV26   | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml-dynamic-rupture           | pml-dynamic-rupture.py -s TPV28   | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml-dynamic-rupture           | pml-dynamic-rupture.py -s TPV29   | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml-dynamic-rupture           | pml-dynamic-rupture.py -s TPV34   | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaSeis               | applications/exaseis/pml-dynamic-rupture           | pml-dynamic-rupture.py -s Husavik | ./ExaSeis               | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ExaGRyPE              | applications/exagrype/gauge-wave                   | gauge-wave.py --solver fv         | ./ExaGRyPE              | -nt 10 -ns 2 -l 1            | -nt 10 -ns 2 -l 1            |
"""


def read_table(table):
    """Returns one dictionary per table row, keyed by the header row."""
    lines = table.strip().splitlines()
    header, *rows = [[cell.strip() for cell in line.split("|")] for line in lines]
    return [dict(zip(header, row)) for row in rows]


def commands(scenario, options, generator_arguments):
    """Returns the commands that generate, build, run and check a scenario."""
    generator = shlex.split(scenario["Generator"])
    result = [[sys.executable] + generator + generator_arguments]
    if not options.build_only:
        arguments = shlex.split(scenario[options.curve + " arguments"])
        result.append(LAUNCHER + [scenario["Executable"]] + arguments)
        if scenario["Error check"]:
            check = [sys.executable, "-m", "postprocessing.check_error_norms"]
            result.append(check + shlex.split(scenario["Error check"]))
    return result


if __name__ == "__main__":
    scenarios = read_table(SCENARIOS)
    groups = list(dict.fromkeys(scenario["Group"] for scenario in scenarios))
    parser = argparse.ArgumentParser(
        epilog=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--group", nargs="+", choices=groups, help="Groups to run, all by default"
    )
    parser.add_argument(
        "--build-only", action="store_true", help="Generate and build without running"
    )
    parser.add_argument(
        "--curve",
        choices=["Peano", "Hilbert"],
        default="Peano",
        help="Space-filling curve of the engine build",
    )
    options, generator_arguments = parser.parse_known_args()
    environment = dict(
        os.environ,
        PYTHONPATH=os.pathsep.join([str(REPOSITORY / "api"), str(REPOSITORY)]),
    )
    failures = []
    for scenario in scenarios:
        if options.group and scenario["Group"] not in options.group:
            continue
        title = scenario["Folder"] + ": " + scenario["Generator"]
        print("=== " + title, flush=True)
        try:
            for command in commands(scenario, options, generator_arguments):
                subprocess.run(
                    command,
                    cwd=REPOSITORY / scenario["Folder"],
                    env=environment,
                    check=True,
                )
        except (subprocess.CalledProcessError, OSError) as error:
            print(error, flush=True)
            failures.append(title)
    for title in failures:
        print("Failed: " + title)
    sys.exit(1 if failures else 0)
