// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _HAPTICS_SIM_H_
#define _HAPTICS_SIM_H_

#include "Simulation.h"
#include "OscObject.h"

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
    virtual void initialize();
    virtual void step();

    //! A step count for dividing down to the visual timestep
    int m_nVisualStepCount;

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
    virtual void on_position()
      { object()->setPos(m_position); }
    virtual void on_rotation()
      { object()->setRot(m_rotation); }
    virtual void on_radius();
    virtual void on_color()
      { object()->m_material.m_diffuse.set(m_color.x, m_color.y, m_color.z); }

    cShapeSphere *m_pSphere;
};

class OscPrismCHAI : public OscPrism, public CHAIObject
{
public:
    OscPrismCHAI(cWorld *world, const char *name, OscBase *parent=NULL);
    virtual ~OscPrismCHAI();

    virtual cMesh *object() { return m_pPrism; }

protected:
    virtual void on_position()
      { object()->setPos(m_position); }
    virtual void on_rotation()
      { object()->setRot(m_rotation); }
    virtual void on_color()
      { object()->m_material.m_diffuse.set(m_color.x, m_color.y, m_color.z); }
    virtual void on_size();

    //! Create a cMesh with a prism structure.
    void createPrism(bool openbox=false);

    cMesh *m_pPrism;
};

#endif // _HAPTICS_SIM_H_
