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
    OscSphereODE *obj = new OscSphereODE(ode_world, ode_space, name, m_parent);
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

/****** ODEObject ******/

ODEObject::ODEObject(dWorldID &odeWorld, dSpaceID &odeSpace)
    : m_odeWorld(odeWorld), m_odeSpace(odeSpace)
{
    m_odeGeom = NULL;
    m_odeBody = NULL;
/*
    m_odeBody = dBodyCreate(m_odeWorld);
    dBodySetPosition(m_odeBody, 0, 0, 0);
*/
}

ODEObject::~ODEObject()
{
    if (m_odeBody)  dBodyDestroy(m_odeBody);
    if (m_odeGeom)  dGeomDestroy(m_odeGeom);
}

/****** OscSphereODE ******/

OscSphereODE::OscSphereODE(dWorldID &odeWorld, dSpaceID &odeSpace, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent), ODEObject(odeWorld, odeSpace)
{
/*
    m_odeGeom = dCreateSphere(m_odeSpace, 0.1);
    dMassSetSphere(&m_odeMass, 0.1, 0.1);
    dBodySetMass(m_odeBody, &m_odeMass);
    dGeomSetBody(m_odeGeom, m_odeBody);
    dBodySetPosition(m_odeBody, 0, 0, 0);
*/
}

void OscSphereODE::onSetRadius()
{
    printf("OscSphereODE::onSetRadius()\n");
}
