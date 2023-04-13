// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "LoadBalancing.h"
#include "State.h"


namespace toolbox {
  namespace loadbalancing {
    class Cascade;
  }
}


/**
 * Cascade of load balancing schemes
 *
 * We run through a series of load balancing schemes. If one is stagnating or
 * switching off, we switch to the next one.
 *
 * I tried to play around with variadic templates, but that did not work for
 * multiple reasons, so I went down the manual way.
 */
template<typename HostedLoadBalancing0, typename HostedLoadBalancing1>
class toolbox::loadbalancing::Cascade: public toolbox::loadbalancing::LoadBalancing {
  public:
    Cascade(
      Configuration* configuration,
      bool deleteInDestructor
    ):
      _configuration(configuration),
      _deleteInDestructor(deleteInDestructor),
      _hostedLoadBalancing0(hostedLoadBalancing0, false),
      _hostedLoadBalancing1(hostedLoadBalancing1, false),
      _activeLoadBalancing(0) {
    }


    /**
     * Delete configuration if this is wanted by the user. Note that we
     * may not delete the object directly in the constructor - even though
     * it seems not be used within this class. It is used by the aggregates.
     */
    virtual ~Cascade() {
      if (deleteInDestructor) {
        delete _configuration;
      }
    }


    /**
     * Inform the active aggregate about finishStep() and then check
     * afterwards if the active load balancing has stagnated. If so,
     * switch to the next one in the cascade.
     */
    void finishStep() override {
      switch (_activeLoadBalancing) {
        case 0:
          {
            _hostedLoadBalancing0.finishStep();
            if (_hostedLoadBalancing0.hasStagnated()()) _activeLoadBalancing++;
          }
          break;
        case 1:
          {
            _hostedLoadBalancing1.finishStep();
          }
          break;
        default:
          assertion1( false, _activeLoadBalancing );
          break;
      }
    }


    bool hasSplitRecently() const override {
      switch (_activeLoadBalancing) {
        case 0:
          return _hostedLoadBalancing0.hasSplitRecently();
        case 1:
          return _hostedLoadBalancing1.hasSplitRecently();
        default:
          assertion1(false,_activeLoadBalancing);
          break;
      }
      return false;
    }


    int getGlobalNumberOfTrees() const override {
      switch (_activeLoadBalancing) {
        case 0:
          return _hostedLoadBalancing0.getGlobalNumberOfTrees();
        case 1:
          return _hostedLoadBalancing1.getGlobalNumberOfTrees();
        default:
          assertion1(false,_activeLoadBalancing);
          break;
      }
      return -1;
    }


    virtual bool hasStagnated() const override {
      return _state == State::SwitchedOff
          or (
            _activeLoadBalancing == 1
            and
            _hostedLoadBalancing1.hasStagnated()
          );
    }


    virtual void finishSimulation() override {
      switch (_activeLoadBalancing) {
        case 0:
          _hostedLoadBalancing0.finishSimulation();
          break;
        case 1:
          _hostedLoadBalancing1.finishSimulation();
          break;
        default:
          assertion1(false,_activeLoadBalancing);
          break;
      }
    }
  private:
    Configuration*        _configuration;
    bool                  _deleteInDestructor;

    HostedLoadBalancing0  _hostedLoadBalancing0;
    HostedLoadBalancing1  _hostedLoadBalancing1;

    /**
     * Pick the active load balancing.
     */
    int                   _activeLoadBalancing;
};
