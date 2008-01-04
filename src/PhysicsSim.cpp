// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "PhysicsSim.h"

bool PhysicsSphereFactory::create(const char *name, float radius)
{
    printf("PhysicsSphereFactory (%s) is creating a sphere object called '%s'\n",
           m_parent->c_name(), name);
    return true;
}

PhysicsSim::PhysicsSim(const char *port)
    : Simulation(port)
{
    m_pPrismFactory = new PrismFactory(this);
    m_pSphereFactory = new PhysicsSphereFactory(this);
}

PhysicsSim::~PhysicsSim()
{
}

