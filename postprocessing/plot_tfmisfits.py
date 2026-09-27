# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import csv, argparse, sys

# This python file should serve as an example of how to compute or plot so called time-frequency misfits,
# which act as a measure of how well two signals match. (see https://doi.org/10.1111/j.1365-246X.2009.04177.x)
# These are a somewhat common measure for the quality of a signal in seismology,
# Our method is based on the obspy python package (https://docs.obspy.org/tutorial/code_snippets/time_frequency_misfit.html),
# which in principle does all of our work for us.
# There are two measures in the time-frequency misfits, these being phase misfits and envelope misfits, which
# you can think of roughly like a measure of the difference in frequency and amplitude between the two
# signals, in the sense of a Fourier analysis

# The heart of the program, given two array-like signals vx and ref (for reference solution),
# this will return the phase and envelope misfit of the two signals.
# vx and ref must have the same size, and are assumed to be measurements of some sort of
# signal at the times in the array-like, evenly spaced values dt.
from obspy.signal.tf_misfit import em, pm, plot_tf_misfits
import matplotlib.pyplot as plt


def misfits(dt, vx, ref):
    # single valued phase misfit
    # phase_misfit = pm(vx, ref, dt=(dt[1]-dt[0]), fmin=(dt[1]-dt[0]), fmax=(dt[-1]-dt[0]))
    # single valued envelope misfit
    # envelope_misfit = em(vx, ref, dt=(dt[1]-dt[0]), fmin=(dt[1]-dt[0]), fmax=(dt[-1]-dt[0]))

    plot_tf_misfits(
        vx, ref, dt=(dt[1] - dt[0]), fmin=(dt[1] - dt[0]), fmax=(dt[-1] - dt[0])
    )
    return


# Returns the time stamps and the values of variable var for the tracer that the filters select.
import numpy as np


def tracer_extractor(
    file_name, var=0, x=None, y=None, z=None, r=None, t_min=None, t_max=None
):
    """
    Extract some metadata about the positions of the various information in the csv file.
    These should always be the same but it's safer to check.
    """

    # The database is written with the plain file name, but users tend to pass it without the
    # extension, so fall back to appending it.
    candidates = [file_name, file_name + ".csv"]
    csvData = None
    for candidate in candidates:
        try:
            with open(candidate, "r") as inputFile:
                csvData = list(csv.reader(inputFile, delimiter=","))
            break
        except FileNotFoundError:
            continue
    if csvData is None:
        raise SystemExit(
            "None of {} exists, please check the file location".format(
                " or ".join(candidates)
            )
        )
    if not csvData:
        raise SystemExit("{} is empty".format(file_name))

    timeIndex = csvData[0].index(" t")
    vxIndex = csvData[0].index(" data(0)") + var
    firstNumberIndex = csvData[0].index("number(0)")
    secondNumberIndex = csvData[0].index(" number(1)")
    positionIndexes = [csvData[0].index(" x(0)"), csvData[0].index(" x(1)")]
    if " x(2)" in csvData[0]:
        positionIndexes.append(csvData[0].index(" x(2)"))

    if z is not None and len(positionIndexes) < 3:
        raise SystemExit(
            "--z was given but {} holds two-dimensional data".format(file_name)
        )

    """
    remove first row which should only contain metadata
    """
    csvData = [row for row in csvData[1:] if row]

    """
    Filter out only the data from tracers at the specified positions
    """

    def position_check(a):
        value = True
        if x != None:
            value = value and abs(float(a[positionIndexes[0]]) - x) < 0.001
        if y != None:
            value = value and abs(float(a[positionIndexes[1]]) - y) < 0.001
        if z != None:
            value = value and abs(float(a[positionIndexes[2]]) - z) < 0.001
        if r != None:
            value = value and int(a[firstNumberIndex]) == r
        if t_min != None:
            value = value and float(a[timeIndex]) >= t_min
        if t_max != None:
            value = value and float(a[timeIndex]) <= t_max
        return value

    csvData = [data for data in csvData if position_check(data)]

    """
    Check how many tracers fit the filter by extracting the total numbers of number(0) and
    number(1) tuples that exist with the filter.
    The {number(0), number(1)} tuples should be unique for each tracer, therefore if there
    exists more than one value for either of these after filtering, there are multiple
    tracers that correspond to the filter.
    In the same way if there are none, none of the tracers correspond to the filter.
    """
    checkFirstIndexLength = set([data[firstNumberIndex] for data in csvData])
    checksecondIndexLength = set([data[secondNumberIndex] for data in csvData])

    if len(checkFirstIndexLength) == 0:
        raise Exception(
            "Specification was valid for none of the tracers, please check that a tracer exists at the specified position"
        )
    elif len(checkFirstIndexLength) > 1 or len(checksecondIndexLength) > 1:
        raise Exception(
            "Specification was valid for more than one tracer, please clarify which you want through use of positions x, y and z"
        )

    """
    Extract only the data from time and from the required value
    """

    dt = []
    vx = []

    for row in csvData:
        dt.append(float(row[timeIndex]))
        vx.append(float(row[vxIndex]))

    """
    Sort this data in the direction of increasing time
    """

    dt, vx = zip(*sorted(zip(dt, vx)))

    return np.array(dt), np.array(vx)


