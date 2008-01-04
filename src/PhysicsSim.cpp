// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "PhysicsSim.h"

bool PhysicsPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("PhysicsPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);
    return true;
}

bool PhysicsSphereFactory::create(const char *name, float x, float y, float z)
{
    cODESphere *sp = new cODESphere(world,ode_world,ode_space,0.01);
    sp->setDynamicPosition(cVector3d(x, y, z));
    sp->setDynamicMass(0.5);
    sp->m_material.setStaticFriction(1);
    sp->m_material.setDynamicFriction(0.5);

    OscSphere *obj = new OscSphere(sp, name, m_parent);
    if (obj)
        return simulation()->add_object(*obj);

    return false;
}

PhysicsSim::PhysicsSim(const char *port)
    : Simulation(port)
{
    m_pPrismFactory = new PhysicsPrismFactory(this);
    m_pSphereFactory = new PhysicsSphereFactory(this);
}

PhysicsSim::~PhysicsSim()
{
}

