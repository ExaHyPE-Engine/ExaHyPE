# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class TPV34(Scenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Imperial Fault, Model 1. Spontaneous rupture on a vertical strike-slip fault. 
    There is a 3D velocity structure in a linear elastic half-space. The velocity structure is derived from 
    SCEC Community Velocity Model CVM-H in the vicinity of the Imperial Fault. 
    Initial shear and normal stresses are proportional to the shear modulus.  

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv34docs.html
    """

    domain_offset   = [3.8,  0., 3.8] #centers the fault which is at 20.
    domain_size     = [32.4, 32.4, 32.4] #leads to a cell size of 0.333 for 81 cells

    end_time = 20.0

    tracer_coordinates = [
          [17.0, 0.0, 10.0],
          [17.0, 2.4, 10.0],
          [17.0, 0.0,  0.0],
          [17.0, 2.4,  0.0],
          [17.0, 0.0, 20.0],
          [17.0, 2.4, 20.0],
          [17.0, 0.0, 30.0],
          [17.0, 2.4, 30.0],
          [17.0, 0.0, 40.0],
          [17.0, 2.4, 40.0],

          [11.0, 0.0, 10.0],
          [11.0, 2.4, 10.0],
          [11.0, 0.0,  0.0],
          [11.0, 2.4,  0.0],
          [11.0, 0.0, 20.0],
          [11.0, 2.4, 20.0],
          [11.0, 0.0, 30.0],
          [11.0, 2.4, 30.0],
          [11.0, 0.0, 40.0],
          [11.0, 2.4, 40.0],

          [ 5.0, 0.0,  5.0],
          [ 5.0, 2.4,  5.0],
          [ 5.0, 0.0, 20.0],
          [ 5.0, 2.4, 20.0],
          [ 5.0, 0.0, 35.0],
          [ 5.0, 2.4, 35.0],

          [20.0, 0.0,  0.0],
          [20.0, 2.4,  0.0],
          [20.0, 0.0, 40.0],
          [20.0, 2.4, 40.0],

          [23.0, 0.0, 10.0],
          [23.0, 2.4, 10.0],
          [23.0, 0.0,  0.0],
          [23.0, 2.4,  0.0],
          [23.0, 0.0, 20.0],
          [23.0, 2.4, 20.0],
          [23.0, 0.0, 30.0],
          [23.0, 2.4, 30.0],
          [23.0, 0.0, 40.0],
          [23.0, 2.4, 40.0],

          [29.0, 0.0, 10.0],
          [29.0, 2.4, 10.0],
          [29.0, 0.0,  0.0],
          [29.0, 2.4,  0.0],
          [29.0, 0.0, 20.0],
          [29.0, 2.4, 20.0],
          [29.0, 0.0, 30.0],
          [29.0, 2.4, 30.0],
          [29.0, 0.0, 40.0],
          [29.0, 2.4, 40.0],

          [35.0, 0.0,  5.0],
          [35.0, 2.4,  5.0],
          [35.0, 0.0, 20.0],
          [35.0, 2.4, 20.0],
          [35.0, 0.0, 35.0],
          [35.0, 2.4, 35.0]
    ]

    fault_coordinates = [
          [20.0,  0.0, 14.0],
          [20.0,  1.0, 14.0],
          [20.0,  2.4, 14.0],
          [20.0,  5.0, 14.0],
          [20.0,  7.5, 14.0],
          [20.0, 10.0, 14.0],
          [20.0, 12.0, 14.0],

          [20.0,  0.0, 8.0],
          [20.0,  1.0, 8.0],
          [20.0,  2.4, 8.0],
          [20.0,  5.0, 8.0],
          [20.0,  7.5, 8.0],
          [20.0, 10.0, 8.0],
          [20.0, 12.0, 8.0],

          [20.0,  0.0, 20.0],
          [20.0,  1.0, 20.0],
          [20.0,  2.4, 20.0],
          [20.0,  5.0, 20.0],
          [20.0,  7.5, 20.0],
          [20.0, 10.0, 20.0],
          [20.0, 12.0, 20.0],

          [20.0,  0.0, 26.0],
          [20.0,  1.0, 26.0],
          [20.0,  2.4, 26.0],
          [20.0,  5.0, 26.0],
          [20.0,  7.5, 26.0],
          [20.0, 10.0, 26.0],
          [20.0, 12.0, 26.0],

          [20.0,  0.0, 32.0],
          [20.0,  1.0, 32.0],
          [20.0,  2.4, 32.0],
          [20.0,  5.0, 32.0],
          [20.0,  7.5, 32.0],
          [20.0, 10.0, 32.0],
          [20.0, 12.0, 32.0],
    ]

    def initial_conditions(self):
        return """
	double rho, cs, cp;

        easi::ArraysAdapter<double> adapter;
        adapter.addBindingPoint("rho",    &rho);
        adapter.addBindingPoint("cs",     &cs);
        adapter.addBindingPoint("cp",     &cp);

        easi::Query query(1,3);
        query.group(0) = 0;
        query.x(0,0) = x[0]-20.0;
        query.x(0,1) = x[1];
        query.x(0,2) = x[2]-20.0;
        context->model->evaluate(query,adapter);

        Q[Shortcuts::rho] = rho;
        Q[Shortcuts::cs]  = cs;
        Q[Shortcuts::cp]  = cp;

"""
    
    def generate_required_files(self, order):

        dictionary = { "MODE": order+1 }
        self.generate_file_from_template(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV34/tpv34.yaml.template",
            "tpv34.yaml", dictionary
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV34/tpv34_fault.yaml", 
            "tpv34_fault.yaml"
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV34/material.yaml", 
            "material.yaml"
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV34/tpv34_mu_mult_plus.nc",
            "tpv34_mu_mult_plus.nc"
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV34/tpv34_rhovsvp.nc", 
            "tpv34_rhovsvp.nc"
        )

        return