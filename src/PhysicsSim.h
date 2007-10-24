// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _PHYSICS_SIM_H_
#define _PHYSICS_SIM_H_

#include "Simulation.h"

class PhysicsSim : public Simulation
{
  public:
    PhysicsSim(const char *port);
    virtual ~PhysicsSim();

  protected:
};

#endif // _PHYSICS_SIM_H_
