// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _VISUAL_SIM_H_
#define _VISUAL_SIM_H_

#include "Simulation.h"

#include <CWorld.h>
#include <CCamera.h>
#include <CLight.h>

class VisualSim : public Simulation
{
  public:
    VisualSim(const char *port);
    virtual ~VisualSim();

    cWorld *world() { return m_chaiWorld; }
    cCamera *camera() { return m_chaiCamera; }
    cLight *light() { return m_chaiLight; }

  protected:
    virtual void step();

    cWorld* m_chaiWorld;            //! the world in which we will create our environment
    cCamera* m_chaiCamera;          //! the camera which is used view the environment in a window
    cLight *m_chaiLight;            //! a light source
};

class VisualPrismFactory : public PrismFactory
{
public:
    VisualPrismFactory(Simulation *parent) : PrismFactory(parent) {}
    virtual ~VisualPrismFactory() {}

    virtual VisualSim* simulation() { return static_cast<VisualSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class VisualSphereFactory : public SphereFactory
{
public:
    VisualSphereFactory(Simulation *parent) : SphereFactory(parent) {}
    virtual ~VisualSphereFactory() {}

    virtual VisualSim* simulation() { return static_cast<VisualSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

#endif // _VISUAL_SIM_H_
