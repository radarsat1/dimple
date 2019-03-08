// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include <thread>
#include <mutex>
#include <condition_variable>

#include "OscValue.h"
#include "ValueTimer.h"
#include "timers/CPrecisionClock.h"
#include "LoQueue.h"

class SphereFactory;
class PrismFactory;
class MeshFactory;
class HingeFactory;
class Hinge2Factory;
class FixedFactory;
class FreeFactory;
class BallJointFactory;
class SlideFactory;
class PistonFactory;
class UniversalFactory;

class OscObject;
class OscConstraint;

//! SimulationReceiver contains copies of information needed to send a
//! simulation a message or stream of messages.
class SimulationReceiver
{
public:
    SimulationReceiver(const char *url, int type);
    SimulationReceiver(Simulation &sim);

    lo_address addr() { return m_addr; }
    float timestep() { return m_fTimestep; }
    int type() { return m_type; }

    LoQueue m_queue;

    void send_lo_message(const char *path, lo_message msg);

protected:
    lo_address m_addr;
    float m_fTimestep;
    int m_type;
    bool m_bUseQueue;
};

//! A Simulation is an OSC-controlled simulation thread which contains
//! a scene graph.  It is inherited by the specific simulation, be it
//! physics, haptics, or other.
class Simulation : public OscBase
{
  public:
    Simulation(const char *port, int type);
    virtual ~Simulation();

    bool start();
    void stop();

    //! An enumeration for the possible inherited types of simulations.
    enum SimulationType {
        ST_UNKNOWN   = 0x00,
        ST_PHYSICS   = 0x01,
        ST_HAPTICS   = 0x02,
        ST_VISUAL    = 0x04,
        ST_INTERFACE = 0x08,
        ST_ALL       = 0x0F
    };

    //! Return the type of this simulation.
    int type() { return m_type; }

    //! Return a string giving the type of this simulation.
    const char* type_str();
    const char* type_str(int type);
    SimulationType str_type(const char *type);

    bool add_object(OscObject& obj);
    bool delete_object(OscObject& obj);
    OscObject* find_object(const char* name);

    bool add_constraint(OscConstraint& obj);
    bool delete_constraint(OscConstraint& obj);

    //! Set the grabbed object or ungrab by setting to NULL.
    virtual void set_grabbed(OscObject *pGrabbed)
        { m_pGrabbedObject = pGrabbed; }

    float timestep() { return m_fTimestep; }

    //! Return the list of receivers for messages from this simulation.
    const std::vector<SimulationReceiver*>& simulationList()
        { return m_receiverList; }

    //! Add a receiver to the list of possible receivers for
    //! messages from this simulation.
    void add_receiver(Simulation *sim, const char *spec,
                      Simulation::SimulationType type,
                      bool initialization);

    //! Add a queue to the list of queues to poll for messages.
    void add_queue(LoQueue *queue)
    // TODO: mutexes here, but this is only done once at the beginning
    // so we're probably safe.
        { m_queueList.push_back(queue); }

    //! Send a message to all simulations in the list.
    void send(bool throttle, const char *path, const char *types, ...);

    //! Send a message to all simulations of one or more specific types.
    void sendtotype(int type, bool throttle, const char *path, const char *types, ...);

    const lo_address addr() { return m_addr; }
    ValueTimer& valuetimer() { return m_valueTimer; }

    OSCSCALAR(Simulation, collide) {};
    OSCVECTOR3(Simulation, gravity) {};
    OSCMETHOD0(Simulation, clear);
    OSCMETHOD0(Simulation, reset_workspace) {};
    OSCMETHOD0(Simulation, drop) { set_grabbed(NULL); }
    OSCMETHOD1S(Simulation, add_receiver);
    OSCMETHOD2S(Simulation, add_receiver_url);
    OSCMETHOD1S(Simulation, remove_receiver);
    OSCVECTOR3(Simulation, scale) {};
    OSCSCALAR(Simulation, stiffness) {};
    OSCSCALAR(Simulation, grab_stiffness) {};
    OSCSCALAR(Simulation, grab_damping) {};
    OSCSCALAR(Simulation, grab_feedback) {};

    void run_unthreaded()
      { run(this); }

  protected:
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    bool m_bStarted;
    bool m_bDone;
    int m_type;
    float m_fTimestep;

