// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "OscObject.h"

class SphereFactory;
class PrismFactory;

//! A Simulation is an OSC-controlled simulation thread which contains
//! a scene graph.  It is inherited by the specific simulation, be it
//! physics, haptics, or other.
class Simulation : public OscBase
{
  public:
    Simulation(const char *port);
    virtual ~Simulation();
    
    bool add_object(OscObject& obj);
    bool delete_object(OscObject& obj) {}

  protected:
    pthread_t m_thread;
    bool m_bDone;

    PrismFactory *m_pPrismFactory;
    SphereFactory *m_pSphereFactory;

    //! Function for simulation thread
    static void* run(void* param);

    // world objects & constraints
    std::map<std::string,OscObject*> world_objects;
    std::map<std::string,OscConstraint*> world_constraints;
};

class ShapeFactory : public OscBase
{
public:
    ShapeFactory(char *name, Simulation *parent);
    virtual ~ShapeFactory();

    virtual Simulation* simulation() { return static_cast<Simulation*>(m_parent); }

protected:
    // message handlers
};

class PrismFactory : public ShapeFactory
{
public:
    PrismFactory(Simulation *parent);
    virtual ~PrismFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, float x, float y, float z) = 0;
};

class SphereFactory : public ShapeFactory
{
public:
    SphereFactory(Simulation *parent);
    virtual ~SphereFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, float x, float y, float z) = 0;
};

#endif // _SIMULATION_H_
