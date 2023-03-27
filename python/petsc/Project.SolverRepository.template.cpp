#include "SolverRepository.h"



using namespace {{NAMESPACE | join("::")}};

{% for SOLVER in SOLVER_INSTANCES %}

{{SOLVER[0]}}  {{SOLVER[1]}};

{% endfor %}


