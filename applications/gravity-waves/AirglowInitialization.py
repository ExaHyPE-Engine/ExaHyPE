def get_initial_steady_state_for_airglow():
    return """
      /***********
      * OH model *
      ************/
      // states v = 9, ..., 0
      for (int vibrationalState = 9; vibrationalState >= 0; --vibrationalState) {
        double productionOfOHState = 0.0;
        double lossOfOHState = 0.0;
        const int stateIndex = Shortcuts::OH0 + vibrationalState;
        // PRODUCTIONS
        // k5 production
        productionOfOHState += ProductionRates::k5(T, vibrationalState) * H * O3;
        if (vibrationalState < 9) {
          // production via Einstein coefficients by quenching from higher states within bandwidth
          for (int higherVibrationalState = std::min(vibrationalState + BANDWIDTH, 9); higherVibrationalState > vibrationalState; --higherVibrationalState) {
            productionOfOHState += EINSTEIN_COEFFICIENTS(higherVibrationalState, vibrationalState) * Q[Shortcuts::OH0 + higherVibrationalState];
          }

          // production via k8 by quenching from all higher states
          for (int higherVibrationalState = 9; higherVibrationalState > vibrationalState; --higherVibrationalState) {
            productionOfOHState += ProductionRates::k8(higherVibrationalState, vibrationalState) * O2 * Q[Shortcuts::OH0 + higherVibrationalState]; 
          }

          // production via k9 by quenching from next higher vibrational state
          productionOfOHState += ProductionRates::k9(vibrationalState + 1) * N2 * Q[stateIndex + 1]; 
        }

        // LOSSES
        // loss via k7 by quenching into hydrogen and molecular oxygen
        lossOfOHState += ProductionRates::k7(vibrationalState) * O;
        if (vibrationalState > 0) {
          // loss via k9 by quenching into next lower vibrational state
          lossOfOHState += ProductionRates::k9(vibrationalState) * N2;
          
          // loss via k8 by quenching into all lower states
          for (int quenchedState = vibrationalState - 1; quenchedState >= 0; --quenchedState) {
            lossOfOHState += ProductionRates::k8(vibrationalState, quenchedState) * O2; 
          }

          // loss via Einstein coefficients by quenching into lower states within bandwidth
          for (int quenchedState = vibrationalState - 1; quenchedState >= std::max(0, vibrationalState - BANDWIDTH); --quenchedState) {
            lossOfOHState += EINSTEIN_COEFFICIENTS(vibrationalState, quenchedState);
          }
        }

        if (productionOfOHState == 0.0 and lossOfOHState == 0.0) {
          Q[stateIndex] = 0.0;
        } else {
          Q[stateIndex] = productionOfOHState / lossOfOHState;
        }
      }

      /***********
      * OI model *
      ***********/

      // Singlet molecular oxygen in odd, non-symmetric sigma state
      const double productionOfO2cu = (ProductionRates::ZETA * ProductionRates::k1(T) * O * O * M);
      const double lossOfO2cu = (ProductionRates::k2() * O2 + ProductionRates::k3() * (1 + ProductionRates::DELTA) * O + ProductionRates::A1());
      Q[Shortcuts::O2cu] = productionOfO2cu / lossOfO2cu;

      // Singlet atomic oxygen in excited state S
      const double productionOfOS = (ProductionRates::DELTA * ProductionRates::k3() * O * Q[Shortcuts::O2cu]);
      const double lossOfOS = (ProductionRates::k4(T) * O2 + ProductionRates::A2() + ProductionRates::A5577());
      Q[Shortcuts::OS] = productionOfOS / lossOfOS;"""


