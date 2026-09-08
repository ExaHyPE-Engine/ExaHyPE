// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StepRepository.h"

#include "runner/RunControl.h"

/** Returns the generated name of one step. */
std::string {FULL_QUALIFIED_CLASSNAME}::toString( Steps value ) {{
  switch (value) {{
    case Steps::Undef: return "Undef";
{STRING_RETURN_CASE}
  }}
  return "<undef>";
}}

/** Returns the program-step identifier of one generated step. */
int {FULL_QUALIFIED_CLASSNAME}::toProgramStep( Steps value ) {{
  switch (value) {{
    case Steps::Undef: return ::runner::RunControl::UndefProgramStep;
{INT_RETURN_CASE}
  }}
  return ::runner::RunControl::Terminate;
}}

/** Returns the generated step for one program-step identifier. */
{FULL_QUALIFIED_CLASSNAME}::Steps {FULL_QUALIFIED_CLASSNAME}::toStepEnum( int value ) {{
  switch (value) {{
{STEP_RETURN_CASE}
  }}
  return Steps::Undef;
}}
