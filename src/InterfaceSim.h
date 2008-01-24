// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _INTERFACE_SIM_H_
#define _INTERFACE_SIM_H_

#include "Simulation.h"

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
        : OscSphere(p, name, parent) {}
    virtual ~OscSphereInterface() {}

protected:
    virtual void on_radius();
};

#endif // _INTERFACE_SIM_H_
