// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{OPEN_NAMESPACE}
  class {CLASSNAME};
{FORWARD_DECLARATIONS};
{CLOSE_NAMESPACE}

class {FULL_QUALIFIED_CLASSNAME} {{
  public:
    enum class Steps {{
      Undef{ENUM_VARIANTS}
    }};

    static std::string toString( Steps value );

    static int   toProgramStep( Steps value );
    static Steps toStepEnum( int value );
}};
