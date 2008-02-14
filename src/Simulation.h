// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "OscObject.h"
#include "ValueTimer.h"
#include "CPrecisionClock.h"

class SphereFactory;
class PrismFactory;

//! Simulation info contains copies of information needed to send a
//! simulation a message or stream of messages.
class SimulationInfo
{
public:
    SimulationInfo(lo_address addr, float timestep, int type)
        : m_addr(addr), m_fTimestep(timestep), m_type(type) {}

    lo_address addr() { return m_addr; }
    float timestep() { return m_fTimestep; }
    int type() { return m_type; }

protected:
    lo_address m_addr;
    float m_fTimestep;
    int m_type;
};

//! A Simulation is an OSC-controlled simulation thread which contains
//! a scene graph.  It is inherited by the specific simulation, be it
//! physics, haptics, or other.
class Simulation : public OscBase
{
  public:
    Simulation(const char *port, int type);
    virtual ~Simulation();

    //! An enumeration for the possible inherited types of simulations.
    enum SimulationType {
        ST_PHYSICS   = 0x01,
        ST_HAPTICS   = 0x02,
        ST_VISUAL    = 0x04,
        ST_INTERFACE = 0x08,
        ST_ALL       = 0x0F
    };

    //! Return the type of this simulation.
    int type() { return m_type; }
    
    bool add_object(OscObject& obj);
    bool delete_object(OscObject& obj) {}

    float timestep() { return m_fTimestep; }

    //! Return the list of receivers for messages from this simulation.
    const std::vector<SimulationInfo>& simulationList()
        { return m_simulationList; }

    //! Add a simulation to the list of possible receivers for
    //! messages from this simulation.
    void add_simulation(Simulation& sim)
        { m_simulationList.push_back(SimulationInfo(sim.m_addr, sim.m_fTimestep, sim.type())); }

    //! Send a message to all simulations in the list.
    void send(bool throttle, const char *path, const char *types, ...);

    //! Send a message to all simulations of one or more specific types.
    void sendtotype(int type, bool throttle, const char *path, const char *types, ...);

    const lo_address addr() { return m_addr; }
    ValueTimer& valuetimer() { return m_valueTimer; }

  protected:
    pthread_t m_thread;
    bool m_bDone;
    int m_type;
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
    std::vector<SimulationInfo> m_simulationList;

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
