// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StepRepository.h"

#include "peano4/parallel/RunControl.h"

std::string {FULL_QUALIFIED_CLASSNAME}::toString( Steps value ) {{
  switch (value) {{
    case Steps::Undef: return "Undef";
{STRING_RETURN_CASE}
  }}
  return "<undef>";
}}

int {FULL_QUALIFIED_CLASSNAME}::toProgramStep( Steps value ) {{
  switch (value) {{
    case Steps::Undef: return ::peano4::parallel::RunControl::UndefProgramStep;
{INT_RETURN_CASE}
  }}
  return ::peano4::parallel::RunControl::Terminate;
}}

{FULL_QUALIFIED_CLASSNAME}::Steps {FULL_QUALIFIED_CLASSNAME}::toStepEnum( int value ) {{
  switch (value) {{
{STEP_RETURN_CASE}
  }}
  return Steps::Undef;
}}
