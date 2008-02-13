// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "OscObject.h"
#include "ValueTimer.h"
#include "CPrecisionClock.h"

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

    float timestep() { return m_fTimestep; }

    const std::vector<Simulation*>& simulationList()
        { return m_simulationList; }

    void add_simulation(Simulation& sim)
        { m_simulationList.push_back(&sim); }

    //! Send a message to all simulations in the list.
    void send(const char *msg, const char *types, ...);

    const lo_address addr() { return m_addr; }
    ValueTimer& valuetimer() { return m_valueTimer; }

  protected:
    pthread_t m_thread;
    bool m_bDone;
    float m_fTimestep;

    PrismFactory *m_pPrismFactory;
    SphereFactory *m_pSphereFactory;

    //! Function for simulation thread.
    static void* run(void* param);
    //! Function for a single step of the simulation.
    virtual void step() = 0;

    //! LibLo address for receiving messages here.
    lo_address m_addr;

    // world objects & constraints
    std::map<std::string,OscObject*> world_objects;
    std::map<std::string,OscConstraint*> world_constraints;

    //! List of other simulations that may receive messages from this one.
    std::vector<Simulation*> m_simulationList;

    //! Timer to ensure simulation steps are distributed in real time.
    cPrecisionClock m_clock;

    //! Object to track values that need to be sent at regular intervals.
    ValueTimer m_valueTimer;
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
