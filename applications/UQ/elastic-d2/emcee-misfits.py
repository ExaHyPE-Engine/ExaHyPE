import csv, subprocess
import arviz as az
import numpy as np
import scipy.stats
import emcee
import matplotlib.pyplot as plt
from cartesian import compile_HHS1
from multiprocessing import Pool

from obspy.signal.tf_misfit import em, pm, plot_tf_misfits

class Benchmark():
    def __init__(self, verbose=False, dimensions=2, level=1):
        self.level=level
        self.verbose = verbose
        self.dimensions=dimensions
        compile_HHS1(dimensions=dimensions)
        candidate = self.run_with_parameters([0.0, 2.0, 0.0])
        # self.data = self.extract_max_velocities_and_timestamps(candidate+"/elastic.csv")
        self.dt, self.data = self.extract_tracers(candidate+"/elastic.csv")
        subprocess.run(["bash", "-c", f"rm -r {candidate}"])

    def get_input_sizes(self):
        return self.dimensions

    def get_output_sizes(self):
        return 1

    def run_with_parameters(self, parameters):
        outputPath = f"sol-"+('-'.join(map(str, parameters)))

        config = """{
    "endTime": 2.0,
    "gridLevels": """ + str(self.level) + """,
    "firstPlotTimeStamp": 0.0,
    "numberOfSnapshots": 0,
    "outputPath": """ + '"' + outputPath + '"' + """,
    "timeStepLogInterval": 10.0,
    "targetDevice": 0,
    "pointSourcePosition": [
        """ + (', '.join(map(str, parameters))) + """
    ]
}"""

        configFile = f"config-{outputPath}.json"
        file = open(configFile, "w")
        file.write(config)
        file.close()

        subprocess.run(["bash", "-c", f"./ExaSeis -cf {configFile}" + (
            "" if self.verbose else ">/dev/null"
            )])
        subprocess.run(["bash", "-c", f"rm -r {configFile}"])
        return outputPath

    def misfits(self, dt, vx, ref, plot=False):
        #single valued phase misfit
        phase_misfit = pm(vx, ref, dt=(dt[1]-dt[0]), fmin=(dt[1]-dt[0]), fmax=(dt[-1]-dt[0]))
        #single valued envelope misfit
        envelope_misfit = em(vx, ref, dt=(dt[1]-dt[0]), fmin=(dt[1]-dt[0]), fmax=(dt[-1]-dt[0]))

        #Would plot the phase misfit, should you desire this.
        if plot:
          plot_tf_misfits(vx, ref, dt=(dt[1]-dt[0]), fmin=(dt[1]-dt[0]), fmax=(dt[-1]-dt[0]))
          # plt.savefig('misfits_u.png', dpi=1000)

        # return phase_misfit, envelope_misfit
        return phase_misfit, envelope_misfit
    # For a given file name representing the csv output to the acoustic model,
    # this parses the csv file and extracts the largest velocities for each tracer,
    # and returns the value of the highest measured velocity as well as the timestamp
    # at which said velocity was measured
    def extract_tracers(self, fileName):
        inputFile = open(fileName, "r")
        csvData = list(csv.reader(inputFile, delimiter=","))

        header = csvData[0]
        timeIndex = header.index(" t")
        dataIndex = header.index(" data(0)")
        firstNumberIndex = header.index("number(0)")
        secondNumberIndex = header.index(" number(1)")
        positionIndexes = [header.index(" x(0)"), header.index(" x(1)")]
        if self.dimensions==3 in header:
            positionIndexes.append(header.index(" x(2)"))

        # Drop the header, plus any trailing blank line the writer may have left behind. Slicing the
        # last row off unconditionally would discard a real snapshot in a file without one.
        # magnitude of velocity
        csvData = [[float(row[firstNumberIndex]), float(row[secondNumberIndex]), float(row[timeIndex]),
            np.sqrt(
                  float(row[dataIndex])**2
                + float(row[dataIndex+1])**2
                + (float(row[dataIndex+2])**2 if self.dimensions==3 else 0)
              )
              ] for row in csvData[1:]]
        # pressure
        # csvData = [[float(row[firstNumberIndex]), float(row[secondNumberIndex]), float(row[timeIndex]), float(row[dataIndex])] for row in csvData[1:]]
        checkFirstIndexLength = set([(data[firstNumberIndex], data[secondNumberIndex]) for data in csvData])

        # max_v, max_dt = [], []
        dt = []
        tracers = []

        for a, b in checkFirstIndexLength:
            data = [d for d in csvData if d[firstNumberIndex]==a and d[secondNumberIndex]==b]
            dt = list(np.array(data).T[timeIndex])
            tracers.append({
              "pos": [a, b],
              "vals": np.array(data).T[3]
            })

        return dt, tracers

    def __call__(self, parameters):

        # Cut-off prior, i.e. if value is outside of domain, return a very large number
        if (
               not ( -2.0 < parameters[0] < 10.)
            or not (0 < parameters[1] < 12.0)
            or (self.dimensions==3 and not (-2.0 < parameters[2] < 10.))
          ):

            return [[-1234]]

        outFile = self.run_with_parameters(parameters)
        # model_output = self.extract_max_velocities_and_timestamps(outFile+"/elastic.csv")
        dt, model_output = self.extract_tracers(outFile+"/elastic.csv")
        subprocess.run(["bash", "-c", f"rm -r {outFile}"])

        phase_misfits = []
        envelope_misfits = []
        for i in range(len(model_output)):
          pm, em = self.misfits(self.dt, model_output[i]["vals"], self.data[i]["vals"])
          phase_misfits.append(pm)
          envelope_misfits.append(em)

        likelihood_std_dev_height   = 0.1
        likelihood_std_dev_time     = 1.5
        likelihood_cov_matrix_diag = [likelihood_std_dev_height**2]*4 + [likelihood_std_dev_time**2]*4

        # posterior = scipy.stats.multivariate_normal.logpdf(model_output, self.data, likelihood_cov_matrix_diag)
        posterior = scipy.stats.multivariate_normal.logpdf(envelope_misfits+phase_misfits, np.zeros(8), likelihood_cov_matrix_diag)
        return [[posterior]]

if __name__ == '__main__':
    dimensions = 2 
    benchmark = Benchmark(dimensions=dimensions, level=2)

    nWalkers = 8 #number of parallel chains sampling
    nSteps = 200
    #start with randomly distributed values within the domain
    p0=np.random.uniform(low=[-2., 0., -2.][0:dimensions], high=[10., 12., 10.][0:dimensions], size=(nWalkers, dimensions))

    with Pool(processes=nWalkers) as pool:
        # More info about the emcee sampler at https://emcee.readthedocs.io/en
        sampler = emcee.EnsembleSampler(nWalkers, benchmark.get_input_sizes(), benchmark, pool=pool)
        # run sampler
        state = sampler.run_mcmc(
            initial_state=p0,
            nsteps=nSteps,
            progress=True
        ) #number of samples

    # plot results
    inference_data = az.from_emcee(sampler)
    az.plot_pair(inference_data)
    plt.title(az.summary(inference_data, round_to=2))
    plt.tight_layout()
    plt.savefig(f'emcee_misfits{nSteps}.png')

    print(az.summary(inference_data, round_to=2))

