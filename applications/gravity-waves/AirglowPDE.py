def get_max_eigenvalue():
    return """
      const {{COMPUTE_PRECISION}} u_n = Q[Shortcuts::u + normal];
      return std::fabs(u_n);
  """


def get_flux():
    return """// set flux for all densities to zero
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
        F[unknown] = 0.0;
      }

      // based on Snively et al. "OH and OI airglow layer modulation by ducted short-period gravity waves: Effects of trapping altitude", 2009
      // advect only those species, which have lifetimes long enough to be affected by gravity waves

      // advect major species densities
      const {{COMPUTE_PRECISION}} u_n = Q[Shortcuts::u + normal];
      F[Shortcuts::O]   = u_n * Q[Shortcuts::O];
      F[Shortcuts::O2]  = u_n * Q[Shortcuts::O2];
      F[Shortcuts::N2]  = u_n * Q[Shortcuts::N2];

      // advect long-lived minor species densities
      F[Shortcuts::O3]  = u_n * Q[Shortcuts::O3];
      F[Shortcuts::H]   = u_n * Q[Shortcuts::H];"""


def get_source_term():
    """!
    Returns source term of production and loss for minor species.
    """
    return """using ProductionRates = applications::exahype2::GravityWaves::Airglow::ProductionRates;

      const {{COMPUTE_PRECISION}} O   = Q[Shortcuts::O];      // atomic oxygen
      const {{COMPUTE_PRECISION}} O2  = Q[Shortcuts::O2];     // molecular oxygen
      const {{COMPUTE_PRECISION}} N2  = Q[Shortcuts::N2];     // nitrogen
      const {{COMPUTE_PRECISION}} O3  = Q[Shortcuts::O3];     // ozone
      const {{COMPUTE_PRECISION}} H   = Q[Shortcuts::H];      // hydrogen
      const {{COMPUTE_PRECISION}} M   = O + O2 + N2;  // total major species density
      const {{COMPUTE_PRECISION}} T   = Q[Shortcuts::T];
      const int BANDWIDTH = EINSTEIN_BANDWIDTH;

      // set source term for all densities to zero
      for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
        S[unknown] = 0.0;
      }

      /*****************
      * Major species *
      *****************/
      // major species do not have any loss or production
      // as it is assumed negligible
      S[Shortcuts::O] = 0.0;
      S[Shortcuts::O2] = 0.0;
      S[Shortcuts::N2] = 0.0;

      /*******************************
      * Production of minor species *
      *******************************/

      // k6 production of ozone
      S[Shortcuts::O3] += (ProductionRates::k6(T) * M * O * O2);
      S[Shortcuts::H] = 0.0;

      // OH model production
      for (int vibrationalState = 0; vibrationalState < 10; ++vibrationalState) {
        const int stateIndex = Shortcuts::OH0 + vibrationalState;

        // k7 production of hydrogen
        S[Shortcuts::H] += ProductionRates::k7(vibrationalState) * O * Q[stateIndex];

        // k5 production of OH(v)
        S[stateIndex] += ProductionRates::k5(T, vibrationalState) * H * O3;

        if (vibrationalState < 9) {
          // production via Einstein coefficients by quenching from higher states within bandwidth
          for (int higherVibrationalState = std::min(vibrationalState + BANDWIDTH, 9); higherVibrationalState > vibrationalState; --higherVibrationalState) {
            S[stateIndex] += EINSTEIN_COEFFICIENTS(higherVibrationalState, vibrationalState) * Q[Shortcuts::OH0 + higherVibrationalState];
          }

          // production via k8 by quenching from all higher states
          for (int higherVibrationalState = 9; higherVibrationalState > vibrationalState; --higherVibrationalState) {
            S[stateIndex] += ProductionRates::k8(higherVibrationalState, vibrationalState) * O2 * Q[Shortcuts::OH0 + higherVibrationalState]; 
          }

          // production via k9 by quenching from next higher vibrational state
          S[stateIndex] += ProductionRates::k9(vibrationalState + 1) * N2 * Q[stateIndex + 1];
        }
      }

      // OI model production
      // Singlet molecular oxygen in odd, non-symmetric sigma state
      S[Shortcuts::O2cu] += (ProductionRates::ZETA * ProductionRates::k1(T) * O * O * M);
      // Singlet atomic oxygen in excited state S
      S[Shortcuts::OS] += (ProductionRates::DELTA * ProductionRates::k3() * O * Q[Shortcuts::O2cu]);

      /*************************
      * Loss of minor species *
      *************************/
      // OH model loss
      for (int vibrationalState = 0; vibrationalState < 10; ++vibrationalState) {
        // loss via k5 from OH(v) production
        S[Shortcuts::H] -= ProductionRates::k5(T, vibrationalState) * H * O3;
        S[Shortcuts::O3] -= ProductionRates::k5(T, vibrationalState) * H * O3;

        const int stateIndex = Shortcuts::OH0 + vibrationalState;
        // loss via Einstein coefficients by quenching into lower states within bandwidth
        for (int quenchedState = vibrationalState - 1; quenchedState >= std::max(0, vibrationalState - BANDWIDTH); --quenchedState) {
          S[stateIndex] -= EINSTEIN_COEFFICIENTS(vibrationalState, quenchedState) * Q[stateIndex];
        }

        // loss via k7 by quenching into hydrogen and molecular oxygen
        S[stateIndex] -= ProductionRates::k7(vibrationalState) * O * Q[stateIndex];

        // loss via k8 by quenching into all lower states
        for (int quenchedState = vibrationalState - 1; quenchedState >= 0; --quenchedState) {
          S[stateIndex] -= ProductionRates::k8(vibrationalState, quenchedState) * O2 * Q[stateIndex]; 
        }

        if (vibrationalState > 0) {
          // loss via k9 by quenching into next lower vibrational state
          S[stateIndex] -= ProductionRates::k9(vibrationalState) * N2 * Q[stateIndex];
        }
      }

      // OI model loss
      S[Shortcuts::O2cu] -= (ProductionRates::k2() * O2 + ProductionRates::k3() * (1 + ProductionRates::DELTA) * O + ProductionRates::A1()) * Q[Shortcuts::O2cu];
      S[Shortcuts::OS] -= (ProductionRates::k4(T) * O2 + ProductionRates::A2() + ProductionRates::A5577()) * Q[Shortcuts::OS];

      /***************************
      * Remove initial tendency *
      ***************************/
      S[Shortcuts::O3] -= Q[Shortcuts::dO3dt0];

      /*******************
      * Apply threshold *
      *******************/
      for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
        const {{COMPUTE_PRECISION}} value = S[unknown];
        S[unknown] = (std::fabs(value) < CHEMISTRY_NUMERICAL_THRESHOLD) ? 0.0 : value;
      }"""


