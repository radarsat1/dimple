// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _INTERFACE_SIM_H_
#define _INTERFACE_SIM_H_

#include "Simulation.h"

// Macros for defining forwarding handlers for OscValue instances to
// the other simulations.

#define FWD_OSCSCALAR(o)                                                \
    virtual void on_##o() {                                             \
        simulation()->send(m_##o.c_path(), "f",                         \
                           m_##o.m_value); }                            \
    static void on_get_##o(void *me, const OscScalar &o, int interval) {\
        ((OscBase*)me)->simulation()->send((o.path()+"/get").c_str(),   \
                                       (interval>=0)?"i":"", interval); }
#define FWD_OSCVECTOR3(o)                                               \
    virtual void on_##o() {                                             \
        simulation()->send(m_##o.c_path(), "fff",                       \
                           m_##o.x, m_##o.y, m_##o.z); }                \
    static void on_get_##o(void *me, const OscVector3 &o, int interval){\
        ((OscBase*)me)->simulation()->send((o.path()+"/get").c_str(),   \
                                        (interval>=0)?"i":"", interval);}
#define FWD_OSCSTRING(o)                                                \
    virtual void on_##o() {                                             \
        simulation()->send(m_##o.c_path(), "s",                         \
                           m_##o.m_value); }                            \
    static void on_get_##o(void *me, const OscString &o, int interval){ \
        ((OscBase*)me)->simulation()->send((o.path()+"/get").c_str(),   \
                                        (interval>=0)?"i":"", interval); }

class InterfaceSim : public Simulation
{
  public:
    InterfaceSim(const char *port);
    virtual ~InterfaceSim();

  protected:
    virtual void step();
};

class InterfacePrismFactory : public PrismFactory
{
public:
    InterfacePrismFactory(Simulation *parent) : PrismFactory(parent) {}
    virtual ~InterfacePrismFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class InterfaceSphereFactory : public SphereFactory
{
public:
    InterfaceSphereFactory(Simulation *parent) : SphereFactory(parent) {}
    virtual ~InterfaceSphereFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class OscSphereInterface : public OscSphere
{
public:
    OscSphereInterface(cGenericObject *p, const char *name, OscBase *parent=NULL)
        : OscSphere(p, name, parent)
        {
            m_position.setGetCallback(on_get_position, this, DIMPLE_THREAD_PHYSICS);
            m_velocity.setGetCallback(on_get_velocity, this, DIMPLE_THREAD_PHYSICS);
            m_accel.setGetCallback(on_get_accel, this, DIMPLE_THREAD_PHYSICS);
            m_color.setGetCallback(on_get_color, this, DIMPLE_THREAD_PHYSICS);
            m_radius.setGetCallback(on_get_radius, this, DIMPLE_THREAD_PHYSICS);
            m_force.setGetCallback(on_get_force, this, DIMPLE_THREAD_PHYSICS);
        }
    virtual ~OscSphereInterface() {}

protected:
    FWD_OSCVECTOR3(position);
    FWD_OSCVECTOR3(velocity);
    FWD_OSCVECTOR3(accel);
    FWD_OSCVECTOR3(color);
    FWD_OSCSCALAR(radius);
    FWD_OSCVECTOR3(force);
};

#endif // _INTERFACE_SIM_H_
