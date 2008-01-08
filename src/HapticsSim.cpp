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

// TODO    m_chaiWorld = dWorldCreate();
    m_fTimestep = HAPTICS_TIMESTEP_MS/1000.0;
    printf("CHAI timestep: %f\n", m_fTimestep);
}

HapticsSim::~HapticsSim()
{
}

void HapticsSim::step()
{
}

/****** CHAIObject ******/

CHAIObject::CHAIObject(cWorld &world)
{
    m_pObject = NULL;
}

CHAIObject::~CHAIObject()
{
    if (m_pObject)
        delete m_pObject;
}

/****** OscSphereCHAI ******/

OscSphereCHAI::OscSphereCHAI(cWorld &world, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent), CHAIObject(world)
{
}

void OscSphereCHAI::onSetRadius()
{
    printf("OscSphereCHAI::onSetRadius(). radius = %f\n", m_radius.m_value);
}
