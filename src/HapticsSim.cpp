// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "HapticsSim.h"

bool HapticsPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("HapticsPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);
    return true;
}

bool HapticsSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereCHAI *obj = new OscSphereCHAI(simulation()->world(),
                                           name, m_parent);
    if (obj)
        return simulation()->add_object(*obj);

    return false;
}

/****** HapticsSim ******/

HapticsSim::HapticsSim(const char *port)
    : Simulation(port)
{
    m_pPrismFactory = new HapticsPrismFactory(this);
    m_pSphereFactory = new HapticsSphereFactory(this);

    m_fTimestep = HAPTICS_TIMESTEP_MS/1000.0;
    printf("CHAI timestep: %f\n", m_fTimestep);

    // create the world object
    m_chaiWorld = new cWorld();
    m_chaiWorld->setBackgroundColor(0.0f,0.0f,0.0f);
}

HapticsSim::~HapticsSim()
{
}

void HapticsSim::step()
{
}

/****** CHAIObject ******/

CHAIObject::CHAIObject(cWorld *world)
{
}

CHAIObject::~CHAIObject()
{
}

/****** OscSphereCHAI ******/

OscSphereCHAI::OscSphereCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent), CHAIObject(world)
{
    m_pSphere = new cShapeSphere(0.01);
    world->addChild(m_pSphere);
}

OscSphereCHAI::~OscSphereCHAI()
{
    if (m_pSphere)
        delete m_pSphere;
}

void OscSphereCHAI::on_radius()
{
    printf("OscSphereCHAI::on_radius(). radius = %f\n", m_radius.m_value);

    if (!m_pSphere)
        return;

    m_pSphere->setRadius(m_radius.m_value);
}
