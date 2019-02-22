// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#include <chrono>
#include <cerrno>

#include <lo/lo.h>

#include "config.h"
#include "dimple.h"
#include "Simulation.h"
#include "OscObject.h"

ShapeFactory::ShapeFactory(char *name, Simulation *parent)
    : OscBase(name, parent)
{
}

ShapeFactory::~ShapeFactory()
{
}

PrismFactory::PrismFactory(Simulation *parent)
    : ShapeFactory("prism", parent)
{
    // Name, Width, Height, Depth
    addHandler("create", "sfff", create_handler);
}

PrismFactory::~PrismFactory()
{
}

int PrismFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                 int argc, void *data, void *user_data)
{
    PrismFactory *me = static_cast<PrismFactory*>(user_data);

    // Optional position, default (0,0,0)
    cVector3d pos;
    if (argc>0)
        pos.x(argv[1]->f);
    if (argc>1)
        pos.y(argv[2]->f);
    if (argc>2)
        pos.z(argv[3]->f);

    OscObject *o = me->simulation()->find_object(&argv[0]->s);
    if (o)
        printf("[%s] Already an object named %s\n",
               me->simulation()->type_str(), &argv[0]->s);
    else
        if (!me->create(&argv[0]->s, pos.x(), pos.y(), pos.z()))
            printf("[%s] Error creating sphere '%s'.\n",
                   me->simulation()->type_str(), &argv[0]->s);

    return 0;
}

SphereFactory::SphereFactory(Simulation *parent)
    : ShapeFactory("sphere", parent)
{
    // Name, Radius
    addHandler("create", "sfff", create_handler);
}

SphereFactory::~SphereFactory()
{
}

int SphereFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    SphereFactory *me = static_cast<SphereFactory*>(user_data);

    // Optional position, default (0,0,0)
    cVector3d pos;
    if (argc>0)
        pos.x(argv[1]->f);
    if (argc>1)
        pos.y(argv[2]->f);
    if (argc>2)
        pos.z(argv[3]->f);

    OscObject *o = me->simulation()->find_object(&argv[0]->s);
    if (o)
        printf("[%s] Already an object named %s\n",
               me->simulation()->type_str(), &argv[0]->s);
    else
        if (!me->create(&argv[0]->s, pos.x(), pos.y(), pos.z()))
            printf("[%s] Error creating sphere '%s'.\n",
                   me->simulation()->type_str(), &argv[0]->s);

    return 0;
}

MeshFactory::MeshFactory(Simulation *parent)
    : ShapeFactory("mesh", parent)
{
    // Name, Filename, Width, Height, Depth
    addHandler("create", "ssfff", create_handler);
}

MeshFactory::~MeshFactory()
{
}

int MeshFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                 int argc, void *data, void *user_data)
{
    MeshFactory *me = static_cast<MeshFactory*>(user_data);

    // Optional position, default (0,0,0)
    cVector3d pos;
    if (argc>1)
        pos.x(argv[2]->f);
    if (argc>2)
        pos.y(argv[3]->f);
    if (argc>3)
        pos.z(argv[4]->f);

    OscObject *o = me->simulation()->find_object(&argv[0]->s);
    if (o)
        printf("[%s] Already an object named %s\n",
               me->simulation()->type_str(), &argv[0]->s);
    else
        if (!me->create(&argv[0]->s, &argv[1]->s, pos.x(), pos.y(), pos.z()))
            printf("[%s] Error creating mesh '%s' (%s).\n",
                   me->simulation()->type_str(), &argv[0]->s, &argv[1]->s);

    return 0;
}

HingeFactory::HingeFactory(Simulation *parent)
    : ShapeFactory("hinge", parent)
{
    // Name, object1, object2, x, y, z, axis x, y, z
    addHandler("create", "sssffffff", create_handler);
}

HingeFactory::~HingeFactory()
{
}

int HingeFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    HingeFactory *me = static_cast<HingeFactory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 9) return -1;

    if (strcmp(&argv[1]->s, "world")!=0)
        object1 = me->simulation()->find_object(&argv[1]->s);
    if (strcmp(&argv[2]->s, "world")!=0)
        object2 = me->simulation()->find_object(&argv[2]->s);

    // Swap objects if one is world
    if (object2 && !object1) {
        object1 = object2;
        object2 = 0;
    }

    // At least one object must exist
    if (!object1) {
        printf("[%s] Error creating hinge constraint '%s', "
               "object '%s' not found.\n",
               me->simulation()->type_str(), &argv[0]->s,
               &argv[1]->s);
        return -1;
    }

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    if (!me->create(&argv[0]->s, object1, object2,
                    argv[3]->f, argv[4]->f, argv[5]->f,
                    argv[6]->f, argv[7]->f, argv[8]->f))
        printf("[%s] Error creating hinge '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

Hinge2Factory::Hinge2Factory(Simulation *parent)
    : ShapeFactory("hinge2", parent)
{
    // Name, object1, object2, x, y, z, a1x, a1y, a1z, a2x, a2y, a2z
    addHandler("create", "sssfffffffff", create_handler);
}

Hinge2Factory::~Hinge2Factory()
{
}

int Hinge2Factory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    Hinge2Factory *me = static_cast<Hinge2Factory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 12) return -1;

    // For Hinge2, both must be objects, 'world' is invalid
    if (   strcmp(&argv[1]->s, "world")==0
        || strcmp(&argv[2]->s, "world")==0)
    {
        printf("[%s] Cannot create hinge2 constraint with 'world', "
               "two objects are required.\n",
               me->simulation()->type_str());
        return -1;
    }

    object1 = me->simulation()->find_object(&argv[1]->s);
    object2 = me->simulation()->find_object(&argv[2]->s);

    // For Hinge2, both objects must exist
    if (!object1 || !object2) {
        printf("[%s] Error creating hinge2 constraint '%s', "
               "object '%s' not found.\n",
               me->simulation()->type_str(), &argv[0]->s,
               object1 ? &argv[2]->s : &argv[1]->s);
        return -1;
    }

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    double x   = argv[3]->f;
    double y   = argv[4]->f;
    double z   = argv[5]->f;
    double a1x = argv[6]->f;
    double a1y = argv[7]->f;
    double a1z = argv[8]->f;
    double a2x = argv[9]->f;
    double a2y = argv[10]->f;
    double a2z = argv[11]->f;

    if (!me->create(&argv[0]->s, object1, object2,
                    x, y, z, a1x, a1y, a1z, a2x, a2y, a2z))
        printf("[%s] Error creating hinge2 constraint '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

FixedFactory::FixedFactory(Simulation *parent)
    : ShapeFactory("fixed", parent)
{
    // Name, object1, object2, x, y, z, axis x, y, z
    addHandler("create", "sss", create_handler);
}

FixedFactory::~FixedFactory()
{
}

int FixedFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    FixedFactory *me = static_cast<FixedFactory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 3) return -1;

    if (strcmp(&argv[1]->s, "world")!=0)
        object1 = me->simulation()->find_object(&argv[1]->s);
    if (strcmp(&argv[2]->s, "world")!=0)
        object2 = me->simulation()->find_object(&argv[2]->s);

    // Swap objects if one is world
    if (object2 && !object1) {
        object1 = object2;
        object2 = 0;
    }

    // At least one object must exist
    if (!object1) {
        printf("[%s] Error creating fixed constraint '%s', "
               "object '%s' not found.\n",
               me->simulation()->type_str(), &argv[0]->s,
               &argv[1]->s);
        return -1;
    }

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    if (!me->create(&argv[0]->s, object1, object2))
        printf("[%s] Error creating fixed constraint '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

FreeFactory::FreeFactory(Simulation *parent)
    : ShapeFactory("free", parent)
{
    // Name, object1, object2, x, y, z, axis x, y, z
    addHandler("create", "sss", create_handler);
}

FreeFactory::~FreeFactory()
{
}

int FreeFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                int argc, void *data, void *user_data)
{
    FreeFactory *me = static_cast<FreeFactory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 3) return -1;

    if (strcmp(&argv[1]->s, "world")!=0)
        object1 = me->simulation()->find_object(&argv[1]->s);
    if (strcmp(&argv[2]->s, "world")!=0)
        object2 = me->simulation()->find_object(&argv[2]->s);

    // Neither object can be the world.
    if (!object1 || !object2) {
        printf("[%s] Error, cannot create free constraint '%s' with world.\n");
        return -1;
    }

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    if (!me->create(&argv[0]->s, object1, object2))
        printf("[%s] Error creating free constraint '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

BallJointFactory::BallJointFactory(Simulation *parent)
    : ShapeFactory("ball", parent)
{
    // Name, object1, object2, x, y, z
    addHandler("create", "sssfff", create_handler);
}

BallJointFactory::~BallJointFactory()
{
}

int BallJointFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    BallJointFactory *me = static_cast<BallJointFactory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 6) return -1;

    if (strcmp(&argv[1]->s, "world")!=0)
        object1 = me->simulation()->find_object(&argv[1]->s);
    if (strcmp(&argv[2]->s, "world")!=0)
        object2 = me->simulation()->find_object(&argv[2]->s);

    // Swap objects if one is world
    if (object2 && !object1) {
        object1 = object2;
        object2 = 0;
    }

    // At least one object must exist
    if (!object1) {
        printf("[%s] Error creating ball constraint '%s', "
               "object '%s' not found.\n",
               me->simulation()->type_str(), &argv[0]->s,
               &argv[1]->s);
        return -1;
    }

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    double x = argv[3]->f;
    double y = argv[4]->f;
    double z = argv[5]->f;

    if (!me->create(&argv[0]->s, object1, object2, x, y, z))
        printf("[%s] Error creating ball constraint '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

SlideFactory::SlideFactory(Simulation *parent)
    : ShapeFactory("slide", parent)
{
    // Name, object1, object2, x, y, z
    addHandler("create", "sssfff", create_handler);
}

SlideFactory::~SlideFactory()
{
}

int SlideFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    SlideFactory *me = static_cast<SlideFactory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 6) return -1;

    if (strcmp(&argv[1]->s, "world")!=0)
        object1 = me->simulation()->find_object(&argv[1]->s);
    if (strcmp(&argv[2]->s, "world")!=0)
        object2 = me->simulation()->find_object(&argv[2]->s);

    // Swap objects if one is world
    if (object2 && !object1) {
        object1 = object2;
        object2 = 0;
    }

    // At least one object must exist
    if (!object1) {
        printf("[%s] Error creating slide constraint '%s', "
               "object '%s' not found.\n",
               me->simulation()->type_str(), &argv[0]->s,
               &argv[1]->s);
        return -1;
    }

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    double ax = argv[3]->f;
    double ay = argv[4]->f;
    double az = argv[5]->f;

    if (!me->create(&argv[0]->s, object1, object2, ax, ay, az))
        printf("[%s] Error creating slide constraint '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

PistonFactory::PistonFactory(Simulation *parent)
    : ShapeFactory("piston", parent)
{
    // Name, object1, object2, x, y, z, axis x, y, z
    addHandler("create", "sssffffff", create_handler);
}

PistonFactory::~PistonFactory()
{
}

int PistonFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    PistonFactory *me = static_cast<PistonFactory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 9) return -1;

    if (strcmp(&argv[1]->s, "world")!=0)
        object1 = me->simulation()->find_object(&argv[1]->s);
    if (strcmp(&argv[2]->s, "world")!=0)
        object2 = me->simulation()->find_object(&argv[2]->s);

    // Swap objects if one is world
    if (object2 && !object1) {
        object1 = object2;
        object2 = 0;
    }

    // At least one object must exist
    if (!object1) {
        printf("[%s] Error creating piston constraint '%s', "
               "object '%s' not found.\n",
               me->simulation()->type_str(), &argv[0]->s,
               &argv[1]->s);
        return -1;
    }

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    if (!me->create(&argv[0]->s, object1, object2,
                    argv[3]->f, argv[4]->f, argv[5]->f,
                    argv[6]->f, argv[7]->f, argv[8]->f))
        printf("[%s] Error creating piston '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

UniversalFactory::UniversalFactory(Simulation *parent)
    : ShapeFactory("universal", parent)
{
    // Name, object1, object2, x, y, z, a1x, a1y, a1z, a2x, a2y, a2z
    addHandler("create", "sssfffffffff", create_handler);
}

UniversalFactory::~UniversalFactory()
{
}

int UniversalFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    UniversalFactory *me = static_cast<UniversalFactory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 12) return -1;

    if (strcmp(&argv[1]->s, "world")!=0)
        object1 = me->simulation()->find_object(&argv[1]->s);
    if (strcmp(&argv[2]->s, "world")!=0)
        object2 = me->simulation()->find_object(&argv[2]->s);

    // Swap objects if one is world
    if (object2 && !object1) {
        object1 = object2;
        object2 = 0;
    }

    // At least one object must exist
    if (!object1) {
        printf("[%s] Error creating universal constraint '%s', "
               "object '%s' not found.\n",
               me->simulation()->type_str(), &argv[0]->s,
               &argv[1]->s);
        return -1;
    }

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    double x   = argv[3]->f;
    double y   = argv[4]->f;
    double z   = argv[5]->f;
    double a1x = argv[6]->f;
    double a1y = argv[7]->f;
    double a1z = argv[8]->f;
    double a2x = argv[9]->f;
    double a2y = argv[10]->f;
    double a2z = argv[11]->f;

    if (!me->create(&argv[0]->s, object1, object2,
                    x, y, z, a1x, a1y, a1z, a2x, a2y, a2z))
        printf("[%s] Error creating universal constraint '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

/****** SimulationReceiver *******/

SimulationReceiver::SimulationReceiver(const char *url, int type)
    : m_type(type), m_queue(0)
{
    m_addr = lo_address_new_from_url(url);
    switch (m_type) {
    case Simulation::ST_VISUAL:
        m_fTimestep = visual_timestep_ms/1000.0;
        break;
    case Simulation::ST_PHYSICS:
        m_fTimestep = physics_timestep_ms/1000.0;
        break;
    case Simulation::ST_HAPTICS:
        m_fTimestep = haptics_timestep_ms/1000.0;
        break;
    }

    m_bUseQueue = false;
}

SimulationReceiver::SimulationReceiver(Simulation &sim)
    : m_addr(sim.addr()), m_fTimestep(sim.timestep()),
      m_type(sim.type()), m_queue(msg_queue_size)
{
    m_bUseQueue = true;
    sim.add_queue(&m_queue);
}

void SimulationReceiver::send_lo_message(const char *path, lo_message msg)
{
#ifdef USE_QUEUES
    if (m_bUseQueue)
        m_queue.write_lo_message(path, msg);
    else
#endif
        lo_send_message(addr(), path, msg);
}

/****** Simulation *******/

Simulation::Simulation(const char *port, int type)
    : OscBase("world", NULL, lo_server_new(port, NULL)),
      m_collide("collide", this),
      m_gravity("gravity", this),
      m_scale("scale", this)
{
    m_addr = lo_address_new("localhost", port);
    m_type = type;

    m_bDone = false;
    m_bStarted = false;
    m_bSelfTimed = true;

    m_collide.setSetCallback(set_collide, this);
    m_gravity.setSetCallback(set_gravity, this);
    m_scale.setSetCallback(set_scale, this);

    m_scale.setValue(1,1,1);
}

Simulation::~Simulation()
{
    stop();

    if (m_server) {
        lo_server_free(m_server);
        m_server = 0;
    }

    lo_address_free(m_addr);

    // Clear lo_server pointers in OscValues, otherwise their
    // destructors will dereference it while removing methods.
    m_collide.m_server = 0;
    m_gravity.m_server = 0;
    m_gravity.m_magnitude.m_server = 0;
    m_scale.m_server = 0;
    m_scale.m_magnitude.m_server = 0;
}

void Simulation::add_receiver(Simulation *sim, const char *spec,
                              Simulation::SimulationType type,
                              bool initialization)
{
    SimulationReceiver *r = NULL;
    if (sim)
        r = new SimulationReceiver(*sim);
    else if (spec[0] != '\0') {
        // Check that we don't already have it in the list
        std::vector<SimulationReceiver*>::iterator it;
        for (it = m_receiverList.begin();
             it != m_receiverList.end();
             it++)
        {
            char *url = lo_address_get_url((*it)->addr());
            if (url && strcmp(url, spec)==0) {
                free(url);
                return;
            }
            if (url) free(url);
        }

        r = new SimulationReceiver(spec, type);

        // Tell remote simulations about us
        if (m_type != ST_INTERFACE && type != ST_INTERFACE) {
            lo_send_from(r->addr(), m_server, LO_TT_IMMEDIATE,
                         "/world/add_receiver", "s", type_str());
        }

        // We don't add remotes to the receiver list on
        // initialization, instead we wait for the /world/add_receiver
        // response, which allows us to set up direct connections
        // while still going through Interface.  However, we make an
        // exception for Visual->Phyics connection; since Visual
        // doesn't send messages to Physics, we must explicitly allow
        // Physics to connect unilaterally to Visual.
        if (initialization && !(type == ST_VISUAL && m_type == ST_PHYSICS))
        {
            delete r;
            return;
        }
    }

    if (r)
        m_receiverList.push_back(r);

#ifdef DEBUG
    printf("[%s] receiver list:\n", type_str());
    std::vector<SimulationReceiver*>::iterator it;
    for (it = m_receiverList.begin();
         it != m_receiverList.end();
         it++)
    {
        char *url = lo_address_get_url((*it)->addr());
        printf("   %s [%s]\n", url, type_str((*it)->type()));
        free(url);
    }
#endif
}

bool Simulation::start()
{
    bool rc = true;
    if (m_bStarted)
        return true;

    m_bDone = false;

    // We'll use a timed wait, so need to get absolute time
    auto now = std::chrono::system_clock::now();
    std::unique_lock<std::mutex> lk(m_mutex);

    try {
        m_thread = std::thread(Simulation::run, this);
    } catch (std::system_error &e) {
        printf("[%s] Error creating simulation thread.", type_str());
        return false;
    }

#if 0 // C++14
    using namespace std::literals::chrono_literals;
    if (m_condvar.wait_until(lk, now+3*1s)==std::cv_status::timeout) {
#else
    std::chrono::seconds sec(1);
    if (m_condvar.wait_until(lk, now+3*sec)==std::cv_status::timeout) {
#endif
        printf("[%s] Timed out during initialization.\n", type_str());
        return false;
    }

    m_bStarted = true;

    return true;
}

void Simulation::stop()
{
    printf("[%s] Ending simulation... ", type_str());

    if (m_type != ST_INTERFACE)
        send(0, "/world/remove_receiver", "s", type_str());

    m_bDone = true;
    if (m_bStarted)
        m_thread.join();
    m_bStarted = false;
    printf("done.\n");
}

void Simulation::initialize()
{
    addHandler("clear", "", Simulation::clear_handler);
    addHandler("reset_workspace", "", Simulation::reset_workspace_handler);
    addHandler("drop",  "", Simulation::drop_handler);
    addHandler("add_receiver", "s", Simulation::add_receiver_handler);
    addHandler("add_receiver_url", "ss", Simulation::add_receiver_url_handler);
    addHandler("remove_receiver", "s", Simulation::remove_receiver_handler);
}

void* Simulation::run(void* param)
{
    Simulation* me = static_cast<Simulation*>(param);

    me->initialize();

    if (me->m_bDone)
        printf("[%s] Error initialization simulation, port %u.\n",
               me->type_str(), lo_server_get_port(me->m_server));
    else {
        printf("[%s] Simulation running on port %u.\n", me->type_str(),
               lo_server_get_port(me->m_server));

        // TODO
        // char s[1024];
        // snprintf(s, 1024, "[%s] Simulation running on port %u.\n", me->type_str(),
        //          lo_server_get_port(me->m_server));
        // me->sendtotype(ST_VISUAL, true, "/world/log", "s", s);
    }

    // Signal parent thread
    me->m_condvar.notify_all();

    std::vector<LoQueue*>::iterator qit;

    int step_ms = (int)(me->m_fTimestep*1000);
    int step_us = (int)(me->m_fTimestep*1000000);
    int step_left = step_ms;
    while (!me->m_bDone)
    {
        me->m_clock.reset();
        me->m_clock.setTimeoutPeriodSeconds(me->m_fTimestep);
        me->m_clock.start();
        step_left = me->m_bSelfTimed ? step_ms : 0;
        while (lo_server_recv_noblock(me->m_server, step_left) > 0) {
            step_left = step_ms-(me->m_clock.getCurrentTimeSeconds()/1000);
            if (step_left < 0) step_left = 0;
        }
#ifdef USE_QUEUES
        for (qit=me->m_queueList.begin();
             qit!=me->m_queueList.end(); qit++) {
            while ((*qit)->read_and_dispatch(me->m_server)) {}
        }
#endif
        me->m_clock.stop();
        me->step();
        me->m_valueTimer.onTimer(step_ms);
    }

    printf("[%s] Simulation done.\n", me->type_str());
    me->m_bStarted = 0;

    return 0;
}

bool Simulation::add_object(OscObject& obj)
{
    world_objects[obj.name()] = &obj;

    printf("[%s] Added object %s\n", type_str(), obj.c_name());
    return true;
}

bool Simulation::delete_object(OscObject& obj)
{
    printf("[%s] Removing object %s\n", type_str(), obj.c_name());

    if (m_pGrabbedObject == &obj)
        set_grabbed(NULL);

    world_objects.erase(obj.name());
    delete &obj;

    return true;
}

OscObject* Simulation::find_object(const char* name)
{
    object_iterator it = world_objects.find(name);
    if (it!=world_objects.end()) return it->second;
    else
        return 0;
}

bool Simulation::add_constraint(OscConstraint& obj)
{
    world_constraints[obj.name()] = &obj;

    printf("[%s] Added constraint %s\n", type_str(), obj.c_name());
    return true;
}

bool Simulation::delete_constraint(OscConstraint& obj)
{
    printf("[%s] Removing constraint %s\n", type_str(), obj.c_name());

    world_constraints.erase(obj.name());
    delete &obj;

    return true;
}

// from liblo internals:
// eventually this will be a public function in liblo,
// but for now we'll reproduce it here.

static void add_varargs(/*lo_address t,*/ lo_message msg, va_list ap,
			const char *types)
{
    int count = 0;
    int i;
    int64_t i64;
    float f;
    char *s;
    lo_blob b;
    uint8_t *m;
    lo_timetag tt;
    double d;

    while (types && *types) {
	count++;
	switch (*types++) {

	case LO_INT32:
	    i = va_arg(ap, int32_t);
	    lo_message_add_int32(msg, i);
	    break;

	case LO_FLOAT:
	    f = (float)va_arg(ap, double);
	    lo_message_add_float(msg, f);
	    break;

	case LO_STRING:
	    s = va_arg(ap, char *);
	    if (s == (char *)LO_MARKER_A) { //error
	    } else
	    lo_message_add_string(msg, s);
	    break;

	case LO_BLOB:
	    b = va_arg(ap, lo_blob);
	    lo_message_add_blob(msg, b);
	    break;

	case LO_INT64:
	    i64 = va_arg(ap, int64_t);
	    lo_message_add_int64(msg, i64);
	    break;

	case LO_TIMETAG:
	    tt = va_arg(ap, lo_timetag);
	    lo_message_add_timetag(msg, tt);
	    break;

	case LO_DOUBLE:
	    d = va_arg(ap, double);
	    lo_message_add_double(msg, d);
	    break;

	case LO_SYMBOL:
	    s = va_arg(ap, char *);
	    if (s == (char *)LO_MARKER_A) { //error
	    } else
	    lo_message_add_symbol(msg, s);
	    break;

	case LO_CHAR:
	    i = va_arg(ap, int);
	    lo_message_add_char(msg, i);
	    break;

	case LO_MIDI:
	    m = va_arg(ap, uint8_t *);
	    lo_message_add_midi(msg, m);
	    break;

	case LO_TRUE:
	    lo_message_add_true(msg);
	    break;

	case LO_FALSE:
	    lo_message_add_false(msg);
	    break;

	case LO_NIL:
	    lo_message_add_nil(msg);
	    break;

	case LO_INFINITUM:
	    lo_message_add_infinitum(msg);
	    break;

	default:
        //error
	    break;
	}
    }
}

void Simulation::send(bool throttle, const char *path, const char *types, ...)
{
    va_list ap;
    lo_message msg = lo_message_new();
    va_start(ap, types);
    add_varargs(msg, ap, types);
    va_end(ap);

    std::vector<SimulationReceiver*>::iterator it;
    for (it=m_receiverList.begin();
         it!=m_receiverList.end();
         it++)
    {
        /* TODO: throttling must take into account the destination as
         * well as the message path. Until then it can't be used in
         * the general case. */
#if 0
        if (throttle && should_throttle(path, **it))
            continue;
#endif

        (*it)->send_lo_message(path, msg);
    }

    lo_message_free(msg);
}

void Simulation::sendtotype(int type, bool throttle, const char *path, const char *types, ...)
{
    va_list ap;
    lo_message msg = lo_message_new();
    va_start(ap, types);
    add_varargs(msg, ap, types);
    va_end(ap);

    std::vector<SimulationReceiver*>::iterator it;
    for (it=m_receiverList.begin();
         it!=m_receiverList.end();
         it++)
    {
        if ((*it)->type() & type)
        {
            if (throttle && should_throttle(path, **it))
                continue;

            (*it)->send_lo_message(path, msg);
        }
    }

    lo_message_free(msg);
}

const char* Simulation::type_str()
{
    return type_str(m_type);
}

const char* Simulation::type_str(int type)
{
    switch (type) {
    case ST_PHYSICS:
        return "physics";
    case ST_HAPTICS:
        return "haptics";
    case ST_VISUAL:
        return "visual";
    case ST_INTERFACE:
        return "interface";
    default:
        return NULL;
    }
}

Simulation::SimulationType Simulation::str_type(const char *type)
{
    if (strcmp(type, "physics")==0)
        return ST_PHYSICS;
    if (strcmp(type, "haptics")==0)
        return ST_HAPTICS;
    if (strcmp(type, "visual")==0)
        return ST_VISUAL;
    if (strcmp(type, "interface")==0)
        return ST_INTERFACE;
    return ST_UNKNOWN;
}

void Simulation::on_clear()
{
    on_drop();

    object_iterator it = world_objects.begin();
    while (it != world_objects.end())
    {
        it->second->on_destroy();
        it = world_objects.begin();
    }
}

void Simulation::on_add_receiver(const char *type)
{
    SimulationType t = str_type(type);
    if (t == ST_UNKNOWN) return;

    lo_address a = lo_message_get_source(m_msg);
    if (!a) return;

    char *url = lo_address_get_url(a);
    if (!url) return;

    add_receiver(0, url, t, false);
#ifdef DEBUG
    printf("[%s] add_receiver(): %s, source = %s\n", type_str(), type, url);
#endif
    free(url);
}

void Simulation::on_add_receiver_url(const char *type, const char *url)
{
    SimulationType t = str_type(type);
    if (t == ST_UNKNOWN) return;

    add_receiver(0, url, t, false);
#ifdef DEBUG
    printf("[%s] add_receiver_url(): %s, source = %s\n", type_str(), type, url);
#endif
}

void Simulation::on_remove_receiver(const char *type)
{
    SimulationType t = str_type(type);
    if (t == ST_UNKNOWN) return;

    lo_address a = lo_message_get_source(m_msg);
    if (!a) return;

    char *url = lo_address_get_url(a);
    if (!url) return;

    int count = 0;
    std::vector<SimulationReceiver*>::iterator it;
    it = m_receiverList.end();
    do {
        if (it != m_receiverList.end()) {
            m_receiverList.erase(it);
            count ++;
        }

        for (it = m_receiverList.begin();
             it != m_receiverList.end();
             it++)
        {
            char *lurl = lo_address_get_url((*it)->addr());
            if (strcmp(url, lurl)==0) {
                free(lurl);
                break;
            }
            free(lurl);
        }
    } while (it != m_receiverList.end());

#ifdef DEBUG
    printf("[%s] remove_receiver(): %s, source = %s; removed %d\n",
           type_str(), type, url, count);
#endif
    free(url);
}

bool Simulation::should_throttle(const char *path, SimulationReceiver& sim_to)
{
    sent_messages_iterator it = sent_messages.find(path);
    if (it!=sent_messages.end())
    {
        it->second ++;
        if (it->second < (sim_to.timestep()/timestep()))
            return true;
        else
            it->second = 0;
    }
    else
        sent_messages[path] = 0;

    return false;
}
