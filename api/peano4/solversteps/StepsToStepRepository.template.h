// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{OPEN_NAMESPACE}
  class {CLASSNAME};
  /**
   * Forward declarations
   */
{FORWARD_DECLARATIONS};
{CLOSE_NAMESPACE}

class {FULL_QUALIFIED_CLASSNAME} {{
  public:
    enum class Steps {{
      Undef{ENUM_VARIANTS}
    }};

    /**
     * Convert step into string
     */
    static std::string toString( Steps value );

    /**
     * Convert step into program step, so you can use it in
     * ::peano4::parallel::Node::getInstance().setNextProgramStep()
     */
    static int   toProgramStep( Steps value );
    static Steps toStepEnum( int value );
}};
