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
    return true;
}

bool InterfaceSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphere *obj = new OscSphereInterface(NULL, name, m_parent);
    if (!(obj && simulation()->add_object(*obj)))
            return false;

    simulation()->send("/world/sphere/create", "sfff", name, x, y, z);

    return true;
}

/****** InterfaceSim ******/

InterfaceSim::InterfaceSim(const char *port)
    : Simulation(port)
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

/****** OscSphereInterface ******/

void OscSphereInterface::on_radius()
{
    InterfaceSim *sim = static_cast<InterfaceSim*>(m_parent);
    sim->send((std::string("/world/")
               +m_name+"/radius").c_str(), "f",
              m_radius.m_value);
}
