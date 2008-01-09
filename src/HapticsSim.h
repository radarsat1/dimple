// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _HAPTICS_SIM_H_
#define _HAPTICS_SIM_H_

#include "Simulation.h"

#include <CWorld.h>
#include <CCamera.h>
#include <CLight.h>
#include <CMeta3dofPointer.h>

class HapticsSim : public Simulation
{
  public:
    HapticsSim(const char *port);
    virtual ~HapticsSim();

    cWorld *world() { return m_chaiWorld; }
    cMeta3dofPointer *cursor() { return m_chaiCursor; }

  protected:
    virtual void step();

    cWorld* m_chaiWorld;            //! the world in which we will create our environment
    cMeta3dofPointer* m_chaiCursor; //! a 3D cursor which represents the haptic deviec
};

class HapticsPrismFactory : public PrismFactory
{
public:
    HapticsPrismFactory(Simulation *parent) : PrismFactory(parent) {}
    virtual ~HapticsPrismFactory() {}

    virtual HapticsSim* simulation() { return static_cast<HapticsSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class HapticsSphereFactory : public SphereFactory
{
public:
    HapticsSphereFactory(Simulation *parent) : SphereFactory(parent) {}
    virtual ~HapticsSphereFactory() {}

    virtual HapticsSim* simulation() { return static_cast<HapticsSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class CHAIObject
{
public:
    CHAIObject(cWorld *world);
    virtual ~CHAIObject();

    virtual cGenericObject *object() = 0;

protected:
};

class OscSphereCHAI : public OscSphere, public CHAIObject
{
public:
    OscSphereCHAI(cWorld *world, const char *name, OscBase *parent=NULL);
    virtual ~OscSphereCHAI();

    virtual cShapeSphere *object() { return m_pSphere; }

protected:
    virtual void onSetRadius();

    cShapeSphere *m_pSphere;
};

#endif // _HAPTICS_SIM_H_
