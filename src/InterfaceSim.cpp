// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "InterfaceSim.h"
#include "HapticsSim.h"

#include <GL/glut.h>
#ifdef USE_FREEGLUT
#include <GL/freeglut_ext.h>
#endif

bool InterfacePrismFactory::create(const char *name, float x, float y, float z)
{
    printf("InterfacePrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);

    OscPrism *obj = new OscPrismInterface(NULL, name, m_parent);
    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->traceOn();

    simulation()->send(0, "/world/prism/create", "sfff", name, x, y, z);

    return true;
}

bool InterfaceSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphere *obj = new OscSphereInterface(NULL, name, m_parent);
    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->traceOn();

    simulation()->send(0, "/world/sphere/create", "sfff", name, x, y, z);

    return true;
}

/****** InterfaceSim ******/

InterfaceSim::InterfaceSim(const char *port)
    : Simulation(port, ST_INTERFACE)
{
    m_pPrismFactory = new InterfacePrismFactory(this);
    m_pSphereFactory = new InterfaceSphereFactory(this);

    m_fTimestep = 1;
}

InterfaceSim::~InterfaceSim()
{
}

void InterfaceSim::step()
{
}
