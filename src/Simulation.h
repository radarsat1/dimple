// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "OscObject.h"

class ShapeFactory : public OscBase
{
public:
    ShapeFactory(char *name, OscBase *parent);
    virtual ~ShapeFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);
};

class PrismFactory : public ShapeFactory
{
public:
    PrismFactory(char *name, OscBase *parent);
    virtual ~PrismFactory();
};

class SphereFactory : public ShapeFactory
{
public:
    SphereFactory(char *name, OscBase *parent);
    virtual ~SphereFactory();
};

class Simulation : public OscBase
{
  public:
    Simulation(int port);
    virtual ~Simulation();

    PrismFactory m_prismFactory;
    SphereFactory m_sphereFactory;

  protected:
    lo_server m_server;

    // world objects & constraints
    std::map<std::string,OscObject*> world_objects;
    std::map<std::string,OscConstraint*> world_constraints;
};

#endif // _SIMULATION_H_
