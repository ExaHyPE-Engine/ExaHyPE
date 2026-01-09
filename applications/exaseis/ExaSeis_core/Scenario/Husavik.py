# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class Husavik(Scenario):
    """
    Simulates an earthquake in the north Iceland region, around the Húsavı́k-Flatey fault.
    The scenario was originally implemented by Leonhard Rannabauer, and most of the files
    can be found in his zenodo repository at https://zenodo.org/records/6386282, except for
    the material parameters which are missing.
    Similar results should be reproduceable from https://zenodo.org/records/8021690 and 
    https://zenodo.org/records/7990543, with the later being where we recovered the
    material parameters from.

    The description of the scenario and results be found in Leonhard Rannabauer's thesis,
      in section 8.5: https://mediatum.ub.tum.de/node?id=1639212
    """

    domain_offset   = [-48.1, 0.0, -48.2]
    domain_size     = [96.4, 96.4, 96.4]

    end_time = 25.0

    #These tracers were simply copied from Leonhard Rannabauer's work, where they were hardcoded
    # for the ExaHyPE1 case where the domain was divided into 241 cells, as such they are likely
    # to be slightly off from the correct position in all cases
    tracer_coordinates = [
      [9.376232362467874,  0.0001, 33.85099388956314],  #body6.75st33.85dp0.09
      [8.246753305766788,  0.0001, 11.638967936983136], #body5.48st11.64dp0.03
      [0.7195568229321125, 0.0001, 17.21881025466928],  #body-1.53st17.22dp-0.00
      [6.327888926556476,  0.0001, 25.284520456084465], #body3.82st25.28dp0.07
      [0.6536504638487628, 0.0001, 8.417249776348854],  #body-1.13st8.42dp-0.19

      [14.164660762183253, 0.0001, 15.356787444428392], #body10.95st15.36dp0.10
      [1.3017916972263137, 0.0001, 15.034320297763315], #body-1.05st15.03dp-0.00
      [25.435497238874177, 0.0001, 15.401601578655573], #body22.92st15.40dp0.16
      [17.176126224582518, 0.0001, 7.010609949850732],  #body15.19st7.01dp-0.07
      [-4.492292804805409, 0.0001, 2.131659303817287],  #body-4.98st2.13dp-0.64

      [0.6555189792723084, 0.0001, -12.686892531023204], #body1.98st-12.69dp-0.04
      [26.253429292802664, 0.0001, -6.9879120653472455], #body27.40st-6.99dp0.04
      [24.94953750613164,  0.0001, -20.029540641285166], #body27.62st-20.03dp-0.74
      [0.8540967046388133, 0.0001, -13.184495969619547], #body2.34st-13.18dp-0.06
      [5.621345523615631,  0.0001, -9.743473279568661],  #body6.72st-9.74dp-0.05

      [-1.6476579634324366, 0.0001, -19.484646795241588], #body0.57st-19.48dp0.06
      [17.766222982618473,  0.0001, -28.1068295777377],   #body21.14st-28.11dp-0.91
      [25.39664055692495,   0.0001, -33.966402224667945], #body28.13st-33.97dp-0.37
      [28.08399500705491,   0.0001, -24.61055416454974],  #body30.73st-24.61dp-0.55
      [36.51336697284584,   0.0001, -42.29182654225224],  #body38.37st-42.29dp-0.13

      [9.376232362467874, -0.08808115836556851,   33.85099388956314],   #body6.75st33.85dp0.00
      [8.246753305766788, -0.027295721133612005,  11.638967936983136],  #body5.48st11.64dp0.00
      [0.7195568229321125, 0.0011934993109008474, 17.21881025466928],   #body-1.53st17.22dp0.00
      [6.327888926556476, -0.06958613567893224,   25.284520456084465],  #body3.82st25.28dp0.00
      [0.6536504638487628, 0.18714757066801724,   8.417249776348854],   #body-1.13st8.42dp0.00.

      [14.164660762183253, -0.09536041761818126,    15.356787444428392],  #body10.95st15.36dp0.00
      [1.3017916972263137,  0.0029445021370122504,  15.034320297763315],  #body-1.05st15.03dp0.00
      [25.435497238874177, -0.1627025699401088,     15.401601578655573],  #body22.92st15.40dp0.00
      [17.176126224582518,  0.0665791035924741,     7.010609949850732],   #body15.19st7.01dp0.00
      [-4.492292804805409,  0.6364933043727774,     2.131659303817287],   #body-4.98st2.13dp0.00

      [0.6555189792723084,  0.0353022881074736,   -12.686892531023204], #body1.98st-12.69dp0.00
      [26.253429292802664, -0.03509966432142164,  -6.9879120653472455], #body27.40st-6.99dp0.00
      [24.94953750613164,   0.7298889865128331,   -20.029540641285166], #body27.62st-20.03dp0.00
      [0.8540967046388133,  0.05651829134625075,  -13.184495969619547], #body2.34st-13.18dp0.00
      [5.621345523615631,   0.0534422072164773,   -9.743473279568661],  #body6.72st-9.74dp0.00

      [-1.6476579634324366, -0.06441300382012116, -19.484646795241588], #body0.57st-19.48dp0.00
      [17.766222982618473,   0.8969251457533431,  -28.1068295777377],   #body21.14st-28.11dp0.00
      [25.39664055692495,    0.3685598288835763,  -33.966402224667945], #body28.13st-33.97dp0.00
      [28.08399500705491,    0.5510383744940981,  -24.61055416454974],  #body30.73st-24.61dp0.00
      [36.51336697284584,    0.12614619980480768, -42.29182654225224]   #body38.37st-42.29dp0.00
    ]

    def initial_conditions(self):
        return """
        double rho, mu, lambda;

        easi::ArraysAdapter<double> adapter;
        adapter.addBindingPoint("rho",    &rho);
        adapter.addBindingPoint("mu",     &mu);
        adapter.addBindingPoint("lambda", &lambda);

        easi::Query query(1,3);
        query.group(0) = 0;
        query.x(0,0) = x[0];
        query.x(0,1) = x[1];
        query.x(0,2) = x[2];
        context->model->evaluate(query,adapter);

        Q[Shortcuts::rho] = rho;
        Q[Shortcuts::cp]  = sqrt((lambda + 2*mu)/rho);
        Q[Shortcuts::cs]  = sqrt(mu/rho);
"""
    
    def generate_required_files(self, order):

        dictionary = { "MODE": order+1 }
        self.generate_file_from_template(
            os.path.dirname(os.path.realpath(__file__))+"/specs/Husavik/husavik.yaml.template",
            "husavik.yaml", dictionary
        )

        for file in ["husavik_fault.yaml", "initial_stress.yaml", "material_parameters.nc", "material.yaml", "north_iceland_fault.nc", "north_iceland_topography_96_4.nc"]:
          self.copy_file_to_current_folder(
              os.path.dirname(os.path.realpath(__file__))+"/specs/Husavik/" + file, 
              file
          )

        return