def get_initial_tendency_postprocessing():
    return """
      if (timeStamp == 0.0 and timeStepSize > 0.0) {
        // initialize initial tendency for ozone
        // this is copied and adapted from toolbox::blockstructured::copyUnknown()
#if DIMENSIONS==2
        for (int x=0; x<{{NUMBER_OF_VOLUMES_PER_AXIS}}; x++)
        for (int y=0; y<{{NUMBER_OF_VOLUMES_PER_AXIS}}; y++) {
          const int ozoneIndex = 3;
          int oldQWithHaloOzoneIndex = ( x + {{HALO_SIZE}} + (y + {{HALO_SIZE}}) * ({{NUMBER_OF_VOLUMES_PER_AXIS}} + 2 * {{HALO_SIZE}}) ) * {{NUMBER_OF_UNKNOWNS + NUMBER_OF_AUXILIARY_VARIABLES}} + ozoneIndex;
          int newQIndex = ( x + y * {{NUMBER_OF_VOLUMES_PER_AXIS}} ) * {{NUMBER_OF_UNKNOWNS + NUMBER_OF_AUXILIARY_VARIABLES}};
          int newQOzoneIndex = newQIndex + ozoneIndex;
          int newQOzoneInitialTendencyIndex = newQIndex + {{NUMBER_OF_UNKNOWNS + NUMBER_OF_AUXILIARY_VARIABLES}} - 1; // dO3dt0 should always be the last auxiliary variable
          // initialize initial tendency for ozone
          QOut[newQOzoneInitialTendencyIndex] = (QOut[newQOzoneIndex] - QIn[oldQWithHaloOzoneIndex]) / timeStepSize; 
          // set ozone change to 0, since with initial tendency removed, there should be no change in ozone
          QOut[newQOzoneIndex] = QIn[oldQWithHaloOzoneIndex];
        }
#else
        for (int x=0; x<{{NUMBER_OF_VOLUMES_PER_AXIS}}; x++)
        for (int y=0; y<{{NUMBER_OF_VOLUMES_PER_AXIS}}; y++)
        for (int z=0; z<{{NUMBER_OF_VOLUMES_PER_AXIS}}; z++){
          const int ozoneIndex = 3;
          int oldQWithHaloOzoneIndex = ( x + {{HALO_SIZE}} + (y + {{HALO_SIZE}}) * ({{NUMBER_OF_VOLUMES_PER_AXIS}} + 2 * {{HALO_SIZE}}) + (z + {{HALO_SIZE}} ) * ({{NUMBER_OF_VOLUMES_PER_AXIS}} + 2 * {{HALO_SIZE}}) * ({{NUMBER_OF_VOLUMES_PER_AXIS}} + 2 * {{HALO_SIZE}})) * {{NUMBER_OF_UNKNOWNS + NUMBER_OF_AUXILIARY_VARIABLES}} + ozoneIndex;
          int newQIndex = ( x + y * {{NUMBER_OF_VOLUMES_PER_AXIS}} + z * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} ) * {{NUMBER_OF_UNKNOWNS + NUMBER_OF_AUXILIARY_VARIABLES}};
          int newQOzoneIndex = newQIndex + ozoneIndex;
          int newQOzoneInitialTendencyIndex = newQIndex + {{NUMBER_OF_UNKNOWNS + NUMBER_OF_AUXILIARY_VARIABLES}} - 1; // dO3dt0 should always be the last auxiliary variable
          // initialize initial tendency for ozone
          QOut[newQOzoneInitialTendencyIndex] = (QOut[newQOzoneIndex] - QIn[oldQWithHaloOzoneIndex]) / timeStepSize;
          // set ozone change to 0, since with initial tendency removed, there should be no change in ozone
          QOut[newQOzoneIndex] = QIn[oldQWithHaloOzoneIndex];
        }
#endif
      }
"""