    /*! True if this simulation should time itself according to m_fTimestep.
     * 
     * Certain simulations work better if they are timed in their
     * subclassed step() function instead of being self-timed in
     * run().  Defaults to true.
     */
    bool m_bSelfTimed;

    PrismFactory *m_pPrismFactory;
    SphereFactory *m_pSphereFactory;
    MeshFactory *m_pMeshFactory;
    HingeFactory *m_pHingeFactory;
    Hinge2Factory *m_pHinge2Factory;
    FixedFactory *m_pFixedFactory;
    FreeFactory *m_pFreeFactory;
    BallJointFactory *m_pBallJointFactory;
    SlideFactory *m_pSlideFactory;
    PistonFactory *m_pPistonFactory;
    UniversalFactory *m_pUniversalFactory;

    //! Track the grabbed object
    OscObject *m_pGrabbedObject;

    //! Function for simulation thread (thread context).
    static void* run(void* param);
    //! Override for initializing the simulation (thread context).
    virtual void initialize();
    //! Override for a single step of the simulation (thread context).
    virtual void step() = 0;

    //! LibLo address for receiving messages here.
    lo_address m_addr;

    // world objects & constraints
    std::map<std::string,OscObject*> world_objects;
    std::map<std::string,OscConstraint*> world_constraints;

    typedef std::map<std::string,OscObject*>::iterator object_iterator;
    typedef std::map<std::string,OscConstraint*>::iterator constraint_iterator;

    //! List of other simulations that may receive messages from this one.
    std::vector<SimulationReceiver*> m_receiverList;

    //! List of FIFO queues to check for incoming messages.
    std::vector<LoQueue*> m_queueList;

    //! Timer to ensure simulation steps are distributed in real time.
    cPrecisionClock m_clock;

    //! Object to track values that need to be sent at regular intervals.
    ValueTimer m_valueTimer;

    //! Map for collecting sent messages, for the purpose of throttling.
    std::map<std::string, int> sent_messages;
    typedef std::map<std::string, int>::iterator sent_messages_iterator;

    //! Decide whether or not to send a message or throttle it.
    bool should_throttle(const char *path, SimulationReceiver& sim_to);
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

class MeshFactory : public ShapeFactory
{
public:
    MeshFactory(Simulation *parent);
    virtual ~MeshFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, const char *filename,
                        float x, float y, float z) = 0;
};

class HingeFactory : public ShapeFactory
{
public:
    HingeFactory(Simulation *parent);
    virtual ~HingeFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, OscObject *object1, OscObject *object2,
                        double x, double y, double z, double ax, double ay, double az) = 0;
};

class Hinge2Factory : public ShapeFactory
{
public:
    Hinge2Factory(Simulation *parent);
    virtual ~Hinge2Factory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, OscObject *object1, OscObject *object2,
                        double x, double y, double z,
                        double a1x, double a1y, double a1z,
                        double a2x, double a2y, double a2z) = 0;
};

class FixedFactory : public ShapeFactory
{
public:
    FixedFactory(Simulation *parent);
    virtual ~FixedFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, OscObject *object1, OscObject *object2) = 0;
};

class FreeFactory : public ShapeFactory
{
public:
    FreeFactory(Simulation *parent);
    virtual ~FreeFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, OscObject *object1, OscObject *object2) = 0;
};

class BallJointFactory : public ShapeFactory
{
public:
    BallJointFactory(Simulation *parent);
    virtual ~BallJointFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, OscObject *object1, OscObject *object2,
                        double x, double y, double z) = 0;
};

class SlideFactory : public ShapeFactory
{
public:
    SlideFactory(Simulation *parent);
    virtual ~SlideFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, OscObject *object1, OscObject *object2,
                        double ax, double ay, double az) = 0;
};

class PistonFactory : public ShapeFactory
{
public:
    PistonFactory(Simulation *parent);
    virtual ~PistonFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, OscObject *object1, OscObject *object2,
                        double x, double y, double z, double ax, double ay, double az) = 0;
};

class UniversalFactory : public ShapeFactory
{
public:
    UniversalFactory(Simulation *parent);
    virtual ~UniversalFactory();

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    // override these functions with a specific factory subclass
    virtual bool create(const char *name, OscObject *object1, OscObject *object2,
                        double x, double y, double z,
                        double a1x, double a1y, double a1z,
                        double a2x, double a2y, double a2z) = 0;
};

#endif // _SIMULATION_H_
