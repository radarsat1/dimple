// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _INTERFACE_SIM_H_
#define _INTERFACE_SIM_H_

#include "Simulation.h"
#include "OscObject.h"

// Macros for defining forwarding handlers for OscValue instances to
// the other simulations.

#define FWD_OSCSCALAR(o,t)                                              \
    virtual void on_##o() {                                             \
        simulation()->send(0,m_##o.c_path(), "f",                       \
                           m_##o.m_value); }                            \
    static void on_get_##o(void *me, OscScalar &o, int interval) {      \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                       (o.path()+"/get").c_str(),       \
                                       (interval>=0)?"i":"", interval); }
#define FWD_OSCVECTOR3(o,t)                                             \
    virtual void on_##o() {                                             \
        simulation()->send(0,m_##o.c_path(), "fff",                     \
                           m_##o.x, m_##o.y, m_##o.z); }                \
    static void on_get_##o(void *me, OscVector3 &o, int interval){      \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                        (o.path()+"/get").c_str(),      \
                                        (interval>=0)?"i":"", interval);}
#define FWD_OSCSTRING(o,t)                                              \
    virtual void on_##o() {                                             \
        simulation()->send(0,m_##o.c_path(), "s",                       \
                           m_##o.m_value); }                            \
    static void on_get_##o(void *me, OscString &o, int interval){       \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                        (o.path()+"/get").c_str(),      \
                                        (interval>=0)?"i":"", interval);}

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

class InterfaceHingeFactory : public HingeFactory
{
public:
    InterfaceHingeFactory(Simulation *parent) : HingeFactory(parent) {}
    virtual ~InterfaceHingeFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z, double ax, double ay, double az);
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

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscObject::on_destroy();
    }

protected:
    FWD_OSCVECTOR3(position,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(velocity,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(accel,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(color,Simulation::ST_VISUAL);
    FWD_OSCSCALAR(radius,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(force,Simulation::ST_PHYSICS);
};

class OscPrismInterface : public OscPrism
{
public:
    OscPrismInterface(cGenericObject *p, const char *name, OscBase *parent=NULL)
        : OscPrism(p, name, parent)
        {
            m_position.setGetCallback(on_get_position, this, DIMPLE_THREAD_PHYSICS);
            m_velocity.setGetCallback(on_get_velocity, this, DIMPLE_THREAD_PHYSICS);
            m_accel.setGetCallback(on_get_accel, this, DIMPLE_THREAD_PHYSICS);
            m_color.setGetCallback(on_get_color, this, DIMPLE_THREAD_PHYSICS);
            m_force.setGetCallback(on_get_force, this, DIMPLE_THREAD_PHYSICS);
            m_size.setGetCallback(on_get_size, this, DIMPLE_THREAD_PHYSICS);
        }
    virtual ~OscPrismInterface() {}

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscObject::on_destroy();
    }

protected:
    FWD_OSCVECTOR3(position,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(velocity,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(accel,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(color,Simulation::ST_VISUAL);
    FWD_OSCVECTOR3(force,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(size,Simulation::ST_PHYSICS);
};

class OscHingeInterface : public OscHinge
{
public:
    OscHingeInterface(const char *name, OscBase *parent,
                      OscObject *object1, OscObject *object2,
                      double x, double y, double z, double ax, double ay, double az)
        : OscHinge(name, parent, object1, object2, x, y, z, ax, ay, az) {}

    virtual ~OscHingeInterface() {}

protected:
};

#endif // _INTERFACE_SIM_H_