from scipy.interpolate import interp1d
import warnings


def PlotTimeFrequencyMisfits(argv=None):
    """!

    Plots the time-frequency misfits of one variable between two tracer files.

    The inputs are CSV trajectory databases of the exahype2.particles tracer action sets.

    @param argv Argument list to parse. Defaults to sys.argv[1:].
    """

    parser = argparse.ArgumentParser(
        description="Script to convert CSV generated by ExaHyPE2 executions into plots"
    )
    parser.add_argument(
        "--f1",
        dest="file_name1",
        type=str,
        required=True,
        help="Name of the first file to be checked, does not need to contain '.csv'",
    )
    parser.add_argument(
        "--f2",
        dest="file_name2",
        type=str,
        required=True,
        help="Name of the second file to be checked, does not need to contain '.csv'",
    )
    parser.add_argument(
        "--v",
        dest="v",
        type=int,
        required=True,
        help="Index of the variable to be plotted",
    )
    parser.add_argument(
        "--tmax",
        dest="t_max",
        type=float,
        default=None,
        help="Maximal time for which data should be plotted",
    )
    parser.add_argument(
        "--tmin",
        dest="t_min",
        type=float,
        default=None,
        help="Minimal time from which data should be plotted",
    )
    parser.add_argument(
        "--x",
        dest="x",
        type=float,
        default=None,
        help="x position of tracer which should be considered",
    )
    parser.add_argument(
        "--y",
        dest="y",
        type=float,
        default=None,
        help="y position of tracer which should be considered",
    )
    parser.add_argument(
        "--z",
        dest="z",
        type=float,
        default=None,
        help="z position of tracer which should be considered",
    )
    parser.add_argument(
        "--o",
        dest="o",
        type=str,
        default=None,
        help="name of output file, if not specified then the output will be shown interactively",
    )
    args = parser.parse_args(argv)

    dt1, vx1 = tracer_extractor(
        args.file_name1,
        var=args.v,
        x=args.x,
        y=args.y,
        z=args.z,
        t_min=args.t_min,
        t_max=args.t_max,
    )
    dt2, vx2 = tracer_extractor(
        args.file_name2,
        var=args.v,
        x=args.x,
        y=args.y,
        z=args.z,
        t_min=args.t_min,
        t_max=args.t_max,
    )

    # We need dt2 to be contained in dt1 for the interpolation to be possible
    if (min(dt2) < min(dt1)) or (max(dt2) > max(dt1)):
        warnings.warn(
            "The second file contains time stamps outside the time interval of the first file, so the misfit uses only its time stamps inside that interval",
            UserWarning,
        )
        dt2, vx2 = zip(
            *(
                (dt, vx)
                for dt, vx in zip(dt2, vx2)
                if (dt >= min(dt1) and dt <= max(dt1))
            )
        )
        dt2 = np.array(dt2)
        vx2 = np.array(vx2)

    interp_func = interp1d(dt1, vx1)
    interpolated_vx1 = interp_func(dt2)

    misfits(dt2, interpolated_vx1, vx2)

    if args.o is not None:
        plt.savefig(args.o)
    else:
        plt.show()


if __name__ == "__main__":
    PlotTimeFrequencyMisfits(sys.argv[1:])
