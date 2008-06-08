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

    virtual void on_clear();

  protected:
    virtual void initialize();
    virtual void step();

    void initGlutWindow();
    static void updateDisplay(int data);
    static void draw();
    static void key(unsigned char key, int x, int y);

    int m_nWidth, m_nHeight;

    cWorld* m_chaiWorld;            //! the world in which we will create our environment
    cCamera* m_chaiCamera;          //! the camera which is used view the environment in a window
    cLight *m_chaiLight;            //! a light source

    /** GLUT callback functions require a pointer to the VisualSim
     ** object, but do not have a user-specified data parameter.  On
     ** the assumption that all callback functions are called
     ** subsquently after the timer callback, this static pointer is
     ** used to point to the one and only VisualSim instance to give
     ** the callback functions context. */
    static VisualSim *m_pGlobalContext;
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
