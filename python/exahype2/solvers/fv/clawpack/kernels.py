# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import jinja2


def create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping( time_step_size ):
  return """
  cellTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
  cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
"""


def create_compute_Riemann_kernel_for_ClawPack():
  Template = jinja2.Template( """
        double wave[{{NUMBER_OF_UNKNOWNS}}][{{NUMBER_OF_UNKNOWNS}}];
        double speed[{{NUMBER_OF_UNKNOWNS}}];

        int num_eqn   = {{NUMBER_OF_UNKNOWNS}};
        int num_aux   = {{NUMBER_OF_AUXILIARY_VARIABLES}};
        int num_waves = {{NUMBER_OF_UNKNOWNS}};

        {{CLAWPACK_RIEMANN_SOLVER}}_(
          {%if DISCRIMINATE_NORMAL %}
            &normal,
          {% endif %}
          &num_eqn,
          &num_aux,
          &num_waves,
          QL,                                 // double* q_l
          QR,                                 // double* q_r
          QL+{{NUMBER_OF_UNKNOWNS}},          // double* aux_l
          QR+{{NUMBER_OF_UNKNOWNS}},          // double* aux_r
          wave,
          speed,
          FL,                                 // double* amdq
          FR                                  // double* apdq
        );

        for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) {
          FL[i] = -FL[i];
        }
""", undefined=jinja2.DebugUndefined)
  
  d= {}
  return Template.render(**d)
