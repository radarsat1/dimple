// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "OscObject.h"

class ShapeFactory : OscBase
{
public:
    ShapeFactory(char *name, char *classname);
    virtual ~ShapeFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);
};

class PrismFactory : ShapeFactory
{
public:
    PrismFactory(char *name, char *classname);
    virtual ~PrismFactory();
};

class SphereFactory : ShapeFactory
{
public:
    SphereFactory(char *name, char *classname);
    virtual ~SphereFactory();
};

class Simulation : OscBase
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