def get_steady_state_for_ozone_MSISE(
    csv_file_path: str,
    csv_separator=";",
    csv_hydrogen_key="Hden(cm-3)",
    csv_height_key="Heit(km)",
    plot_path: str = "",
    transparent_background: bool = False,
):
    """!
    csv_file_path:
    Path to MSISE CSV file containing hydrogen profile.

    csv_separator:
    Delimiter used in CSV file. Default is ";".

    csv_hydrogen_key:
    Key for hydrogen profile used in CSV file. Default is "Hden(cm-3)".

    csv_height_key:
    Key for height in km used in CSV file. Default is "Heit(km)".

    plot_path:
    If the exponential fit shall be plotted and saved, pass over a path here. Default is "" meaning no plotting.

    transparent_background:
    Transparent background for plot if True. Does nothing if plot_path is "". Default is False.

    This function can be used to create an ozone initial state,
    which depends on an exponential fit of the hydrogen density
    instead of the actual hydrogen profile.

    This is due to the hydrogen tending to 0 shortly after
    the OH peak altitude of ca. 83km. Ozone would thus tend to infinity
    for these lower altitudes. However, its actual minimum should be lower
    than where the hydrogen density reaches 0 (ca. 75km-80km).

    Therefore, the hydrogen density is fit to an exponential curve allowing
    the ozone to decay for altitudes less than 80km. This additional ozone
    is not in steady state and thus needs further correction in the source term.
    """

    # calculate ozone based on vertically exponentially decaying fit of hydrogen profile
    import pandas as pd
    from scipy.optimize import curve_fit
    import numpy as np

    # DATA PREPROCESSING

    # read in MSISE profile from CSV
    dataframe = pd.read_csv(csv_file_path, delimiter=csv_separator)
    # filter columns; we only need height and hydrogen
    dataframe = dataframe[[csv_height_key, csv_hydrogen_key]]
    # km to m
    dataframe["Heit(m)"] = dataframe[csv_height_key].mul(1e3)
    # sort by height descending, makes it easier later to find sign switch in derivative
    dataframe = dataframe.sort_values(by="Heit(m)", ascending=False)

    # HYDROGEN DERIVATIVE BY HEIGHT

    # calculate dH/dy
    hydrogen = dataframe[csv_hydrogen_key].to_numpy()
    derivative_of_hydrogen = np.diff(hydrogen)

    # identify index at which sign flips from + to - indicating last value taken for exponential fit
    sign_switch_index = 0
    for i in range(0, derivative_of_hydrogen.size):
        # i marks turning point and it is also the last turning point
        if derivative_of_hydrogen[i] < 0 and np.all(derivative_of_hydrogen[i:] <= 0):
            sign_switch_index = i
            break
    else:
        # no sign flip detected in derivative => no exponential fit possible
        raise ValueError(
            "Expected decrease in hydrogen in lower atmosphere. Exponential fit for ozone not possible"
        )

    # EXPONENTIAL FIT

    # extract values from numpy array / dataframe that are used to fit exponential function
    # revert sorting by height by going in -1 stride
    hydrogen_values_to_fit = hydrogen[sign_switch_index::-1]
    height_values_to_fit = dataframe["Heit(m)"].to_numpy()[sign_switch_index::-1]
    hydrogen_log = np.log(hydrogen_values_to_fit)

    # Define the exponential function to fit
    def exp_func(x, a, b, c):
        return a * np.exp(b * x) + c

    def lin_func(x, a, b):
        return a + b * x

    # fit linear function of logarithmic values as initial guess
    (a, b), _ = curve_fit(lin_func, height_values_to_fit, hydrogen_log)
    a = np.exp(a)
    # fit exponential function using fit of logarithmic function as initial guess
    (a, b, c), _ = curve_fit(
        exp_func,
        height_values_to_fit,
        hydrogen_values_to_fit,
        p0=[a, b, np.max(hydrogen_values_to_fit)],
    )

    if plot_path != "":
        print("a:", a, ", b:", b, ", c:", c)
        import matplotlib.pyplot as plt
        from os.path import splitext

        # Generate fitted curve
        hydrogen_fit = exp_func(dataframe["Heit(m)"], a, b, c)

        plt.plot(hydrogen, dataframe["Heit(m)"], label="MSISE")
        plt.plot(
            hydrogen_fit, dataframe["Heit(m)"], color="red", label="Exponential Fit"
        )

        # scales
        plt.xscale("log")

        # labels
        plt.ylabel("Height (km)")
        plt.xlabel(r"[H] (cm$^{-3}$)")
        plt.title("Exponential Fit for Hydrogen Profile")
        plt.ylim(bottom=0, top=dataframe["Heit(m)"].max())
        plt.legend(loc="upper left")
        current_yticks = plt.yticks()
        plt.yticks(
            ticks=current_yticks[0], labels=((current_yticks[0] / 1e3).astype(np.int64))
        )
        plt.tight_layout()
        plt.savefig(plot_path, transparent=transparent_background)

        plt.clf()

        # Generate fitted curve for subsection
        hydrogen_fit = exp_func(height_values_to_fit, a, b, c)

        plt.plot(hydrogen_values_to_fit, height_values_to_fit, label="MSISE")
        plt.plot(
            hydrogen_fit, height_values_to_fit, color="red", label="Exponential Fit"
        )

        # scales
        plt.xscale("log")
        plt.ylim(height_values_to_fit[0], height_values_to_fit[-1])

        # labels
        plt.ylabel("Height (km)")
        plt.xlabel(r"[H] (cm$^{-3}$)")

        height_min_km = height_values_to_fit[0] / 1e3
        height_max_km = height_values_to_fit[-1] / 1e3

        height_min_km = (
            height_min_km if not height_min_km.is_integer() else int(height_min_km)
        )
        height_max_km = (
            height_max_km if not height_max_km.is_integer() else int(height_max_km)
        )

        plt.title(
            f"Exponential Fit for Hydrogen Profile from {height_min_km}km to {height_max_km}km"
        )
        plt.legend()
        current_yticks = plt.yticks()
        plt.yticks(
            ticks=current_yticks[0], labels=((current_yticks[0] / 1e3).astype(np.int64))
        )
        plt.tight_layout()
        plt.savefig(
            "_subsection".join(splitext(plot_path)), transparent=transparent_background
        )

    # OZONE FUNCTION

    return (
        """{
        auto hydrogenFit = [](const double altitude) {
          return """
        + str(a)
        + """ * tarch::la::pow(tarch::la::E, """
        + str(b)
        + """ * altitude) + """
        + str(c)
        + """;
        };
        const double hydrogenPrime = hydrogenFit(x(1));
        double lossOfO3 = 0.0;
        for (int vibrationalState = 0; vibrationalState < 10; ++vibrationalState) {
          lossOfO3 += ProductionRates::k5(T, vibrationalState) * hydrogenPrime;
        }

        // k6 production of ozone
        const double productionOfO3 = (ProductionRates::k6(T) * M * O * O2);
        Q[Shortcuts::O3] = productionOfO3 / lossOfO3;
      }
      const {{COMPUTE_PRECISION}} O3 = Q[Shortcuts::O3];"""
    )
